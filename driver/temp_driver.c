/*
 * LTempGuard - Linux-Based IoT Temperature Monitoring and Alert System
 * File: temp_driver.c
 * Description: Linux Character Device Driver providing temperature sensor abstraction,
 *              dynamic major allocation, ASCII read/write simulation, and ioctl controls.
 * Author: LTempGuard Engineering Team
 * License: Dual GPL/MIT
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/version.h>
#include <linux/ktime.h>
#include <linux/string.h>

#include "temp_ioctl.h"

#define DRIVER_NAME "temp_sensor"
#define CLASS_NAME  "temp_sensor_class"
#define BUFFER_SIZE 64

MODULE_LICENSE("GPL");
MODULE_AUTHOR("LTempGuard Engineering Team");
MODULE_DESCRIPTION("LTempGuard Linux Character Device Driver for IoT Temperature Monitoring");
MODULE_VERSION("1.0.0");

/* Driver State Structure */
struct temp_device_context {
    dev_t dev_num;
    struct cdev cdev;
    struct class *dev_class;
    struct device *dev_device;

    /* In-kernel state: millicelsius (mC) to avoid floating point in kernel */
    int32_t current_temp_mC;
    int32_t warning_threshold_mC;
    int32_t critical_threshold_mC;

    /* Telemetry Counters */
    uint32_t read_count;
    uint32_t write_count;
    uint32_t ioctl_count;
    ktime_t last_update_time;

    /* Concurrency control */
    struct mutex lock;
};

static struct temp_device_context g_temp_ctx;

/**
 * parse_ascii_temp_to_mC() - Parses ASCII float string into integer millicelsius.
 * @buf: Null-terminated string (e.g. "37.5\n", "-12.25", "40")
 * @out_mC: Output pointer for millicelsius value
 *
 * Safe integer-based parser without floating-point instructions.
 * Return: 0 on success, -EINVAL on invalid format or overflow.
 */
static int parse_ascii_temp_to_mC(const char *buf, int32_t *out_mC)
{
    int sign = 1;
    int32_t int_part = 0;
    int32_t frac_part = 0;
    int frac_digits = 0;
    const char *p = buf;

    /* Skip leading whitespace */
    while (*p == ' ' || *p == '\t')
        p++;

    /* Parse optional sign */
    if (*p == '-') {
        sign = -1;
        p++;
    } else if (*p == '+') {
        p++;
    }

    if (*p == '\0' || *p == '\n')
        return -EINVAL;

    /* Parse integer part */
    while (*p >= '0' && *p <= '9') {
        int digit = *p - '0';
        if (int_part > (TEMP_MAX_VALID_MC / 1000) + 10)
            return -EINVAL; /* Potential overflow */
        int_part = int_part * 10 + digit;
        p++;
    }

    /* Parse decimal part if present */
    if (*p == '.') {
        p++;
        while (*p >= '0' && *p <= '9' && frac_digits < 3) {
            frac_part = frac_part * 10 + (*p - '0');
            frac_digits++;
            p++;
        }
        /* Skip any additional precision digits beyond millicelsius */
        while (*p >= '0' && *p <= '9')
            p++;
    }

    /* Normalize fraction to thousandths (millicelsius) */
    while (frac_digits < 3) {
        frac_part *= 10;
        frac_digits++;
    }

    /* Verify trailer characters */
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
        p++;

    if (*p != '\0')
        return -EINVAL; /* Stray non-numeric characters detected */

    *out_mC = sign * (int_part * 1000 + frac_part);
    return 0;
}

/**
 * format_mC_to_ascii() - Formats integer millicelsius into ASCII string.
 * @mC: Temperature in millicelsius
 * @buf: Destination buffer
 * @buf_size: Buffer capacity
 *
 * Return: Number of characters written.
 */
static int format_mC_to_ascii(int32_t mC, char *buf, size_t buf_size)
{
    int sign = (mC < 0) ? -1 : 1;
    int32_t abs_mC = (mC < 0) ? -mC : mC;
    int32_t int_part = abs_mC / 1000;
    int32_t frac_part = abs_mC % 1000;

    if (sign < 0) {
        return snprintf(buf, buf_size, "-%d.%03d\n", int_part, frac_part);
    } else {
        return snprintf(buf, buf_size, "%d.%03d\n", int_part, frac_part);
    }
}

/* ========================================================================= */
/* VFS File Operations                                                       */
/* ========================================================================= */

static int temp_driver_open(struct inode *inodep, struct file *filep)
{
    pr_info("LTEMPGUARD: Device /dev/%s opened (pid: %d)\n", DRIVER_NAME, current->pid);
    return 0;
}

static int temp_driver_release(struct inode *inodep, struct file *filep)
{
    pr_info("LTEMPGUARD: Device /dev/%s released (pid: %d)\n", DRIVER_NAME, current->pid);
    return 0;
}

static ssize_t temp_driver_read(struct file *filep, char __user *user_buf,
                                size_t count, loff_t *offset)
{
    char kbuf[BUFFER_SIZE];
    int len;
    ssize_t bytes_to_copy;

    /* Handle EOF */
    if (*offset > 0)
        return 0;

    if (mutex_lock_interruptible(&g_temp_ctx.lock))
        return -ERESTARTSYS;

    len = format_mC_to_ascii(g_temp_ctx.current_temp_mC, kbuf, sizeof(kbuf));
    g_temp_ctx.read_count++;

    mutex_unlock(&g_temp_ctx.lock);

    if (len < 0)
        return -EFAULT;

    bytes_to_copy = min_t(ssize_t, count, (ssize_t)len);

    if (copy_to_user(user_buf, kbuf, bytes_to_copy))
        return -EFAULT;

    *offset += bytes_to_copy;
    return bytes_to_copy;
}

static ssize_t temp_driver_write(struct file *filep, const char __user *user_buf,
                                 size_t count, loff_t *offset)
{
    char kbuf[BUFFER_SIZE];
    size_t bytes_to_copy;
    int32_t parsed_mC;
    int ret;

    if (count == 0 || count >= sizeof(kbuf))
        return -EINVAL;

    bytes_to_copy = min_t(size_t, count, sizeof(kbuf) - 1);

    if (copy_from_user(kbuf, user_buf, bytes_to_copy))
        return -EFAULT;

    kbuf[bytes_to_copy] = '\0';

    ret = parse_ascii_temp_to_mC(kbuf, &parsed_mC);
    if (ret != 0) {
        pr_warn("LTEMPGUARD: Invalid temperature string received: '%s'\n", kbuf);
        return -EINVAL;
    }

    if (parsed_mC < TEMP_MIN_VALID_MC || parsed_mC > TEMP_MAX_VALID_MC) {
        pr_warn("LTEMPGUARD: Temperature out of physical bounds [%d, %d]: %d mC\n",
                TEMP_MIN_VALID_MC, TEMP_MAX_VALID_MC, parsed_mC);
        return -EINVAL;
    }

    if (mutex_lock_interruptible(&g_temp_ctx.lock))
        return -ERESTARTSYS;

    g_temp_ctx.current_temp_mC = parsed_mC;
    g_temp_ctx.write_count++;
    g_temp_ctx.last_update_time = ktime_get();

    mutex_unlock(&g_temp_ctx.lock);

    pr_info("LTEMPGUARD: Sensor updated via write: %d mC\n", parsed_mC);
    return count;
}

static long temp_driver_ioctl(struct file *filep, unsigned int cmd, unsigned long arg)
{
    int ret = 0;

    if (mutex_lock_interruptible(&g_temp_ctx.lock))
        return -ERESTARTSYS;

    g_temp_ctx.ioctl_count++;

    switch (cmd) {
    case TEMP_IOC_GET_TEMP: {
        int32_t temp = g_temp_ctx.current_temp_mC;
        if (copy_to_user((int32_t __user *)arg, &temp, sizeof(temp)))
            ret = -EFAULT;
        break;
    }

    case TEMP_IOC_SET_TEMP: {
        int32_t temp;
        if (copy_from_user(&temp, (int32_t __user *)arg, sizeof(temp))) {
            ret = -EFAULT;
            break;
        }
        if (temp < TEMP_MIN_VALID_MC || temp > TEMP_MAX_VALID_MC) {
            ret = -EINVAL;
            break;
        }
        g_temp_ctx.current_temp_mC = temp;
        g_temp_ctx.last_update_time = ktime_get();
        break;
    }

    case TEMP_IOC_GET_CONFIG: {
        struct temp_config_payload cfg;
        cfg.warning_threshold_mC = g_temp_ctx.warning_threshold_mC;
        cfg.critical_threshold_mC = g_temp_ctx.critical_threshold_mC;
        if (copy_to_user((struct temp_config_payload __user *)arg, &cfg, sizeof(cfg)))
            ret = -EFAULT;
        break;
    }

    case TEMP_IOC_SET_CONFIG: {
        struct temp_config_payload cfg;
        if (copy_from_user(&cfg, (struct temp_config_payload __user *)arg, sizeof(cfg))) {
            ret = -EFAULT;
            break;
        }
        /* Validate thresholds: warn must be less than critical, both within limits */
        if (cfg.warning_threshold_mC >= cfg.critical_threshold_mC ||
            cfg.warning_threshold_mC < TEMP_MIN_VALID_MC ||
            cfg.critical_threshold_mC > TEMP_MAX_VALID_MC) {
            pr_warn("LTEMPGUARD: Invalid threshold configuration (warn=%d, crit=%d)\n",
                    cfg.warning_threshold_mC, cfg.critical_threshold_mC);
            ret = -EINVAL;
            break;
        }
        g_temp_ctx.warning_threshold_mC = cfg.warning_threshold_mC;
        g_temp_ctx.critical_threshold_mC = cfg.critical_threshold_mC;
        pr_info("LTEMPGUARD: Thresholds updated (warn=%d mC, crit=%d mC)\n",
                cfg.warning_threshold_mC, cfg.critical_threshold_mC);
        break;
    }

    case TEMP_IOC_GET_STATUS: {
        struct temp_status_payload status;
        status.current_temp_mC = g_temp_ctx.current_temp_mC;
        status.warning_threshold_mC = g_temp_ctx.warning_threshold_mC;
        status.critical_threshold_mC = g_temp_ctx.critical_threshold_mC;
        status.read_count = g_temp_ctx.read_count;
        status.write_count = g_temp_ctx.write_count;
        status.ioctl_count = g_temp_ctx.ioctl_count;
        status.last_update_ns = ktime_to_ns(g_temp_ctx.last_update_time);

        if (copy_to_user((struct temp_status_payload __user *)arg, &status, sizeof(status)))
            ret = -EFAULT;
        break;
    }

    case TEMP_IOC_RESET_STATS: {
        g_temp_ctx.read_count = 0;
        g_temp_ctx.write_count = 0;
        g_temp_ctx.ioctl_count = 0;
        pr_info("LTEMPGUARD: Telemetry statistics reset\n");
        break;
    }

    default:
        pr_warn("LTEMPGUARD: Unknown IOCTL command: 0x%x\n", cmd);
        ret = -ENOTTY;
        break;
    }

    mutex_unlock(&g_temp_ctx.lock);
    return ret;
}

static const struct file_operations temp_fops = {
    .owner          = THIS_MODULE,
    .open           = temp_driver_open,
    .release        = temp_driver_release,
    .read           = temp_driver_read,
    .write          = temp_driver_write,
    .unlocked_ioctl = temp_driver_ioctl,
};

/* ========================================================================= */
/* Module Lifecycle: Initialization & Teardown                               */
/* ========================================================================= */

static int __init temp_driver_init(void)
{
    int ret;

    pr_info("LTEMPGUARD: Initializing character device driver...\n");

    /* Initialize state defaults */
    memset(&g_temp_ctx, 0, sizeof(g_temp_ctx));
    mutex_init(&g_temp_ctx.lock);
    g_temp_ctx.current_temp_mC = TEMP_DEFAULT_INIT_MC;
    g_temp_ctx.warning_threshold_mC = TEMP_DEFAULT_WARN_MC;
    g_temp_ctx.critical_threshold_mC = TEMP_DEFAULT_CRIT_MC;
    g_temp_ctx.last_update_time = ktime_get();

    /* Step 1: Dynamically allocate major/minor numbers */
    ret = alloc_chrdev_region(&g_temp_ctx.dev_num, 0, 1, DRIVER_NAME);
    if (ret < 0) {
        pr_err("LTEMPGUARD: Failed to allocate chrdev region (err: %d)\n", ret);
        return ret;
    }
    pr_info("LTEMPGUARD: Allocated Major %d, Minor %d\n",
            MAJOR(g_temp_ctx.dev_num), MINOR(g_temp_ctx.dev_num));

    /* Step 2: Initialize and bind cdev */
    cdev_init(&g_temp_ctx.cdev, &temp_fops);
    g_temp_ctx.cdev.owner = THIS_MODULE;

    ret = cdev_add(&g_temp_ctx.cdev, g_temp_ctx.dev_num, 1);
    if (ret < 0) {
        pr_err("LTEMPGUARD: Failed to add cdev (err: %d)\n", ret);
        goto fail_cdev;
    }

    /* Step 3: Create device class (Linux kernel version compatible) */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
    g_temp_ctx.dev_class = class_create(CLASS_NAME);
#else
    g_temp_ctx.dev_class = class_create(THIS_MODULE, CLASS_NAME);
#endif
    if (IS_ERR(g_temp_ctx.dev_class)) {
        ret = PTR_ERR(g_temp_ctx.dev_class);
        pr_err("LTEMPGUARD: Failed to create device class (err: %d)\n", ret);
        goto fail_class;
    }

    /* Step 4: Create device node (/dev/temp_sensor) */
    g_temp_ctx.dev_device = device_create(g_temp_ctx.dev_class, NULL,
                                          g_temp_ctx.dev_num, NULL, DRIVER_NAME);
    if (IS_ERR(g_temp_ctx.dev_device)) {
        ret = PTR_ERR(g_temp_ctx.dev_device);
        pr_err("LTEMPGUARD: Failed to create device node (err: %d)\n", ret);
        goto fail_device;
    }

    pr_info("LTEMPGUARD: Driver successfully registered. Device node: /dev/%s\n", DRIVER_NAME);
    return 0;

fail_device:
    class_destroy(g_temp_ctx.dev_class);
fail_class:
    cdev_del(&g_temp_ctx.cdev);
fail_cdev:
    unregister_chrdev_region(g_temp_ctx.dev_num, 1);
    mutex_destroy(&g_temp_ctx.lock);
    return ret;
}

static void __exit temp_driver_exit(void)
{
    pr_info("LTEMPGUARD: Tearing down character device driver...\n");

    if (g_temp_ctx.dev_device)
        device_destroy(g_temp_ctx.dev_class, g_temp_ctx.dev_num);

    if (g_temp_ctx.dev_class)
        class_destroy(g_temp_ctx.dev_class);

    cdev_del(&g_temp_ctx.cdev);
    unregister_chrdev_region(g_temp_ctx.dev_num, 1);
    mutex_destroy(&g_temp_ctx.lock);

    pr_info("LTEMPGUARD: Driver unregistered cleanly. All resources freed.\n");
}

module_init(temp_driver_init);
module_exit(temp_driver_exit);

/*
 * LTempGuard - Linux-Based IoT Temperature Monitoring and Alert System
 * File: temp_ioctl.h
 * Description: Shared Kernel-User ABI definitions and IOCTL commands.
 * Author: LTempGuard Engineering Team
 */

#ifndef LTEMPGUARD_TEMP_IOCTL_H
#define LTEMPGUARD_TEMP_IOCTL_H

#ifdef __KERNEL__
#include <linux/ioctl.h>
#include <linux/types.h>
#else
#include <sys/ioctl.h>
#include <stdint.h>
typedef int32_t __s32;
typedef uint32_t __u32;
typedef uint64_t __u64;
#endif

#define TEMP_IOC_MAGIC 'T'

/**
 * struct temp_status_payload - Runtime telemetry exported by kernel driver.
 * @current_temp_mC: Current temperature reading in millicelsius (mC).
 * @warning_threshold_mC: Active warning threshold in mC.
 * @critical_threshold_mC: Active critical threshold in mC.
 * @read_count: Cumulative successful VFS read() calls.
 * @write_count: Cumulative successful VFS write() calls.
 * @ioctl_count: Cumulative successful ioctl() calls.
 * @last_update_ns: Kernel monotonic timestamp of last state update in nanoseconds.
 */
struct temp_status_payload {
    __s32 current_temp_mC;
    __s32 warning_threshold_mC;
    __s32 critical_threshold_mC;
    __u32 read_count;
    __u32 write_count;
    __u32 ioctl_count;
    __u64 last_update_ns;
};

/**
 * struct temp_config_payload - Threshold parameters passed between user & kernel.
 * @warning_threshold_mC: Desired warning threshold in mC (default: 40000 mC = 40.0 C).
 * @critical_threshold_mC: Desired critical threshold in mC (default: 60000 mC = 60.0 C).
 */
struct temp_config_payload {
    __s32 warning_threshold_mC;
    __s32 critical_threshold_mC;
};

/* IOCTL Command Definitions */
#define TEMP_IOC_GET_TEMP     _IOR(TEMP_IOC_MAGIC, 1, __s32)
#define TEMP_IOC_SET_TEMP     _IOW(TEMP_IOC_MAGIC, 2, __s32)
#define TEMP_IOC_GET_STATUS   _IOR(TEMP_IOC_MAGIC, 3, struct temp_status_payload)
#define TEMP_IOC_GET_CONFIG   _IOR(TEMP_IOC_MAGIC, 4, struct temp_config_payload)
#define TEMP_IOC_SET_CONFIG   _IOW(TEMP_IOC_MAGIC, 5, struct temp_config_payload)
#define TEMP_IOC_RESET_STATS  _IO(TEMP_IOC_MAGIC, 6)

/* Sensor physical bounds (-50.0 C to +150.0 C in millicelsius) */
#define TEMP_MIN_VALID_MC    (-50000)
#define TEMP_MAX_VALID_MC    (150000)

/* Default operating thresholds */
#define TEMP_DEFAULT_WARN_MC (40000)   /* 40.0 C */
#define TEMP_DEFAULT_CRIT_MC (60000)   /* 60.0 C */
#define TEMP_DEFAULT_INIT_MC (25000)   /* 25.0 C (Room ambient) */

#endif /* LTEMPGUARD_TEMP_IOCTL_H */

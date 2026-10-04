# LTempGuard Top-Level Coordinator Makefile
.PHONY: all driver app clean load unload test help

all: driver app

driver:
	@echo "=== Building Kernel Module ==="
	$(MAKE) -C driver

app:
	@echo "=== Building C++ Userspace Application ==="
	@mkdir -p build
	@cd build && cmake .. && $(MAKE) --no-print-directory

test:
	@echo "=== Running Tests ==="
	@mkdir -p build
	@cd build && cmake .. && $(MAKE) --no-print-directory
	@./build/tests/test_runner

clean:
	@echo "=== Cleaning Driver and Userspace Build ==="
	$(MAKE) -C driver clean 2>/dev/null || true
	rm -rf build bin

load:
	@echo "=== Loading Kernel Driver ==="
	sudo bash scripts/load_driver.sh

unload:
	@echo "=== Unloading Kernel Driver ==="
	sudo bash scripts/unload_driver.sh

help:
	@echo "LTempGuard Build & Control Options:"
	@echo "  make all      - Build driver and C++ application"
	@echo "  make driver   - Compile Linux kernel module"
	@echo "  make app      - Compile C++ userspace application"
	@echo "  make test     - Compile and run all tests"
	@echo "  make load     - Insert kernel module and prepare /dev/temp_sensor"
	@echo "  make unload   - Remove kernel module cleanly"
	@echo "  make clean    - Clean all build artifacts"

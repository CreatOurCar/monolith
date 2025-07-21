IDF_PATH := $(HOME)/esp/esp-idf
PATH := $(HOME)/.wokwi/bin:$(PATH)

build:
	export IDF_PATH=$(IDF_PATH) && \
	export IDF_PATH_FORCE=1 && \
	. $(IDF_PATH)/export.sh && \
	idf.py build

run: build
	export IDF_PATH=$(IDF_PATH) && \
	export IDF_PATH_FORCE=1 && \
	. $(IDF_PATH)/export.sh && \
	idf.py -b 921600 flash monitor

sim: build
	wokwi-cli .

clean:
	rm -rf build

config:
	export IDF_PATH=$(IDF_PATH) && \
	export IDF_PATH_FORCE=1 && \
	. $(IDF_PATH)/export.sh && \
	idf.py menuconfig

dump:
	export IDF_PATH=$(IDF_PATH) && \
	export IDF_PATH_FORCE=1 && \
	. $(IDF_PATH)/export.sh && \
	xtensa-esp32s3-elf-objdump -dS build/monolith.elf

.PHONY: build run sim clean config dump

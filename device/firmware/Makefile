IDF_PATH := $(HOME)/esp/esp-idf
PATH := $(HOME)/.wokwi/bin:$(PATH)

build:
	export IDF_PATH=$(IDF_PATH) && \
	export IDF_PATH_FORCE=1 && \
	. $(IDF_PATH)/export.sh && \
	idf.py build

run: build
	. $(HOME)/esp/esp-idf/export.sh && \
	idf.py flash monitor

sim: build
	wokwi-cli .

clean:
	rm -rf build

.PHONY: build run sim clean

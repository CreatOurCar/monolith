PATH := $(HOME)/.wokwi/bin:$(PATH)

build:
	. $(HOME)/esp/esp-idf/export.sh && \
	idf.py build

run: build
	. $(HOME)/esp/esp-idf/export.sh && \
	idf.py flash monitor

sim: build
	wokwi-cli .

clean:
	rm -rf build

.PHONY: build run sim clean

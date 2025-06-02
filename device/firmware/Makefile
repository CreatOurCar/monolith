LIBCLANG_PATH := $(HOME)/.rustup/toolchains/esp/xtensa-esp32-elf-clang/esp-19.1.2_20250225/esp-clang/lib
PATH := $(HOME)/.rustup/toolchains/esp/xtensa-esp-elf/esp-14.2.0_20240906/xtensa-esp-elf/bin:$(HOME)/.wokwi/bin:$(PATH)

build:
	cargo build --release

run:
	cargo run --release

sim: build
	wokwi-cli .

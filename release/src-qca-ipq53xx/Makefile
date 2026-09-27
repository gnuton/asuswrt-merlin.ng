# Native experimental RT-BE90U entry point. The SDK sources and platform-specific
# overrides are tracked here; common inputs come from release/src in this tree.
.DEFAULT_GOAL := help
.PHONY: help rt-be90u check prepare clean

help:
	@echo 'make rt-be90u  Build the experimental RT-BE90U image with the IPQ53xx toolchain'
	@echo 'make check     Validate repository inputs without building'
	@echo 'make prepare   Assemble private build sources without compiling'
	@echo 'make clean     Remove generated build sources/output; retain collected images'

rt-be90u:
	+../../tools/rt-be90u/build-native.sh "$(MAKE)"

check:
	python3 ../../tools/rt-be90u/prepare-native.py --check

prepare:
	flock .build.lock python3 ../../tools/rt-be90u/prepare-native.py

clean:
	flock .build.lock rm -rf -- .build

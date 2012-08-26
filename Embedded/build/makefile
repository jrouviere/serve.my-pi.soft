
CC := avr32-gcc
RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

LIBS := -ldsp-at32ucr3-dspspeed_opt -lm
VPATH = ../src:../../API/src

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: openscb_firmware.elf

# Tool invocations
openscb_firmware.elf: out_dirs $(OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: 32-bit AVR/GNU C Linker'
	$(CC) -nostartfiles -O1 -g3 -L"../src/asf-2.0.0/dsplib" --gc-sections \
		-Wl,-e,_trampoline -T../src/asf-2.0.0/linker_scripts/link_uc3bx128.lds \
		-mpart=uc3b0128 -Wl,--gc-sections --rodata-writable -Wl,--direct-data \
		-o"openscb_firmware.elf" $(OBJS) $(USER_OBJS) $(LIBS)
	$(MAKE) --no-print-directory post-build

clean:
	-$(RM) $(OBJS) $(C_DEPS) openscb_firmware.elf openscb_firmware.hex openscb_firmware.dis
	-@echo ' '


out_dirs:
	@echo $(subst /,\,$(DIRS))

post-build:
	-@echo 'Generating hex file'
	-avr32-objcopy -O ihex openscb_firmware.elf openscb_firmware.hex
	-@echo 'Generating disassembly file'
	-avr32-objdump -S openscb_firmware.elf > openscb_firmware.dis
	-avr32-objdump -ht openscb_firmware.elf > openscb_firmware.map
	-@echo ' '

.PHONY: all out_dirs clean dependents
.SECONDARY: post-build

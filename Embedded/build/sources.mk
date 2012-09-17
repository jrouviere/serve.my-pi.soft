
OBJS += \
asf-2.0.0/avr32_interrupt.o \
asf-2.0.0/flashc.o \
asf-2.0.0/usart.o \
asf-2.0.0/gpio.o \
asf-2.0.0/pm.o \
asf-2.0.0/pm_conf_clocks.o \
asf-2.0.0/power_clocks_lib.o \
asf-2.0.0/startup_uc3.o \
asf-2.0.0/tc.o \
asf-2.0.0/trampoline_uc3.o \
asf-2.0.0/usb_descriptors.o \
asf-2.0.0/usb_device_task.o \
asf-2.0.0/usb_drv.o \
asf-2.0.0/usb_specific_request.o \
asf-2.0.0/usb_standard_request.o \
asf-2.0.0/usb_task.o \
asf-2.0.0/wdt.o 

OBJS += \
asf-2.0.0/debug/debug.o 

OBJS += \
asf-2.0.0/freertos/source/croutine.o \
asf-2.0.0/freertos/source/list.o \
asf-2.0.0/freertos/source/queue.o \
asf-2.0.0/freertos/source/tasks.o 

OBJS += \
asf-2.0.0/freertos/source/portable/memmang/heap_3.o 

OBJS += \
asf-2.0.0/freertos/source/portable/gcc/avr32_uc3/port.o 

OBJS += \
asf-2.0.0/interrupt/exception.o 

OBJS += \
controller/api_ctrl.o \
controller/frame_ctrl.o

OBJS += \
io/rx_input.o \
io/servo_out_bb.o 

OBJS += \
calibration.o \
core.o \
exception_handler.o \
main.o \
out_ctrl.o \
pc_comm.o \
pc_comm_usb.o \
pc_comm_uart.o \
rc_utils.o \
sys_conf.o \
system.o \
trace.o

OBJS += \
openscb_advanced.o \
openscb_dev.o \
openscb_utils.o \
openscb.o

CDEPS += $(patsubst %.o,%.d,$(OBJS))

DIRS += \
asf-2.0.0/interrupt \
asf-2.0.0/preprocessor \
asf-2.0.0/dsplib/include \
asf-2.0.0/freertos/source/portable/gcc/avr32_uc3 \
asf-2.0.0/freertos/source/portable/memmang \
asf-2.0.0/freertos/source/include \
asf-2.0.0/freertos \
asf-2.0.0/debug \
asf-2.0.0 \
controller \
io \
conf \

CFLAGS += -DFREERTOS_USED \
-O1 -fdata-sections -g3 -Wall -Wstrict-aliasing=2 -fno-strict-aliasing \
-c -fmessage-length=0 \
-mpart=uc3b0128 -masm-addr-pseudos \
-muse-rodata-section -I../../API/src/ -I../src/

CFLAGS += $(foreach dir,$(DIRS),-I../src/$(dir))

%.o: %.c
	@echo 'Building file: $<'
	@echo 'Invoking: 32-bit AVR/GNU C Compiler'
	$(CC) $(CFLAGS) -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

%.o: %.x
	@echo 'Building file: $<'
	@echo 'Invoking: 32-bit AVR/GNU Preprocessing Assembler'
	$(CC) -O2 -x assembler-with-cpp -c -mpart=uc3b0128 -Wa,-g3 -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

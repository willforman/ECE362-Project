################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/commands.c \
../src/dac.c \
../src/diskio.c \
../src/display.c \
../src/ff.c \
../src/ffunicode.c \
../src/fifo.c \
../src/lcd.c \
../src/main.c \
../src/sdcard.c \
../src/stm.c \
../src/syscalls.c \
../src/system_stm32f0xx.c \
../src/timer.c \
../src/tty.c \
../src/wav.c 

OBJS += \
./src/commands.o \
./src/dac.o \
./src/diskio.o \
./src/display.o \
./src/ff.o \
./src/ffunicode.o \
./src/fifo.o \
./src/lcd.o \
./src/main.o \
./src/sdcard.o \
./src/stm.o \
./src/syscalls.o \
./src/system_stm32f0xx.o \
./src/timer.o \
./src/tty.o \
./src/wav.o 

C_DEPS += \
./src/commands.d \
./src/dac.d \
./src/diskio.d \
./src/display.d \
./src/ff.d \
./src/ffunicode.d \
./src/fifo.d \
./src/lcd.d \
./src/main.d \
./src/sdcard.d \
./src/stm.d \
./src/syscalls.d \
./src/system_stm32f0xx.d \
./src/timer.d \
./src/tty.d \
./src/wav.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m0 -mthumb -mfloat-abi=soft -DSTM32 -DSTM32F0 -DSTM32F091RCTx -DDEBUG -DSTM32F091 -DUSE_STDPERIPH_DRIVER -I"/Users/nathangovindarajan/Documents/workspace/finalProj/StdPeriph_Driver/inc" -I"/Users/nathangovindarajan/Documents/workspace/finalProj/inc" -I"/Users/nathangovindarajan/Documents/workspace/finalProj/CMSIS/device" -I"/Users/nathangovindarajan/Documents/workspace/finalProj/CMSIS/core" -O0 -g3 -Wall -fmessage-length=0 -ffunction-sections -c -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



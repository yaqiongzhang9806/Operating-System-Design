################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/Data.c \
../src/DiskQueue.c \
../src/Message.c \
../src/MessageQueue.c \
../src/ReadyQueue.c \
../src/TimerQueue.c \
../src/base.c \
../src/sample.c \
../src/statePrinter.c \
../src/test.c \
../src/z502.c 

OBJS += \
./src/Data.o \
./src/DiskQueue.o \
./src/Message.o \
./src/MessageQueue.o \
./src/ReadyQueue.o \
./src/TimerQueue.o \
./src/base.o \
./src/sample.o \
./src/statePrinter.o \
./src/test.o \
./src/z502.o 

C_DEPS += \
./src/Data.d \
./src/DiskQueue.d \
./src/Message.d \
./src/MessageQueue.d \
./src/ReadyQueue.d \
./src/TimerQueue.d \
./src/base.d \
./src/sample.d \
./src/statePrinter.d \
./src/test.d \
./src/z502.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/yaqiong/workspace/test28/inc" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



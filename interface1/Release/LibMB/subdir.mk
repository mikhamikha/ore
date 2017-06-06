################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
/home/LibMB/modbus-data.c \
/home/LibMB/modbus-rtu.c \
/home/LibMB/modbus-tcp.c \
/home/LibMB/modbus.c 

OBJS += \
./LibMB/modbus-data.o \
./LibMB/modbus-rtu.o \
./LibMB/modbus-tcp.o \
./LibMB/modbus.o 

C_DEPS += \
./LibMB/modbus-data.d \
./LibMB/modbus-rtu.d \
./LibMB/modbus-tcp.d \
./LibMB/modbus.d 


# Each subdirectory must supply rules for building sources it contributes
LibMB/modbus-data.o: /home/LibMB/modbus-data.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	arm-9tdmi-linux-gnueabi-gcc -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

LibMB/modbus-rtu.o: /home/LibMB/modbus-rtu.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	arm-9tdmi-linux-gnueabi-gcc -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

LibMB/modbus-tcp.o: /home/LibMB/modbus-tcp.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	arm-9tdmi-linux-gnueabi-gcc -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

LibMB/modbus.o: /home/LibMB/modbus.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	arm-9tdmi-linux-gnueabi-gcc -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



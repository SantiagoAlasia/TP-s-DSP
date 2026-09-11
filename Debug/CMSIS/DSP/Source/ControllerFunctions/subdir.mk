################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../CMSIS/DSP/Source/ControllerFunctions/ControllerFunctions.c 

C_DEPS += \
./CMSIS/DSP/Source/ControllerFunctions/ControllerFunctions.d 

OBJS += \
./CMSIS/DSP/Source/ControllerFunctions/ControllerFunctions.o 


# Each subdirectory must supply rules for building sources it contributes
CMSIS/DSP/Source/ControllerFunctions/%.o: ../CMSIS/DSP/Source/ControllerFunctions/%.c CMSIS/DSP/Source/ControllerFunctions/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -D__REDLIB__ -DCPU_MCXN947VDF -DCPU_MCXN947VDF_cm33 -DCPU_MCXN947VDF_cm33_core0 -DSDK_OS_BAREMETAL -DSERIAL_PORT_TYPE_UART=1 -DSDK_DEBUGCONSOLE=0 -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=0 -DDISABLEFLOAT16 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -I"/home/santiagoalasia/Documentos/MCUXpresso_25.6.136/workspace/TP1/drivers" -I"/home/santiagoalasia/Documentos/MCUXpresso_25.6.136/workspace/TP1/device" -I"/home/santiagoalasia/Documentos/MCUXpresso_25.6.136/workspace/TP1/utilities/debug_console" -I"/home/santiagoalasia/Documentos/MCUXpresso_25.6.136/workspace/TP1/component/uart" -I"/home/santiagoalasia/Documentos/MCUXpresso_25.6.136/workspace/TP1/utilities/debug_console/config" -I"/home/santiagoalasia/Documentos/MCUXpresso_25.6.136/workspace/TP1/component/serial_manager" -I"/home/santiagoalasia/Documentos/MCUXpresso_25.6.136/workspace/TP1/component/lists" -I"/home/santiagoalasia/Documentos/MCUXpresso_25.6.136/workspace/TP1/device/periph" -I"/home/santiagoalasia/Documentos/MCUXpresso_25.6.136/workspace/TP1/utilities" -I"/home/santiagoalasia/Documentos/MCUXpresso_25.6.136/workspace/TP1/CMSIS" -I"/home/santiagoalasia/Documentos/MCUXpresso_25.6.136/workspace/TP1/CMSIS/m-profile" -I"/home/santiagoalasia/Documentos/MCUXpresso_25.6.136/workspace/TP1/utilities/str" -I"/home/santiagoalasia/Documentos/MCUXpresso_25.6.136/workspace/TP1/CMSIS/DSP/Include" -I"/home/santiagoalasia/Documentos/MCUXpresso_25.6.136/workspace/TP1/CMSIS/DSP/PrivateInclude" -I"/home/santiagoalasia/Documentos/MCUXpresso_25.6.136/workspace/TP1/CMSIS/DSP/Source/DistanceFunctions" -I"/home/santiagoalasia/Documentos/MCUXpresso_25.6.136/workspace/TP1/board" -I"/home/santiagoalasia/Documentos/MCUXpresso_25.6.136/workspace/TP1/source" -O0 -fno-common -g3 -gdwarf-4 -Wall -c -ffunction-sections -fdata-sections -fno-builtin -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-CMSIS-2f-DSP-2f-Source-2f-ControllerFunctions

clean-CMSIS-2f-DSP-2f-Source-2f-ControllerFunctions:
	-$(RM) ./CMSIS/DSP/Source/ControllerFunctions/ControllerFunctions.d ./CMSIS/DSP/Source/ControllerFunctions/ControllerFunctions.o

.PHONY: clean-CMSIS-2f-DSP-2f-Source-2f-ControllerFunctions


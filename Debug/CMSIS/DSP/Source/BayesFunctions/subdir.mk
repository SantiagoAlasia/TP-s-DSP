################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../CMSIS/DSP/Source/BayesFunctions/BayesFunctions.c \
../CMSIS/DSP/Source/BayesFunctions/BayesFunctionsF16.c 

C_DEPS += \
./CMSIS/DSP/Source/BayesFunctions/BayesFunctions.d \
./CMSIS/DSP/Source/BayesFunctions/BayesFunctionsF16.d 

OBJS += \
./CMSIS/DSP/Source/BayesFunctions/BayesFunctions.o \
./CMSIS/DSP/Source/BayesFunctions/BayesFunctionsF16.o 


# Each subdirectory must supply rules for building sources it contributes
CMSIS/DSP/Source/BayesFunctions/%.o: ../CMSIS/DSP/Source/BayesFunctions/%.c CMSIS/DSP/Source/BayesFunctions/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -D__REDLIB__ -DCPU_MCXN947VDF -DCPU_MCXN947VDF_cm33 -DCPU_MCXN947VDF_cm33_core0 -DSDK_OS_BAREMETAL -DSERIAL_PORT_TYPE_UART=1 -DSDK_DEBUGCONSOLE=0 -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=0 -DDISABLEFLOAT16 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -I"/home/santiagoalasia/Documentos/MCUXpresso_25.6.136/workspace/TP-s-DSP/drivers" -I"/home/santiagoalasia/Documentos/MCUXpresso_25.6.136/workspace/TP-s-DSP/device" -I"/home/santiagoalasia/Documentos/MCUXpresso_25.6.136/workspace/TP-s-DSP/utilities/debug_console" -I"/home/santiagoalasia/Documentos/MCUXpresso_25.6.136/workspace/TP-s-DSP/component/uart" -I"/home/santiagoalasia/Documentos/MCUXpresso_25.6.136/workspace/TP-s-DSP/utilities/debug_console/config" -I"/home/santiagoalasia/Documentos/MCUXpresso_25.6.136/workspace/TP-s-DSP/component/serial_manager" -I"/home/santiagoalasia/Documentos/MCUXpresso_25.6.136/workspace/TP-s-DSP/component/lists" -I"/home/santiagoalasia/Documentos/MCUXpresso_25.6.136/workspace/TP-s-DSP/device/periph" -I"/home/santiagoalasia/Documentos/MCUXpresso_25.6.136/workspace/TP-s-DSP/utilities" -I"/home/santiagoalasia/Documentos/MCUXpresso_25.6.136/workspace/TP-s-DSP/CMSIS" -I"/home/santiagoalasia/Documentos/MCUXpresso_25.6.136/workspace/TP-s-DSP/CMSIS/m-profile" -I"/home/santiagoalasia/Documentos/MCUXpresso_25.6.136/workspace/TP-s-DSP/utilities/str" -I"/home/santiagoalasia/Documentos/MCUXpresso_25.6.136/workspace/TP-s-DSP/CMSIS/DSP/Include" -I"/home/santiagoalasia/Documentos/MCUXpresso_25.6.136/workspace/TP-s-DSP/CMSIS/DSP/PrivateInclude" -I"/home/santiagoalasia/Documentos/MCUXpresso_25.6.136/workspace/TP-s-DSP/CMSIS/DSP/Source/DistanceFunctions" -I"/home/santiagoalasia/Documentos/MCUXpresso_25.6.136/workspace/TP-s-DSP/board" -I"/home/santiagoalasia/Documentos/MCUXpresso_25.6.136/workspace/TP-s-DSP/source" -O0 -fno-common -g3 -gdwarf-4 -Wall -c -ffunction-sections -fdata-sections -fno-builtin -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-CMSIS-2f-DSP-2f-Source-2f-BayesFunctions

clean-CMSIS-2f-DSP-2f-Source-2f-BayesFunctions:
	-$(RM) ./CMSIS/DSP/Source/BayesFunctions/BayesFunctions.d ./CMSIS/DSP/Source/BayesFunctions/BayesFunctions.o ./CMSIS/DSP/Source/BayesFunctions/BayesFunctionsF16.d ./CMSIS/DSP/Source/BayesFunctions/BayesFunctionsF16.o

.PHONY: clean-CMSIS-2f-DSP-2f-Source-2f-BayesFunctions


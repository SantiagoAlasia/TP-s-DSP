################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../source/main.c \
../source/semihost_hardfault.c 

C_DEPS += \
./source/main.d \
./source/semihost_hardfault.d 

OBJS += \
./source/main.o \
./source/semihost_hardfault.o 


# Each subdirectory must supply rules for building sources it contributes
source/%.o: ../source/%.c source/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -std=gnu99 -D__REDLIB__ -DMCUXPRESSO_SDK -DCPU_MCXN947VDF_cm33_core1 -D__USE_SHMEM -DMCMGR_HANDLE_EXCEPTIONS -D__SEMIHOST_HARDFAULT_DISABLE -DSDK_DEBUGCONSOLE=1 -DMCUX_META_BUILD -DMCXN947_cm33_core1_SERIES -DMULTICORE_APP=1 -DCPU_MCXN947VDF -DCPU_MCXN947VDF_cm33_nodsp -DCR_INTEGER_PRINTF -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -D__MULTICORE_M33SLAVE -I"/home/santiagoalasia/Documentos/UNC/ProcesamientoDigitalDeSeñales/TP-s-DSP/frdmmcxn947/multicore_manager_secondary_core/source" -I/home/santiagoalasia/Documentos/UNC/ProcesamientoDigitalDeSeñales/TP-s-DSP/frdmmcxn947/shared -I"/home/santiagoalasia/Documentos/UNC/ProcesamientoDigitalDeSeñales/TP-s-DSP/frdmmcxn947/multicore_manager_secondary_core/drivers" -I"/home/santiagoalasia/Documentos/UNC/ProcesamientoDigitalDeSeñales/TP-s-DSP/frdmmcxn947/multicore_manager_secondary_core/CMSIS" -I"/home/santiagoalasia/Documentos/UNC/ProcesamientoDigitalDeSeñales/TP-s-DSP/frdmmcxn947/multicore_manager_secondary_core/CMSIS/m-profile" -I"/home/santiagoalasia/Documentos/UNC/ProcesamientoDigitalDeSeñales/TP-s-DSP/frdmmcxn947/multicore_manager_secondary_core/device" -I"/home/santiagoalasia/Documentos/UNC/ProcesamientoDigitalDeSeñales/TP-s-DSP/frdmmcxn947/multicore_manager_secondary_core/device/periph" -I"/home/santiagoalasia/Documentos/UNC/ProcesamientoDigitalDeSeñales/TP-s-DSP/frdmmcxn947/multicore_manager_secondary_core/utilities" -I"/home/santiagoalasia/Documentos/UNC/ProcesamientoDigitalDeSeñales/TP-s-DSP/frdmmcxn947/multicore_manager_secondary_core/utilities/str" -I"/home/santiagoalasia/Documentos/UNC/ProcesamientoDigitalDeSeñales/TP-s-DSP/frdmmcxn947/multicore_manager_secondary_core/utilities/debug_console_lite" -I"/home/santiagoalasia/Documentos/UNC/ProcesamientoDigitalDeSeñales/TP-s-DSP/frdmmcxn947/multicore_manager_secondary_core/component/uart" -I"/home/santiagoalasia/Documentos/UNC/ProcesamientoDigitalDeSeñales/TP-s-DSP/frdmmcxn947/multicore_manager_secondary_core/mcmgr/mcmgr/inc" -I"/home/santiagoalasia/Documentos/UNC/ProcesamientoDigitalDeSeñales/TP-s-DSP/frdmmcxn947/multicore_manager_secondary_core/mcmgr/mcmgr/inc/platform/mcxnx4x" -I"/home/santiagoalasia/Documentos/UNC/ProcesamientoDigitalDeSeñales/TP-s-DSP/frdmmcxn947/multicore_manager_secondary_core/board" -O0 -fno-common -g3 -gdwarf-4 -mcpu=cortex-m33+nodsp -c -ffunction-sections -fdata-sections -fno-builtin -imacros "/home/santiagoalasia/Documentos/UNC/ProcesamientoDigitalDeSeñales/TP-s-DSP/frdmmcxn947/multicore_manager_secondary_core/source/mcux_config.h" -imacros "/home/santiagoalasia/Documentos/UNC/ProcesamientoDigitalDeSeñales/TP-s-DSP/frdmmcxn947/multicore_manager_secondary_core/source/mcuxsdk_version.h" -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m33+nodsp -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-source

clean-source:
	-$(RM) ./source/main.d ./source/main.o ./source/semihost_hardfault.d ./source/semihost_hardfault.o

.PHONY: clean-source


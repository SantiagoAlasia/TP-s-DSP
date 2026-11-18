################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../startup/boot_multicore_slave.c \
../startup/startup_mcxn947_cm33_core0.c 

C_DEPS += \
./startup/boot_multicore_slave.d \
./startup/startup_mcxn947_cm33_core0.d 

OBJS += \
./startup/boot_multicore_slave.o \
./startup/startup_mcxn947_cm33_core0.o 


# Each subdirectory must supply rules for building sources it contributes
startup/%.o: ../startup/%.c startup/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -std=gnu99 -D__REDLIB__ -DMCUXPRESSO_SDK -DCPU_MCXN947VDF_cm33_core0 -D__USE_SHMEM -DMCMGR_HANDLE_EXCEPTIONS -D__SEMIHOST_HARDFAULT_DISABLE -DSDK_DEBUGCONSOLE=1 -DMCUX_META_BUILD -DMCXN947_cm33_core0_SERIES -DMULTICORE_APP=1 -DCPU_MCXN947VDF -DCPU_MCXN947VDF_cm33 -DCR_INTEGER_PRINTF -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -D__MULTICORE_MASTER -D__MULTICORE_MASTER_SLAVE_M33SLAVE -I"/home/santiagoalasia/Documentos/UNC/ProcesamientoDigitalDeSeñales/TP-s-DSP/frdmmcxn947/multicore_manager_primary_core/source" -I/home/santiagoalasia/Documentos/UNC/ProcesamientoDigitalDeSeñales/TP-s-DSP/frdmmcxn947/shared -I"/home/santiagoalasia/Documentos/UNC/ProcesamientoDigitalDeSeñales/TP-s-DSP/frdmmcxn947/multicore_manager_primary_core/drivers" -I"/home/santiagoalasia/Documentos/UNC/ProcesamientoDigitalDeSeñales/TP-s-DSP/frdmmcxn947/multicore_manager_primary_core/CMSIS" -I"/home/santiagoalasia/Documentos/UNC/ProcesamientoDigitalDeSeñales/TP-s-DSP/frdmmcxn947/multicore_manager_primary_core/CMSIS/m-profile" -I"/home/santiagoalasia/Documentos/UNC/ProcesamientoDigitalDeSeñales/TP-s-DSP/frdmmcxn947/multicore_manager_primary_core/device" -I"/home/santiagoalasia/Documentos/UNC/ProcesamientoDigitalDeSeñales/TP-s-DSP/frdmmcxn947/multicore_manager_primary_core/device/periph" -I"/home/santiagoalasia/Documentos/UNC/ProcesamientoDigitalDeSeñales/TP-s-DSP/frdmmcxn947/multicore_manager_primary_core/utilities" -I"/home/santiagoalasia/Documentos/UNC/ProcesamientoDigitalDeSeñales/TP-s-DSP/frdmmcxn947/multicore_manager_primary_core/utilities/str" -I"/home/santiagoalasia/Documentos/UNC/ProcesamientoDigitalDeSeñales/TP-s-DSP/frdmmcxn947/multicore_manager_primary_core/utilities/debug_console_lite" -I"/home/santiagoalasia/Documentos/UNC/ProcesamientoDigitalDeSeñales/TP-s-DSP/frdmmcxn947/multicore_manager_primary_core/component/uart" -I"/home/santiagoalasia/Documentos/UNC/ProcesamientoDigitalDeSeñales/TP-s-DSP/frdmmcxn947/multicore_manager_primary_core/mcmgr/mcmgr/inc" -I"/home/santiagoalasia/Documentos/UNC/ProcesamientoDigitalDeSeñales/TP-s-DSP/frdmmcxn947/multicore_manager_primary_core/mcmgr/mcmgr/inc/platform/mcxnx4x" -I"/home/santiagoalasia/Documentos/UNC/ProcesamientoDigitalDeSeñales/TP-s-DSP/frdmmcxn947/multicore_manager_primary_core/board" -O0 -fno-common -g3 -gdwarf-4 -mcpu=cortex-m33 -c -ffunction-sections -fdata-sections -fno-builtin -imacros "/home/santiagoalasia/Documentos/UNC/ProcesamientoDigitalDeSeñales/TP-s-DSP/frdmmcxn947/multicore_manager_primary_core/source/mcux_config.h" -imacros "/home/santiagoalasia/Documentos/UNC/ProcesamientoDigitalDeSeñales/TP-s-DSP/frdmmcxn947/multicore_manager_primary_core/source/mcuxsdk_version.h" -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-startup

clean-startup:
	-$(RM) ./startup/boot_multicore_slave.d ./startup/boot_multicore_slave.o ./startup/startup_mcxn947_cm33_core0.d ./startup/startup_mcxn947_cm33_core0.o

.PHONY: clean-startup


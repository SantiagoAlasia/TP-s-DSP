################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include utilities/str/subdir.mk
-include utilities/debug_console_lite/subdir.mk
-include utilities/subdir.mk
-include startup/subdir.mk
-include source/subdir.mk
-include mcmgr/mcmgr/src/subdir.mk
-include drivers/subdir.mk
-include device/subdir.mk
-include component/uart/subdir.mk
-include board/subdir.mk
ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

OPTIONAL_TOOL_DEPS := \
$(wildcard ../makefile.defs) \
$(wildcard ../makefile.init) \
$(wildcard ../makefile.targets) \


BUILD_ARTIFACT_NAME := frdmmcxn947_multicore_manager_secondary_core
BUILD_ARTIFACT_EXTENSION := axf
BUILD_ARTIFACT_PREFIX :=
BUILD_ARTIFACT := $(BUILD_ARTIFACT_PREFIX)$(BUILD_ARTIFACT_NAME)$(if $(BUILD_ARTIFACT_EXTENSION),.$(BUILD_ARTIFACT_EXTENSION),)

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all:
	+@$(MAKE) --no-print-directory main-build && $(MAKE) --no-print-directory post-build

# Main-build Target
main-build: frdmmcxn947_multicore_manager_secondary_core.axf

# Tool invocations
frdmmcxn947_multicore_manager_secondary_core.axf: $(OBJS) $(USER_OBJS) makefile $(OPTIONAL_TOOL_DEPS)
	@echo 'Building target: $@'
	@echo 'Invoking: MCU Linker'
	arm-none-eabi-gcc -nostdlib -Xlinker --defsym=__use_shmem__=1 -Xlinker --defsym=__multicore__=1 -Xlinker -no-warn-rwx-segments -Xlinker -Map="frdmmcxn947_multicore_manager_secondary_core.map" -Xlinker --gc-sections -Xlinker -print-memory-usage -Xlinker --sort-section=alignment -Xlinker --cref -mcpu=cortex-m33+nodsp -mthumb -T frdmmcxn947_multicore_manager_secondary_core_Debug.ld -o "frdmmcxn947_multicore_manager_secondary_core.axf" $(OBJS) $(USER_OBJS) $(LIBS) && arm-none-eabi-objcopy --target elf32-littlearm --verbose --strip-all --remove-section=.ARM.attributes --redefine-sym __vectors_start__=__vectors_start___core_m33slave --keep-symbol __vectors_start___core_m33slave --redefine-sym _data=__start_data_core_m33slave --keep-symbol __start_data_core_m33slave --redefine-sym _edata=__end_data_core_m33slave --keep-symbol __end_data_core_m33slave --remove-section=".bss*" --remove-section=".noinit*" --rename-section .ARM.exidx=".core_m33slave.ARM.exidx" --rename-section .ARM.extab=".core_m33slave.ARM.extab" --rename-section .text=".core_m33slave" --rename-section .data=".core_m33slave.data" --rename-section .data_RAM2=".core_m33slave.data_RAM2" "frdmmcxn947_multicore_manager_secondary_core.axf" "frdmmcxn947_multicore_manager_secondary_core.axf.o" && mcux-fixelf "frdmmcxn947_multicore_manager_secondary_core.axf.o"
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) frdmmcxn947_multicore_manager_secondary_core.axf
	-@echo ' '

post-build:
	-@echo 'Performing post-build steps'
	-arm-none-eabi-size "frdmmcxn947_multicore_manager_secondary_core.axf"; # arm-none-eabi-objcopy -v -O binary "frdmmcxn947_multicore_manager_secondary_core.axf" "frdmmcxn947_multicore_manager_secondary_core.bin" ; # checksum -p MCXN947 -d "frdmmcxn947_multicore_manager_secondary_core.bin";
	-@echo ' '

.PHONY: all clean dependents main-build post-build

-include ../makefile.targets

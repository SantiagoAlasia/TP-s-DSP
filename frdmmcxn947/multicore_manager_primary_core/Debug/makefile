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


BUILD_ARTIFACT_NAME := frdmmcxn947_multicore_manager_primary_core
BUILD_ARTIFACT_EXTENSION := axf
BUILD_ARTIFACT_PREFIX :=
BUILD_ARTIFACT := $(BUILD_ARTIFACT_PREFIX)$(BUILD_ARTIFACT_NAME)$(if $(BUILD_ARTIFACT_EXTENSION),.$(BUILD_ARTIFACT_EXTENSION),)

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all:
	+@$(MAKE) --no-print-directory main-build && $(MAKE) --no-print-directory post-build

# Main-build Target
main-build: frdmmcxn947_multicore_manager_primary_core.axf

# Tool invocations
frdmmcxn947_multicore_manager_primary_core.axf: $(OBJS) $(USER_OBJS) makefile $(OPTIONAL_TOOL_DEPS) /home/santiagoalasia/Documentos/UNC/ProcesamientoDigitalDeSeñales/TP-s-DSP/frdmmcxn947/multicore_manager_secondary_core/Debug/frdmmcxn947_multicore_manager_secondary_core.axf.o
	@echo 'Building target: $@'
	@echo 'Invoking: MCU Linker'
	arm-none-eabi-gcc -nostdlib -Xlinker --defsym=__use_shmem__=1 -Xlinker --defsym=__multicore__=1 -Xlinker -no-warn-rwx-segments -Xlinker -Map="frdmmcxn947_multicore_manager_primary_core.map" -Xlinker --gc-sections -Xlinker -print-memory-usage -Xlinker --sort-section=alignment -Xlinker --cref -mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -T frdmmcxn947_multicore_manager_primary_core_Debug.ld -o "frdmmcxn947_multicore_manager_primary_core.axf" $(OBJS) $(USER_OBJS) $(LIBS) "/home/santiagoalasia/Documentos/UNC/ProcesamientoDigitalDeSeñales/TP-s-DSP/frdmmcxn947/multicore_manager_secondary_core/Debug/frdmmcxn947_multicore_manager_secondary_core.axf.o"
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) frdmmcxn947_multicore_manager_primary_core.axf
	-@echo ' '

post-build:
	-@echo 'Performing post-build steps'
	-arm-none-eabi-size "frdmmcxn947_multicore_manager_primary_core.axf"; # arm-none-eabi-objcopy -v -O binary "frdmmcxn947_multicore_manager_primary_core.axf" "frdmmcxn947_multicore_manager_primary_core.bin" ; # checksum -p MCXN947 -d "frdmmcxn947_multicore_manager_primary_core.bin";
	-@echo ' '

.PHONY: all clean dependents main-build post-build

-include ../makefile.targets

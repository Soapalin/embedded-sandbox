################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Lab0/.metadata/.plugins/org.eclipse.cdt.make.core/specs.cpp 

C_SRCS += \
../Lab0/.metadata/.plugins/org.eclipse.cdt.make.core/specs.c 

OBJS += \
./Lab0/.metadata/.plugins/org.eclipse.cdt.make.core/specs.o 

C_DEPS += \
./Lab0/.metadata/.plugins/org.eclipse.cdt.make.core/specs.d 

CPP_DEPS += \
./Lab0/.metadata/.plugins/org.eclipse.cdt.make.core/specs.d 


# Each subdirectory must supply rules for building sources it contributes
Lab0/.metadata/.plugins/org.eclipse.cdt.make.core/%.o: ../Lab0/.metadata/.plugins/org.eclipse.cdt.make.core/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections  -g3 -I"/home/angus/Documents/UTS/Year 3/AUTUMN/Embedded Software/Software/Lab0/Library" -I"/home/angus/Documents/UTS/Year 3/AUTUMN/Embedded Software/Software/Lab0/Static_Code/IO_Map" -I"/home/angus/Documents/UTS/Year 3/AUTUMN/Embedded Software/Software/Lab0/Sources" -I"/home/angus/Documents/UTS/Year 3/AUTUMN/Embedded Software/Software/Lab0/Generated_Code" -I"/home/angus/Documents/UTS/Year 3/AUTUMN/Embedded Software/Software/Lab0/Static_Code/PDD" -std=c99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Lab0/.metadata/.plugins/org.eclipse.cdt.make.core/%.o: ../Lab0/.metadata/.plugins/org.eclipse.cdt.make.core/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C++ Compiler'
	arm-none-eabi-g++ -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections  -g3 -I"/home/angus/Documents/UTS/Year 3/AUTUMN/Embedded Software/Software/Lab0/Static_Code/IO_Map" -I"/home/angus/Documents/UTS/Year 3/AUTUMN/Embedded Software/Software/Lab0/Sources" -I"/home/angus/Documents/UTS/Year 3/AUTUMN/Embedded Software/Software/Lab0/Generated_Code" -I"/home/angus/Documents/UTS/Year 3/AUTUMN/Embedded Software/Software/Lab0/Static_Code/PDD" -std=gnu++11 -fabi-version=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



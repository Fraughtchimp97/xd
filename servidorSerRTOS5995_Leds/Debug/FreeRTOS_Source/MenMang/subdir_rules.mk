################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
FreeRTOS_Source/MenMang/%.obj: ../FreeRTOS_Source/MenMang/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: MSP430 Compiler'
	"/home/andres/ti/ccs1120/ccs/tools/compiler/ti-cgt-msp430_21.6.0.LTS/bin/cl430" -vmspx --code_model=large --data_model=small --use_hw_mpy=F5 --include_path="/home/andres/ti/ccs1120/ccs/ccs_base/msp430/include" --include_path="/home/andres/ti/ccs1120/ccs/tools/compiler/ti-cgt-msp430_21.6.0.LTS/include" --include_path="/home/andres/workspace_v11/servidorSerRTOS5995_Leds" --include_path="/home/andres/workspace_v11/servidorSerRTOS5995_Leds/FreeRTOS_Source/portable/CCS/MSP430X" --include_path="/home/andres/workspace_v11/servidorSerRTOS5995_Leds/FreeRTOS_Source/include" --include_path="/home/andres/workspace_v11/servidorSerRTOS5995_Leds/driverlib/MSP430FR5xx_6xx" --advice:power="all" --advice:hw_config=all --define=__MSP430FR5994__ --define=_MPU_ENABLE -g --printf_support=full --diag_warning=225 --diag_wrap=off --display_error_number --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU40 --preproc_with_compile --preproc_dependency="FreeRTOS_Source/MenMang/$(basename $(<F)).d_raw" --obj_directory="FreeRTOS_Source/MenMang" $(GEN_OPTS__FLAG) "$(shell echo $<)"
	@echo 'Finished building: "$<"'
	@echo ' '



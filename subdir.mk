################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Drivers/BSP/EEPRMA2/eeprma2_m24.c 

OBJS += \
./Drivers/BSP/EEPRMA2/eeprma2_m24.o 

C_DEPS += \
./Drivers/BSP/EEPRMA2/eeprma2_m24.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/BSP/EEPRMA2/%.o Drivers/BSP/EEPRMA2/%.su Drivers/BSP/EEPRMA2/%.cyclo: ../Drivers/BSP/EEPRMA2/%.c Drivers/BSP/EEPRMA2/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG_ONE_BOARD_COMPILER_FLAG=0 -DDEBUG -DSAQ_GEN2 -DUSE_HAL_DRIVER -DSTM32H743xx -DTRACEALYZER=1 -DUSE_PWR_LDO_SUPPLY -c -I../Core/Inc -I../Middlewares/Third_Party/TraceRecorder/streamports/Jlink_RTT/include -I../Middlewares/Third_Party/TraceRecorder/streamports/Jlink_RTT/config -I../../SAQ01_FW_COMMON/lib/inc -I../Middlewares/Third_Party/TraceRecorder/include -I../Middlewares/Third_Party/TraceRecorder/config -I../Middlewares/Third_Party/TraceRecorder/kernelports/FreeRTOS/config -I../Middlewares/Third_Party/TraceRecorder/kernelports/FreeRTOS/include -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../LWIP/App -I../LWIP/Target -I../Middlewares/Third_Party/LwIP/src/include -I../Middlewares/Third_Party/LwIP/system -I../Drivers/BSP/Components/lan8742 -I../Middlewares/Third_Party/LwIP/src/include/netif/ppp -I../Middlewares/Third_Party/LwIP/src/include/lwip -I../Middlewares/Third_Party/LwIP/src/include/lwip/apps -I../Middlewares/Third_Party/LwIP/src/include/lwip/priv -I../Middlewares/Third_Party/LwIP/src/include/lwip/prot -I../Middlewares/Third_Party/LwIP/src/include/netif -I../Middlewares/Third_Party/LwIP/src/include/compat/posix -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/arpa -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/net -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/sys -I../Middlewares/Third_Party/LwIP/src/include/compat/stdc -I../Middlewares/Third_Party/LwIP/system/arch -I../Drivers/BSP/Components/M24xx -I../EEPROM/Target -I../Drivers/BSP/Components/M95xx -I../Drivers/BSP/EEPRMA2 -I../../SAQ01_FW_COMMON/inc -I../../SAQ01_FW_COMMON/nuvc/inc -I../../SAQ01_FW_MB/inc -I../../SAQ01_FW_MB/lib/inc -Og -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-BSP-2f-EEPRMA2

clean-Drivers-2f-BSP-2f-EEPRMA2:
	-$(RM) ./Drivers/BSP/EEPRMA2/eeprma2_m24.cyclo ./Drivers/BSP/EEPRMA2/eeprma2_m24.d ./Drivers/BSP/EEPRMA2/eeprma2_m24.o ./Drivers/BSP/EEPRMA2/eeprma2_m24.su

.PHONY: clean-Drivers-2f-BSP-2f-EEPRMA2


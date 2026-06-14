# VectorLink 2.4G Controller

基于 STM32F103C8T6 的 2.4 GHz 遥控器固件工程。工程使用 STM32 HAL、FreeRTOS、C/C++，由 STM32CubeMX 生成底层初始化代码，并使用 EIDE + Arm Compiler 6 构建。

## 当前状态

- STM32CubeMX 外设与时钟配置已完成。
- EIDE 工程已配置并成功编译。
- 嘉立创网表与 `.ioc`/生成代码的 MCU 引脚已交叉核对。
- 遥控器 V1 应用层、设备驱动和无线协议已实现并通过离线编译与主机测试。
- 尚未进行实物硬件、电气及时序验证。

## 硬件概览

- MCU：STM32F103C8T6，72 MHz
- RTOS：FreeRTOS，应用层计划使用原生 API
- 射频：NRF24L01P-R + AT2401C
- 显示：0.96 英寸 128x64 I2C OLED
- 输入：双摇杆四路 ADC、12 个低有效按键
- 调试通信：SWD、CH340N USART1
- 电源：单节锂电池、USB Type-C 充电、3.3 V LDO

## 工程入口

- CubeMX 配置：[VectorLink_2_4G_Controller.ioc](VectorLink_2_4G_Controller.ioc)
- EIDE 工程：[MDK-ARM/VectorLink_2_4G_Controller.code-workspace](MDK-ARM/VectorLink_2_4G_Controller.code-workspace)
- Keil 工程：[MDK-ARM/VectorLink_2_4G_Controller.uvprojx](MDK-ARM/VectorLink_2_4G_Controller.uvprojx)
- CubeMX 指南：[docs/STM32CubeMX配置指南.md](docs/STM32CubeMX配置指南.md)
- 硬件引脚真值表：[docs/HARDWARE_PINOUT.md](docs/HARDWARE_PINOUT.md)
- 移植与开发准备：[docs/PORTING_AND_DEVELOPMENT.md](docs/PORTING_AND_DEVELOPMENT.md)
- 固件架构：[docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)
- 无线协议：[docs/RADIO_PROTOCOL.md](docs/RADIO_PROTOCOL.md)
- 开发进度：[docs/DEVELOPMENT_STATUS.md](docs/DEVELOPMENT_STATUS.md)
- 上板检查：[docs/HARDWARE_BRINGUP.md](docs/HARDWARE_BRINGUP.md)
- 代码规范：[docs/CODING_STYLE.md](docs/CODING_STYLE.md)
- 源码阅读指南：[docs/READING_GUIDE.md](docs/READING_GUIDE.md)
- 分阶段学习练习：[docs/learning/EXERCISES.md](docs/learning/EXERCISES.md)

## 构建

1. 在 EIDE 中打开 `MDK-ARM/VectorLink_2_4G_Controller.code-workspace`。
2. 选择 `VectorLink_2_4G_Controller` target。
3. 使用 Arm Compiler 6 构建。

主机逻辑测试：

```powershell
cmake -S Tests -B Tests/build -G "MinGW Makefiles"
cmake --build Tests/build
ctest --test-dir Tests/build --output-on-failure
```

`build/`、Keil pack 和中间文件均不纳入版本控制。最新 Flash/RAM 使用量记录在开发进度文档中。

## CubeMX 再生成约束

- HAL timebase 使用 TIM2，FreeRTOS 使用 SysTick。
- 保留 SWD，禁用完整 JTAG，以释放 PB3/PB4。
- 生成代码后必须复核 ADC 五通道顺序和 NRF IRQ 下降沿。
- 用户代码只能写入 CubeMX 的 `USER CODE` 区域，或放入后续独立的 C++ 应用目录。

## License

本项目使用 [MIT License](LICENSE)。STM32 HAL、CMSIS 和 FreeRTOS 保留各自上游许可证。

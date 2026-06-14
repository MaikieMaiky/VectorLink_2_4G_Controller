# VectorLink 2.4G Controller - STM32CubeMX 配置指南

## 1. 工程基础设置

- MCU：`STM32F103C8T6`
- Package：`LQFP48`
- Project Name：建议 `VectorLink_2_4G_Controller`
- Toolchain / IDE：`MDK-ARM V5`
- Firmware Package：选择本机已安装的最新稳定 `STM32Cube FW_F1`
- `Project Manager > Code Generator`：
  - 勾选 `Keep User Code when re-generating`
  - 勾选 `Generate peripheral initialization as a pair of '.c/.h' files per peripheral`
  - 勾选 `Delete previously generated files when not re-generated`

## 2. SYS、RCC 与时钟树

### SYS

- `Debug`：`Serial Wire`
- `Timebase Source`：`TIM2`

只能选择 Serial Wire，不能启用完整 JTAG，因为 PB3、PB4 用于 K9、K10。

### RCC

- `High Speed Clock (HSE)`：`Crystal/Ceramic Resonator`
- 外部晶振频率：`8 MHz`

### Clock Configuration

- PLL Source：`HSE`
- PLL Multiplier：`x9`
- System Clock Mux：`PLLCLK`
- SYSCLK：`72 MHz`
- AHB Prescaler：`/1`，HCLK = `72 MHz`
- APB1 Prescaler：`/2`，PCLK1 = `36 MHz`
- APB2 Prescaler：`/1`，PCLK2 = `72 MHz`
- ADC Prescaler：`/6`，ADC clock = `12 MHz`

## 3. 完整引脚表

| 功能 | 网络名 | 引脚 | CubeMX 模式 | GPIO Label / 备注 |
|---|---|---:|---|---|
| 左摇杆水平 | LH | PA0 | ADC1_IN0 | `JOY_LH` |
| 左摇杆垂直 | LV | PA1 | ADC1_IN1 | `JOY_LV` |
| 右摇杆水平 | RH | PA2 | ADC1_IN2 | `JOY_RH` |
| 右摇杆垂直 | RV | PA3 | ADC1_IN3 | `JOY_RV` |
| 电池检测 | BATFD | PA4 | ADC1_IN4 | `BAT_SENSE` |
| 按键 1 | K1 | PA6 | GPIO_Input | Pull-up，`KEY1` |
| 按键 2 | K2 | PA7 | GPIO_Input | Pull-up，`KEY2` |
| NRF24 IRQ | IRQ | PA8 | GPIO_EXTI8 | Falling edge，No pull，`NRF_IRQ` |
| 串口发送 | UART_TX | PA9 | USART1_TX | `UART_TX` |
| 串口接收 | UART_RX | PA10 | USART1_RX | `UART_RX` |
| 按键 7 | K7 | PA11 | GPIO_Input | Pull-up，`KEY7` |
| 按键 8 | K8 | PA12 | GPIO_Input | Pull-up，`KEY8` |
| SWD 数据 | SWDIO | PA13 | SYS_JTMS-SWDIO | 调试口 |
| SWD 时钟 | SWCLK | PA14 | SYS_JTCK-SWCLK | 调试口 |
| 按键 3 | K3 | PB0 | GPIO_Input | Pull-up，`KEY3` |
| 按键 4 | K4 | PB1 | GPIO_Input | Pull-up，`KEY4` |
| 按键 5 | K5 | PB2 | GPIO_Input | Pull-up，`KEY5` |
| 按键 9 | K9 | PB3 | GPIO_Input | Pull-up，`KEY9` |
| 按键 10 | K10 | PB4 | GPIO_Input | Pull-up，`KEY10` |
| 按键 11 | K11 | PB5 | GPIO_Input | Pull-up，`KEY11` |
| 按键 12 | K12 | PB6 | GPIO_Input | Pull-up，`KEY12` |
| 无源蜂鸣器 | BUZZ | PB7 | TIM4_CH2 | PWM |
| OLED SCL | SCL | PB8 | I2C1_SCL | 必须启用 I2C1 Remap |
| OLED SDA | SDA | PB9 | I2C1_SDA | 必须启用 I2C1 Remap |
| 按键 6 | K6 | PB10 | GPIO_Input | Pull-up，`KEY6` |
| NRF24 CE | CE | PB11 | GPIO_Output | Push-pull，初始 Low，`NRF_CE` |
| NRF24 CSN | CSN | PB12 | GPIO_Output | Push-pull，初始 High，`NRF_CSN` |
| NRF24 SCK | SCK | PB13 | SPI2_SCK | `NRF_SCK` |
| NRF24 MISO | MISO | PB14 | SPI2_MISO | `NRF_MISO` |
| NRF24 MOSI | MOSI | PB15 | SPI2_MOSI | `NRF_MOSI` |
| 晶振输入 | OSC_IN | PD0 | RCC_OSC_IN | HSE |
| 晶振输出 | OSC_OUT | PD1 | RCC_OSC_OUT | HSE |

所有按键均为低电平按下，暂不配置按键 EXTI，后续由 FreeRTOS 任务周期扫描和消抖。

## 4. ADC1 与 DMA

### ADC1 Parameter Settings

- Scan Conversion Mode：`Enabled`
- Continuous Conversion Mode：`Enabled`
- Discontinuous Conversion Mode：`Disabled`
- External Trigger Conversion Source：`Software Start`
- Data Alignment：`Right alignment`
- Number Of Conversion：`5`

规则通道顺序：

| Rank | Channel | 引脚 | Sampling Time |
|---:|---|---:|---|
| 1 | IN0 | PA0 | `55.5 Cycles` |
| 2 | IN1 | PA1 | `55.5 Cycles` |
| 3 | IN2 | PA2 | `55.5 Cycles` |
| 4 | IN3 | PA3 | `55.5 Cycles` |
| 5 | IN4 | PA4 | `239.5 Cycles` |

### ADC1 DMA Settings

- DMA Request：`ADC1`
- Channel：`DMA1 Channel1`
- Direction：`Peripheral to Memory`
- Priority：`Low` 或 `Medium`
- Mode：`Circular`
- Peripheral Increment：`Disabled`
- Memory Increment：`Enabled`
- Peripheral Data Width：`Half Word`
- Memory Data Width：`Half Word`

启用 `DMA1 Channel1 global interrupt`，抢占优先级设为 `5`。

## 5. SPI2 - NRF24L01

- Mode：`Full-Duplex Master`
- Hardware NSS Signal：`Disable` / Software NSS
- Frame Format：`Motorola`
- Data Size：`8 Bits`
- First Bit：`MSB First`
- Clock Polarity：`Low`
- Clock Phase：`1 Edge`
- Baud Rate Prescaler：`16`
- CRC Calculation：`Disabled`

SPI2 位于 APB1，PCLK1 为 36 MHz，因此初始 SPI 时钟为 `2.25 MHz`。CSN 由 PB12 软件控制，务必将默认输出电平设为 High。

## 6. I2C1 - OLED

- Mode：`I2C`
- I2C Speed Mode：`Fast Mode`
- Clock Speed：`400000 Hz`
- Duty Cycle：`2`
- Addressing Mode：`7-bit`
- Dual Address、General Call、Clock Stretching：保持默认
- 在引脚或 AFIO 设置中启用 `I2C1 Remap`，确认最终使用 PB8/PB9，而不是 PB6/PB7

原理图需要在 SCL、SDA 上各增加一个 `4.7 kOhm` 外部上拉到 3.3 V。若后续屏幕通信不稳定，再将速度降至 100 kHz。

## 7. USART1 - CH340N

- Mode：`Asynchronous`
- Baud Rate：`115200 Bits/s`
- Word Length：`8 Bits`
- Parity：`None`
- Stop Bits：`1`
- Data Direction：`Receive and Transmit`
- Hardware Flow Control：`None`
- Oversampling：`16`
- 暂不启用 USART1 DMA 和全局中断

制板前必须修正原理图交叉接线：

- CH340N `TXD` -> STM32 `PA10 / USART1_RX`
- CH340N `RXD` -> STM32 `PA9 / USART1_TX`

## 8. TIM4 PWM - 无源蜂鸣器

- TIM4 Clock Source：`Internal Clock`
- Channel 2：`PWM Generation CH2`
- Prescaler：`71`
- Counter Period：`416 - 1`，即填写 `415`
- Counter Mode：`Up`
- Auto-reload Preload：`Enable`
- PWM Pulse：`208`
- PWM Polarity：`High`

TIM4 输入时钟为 72 MHz，分频后计数频率 1 MHz；周期 416 对应约 `2403.8 Hz`，占空比约 50%。启动时是否发声由后续软件决定，生成后不要在 `main()` 中自动启动 PWM。

## 9. NRF24 IRQ 与 NVIC

- PA8：`GPIO_EXTI8`
- Trigger：`Falling edge`
- Pull：`No pull`
- 开启 `EXTI line[9:5] interrupts`
- 抢占优先级：`5`
- 子优先级：`0`
- Priority Group：`NVIC_PRIORITYGROUP_4`

中断回调中只做通知或置位，后续由任务处理 SPI 通信。

## 10. FreeRTOS

- Middleware：启用 `FreeRTOS`
- Interface：选择 `CMSIS_V2`
- `CMSIS_V2` 仅作为 CubeMX 的 FreeRTOS 生成接口，不作为本项目应用层 API
- 应用层统一直接包含 `FreeRTOS.h`、`task.h`、`queue.h`、`semphr.h`、`event_groups.h` 等头文件，并使用 FreeRTOS 原生 API
- 任务、队列、互斥锁和事件组分别使用 `xTaskCreate`、`xQueueCreate`、`xSemaphoreCreateMutex` 和 `xEventGroupCreate` 等原生接口
- 不在自行编写的应用代码中使用 `osThreadNew`、`osMessageQueueNew`、`osMutexNew` 等 CMSIS-RTOS API
- Memory Management：`heap_4`
- `TOTAL_HEAP_SIZE`：初始 `8192 Bytes`
- Tick Rate：`1000 Hz`
- `USE_PREEMPTION`：Enabled
- 仅保留 CubeMX 自动生成的默认任务；下一阶段将评估是否移除默认 CMSIS 任务，并改由用户代码通过 `xTaskCreate` 创建全部应用任务
- 默认任务 Stack Size：先使用 CubeMX 默认最小值；生成后以栈高水位实测结果调整
- 不在本阶段创建输入、通信或显示任务

确认 FreeRTOS 使用 SysTick，而 HAL timebase 使用 TIM2。SysTick/PendSV 保持最低优先级；允许调用 FreeRTOS `FromISR` API 的外设中断使用数值 `5` 或更大。

### 最小栈调试与余量监控

在 `FreeRTOS > Config parameters` 中设置：

- `CHECK_FOR_STACK_OVERFLOW`：`2`
- `USE_TRACE_FACILITY`：`Enabled`
- `USE_STATS_FORMATTING_FUNCTIONS`：`Disabled`
- `GENERATE_RUN_TIME_STATS`：`Disabled`
- `RECORD_STACK_HIGH_ADDRESS`：如果当前 CubeMX/FreeRTOS 版本提供该选项则启用，便于调试器识别栈边界

在 `FreeRTOS > Include parameters` 中启用：

- `uxTaskGetStackHighWaterMark`：Enabled
- `uxTaskGetStackHighWaterMark2`：若当前版本提供则 Enabled

保持以下方案以降低资源消耗：

- 不创建常驻栈监控任务，不启用运行时间统计和格式化任务列表。
- 调试时使用 Keil/EIDE 的 FreeRTOS 线程视图，并直接调用原生 `uxTaskGetStackHighWaterMark()` 查看任务历史最小剩余栈。
- 栈高水位的单位是 `StackType_t` 项；STM32F103 上通常每项为 4 Bytes，换算时以实际 `sizeof(StackType_t)` 为准。
- 保留 `vApplicationStackOverflowHook()`。检测到溢出时禁止继续调度并停在可调试循环中；具体 Hook 实现留到生成代码检查后的下一阶段添加。
- 后续任务的目标最低余量建议至少保留 `64` 个 StackType_t 项；低于该值时增加对应任务栈。该值是初始调试警戒线，不代表所有任务的最终安全阈值。

## 11. 生成前检查

- Pinout 页面没有红色冲突或黄色警告。
- PB3/PB4 显示为 GPIO Input，PA13/PA14 保留 SWD。
- PB7 显示 TIM4_CH2；PB8/PB9 显示 I2C1 SCL/SDA。
- PB12 是普通 GPIO Output，不是 SPI2_NSS，默认 High。
- ADC1 正好有 5 个 Rank，DMA 为 Circular + Half Word。
- HAL Timebase 是 TIM2，FreeRTOS 的 CubeMX Interface 为 CMSIS-RTOS v2，但项目应用层 API 统一使用 FreeRTOS 原生接口。
- `CHECK_FOR_STACK_OVERFLOW=2`，并已包含 `uxTaskGetStackHighWaterMark()`。
- NVIC 中 DMA1 Channel1 和 EXTI9_5 优先级均为 5。
- Project Toolchain 为 MDK-ARM V5，外设生成独立 `.c/.h`。

## 12. 生成后提交检查的文件

完成 `GENERATE CODE` 后，保留生成结果在当前工程目录。检查时重点读取：

- `*.ioc`
- `Core/Inc/main.h`
- `Core/Src/main.c`
- `Core/Src/gpio.c`
- `Core/Src/adc.c`
- `Core/Src/spi.c`
- `Core/Src/i2c.c`
- `Core/Src/usart.c`
- `Core/Src/tim.c`
- `Core/Src/freertos.c`
- `Core/Src/stm32f1xx_hal_timebase_tim.c`
- `Core/Src/stm32f1xx_it.c`
- `MDK-ARM/*.uvprojx`

电池分压电阻为 100 kOhm / 100 kOhm，后续软件换算基础关系为 `VBAT = ADC_Voltage * 2`。实际电量显示还需结合 VDDA 校准和电池放电曲线，留到下一开发阶段处理。

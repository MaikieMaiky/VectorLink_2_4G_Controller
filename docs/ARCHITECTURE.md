# 固件架构

## 设计目标

固件保持三层简单结构，不为单板项目引入复杂框架：

- `Board`：封装当前 PCB 的 ADC DMA 和 GPIO 引脚读取。
- `Drivers/User`：实现输入处理、蜂鸣器、OLED 和 NRF24L01。
- `App` 与 `Protocol`：负责 FreeRTOS 任务、共享状态、界面行为和无线数据格式。

CubeMX 生成代码继续使用 C。`VectorLink_InitializeApp()` 是生成层进入 C++ 应用层的唯一初始化入口。

## 任务

| 任务 | 周期/触发 | 优先级 | 责任 |
|---|---:|---:|---|
| Radio | 20 ms + NRF IRQ | Idle+4 | 发送控制包、读取 ACK Payload、链路统计 |
| Input | 10 ms | Idle+3 | ADC 快照、摇杆处理、按键消抖、电池计算 |
| UI | 10 ms/100 ms | Idle+2 | 蜂鸣器状态机、OLED 刷新和告警 |

CubeMX 默认任务在创建上述任务后调用 `vTaskDelete(NULL)`，其内存由 Idle Task 回收。

## 数据流

```text
ADC DMA + GPIO -> InputTask -> AppSnapshot -> RadioTask -> NRF24L01
                                  |
                                  +----------> UiTask -> OLED/Buzzer

NRF IRQ -> task notification -> RadioTask -> ACK Payload -> TelemetryState
```

共享的 `AppSnapshot` 体积较小，由短临界区整体复制。OLED 的 1 KiB 帧缓冲使用静态存储，不占用 UI 任务栈。

## 初始化顺序

1. CubeMX 初始化 HAL、时钟、GPIO、DMA、ADC、I2C、SPI、TIM 和 USART。
2. `MX_FREERTOS_Init()` 调用 `VectorLink_InitializeApp()`。
3. 应用层校准 ADC 并启动五通道循环 DMA。
4. 创建 Input、Radio 和 UI 三个任务。
5. UI 任务初始化 OLED 和蜂鸣器；Radio 任务初始化 NRF24L01。
6. 外设初始化失败只影响对应功能。ADC 或任务创建失败进入 `Error_Handler()`。

## 并发约束

- 只有 Radio 任务访问 SPI2 和 NRF24L01 寄存器。
- 只有 UI 任务访问 I2C1、OLED 帧缓冲和 TIM4 蜂鸣器状态。
- EXTI 回调只发送任务通知，不在中断中操作 SPI。
- ADC DMA 不使用完成回调；读取时连续复制两次以降低跨扫描读取的风险。
- 不在任务循环中使用动态内存分配。

## 配置入口

可调整参数集中在 `App/Inc/app_config.hpp`：

- 任务周期与失联时间。
- 摇杆中心、端点、死区和方向反转。
- ADC 参考电压、电池分压和报警阈值。
- OLED 地址、NRF 频道和无线地址。

上板后应优先修改这些配置，而不是在驱动实现中散落常量。

更详细的调用链、C++ 设计和任务数据流讲解见[源码阅读指南](READING_GUIDE.md)。

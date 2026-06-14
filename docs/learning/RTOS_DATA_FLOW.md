# RTOS 与数据流讲解

## 启动调用链

```text
main()
 -> CubeMX peripheral initialization
 -> osKernelInitialize()
 -> MX_FREERTOS_Init()
 -> VectorLink_InitializeApp()
 -> BoardAdc::Initialize()
 -> xTaskCreate(Input, Radio, UI)
 -> osKernelStart()
```

CubeMX 默认任务随后执行 `vTaskDelete(NULL)`。使用 `NULL` 表示删除当前任务，其动态栈由 Idle Task 负责回收。

## 为什么是三个任务

- Input 需要稳定 10 ms 采样周期，但不应被 OLED I2C 刷新拖慢。
- Radio 需要最高应用优先级及时处理 IRQ 和 20 ms 控制周期。
- UI 可以较低优先级运行，显示延迟几十毫秒不会影响控制安全。

任务不是按“每个类一个任务”划分，而是按时序、外设所有权和阻塞特性划分。

## 优先级

```text
Radio: Idle + 4
Input: Idle + 3
UI:    Idle + 2
```

Radio 的阻塞等待会让出 CPU；收到 IRQ 后优先处理。Input 保证采样稳定。UI 最低，避免 I2C 和绘图影响控制链路。

## 周期调度

任务使用 `vTaskDelayUntil()` 而非 `vTaskDelay()`。

假设任务周期 10 ms、执行耗时 1 ms：

- `vTaskDelay(10)` 的实际启动间隔约 11 ms，会逐步漂移。
- `vTaskDelayUntil()` 以绝对唤醒点为基准，启动间隔保持约 10 ms。

如果某次执行超过一个周期，FreeRTOS 不会创造丢失的时间；需要通过性能测量发现这种情况。

## 共享状态

三个任务通过 `g_state` 交换数据。写入和读取都在短临界区完成：

```cpp
taskENTER_CRITICAL();
const AppSnapshot state = g_state;
taskEXIT_CRITICAL();
```

关键点：临界区内只做固定大小内存复制，不进行 SPI、I2C、格式化或等待。

这里没有使用队列，因为消费者需要的是“最新状态”，不是每一条历史事件。队列更适合不能丢失的离散消息。

## 蜂鸣器请求

蜂鸣器请求也是共享状态，但只保存一个最高优先级待处理模式。它不是完整事件队列：低优先级提示允许被故障提示覆盖。

这种设计符合声音提示需求，但若未来每个按键音都必须播放，就应改用 FreeRTOS queue。

## IRQ 与任务通知

NRF IRQ 路径：

```text
PA8 falling edge
 -> EXTI9_5_IRQHandler
 -> HAL_GPIO_EXTI_IRQHandler
 -> HAL_GPIO_EXTI_Callback
 -> vTaskNotifyGiveFromISR
 -> RadioTask wakes
 -> SPI reads STATUS
```

中断不操作 SPI，因为 HAL SPI 是阻塞调用，并且 RadioTask 是 SPI2 的唯一所有者。任务通知比二值信号量更轻，适合“一次 IRQ 唤醒一个固定任务”。

`portYIELD_FROM_ISR()` 让刚被唤醒且优先级更高的 RadioTask 可在退出中断后立即运行。

## 外设所有权

| 外设 | 唯一使用者 |
|---|---|
| ADC DMA | BoardAdc 启动，Input 读取 |
| GPIO keys | InputTask |
| SPI2 / NRF | RadioTask |
| I2C1 / OLED | UiTask |
| TIM4 / buzzer | UiTask |

唯一所有者减少 mutex 数量，也让死锁分析简单。

## 错误处理

- ADC 初始化或任务创建失败：进入 `Error_Handler()`，因为系统核心功能无法成立。
- NRF 初始化失败：每秒重试，其他任务继续运行。
- OLED 失败：每 5 秒重试，不阻断输入和无线。
- 栈溢出或 malloc 失败：关闭中断并停机，便于调试器捕获。

## 常见误区

- 临界区不是 mutex；它会影响中断响应，只能保持极短。
- 高优先级任务如果从不阻塞，会饿死低优先级任务。
- 任务通知计数不是无线 STATUS，本项目醒来后仍必须读 NRF 寄存器。
- `HAL_GetTick()` 与 FreeRTOS tick 当前都是毫秒尺度，但用途和实现来源不同。

## 自测问题

1. 为什么 OLED 不与 Radio 放进同一个任务？
2. 为什么 `g_state` 适合快照，而按键边沿事件未来可能更适合队列？
3. 如果在 EXTI 回调中直接调用 `HAL_SPI_TransmitReceive()`，有哪些风险？

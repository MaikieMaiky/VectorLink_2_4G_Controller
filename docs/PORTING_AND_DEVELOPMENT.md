# 代码移植与开发准备

本文档定义下一阶段的实现边界。当前阶段不把参考工程代码直接复制到生成目录。

## 技术约束

- CubeMX 生成层保持 C 与 HAL 风格。
- 应用与设备驱动层使用 C++17。
- C++ 调用 HAL/CubeMX 接口时使用 `extern "C"` 包装 C 头文件。
- FreeRTOS 应用代码使用原生 API，如 `xTaskCreate`、`xQueueCreate`、任务通知和信号量。
- CMSIS-RTOS v2 仅保留为 CubeMX 生成与启动适配层，不用于新增业务接口。
- 不在中断中进行 OLED 刷屏、阻塞 SPI/I2C 或复杂协议处理。

## 建议目录

后续新增代码建议放在 CubeMX 管理范围之外：

```text
App/
  Inc/
  Src/
Board/
  Inc/
  Src/
Drivers/User/
  Inc/
  Src/
Protocol/
  Inc/
  Src/
```

- `Board`：引脚语义、ADC 缓冲区、板级初始化和硬件抽象。
- `Drivers/User`：NRF24L01、OLED、按键、摇杆、蜂鸣器、电池检测。
- `Protocol`：遥控数据结构、序列化、版本和校验。
- `App`：任务、状态机、业务逻辑和 UI。

## 当前 C++ 入口

- `App/Src/app_main.cpp` 是 C++17 应用入口，新增业务代码应优先放在 `App` 及后续用户目录中。
- `App/Inc/app_main.h` 提供 C ABI 的 `VectorLink_InitializeApp()`，供 CubeMX 生成的 C 文件调用。
- `MX_FREERTOS_Init()` 在 `USER CODE BEGIN RTOS_THREADS` 区域调用 `VectorLink_InitializeApp()`，CubeMX 重新生成时会保留。
- `VectorLink_InitializeApp()` 使用原生 FreeRTOS `xTaskCreate()` 创建应用任务，不经 CMSIS-RTOS v2 封装。
- EIDE 使用 Arm Compiler 6、C++17 和 LTO。FreeRTOS 的 `vTaskSwitchContext()` 带有防止 LTO 内联优化的兼容属性，以保留汇编端口需要的任务切换符号。
- 当前代码不依赖动态内存分配、RTTI 或异常；后续嵌入式 C++ 代码也应优先使用静态对象和明确生命周期。

## 初始化顺序

1. HAL、系统时钟和 CubeMX 外设初始化。
2. 保证 NRF CE=0、CSN=1，蜂鸣器 PWM 未启动。
3. ADC 校准后启动 5 通道循环 DMA。
4. 初始化 I2C OLED，并允许失败降级，不阻止遥控核心功能启动。
5. 初始化 NRF24L01，读取寄存器验证 SPI 通信。
6. 创建原生 FreeRTOS 任务、队列和通知对象。
7. 启动调度器后由任务进入正常工作状态。

## 第一批驱动接口

### Board ADC

- 单一 `uint16_t[5]` DMA 缓冲区。
- 固定索引：LH=0、LV=1、RH=2、RV=3、BAT=4。
- 对上层输出经过快照保护的数据，避免读取 DMA 更新中的不一致组合。

### Input

- 12 键位图，低有效统一转换为逻辑按下状态。
- 周期扫描建议 5-10 ms，软件消抖 15-30 ms。
- 摇杆提供原始值、校准值和归一化值；方向翻转由校准配置决定。

### NRF24L01

- SPI2 使用 HAL 硬件 SPI，禁止移植参考工程的软件模拟 SPI。
- IRQ ISR 仅通过任务通知唤醒射频任务。
- 驱动提供寄存器读写、FIFO、状态清除、收发模式和链路统计。
- AT2401C 控制由硬件网络完成，不增加虚构的 TXEN/RXEN GPIO。

### OLED

- 使用 HAL I2C1 PB8/PB9，不移植参考工程的软件模拟 I2C。
- 驱动失败不能阻塞输入采集和无线通信。
- 显存大小需计入 STM32F103C8 的 20 KiB RAM 预算。

### Buzzer

- TIM4_CH2 PWM，仅在需要提示音时启动。
- 提供频率、占空比和时长控制，不在任务中忙等待。

## FreeRTOS 初步任务划分

正式实现前再根据驱动复杂度确认，初步建议：

| 任务 | 责任 | 触发方式 |
|---|---|---|
| InputTask | 按键消抖、摇杆滤波、电池采样 | 5-10 ms 周期 |
| RadioTask | NRF 收发、IRQ 处理、链路状态 | IRQ 通知 + 周期发送 |
| UiTask | OLED 页面和提示状态 | 低频周期/事件 |

蜂鸣器可使用软件定时器或由 UI 任务管理，暂不单独创建任务。

## 内存与调试基线

- 当前 FreeRTOS heap：8192 bytes。
- 已启用 `configCHECK_FOR_STACK_OVERFLOW=2`。
- 已启用 `uxTaskGetStackHighWaterMark()` 与栈高地址记录。
- 每个任务上板稳定运行后记录最低栈高水位，初始警戒线为 64 个 `StackType_t`。
- 栈溢出 Hook 后续应关闭中断并停机，不能空函数返回。
- 每增加 OLED 显存、队列或协议缓冲区后重新核对 map 文件 RAM 使用量。

## 参考工程使用原则

参考工程可用于理解 OLED 绘图、按键语义、摇杆映射和 NRF 寄存器流程，但不能直接照搬：

- 参考工程引脚分配与当前硬件不同。
- 参考工程 OLED 和 NRF 使用软件模拟总线；当前工程使用硬件 I2C/SPI。
- 参考工程没有完整的 FreeRTOS 并发与 ISR 约束。
- 任何移植代码都应先拆出协议无关逻辑，再适配当前板级接口。

## 下一阶段验收顺序

1. 板级 GPIO、UART 日志和 FreeRTOS 调度运行。
2. ADC DMA 五通道顺序及摇杆/电池原始值正确。
3. 按键位图和消抖正确。
4. OLED 硬件 I2C 显示测试。
5. NRF SPI 寄存器读写和 IRQ 测试。
6. 单向发送、应答和丢包统计。
7. 定义并冻结第一版遥控协议。

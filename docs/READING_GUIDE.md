# VectorLink 源码阅读指南

本文档是阅读固件的总入口。建议不要从 NRF 驱动第一行开始硬啃，而是先建立运行时主线，再逐层进入算法和寄存器。

## 你需要先知道什么

本工程由两部分组成：

- CubeMX 生成的 C 层负责 MCU、时钟和外设初始化。
- 自有 C++ 层负责状态、算法、任务、驱动和协议。

C 与 C++ 通过 `VectorLink_InitializeApp()` 连接。项目没有使用继承体系、模板框架、异常或动态多态，C++ 主要用于类型约束、对象状态和职责封装。

## 推荐阅读顺序

### 第一遍：只追踪启动和任务

1. `Core/Src/main.c`：找到 `MX_FREERTOS_Init()` 和调度器启动。
2. `Core/Src/freertos.c`：找到 `VectorLink_InitializeApp()` 和默认任务自删除。
3. `App/Src/app_main.cpp`：只读 `Application::Initialize()`、三个 `TaskEntry()` 和 EXTI 回调。
4. `App/Inc/app_config.hpp`：理解所有周期、阈值和硬件默认参数。
5. `App/Inc/app_state.hpp`：理解任务之间交换的数据形状。

这一遍的目标是回答：谁创建任务、每个任务多久执行、每个外设归谁使用。

### 第二遍：沿数据走向阅读

输入链路：

```text
ADC DMA / GPIO
  -> BoardAdc / BoardGpio
  -> Joystick / Buttons / BatteryMonitor
  -> InputState
  -> AppSnapshot
```

无线链路：

```text
AppSnapshot
  -> ControlData
  -> EncodeControl
  -> Nrf24l01::StartTransmit
  -> NRF IRQ
  -> CompleteTransmit
  -> DecodeTelemetry
```

界面链路：

```text
AppSnapshot
  -> OledDisplay::Render
  -> framebuffer
  -> I2C screen update

application event
  -> RequestBuzzer
  -> Buzzer::Play / Update
  -> TIM4 PWM
```

### 第三遍：理解设计选择

按以下章节阅读：

1. [C++ 设计](learning/CPP_DESIGN.md)
2. [RTOS 与数据流](learning/RTOS_DATA_FLOW.md)
3. [输入系统](learning/INPUT_SYSTEM.md)
4. [无线系统](learning/RADIO_SYSTEM.md)
5. [显示与提示](learning/UI_SYSTEM.md)
6. [协议与测试](learning/PROTOCOL_AND_TESTING.md)
7. [练习路线](learning/EXERCISES.md)

## 阅读时重点观察

- `.hpp` 先看公开接口，`.cpp` 再看内部实现。
- 名称以单位结尾，例如 `_ms`、`_mv`、`_hz`，不要忽略单位。
- `Board` 只回答“这块 PCB 怎么读写”；算法不应放进去。
- `Drivers/User` 保存设备或算法状态；任务策略不应放进去。
- `App` 决定何时调用、失败后怎么办以及状态如何共享。
- `Protocol` 只处理字节，不访问 HAL 或 FreeRTOS。

## 建议使用 Git 辅助阅读

这些提交对应清晰的演进阶段：

```text
8396f67  C++ 与代码规范基线
11e519c  输入算法、协议和主机测试
4c72ad0  板级、OLED、蜂鸣器和 NRF 驱动
f53cc3f  三任务应用整合
8ad30ef  公共接口和关键逻辑注释
```

可使用 `git show <commit>` 阅读每阶段新增的最小变化，也可以比较 `11e519c..f53cc3f` 理解纯逻辑如何接入硬件任务。

## 无硬件时应如何判断代码

可以可靠检查：

- 类型、边界、协议字节布局和 CRC。
- 按键消抖、摇杆映射、电池换算和链路统计。
- 任务所有权、阻塞上限和错误恢复路径。
- 编译、链接、RAM/Flash 使用。

不能仅靠阅读确认：

- 摇杆实际方向、中心和端点。
- OLED 地址、控制器兼容性和显示方向。
- NRF 模块、IRQ、AT2401C 和射频距离。
- 电池 ADC 误差及真实电量曲线。
- 蜂鸣器实际响度和谐振频率。

这些项目必须结合 [上板检查清单](HARDWARE_BRINGUP.md) 验证。

# VectorLink 代码风格规范

本文档规定 VectorLink 固件中自行编写代码的格式化与命名规则。

## 适用范围

本规范适用于：

- `App`、`Board`、`Drivers/User`、`Protocol` 等自行创建目录中的 C/C++ 代码。
- CubeMX 生成文件中 `USER CODE BEGIN` 与对应 `USER CODE END` 之间的用户代码。
- 项目自行创建的测试代码、工具代码和公共接口。

以下代码不进行整体格式化或批量重命名：

- CubeMX 自动生成且位于 `USER CODE` 区域之外的代码。
- STM32 HAL、CMSIS、FreeRTOS 等第三方源码。
- 对第三方源码进行的必要兼容性补丁。此类补丁应尽量保持原文件风格，减小升级时的差异。

## 基础格式

- 使用接近 Visual Studio 和 VS Code Microsoft C/C++ 扩展的 Allman 风格。
- 左大括号独占一行。
- 使用 2 个空格缩进，不使用 Tab。
- 建议最大行宽为 100 个字符。
- 文件编码统一使用 UTF-8。
- 新建文本文件统一使用 LF 换行符。
- 删除行尾空白，并保证文件末尾有一个换行符。
- 指针和引用符号靠近类型，例如 `RadioDriver* driver`、`const Packet& packet`。
- 自有源文件中的 `#include` 应按类别分组并保持稳定顺序。

示例：

```cpp
void RadioDriver::StartTransmission(const Packet& packet)
{
  if (!IsReady())
  {
    return;
  }

  WritePayload(packet);
  SetChipEnable(true);
}
```

## Include 顺序

C++ 源文件中的头文件建议按以下顺序排列，每组之间空一行：

1. 当前源文件对应的头文件。
2. C++ 标准库头文件。
3. 项目自有头文件。
4. HAL、CMSIS 和 FreeRTOS 等 C 语言头文件。

C 语言头文件在 C++ 文件中通过 `extern "C"` 引入：

```cpp
#include "radio_driver.h"

#include "packet.h"

extern "C"
{
#include "FreeRTOS.h"
#include "spi.h"
#include "task.h"
}
```

## 命名规则

| 对象 | 命名风格 | 示例 |
|---|---|---|
| 文件和目录 | `snake_case` | `radio_driver.cpp` |
| 命名空间 | 小写 | `vectorlink` |
| 类、结构体和类型别名 | `PascalCase` | `RadioDriver` |
| 枚举类型 | `PascalCase` | `LinkState` |
| 作用域枚举值 | `PascalCase` | `LinkState::Connected` |
| C++ 函数和成员函数 | `PascalCase` | `InitializeRadio()` |
| 局部变量和参数 | `snake_case` | `battery_voltage_mv` |
| 私有数据成员 | `snake_case_` | `task_handle_` |
| 编译期常量 | `kPascalCase` | `kTaskStackDepth` |
| 宏 | `UPPER_SNAKE_CASE` | `VECTORLINK_ASSERT` |
| 头文件保护宏 | `UPPER_SNAKE_CASE` | `VECTORLINK_RADIO_DRIVER_H` |
| C ABI 接口 | `VectorLink_` 前缀 | `VectorLink_InitializeApp()` |
| FreeRTOS 任务入口 | 功能名加 `TaskEntry` | `RadioTaskEntry()` |
| ISR 和 HAL 回调 | 保持框架规定名称 | `HAL_GPIO_EXTI_Callback()` |

## 函数命名语义

- 初始化操作统一使用 `Initialize()` 或带对象名称的 `InitializeRadio()`。
- 布尔查询使用 `Is`、`Has` 或 `Can` 开头，例如 `IsConnected()`。
- 获取值使用 `Get`，修改状态使用清晰的动词，例如 `GetBatteryVoltage()`、`Start()`、`Stop()`。
- FreeRTOS ISR 专用操作保留 `FromIsr` 后缀，例如 `NotifyFromIsr()`。
- 不使用含义重叠的 `Init()`、`Begin()`、`Setup()` 表示同一种初始化操作。

## 变量与单位

- 名称应表达用途，不使用无明确含义的缩写。
- 具有物理单位或时间单位的变量必须包含单位后缀。
- 除循环计数器等局部场景外，避免单字母变量。
- 避免可变全局变量，不强制使用 `g_` 前缀掩盖全局状态。

```cpp
constexpr uint32_t kRadioTimeoutMs = 100;
uint16_t battery_voltage_mv = 0;
uint32_t buzzer_frequency_hz = 2400;
```

## C++ 使用约束

- 项目业务代码使用 C++17。
- CubeMX 生成层保持 C 语言，通过小型 C ABI 接口连接 C++ 应用层。
- 优先使用明确生命周期的静态对象和栈对象。
- 默认不使用异常和 RTTI。
- 避免在任务运行期间进行无边界的动态内存分配。
- 中断服务程序中不进行阻塞操作、动态内存分配或复杂业务处理。
- 构造函数不应启动调度相关操作或依赖尚未初始化的 HAL 外设。

## FreeRTOS 规则

- 新增业务代码使用 FreeRTOS 原生 API。
- 任务函数必须明确以 `TaskEntry` 结尾。
- 任务栈深度常量使用 `StackType_t` 数量，不使用字节数混淆表达。
- 任务优先级、栈深度和周期均定义为具名常量。
- ISR 中只调用以 `FromISR` 结尾的 FreeRTOS API，并正确请求上下文切换。
- 时间值使用 `pdMS_TO_TICKS()` 转换，不直接假定 Tick 频率。

```cpp
class InputTask final
{
 public:
  static bool Start();

 private:
  static constexpr uint16_t kStackDepth = 256;
  static constexpr UBaseType_t kPriority = tskIDLE_PRIORITY + 2;
  static constexpr uint32_t kScanPeriodMs = 10;

  static void TaskEntry(void* context);
  static TaskHandle_t task_handle_;
};
```

## CubeMX 用户代码

- 仅在 CubeMX 标记的 `USER CODE` 区域内编写或调用用户逻辑。
- `USER CODE` 区域中的代码使用本规范，但不格式化所在文件的其他生成内容。
- 生成文件中只保留必要的接口调用，复杂实现放在 CubeMX 管理范围之外。
- 不修改 CubeMX 生成函数的名称，即使其命名不符合本规范。

```c
/* USER CODE BEGIN Includes */
#include "app_main.h"
/* USER CODE END Includes */

/* USER CODE BEGIN RTOS_THREADS */
VectorLink_InitializeApp();
/* USER CODE END RTOS_THREADS */
```

## 注释规则

- 注释说明设计原因、硬件限制、并发约束或不明显的行为，不复述代码本身。
- 公共接口可使用简洁的 Doxygen 注释。
- `TODO` 应说明待办事项及原因，必要时关联 Issue。
- 对 LTO、编译器属性和第三方补丁等特殊处理必须留下原因说明。

## 格式化边界

自动格式化工具应只处理自行维护的文件。CubeMX 生成文件的用户区域暂时采用编辑器选中代码后格式化的方式，避免工具改写整个生成文件。第三方目录不纳入自动格式化范围。

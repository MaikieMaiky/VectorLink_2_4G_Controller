# C++ 设计讲解

## 本项目为何使用 C++

这里的 C++ 不是为了建立复杂继承框架，而是为了解决 C 固件中的几个常见问题：

- 用类把状态和操作放在一起，例如 `Buttons` 同时拥有计数器和 `Update()`。
- 用强类型枚举避免混用无意义整数，例如 `BatteryLevel`。
- 用 `std::array` 固定长度并保留长度信息。
- 用构造函数保证对象开始工作前已经得到配置。
- 用命名空间避免全局符号污染。

## namespace

`namespace vectorlink` 是项目级名字空间。`vectorlink::Joystick` 明确表示该类型属于本项目，而不是 HAL 或标准库。

`namespace vectorlink::protocol` 是嵌套名字空间，表示协议代码是项目的一部分，但有更窄的职责。

实现文件中的匿名命名空间：

```cpp
namespace
{
constexpr uint8_t kCommandNop = 0xFF;
}
```

其中符号只在当前编译单元可见，类似 C 中的文件级 `static`，但可同时容纳类型、函数和常量。它用来隐藏驱动寄存器常量和任务内部对象。

## constexpr 与 const

`constexpr` 表示值可在编译期确定，例如任务周期、协议长度和寄存器位。这些值不会成为可修改运行时状态。

`const` 表示通过当前接口不可修改，但不必是编译期值。例如：

```cpp
const AppSnapshot snapshot = ReadState();
```

此处强调当前任务取得快照后不会修改它。

成员函数末尾的 `const`：

```cpp
uint8_t SuccessRate() const;
```

表示函数不会修改对象成员，因此可对只读对象调用，也明确了查询函数的意图。

## static 的三种用途

1. 类内静态常量：`BoardAdc::kChannelCount` 属于类型，不属于某个实例。
2. 静态成员函数：`BoardAdc::Initialize()` 不依赖 `this`，因为 ADC 硬件只有一个实例。
3. 静态存储成员：DMA 缓冲必须在整个程序生命周期存在，不能放在函数栈上。

不要把所有函数都改成 `static`。需要保存独立状态的 `Joystick`、`Buttons`、`Buzzer`、`Nrf24l01` 使用普通对象更清楚。

## final 与 explicit

`class Joystick final` 表示该类不设计为基类。本项目使用组合，不需要继承扩展这些驱动。`final` 是设计声明，不是性能技巧。

`explicit Joystick(config)` 禁止编译器把一个配置数组隐式转换成 `Joystick`。构造动作需要明确写出，避免意外临时对象。

## 值语义

`InputState`、`RadioState`、`TelemetryState` 和 `AppSnapshot` 都是值类型：它们可整体复制，副本与源对象独立。

这让 `ReadState()` 可以在临界区内复制一次，然后退出临界区慢慢使用。任务不会持有共享对象的引用或指针，因此生命周期更简单。

代价是复制一些字节。当前状态很小，这比引入消息总线、引用计数或复杂锁更合适。

## 对象生命周期

- `Joystick` 和 `Buttons` 是任务局部对象，任务永不返回，因此对象与任务同寿命。
- `g_oled` 是静态对象，因为它包含 1 KiB 帧缓冲，若放在 UI 任务栈会增加栈压力。
- `BoardAdc::samples_` 是静态 DMA 缓冲，DMA 在后台持续访问它。
- 项目不在任务循环中使用 `new`，避免碎片和失败路径复杂化。

## extern C 与 ABI

C++ 会对函数名做 name mangling，以支持重载。CubeMX 生成的 C 文件不知道 C++ 修饰后的符号名，因此入口声明使用：

```cpp
extern "C" void VectorLink_InitializeApp(void);
```

同样，C++ 文件包含 HAL 的 C 头文件时放在 `extern "C"` 块中，确保声明使用 C linkage。

`extern "C"` 不会把函数变成 C 语言，也不会禁止函数内部调用 C++。它只规定链接符号和调用约定。

## 设计边界

- 没有虚函数和动态多态，因此不需要 RTTI。
- 没有异常；错误通过 `bool`、枚举结果或状态字段表达。
- 没有通用抽象总线接口，因为当前只有一个 SPI 和一个 I2C 使用者。
- 如果未来需要主机模拟 HAL，再评估是否引入窄接口，不提前抽象。

## 常见误区

- 匿名命名空间不是对象私有成员，它限制的是编译单元可见性。
- `constexpr` 不等于宏；它有类型、作用域并受编译器检查。
- `final` 不代表对象不可修改，只是不允许继承。
- `const` 成员函数仍可读取成员，只是不应修改普通成员。
- `std::array` 没有动态分配，其存储与普通固定数组一样内联存在。

## 自测问题

1. 为什么 `LinkQuality` 是普通对象，而 `BoardAdc` 只有静态接口？
2. 为什么 `AppSnapshot` 适合值复制，`OledDisplay` 不适合频繁复制？
3. 如果删除 `extern "C"`，C 文件调用 C++ 入口时会在哪个阶段失败？

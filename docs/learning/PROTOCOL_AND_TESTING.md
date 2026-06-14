# 协议与测试讲解

## 为什么不直接发送结构体

下面的写法看似简单，但不可作为稳定协议：

```cpp
radio.Send(reinterpret_cast<uint8_t*>(&data), sizeof(data));
```

原因包括：

- 编译器可能插入 padding。
- CPU 字节序可能不同。
- `bool` 和枚举布局不适合作为线协议承诺。
- 改变成员顺序会无意改变协议。
- 未初始化填充字节可能泄漏随机数据。

因此本项目使用固定 `std::array<uint8_t, 24>`，按明确偏移写入。

## 字节布局

公共头：

```text
0..1  "VL"
2     version
3     message type
4..5  sequence, little endian
22..23 CRC16
```

详细字段见 `docs/RADIO_PROTOCOL.md`。

`WriteU16()` 明确先写低字节。接收端无论使用什么语言，都可以按文档重建相同值。

## 有符号数

轴值为 `int16_t`，写入时转换为 `uint16_t`：

```cpp
WriteU16(packet, offset, static_cast<uint16_t>(axis));
```

转换保留二进制补码位模式。解码时再将读取的 `uint16_t` 转回 `int16_t`。例如 `-1` 在线上是 `FF FF`。

## CRC

采用 CRC-16/CCITT-FALSE：

- polynomial：`0x1021`
- initial：`0xFFFF`
- 不反射
- final XOR：0

CRC 只覆盖字节 0..21，结果放入 22..23。NRF 自身已有硬件 CRC，应用层 CRC 的作用是固定协议完整性规则并发现错误解释或错误版本处理。

## 解码失败语义

`DecodeTelemetry()` 先验证魔数、版本、类型和 CRC。任何失败都返回 false，并且不修改输出对象。

这是重要接口契约：调用者可以继续保留上一份有效遥测，而不是得到半更新状态。

## LinkQuality 的可测试性

协议、输入算法和 LinkQuality 不包含 HAL/FreeRTOS 头文件，因此可直接由桌面 `g++` 编译。

这种边界不是为了追求“纯架构”，而是把确定性算法与硬件副作用分开，让无硬件阶段仍能验证关键逻辑。

## 测试工程

`Tests/CMakeLists.txt` 只编译纯逻辑源文件。运行：

```powershell
cmake -S Tests -B Tests/build -G "MinGW Makefiles"
cmake --build Tests/build
ctest --test-dir Tests/build --output-on-failure
```

`test_main.cpp` 使用简单 `Expect()`，没有引入大型测试框架。对于当前规模，它让测试依赖和学习成本保持最低。

## 当前覆盖内容

- 摇杆中心、死区、端点和反向。
- 按键抖动、按下沿、长按和释放沿。
- 电池换算与阈值边界。
- 协议魔数、小端序、遥测解码和 CRC 破坏拒绝。
- 32 包链路成功率和失联超时。

## 建议继续补充的测试

- 协议错误版本和错误消息类型。
- 多按键同时消抖。
- 摇杆中心非对称校准。
- tick 接近 `uint32_t` 回绕时的连接判断。
- 电池 ADC 0、满量程和过阈值输入。

## AC6 构建与主机测试的区别

- 主机测试证明算法行为，但不编译 HAL 驱动。
- AC6 全量构建证明 ARM 类型、链接、LTO 和工程文件一致。
- 两者都不能证明硬件电气和射频时序。

三种验证必须分开理解，不能用“能编译”代替“能上板工作”。

## 常见误区

- `sizeof(ControlData)` 与协议长度没有关系。
- CRC 正确不代表业务字段合理，只证明字节未按当前算法损坏。
- 单元测试不应依赖真实延时，否则速度慢且不稳定。
- 测试内部实现细节会阻碍重构，应优先测试公开行为和边界。

## 自测问题

1. `-1000` 转为 `uint16_t` 后为何还能无损恢复？
2. 为什么 Decode 失败时不清空旧遥测？
3. 哪些错误主机测试能发现，而 AC6 编译不能发现？

# 输入系统讲解

## 调用链

```text
BoardAdc::ReadSnapshot() ----> Joystick::Process() ------> axes
                         \----> BatteryMonitor::Update() -> battery
BoardGpio::ReadPressedButtons() -> Buttons::Update() ----> buttons/edges
                                                          |
                                                          -> InputState
```

## ADC DMA

ADC1 连续扫描五个通道，DMA 循环写入：

```text
index 0 LH
index 1 LV
index 2 RH
index 3 RV
index 4 battery
```

`samples_` 使用静态存储，因为 DMA 在函数返回后仍持续访问。`volatile` 告诉编译器该内存可能在普通代码之外被修改，但它不保证五个值属于同一轮扫描。

`ReadSnapshot()` 连续复制两次，若一致则接受。这不是严格硬件原子快照，但在无需停止 DMA 的前提下降低跨扫描混合概率。若未来需要严格同步，可在 DMA 完成回调中切换双缓冲或增加序列计数。

DMA 半传输和完成中断被禁用，因为应用只轮询最新数据。否则连续 ADC 会频繁触发无用途的中断。

## 摇杆滤波

滤波公式：

```text
filtered += (raw - filtered) / 4
```

它是整数一阶低通。优点是无浮点、状态少；代价是响应有延迟，并且整数除法会产生小量量化。

第一次采样直接作为滤波初值，避免从 0 缓慢爬升。

## 死区与映射

中心死区：

```text
[center - dead_zone, center + dead_zone] -> 0
```

死区之外分别映射：

```text
minimum ... lower_edge -> -1000 ... 0
upper_edge ... maximum -> 0 ... 1000
```

上下两段分别计算，允许实际中心不在 ADC 范围正中。最后使用 `clamp` 防止异常 ADC 或校准值超出输出范围。

`reversed` 在归一化后取反，不改变校准端点含义。

## 按键低有效转换

硬件按下读到低电平。`BoardGpio` 先把它转换为“bit=1 表示按下”，后续算法不再关心电气极性。

bit0..bit11 对应 K1..K12。这样按键状态可直接放入协议的 16 位字段。

## 消抖

每个按键都有独立计数器：

- 原始值等于稳定值：计数器清零。
- 原始值不同：计数器递增。
- 连续三次不同：接受新状态。

输入周期为 10 ms，因此需要约 30 ms 稳定时间。状态变化后通过前后位图计算：

```text
pressed  = current & ~previous
released = previous & ~current
```

长按只保持 `current`，不会重复产生 `pressed`。

## 电池换算

原理图为 100k/100k 分压：

```text
VBAT_mV = ADC / 4095 * 3300 * 2
```

实现使用 32 位整数中间值，并在除法前加半个分母实现四舍五入。

百分比当前按 3.3 V 到 4.2 V 线性映射。锂电池放电曲线并非线性，因此它仅用于首版 UI。上板后应先校准电压，再考虑查表百分比。

## 可调参数

全部位于 `app_config.hpp`：中心、端点、死区、反向、VDDA、分压比例和电池阈值。

## 常见误区

- `volatile` 不等于线程安全，也不保证 DMA 多通道一致性。
- 死区不能只简单把小值设 0 而不重新映射，否则越过死区时输出会跳变。
- 按键消抖计数必须逐键保存，不能用一个全局计数器处理同时按键。
- ADC 参考电压默认 3300 mV 不代表实板一定准确。

## 自测问题

1. 中心为 2100、最小 100、最大 4000 时为何不能使用同一个比例？
2. 10 ms 周期和三次采样为何近似 30 ms，而不是严格保证 30 ms？
3. 若 ADC 参考电压实测为 3.25 V，软件显示会向哪个方向偏差？

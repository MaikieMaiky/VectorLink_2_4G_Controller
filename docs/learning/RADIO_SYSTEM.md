# 无线系统讲解

## 分层关系

```text
RadioTask
  -> protocol::EncodeControl
  -> Nrf24l01 public API
  -> register/payload helpers
  -> HAL SPI2 + CE/CSN GPIO
```

`RadioTask` 决定发送周期、超时、重试初始化和状态统计；`Nrf24l01` 只负责芯片操作，不知道摇杆或 OLED。

## CE 与 CSN

- CSN 是 SPI 片选，低电平包围一条完整命令。
- CE 控制射频状态，不是 SPI 片选。
- 初始化和写寄存器时 CE 保持低。
- PTX 模式写入 Payload 后，CE 高电平超过 10 us 启动发送。

混淆 CE 和 CSN 是 NRF 驱动最常见的问题之一。

## SPI 命令模式

NRF 每条 SPI 事务的第一个返回字节都是 STATUS。寄存器读写流程：

```text
CSN low
send command | register
exchange data bytes
CSN high
```

`Transfer()` 使用 `HAL_SPI_TransmitReceive()`，调用是阻塞的但有 10 ms 上限。只有 RadioTask 调用，因此不需要 SPI mutex。

## 初始化流程

初始化顺序的目的：

1. CE/CSN 进入安全电平。
2. 读取 RF_CH 判断 SPI 是否有响应；`0xFF` 常表示 MISO 悬空或器件未连接。
3. 配置两字节 CRC、pipe0 自动应答、五字节地址、自动重传、频道和 1 Mbps。
4. TX_ADDR 与 RX_ADDR_P0 使用同一地址，以便接收发送目标的自动 ACK。
5. 开启动态 Payload 与 ACK Payload。
6. 清 FIFO 和中断标志。
7. PWR_UP 后等待器件启动。

部分兼容芯片必须先执行 `ACTIVATE 0x73` 才能写 FEATURE，因此代码先尝试普通写入，读回失败再激活。

## 两阶段发送

发送被拆成两步：

### StartTransmit

- 检查驱动状态与长度。
- 进入 PTX 配置。
- 清旧状态与 TX FIFO。
- 写入 Payload。
- 脉冲 CE。
- 记录开始时间并设置 `transmit_pending_`。

### CompleteTransmit

RadioTask 被 IRQ 唤醒后读取 STATUS：

- `MAX_RT`：达到最大重发次数，清标志并清 TX FIFO。
- `TX_DS`：发送和 ACK 成功。
- `RX_DR + TX_DS`：ACK 中还带有 Payload，继续读取遥测。
- 两者都没有：仍为 Pending，可能是错误 IRQ 或状态尚未就绪。

若 IRQ 丢失，RadioTask 的有限等待结束后调用 `CheckTransmitTimeout()` 清理状态，避免驱动永远停留在 pending。

## IRQ 为什么只做通知

EXTI 运行于中断上下文。直接在其中操作 SPI 会带来：

- 阻塞中断。
- HAL 超时和调度依赖不适合 ISR。
- 与任务的 SPI 访问形成并发。
- 难以实现完整错误恢复。

因此 ISR 只调用 `vTaskNotifyGiveFromISR()`，所有寄存器访问在 RadioTask 完成。

## ACK Payload

自动 ACK 原本只表示“接收端收到”。ACK Payload 允许接收端预先把遥测数据放入 ACK 中，控制器一次发送即可同时得到确认和遥测。

注意 ACK Payload 通常是接收端提前准备的，可能对应上一周期状态，因此协议中包含 `acknowledged_sequence`，上层可判断遥测确认了哪个控制包。

## 动态 Payload 宽度

读取 Payload 前先执行 `R_RX_PL_WID`。合法范围为 1..32。芯片规范要求遇到非法宽度时清 RX FIFO，否则接收状态可能无法恢复。

应用协议进一步要求长度恰好为 24；驱动接受任意合法 NRF 长度，协议层负责更严格验证。

## 链路质量

`LinkQuality` 使用 32 位历史窗口，每次左移并写入最新结果。成功率为窗口中 1 的数量除以有效样本数。

连接状态不是只看最近一包，而是检查是否曾成功并且最后成功距离当前不超过 500 ms。计时使用无符号减法，因此可自然处理 tick 回绕，只要超时远小于 32 位时间范围。

## 故障恢复

- 初始化失败：RadioTask 每秒重试。
- SPI BusError：驱动标记未初始化，回到初始化流程。
- MAX_RT：本包失败，下一周期继续发送。
- 超时：清 FIFO 和状态后继续。
- 失联：更新 UI 和蜂鸣器，但输入任务持续运行。

## 常见误区

- `RF_SETUP=0x06` 在当前芯片定义下是 1 Mbps，不要只凭十六进制猜含义。
- TX 成功不代表 ACK Payload 一定存在。
- 清 STATUS 是“向相应位写 1”，不是写 0。
- MAX_RT 后不清 TX FIFO 会导致旧 Payload 影响下一次发送。
- `HAL_Delay()` 只用于初始化短等待，不应放在 20 ms 任务主循环中。

## 自测问题

1. 为什么 TX_ADDR 和 RX_ADDR_P0 在 PTX 自动应答模式下需要相同？
2. 如果 IRQ 引脚断路，当前代码如何从一次发送中恢复？
3. 为什么驱动接受 1..32 字节，而应用只接受 24 字节？

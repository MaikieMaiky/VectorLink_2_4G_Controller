# 硬件引脚与网表核对记录

本文档用于代码移植和驱动开发时的硬件真值依据。

## 核对来源

- 嘉立创导出网表：`2.4G_遥控器.enet`
- 网表 SHA-256：`B4F612F122890F490AAB2DB837564DC9BB7C0D792AB1202207DBA4438B6DD200`
- 核对日期：2026-06-14
- 固件依据：`VectorLink_2_4G_Controller.ioc`、`Core/Inc/main.h` 和各外设初始化文件

结论：MCU 实际连线与当前 CubeMX 配置一致。部分网表网络使用物理引脚名，而 CubeMX 使用功能标签，文档已统一为软件侧名称。

## MCU 引脚真值表

| 功能 | 原理图/网表网络 | MCU 引脚 | 软件标签 | 外设模式 | 电气说明 |
|---|---|---:|---|---|---|
| 左摇杆水平 | LH | PA0 | JOY_LH | ADC1_IN0 | 模拟输入 |
| 左摇杆垂直 | LV | PA1 | JOY_LV | ADC1_IN1 | 模拟输入 |
| 右摇杆水平 | PA2 | PA2 | JOY_RH | ADC1_IN2 | 网表网络名为 PA2 |
| 右摇杆垂直 | PA3 | PA3 | JOY_RV | ADC1_IN3 | 网表网络名为 PA3 |
| 电池检测 | BATFD | PA4 | BAT_SENSE | ADC1_IN4 | 100k/100k 分压 |
| 按键 1 | K1 | PA6 | KEY1 | GPIO input | 内部上拉，低有效 |
| 按键 2 | K2 | PA7 | KEY2 | GPIO input | 内部上拉，低有效 |
| NRF IRQ | IRQ | PA8 | NRF_IRQ | EXTI8 | 低有效，下降沿 |
| 调试串口 TX | PA9 | PA9 | USART1_TX | USART1 | 接 CH340 RXD |
| 调试串口 RX | PA10 | PA10 | USART1_RX | USART1 | 接 CH340 TXD |
| 按键 7 | K7 | PA11 | KEY7 | GPIO input | 内部上拉，低有效 |
| 按键 8 | K8 | PA12 | KEY8 | GPIO input | 内部上拉，低有效 |
| SWD 数据 | PA13 | PA13 | SWDIO | SWD | H1 pin 2 |
| SWD 时钟 | PA14 | PA14 | SWCLK | SWD | H1 pin 3 |
| 按键 3 | K3 | PB0 | KEY3 | GPIO input | 内部上拉，低有效 |
| 按键 4 | K4 | PB1 | KEY4 | GPIO input | 内部上拉，低有效 |
| 按键 5 | K5 | PB2 | KEY5 | GPIO input | 内部上拉，低有效 |
| 按键 9 | K9 | PB3 | KEY9 | GPIO input | 禁用 JTAG 后可用 |
| 按键 10 | K10 | PB4 | KEY10 | GPIO input | 禁用 JTAG 后可用 |
| 按键 11/左摇杆按压 | K11 | PB5 | KEY11 | GPIO input | 内部上拉，低有效 |
| 按键 12/右摇杆按压 | K12 | PB6 | KEY12 | GPIO input | 内部上拉，低有效 |
| 蜂鸣器 | BUZZ | PB7 | BUZZ_PWM | TIM4_CH2 | MOSFET 低端驱动 |
| OLED SCL | PB8 | PB8 | SCL | I2C1_SCL remap | R19 4.7k 上拉 |
| OLED SDA | PB9 | PB9 | SDA | I2C1_SDA remap | R8 4.7k 上拉 |
| 按键 6 | K6 | PB10 | KEY6 | GPIO input | 内部上拉，低有效 |
| NRF CE | CE | PB11 | NRF_CE | GPIO output | 初始低 |
| NRF CSN | CSN | PB12 | NRF_CSN | GPIO output | 初始高 |
| NRF SCK | PB13 | PB13 | NRF_SCK | SPI2_SCK | 36 MHz/16 = 2.25 MHz |
| NRF MISO | MISO | PB14 | NRF_MISO | SPI2_MISO | SPI Mode 0 |
| NRF MOSI | MOSI | PB15 | NRF_MOSI | SPI2_MOSI | SPI Mode 0 |
| HSE 输入 | OSC_IN | PD0 | OSC_IN | RCC | 8 MHz 晶振 |
| HSE 输出 | OSC_OUT | PD1 | OSC_OUT | RCC | 8 MHz 晶振 |

未使用的 PA5、PA15、PC13-PC15 保持默认状态。BOOT0 由 10k 下拉，NRST 为 10k 上拉加 0.1 uF。

## 关键器件连线确认

### CH340N

网表已确认串口正确交叉：

- CH340N pin 6 TXD -> PA10 / USART1_RX
- CH340N pin 7 RXD -> PA9 / USART1_TX
- VCC 与 V3 均为 3.3 V

### OLED

- U10 pin 3 SCL -> PB8
- U10 pin 4 SDA -> PB9
- R19：PB8 至 3.3 V，4.7k
- R8：PB9 至 3.3 V，4.7k

因此固件应使用重映射后的 I2C1 PB8/PB9，不能恢复到默认 PB6/PB7。

### NRF24L01P-R 与 AT2401C

- SPI2：PB13/PB14/PB15
- CSN：PB12，软件片选
- CE：PB11
- IRQ：PA8，低有效下降沿
- AT2401C RXEN 通过 R6 跟随 CE，并由 R7 下拉
- AT2401C TXEN 由 NRF24L01 的 VDD_PA 经 R4 驱动

驱动层不需要增加独立 PA/LNA 控制 GPIO，但必须遵循 NRF24L01 的 CE 时序。

### 输入与模拟量

- SW1 输出 LH/LV，中心按压接 K11。
- SW2 输出网络名为 PA2/PA3，软件含义为 RH/RV，中心按压接 K12。
- K1-K10 独立按键均接地，配有 0.1 uF 电容；K11/K12 同样低有效。
- BATFD 为 VBAT 经 100k/100k 分压，理想换算为 `VBAT = ADC_VOLTAGE * 2`。

### 蜂鸣器

PB7 驱动 Q2 栅极，Q2 控制 2.4 kHz 无源蜂鸣器低端。TIM4_CH2 当前约 2.404 kHz、50% 占空比。

## 软件必须保持的约束

- ADC DMA 缓冲区顺序固定为 `LH, LV, RH, RV, BAT_SENSE`。
- NRF IRQ 只能配置下降沿，ISR 中不执行阻塞 SPI。
- NRF CSN 上电默认高，CE 默认低。
- 按键按下读数为 0，需要软件消抖。
- PB3/PB4 与 JTAG 冲突，只保留 SWD。
- OLED 地址和控制器型号需上板探测确认；网表标注为 SSD1315，常用 7-bit 地址预计为 `0x3C`，但不能仅凭网表视为实测结论。

## 尚需上板验证

- 四路摇杆方向、中心值、端点范围与死区。
- 实际 VDDA、电池分压误差及电量曲线。
- OLED I2C 地址和初始化命令兼容性。
- NRF24L01 芯片识别、IRQ 时序、发射功率和 AT2401C 工作状态。
- 蜂鸣器实际谐振频率和合适占空比。
- 所有按键的丝印功能定义及长按/组合键策略。


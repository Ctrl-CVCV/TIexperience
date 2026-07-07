# MSPM0G3519开发笔记
## 一、关于DAP烧录配置
在烧录时，配置选择pyocd，目标芯片名称改为MSPM0G3519。
在此之前，需要安装pyOCD 和 MSPM0 的 pack：
```
pip install pyocd
pyocd pack install MSPM0G3519

```
其中可能会存在pyOCD 0.44.1 处理 MSPM0GX51X_DFP 1.0.0 包的 Flash 区域定义时，两个不重叠的内存范围触发了 MemoryRange 断言失败的问题，需要做如下修改：在创建 MemoryRange 前增加重叠检测，非重叠区域直接跳过
改动点所在位置：F:\Python\Lib\site-packages\pyocd\target\pack\flm_region_builder.py:192-193

.eide/eide.yml处做如下修改：pyOCD 配置：baseAddr 改为 0x0，targetName 改为 mspm0g3519，speed 降为 1M
然后再.eide/g3519_uart.pyocd.yaml中填写有效的配置文件

## 二、OLED屏幕 SysConfig 配置

### 硬件连接

| OLED 引脚 | MSPM0 引脚 | 说明 |
|-----------|-----------|------|
| CS  (片选)  | PC9  | `OLED_CS`  |
| DC  (数据/命令) | PC8  | `OLED_DC`  |
| RES (复位)    | PB23 | `OLED_RES` |
| SCK (时钟)    | PB3  | SPI0_SCLK |
| MOSI (数据)   | PB2  | SPI0_PICO |

### GPIO 配置

```js
GPIO3.$name = "OLED";
GPIO3.associatedPins.create(3);
GPIO3.associatedPins[0].$name       = "RES";
GPIO3.associatedPins[0].pin.$assign = "PB23";
GPIO3.associatedPins[1].$name       = "DC";
GPIO3.associatedPins[1].pin.$assign = "PC8";
GPIO3.associatedPins[2].$name       = "CS";
GPIO3.associatedPins[2].internalResistor = "PULL_UP";
GPIO3.associatedPins[2].pin.$assign = "PC9";
```

### SPI0 配置

```js
SPI1.$name                      = "SPI_0";
SPI1.frameFormat                = "MOTO3";   // CPOL=1, CPHA=1
SPI1.direction                  = "PICO";    // 控制器模式，仅发
SPI1.polarity                   = "1";
SPI1.phase                      = "1";
SPI1.peripheral.sclkPin.$assign = "PB3";
SPI1.peripheral.mosiPin.$assign = "PB2";
```

### 时钟树 (80MHz)

```
SYSOSC(32MHz) → SYSPLL(×5, ÷1, QDIV=5, PDIV=1) → 160MHz VCO
                                                    ↓
                                            UDIV(÷2) → 80MHz MCLK
                                                CLK0 = 80MHz
Flash Wait State = 2
ULPCLK = MCLK/2 = 40MHz
```

```js
const divider9       = system.clockTree["UDIV"];
divider9.divideValue = 2;
const multiplier2         = system.clockTree["PLL_QDIV"];
multiplier2.multiplyValue = 5;
const mux8       = system.clockTree["HSCLKMUX"];
mux8.inputSelect = "HSCLKMUX_SYSPLL0";
const mux10       = system.clockTree["MFPCLKMUX"];
mux10.inputSelect = "MFPCLKMUX_HFCLK";
const mux12       = system.clockTree["SYSPLLMUX"];
mux12.inputSelect = "SYSPLLMUX_SYSOSC";
SYSCTL.clockTreeEn = true;
```

### 关键参数

| 参数 | 值 |
|------|-----|
| MCLK | 80MHz |
| SPI 位速率 | 20MHz (80MHz / 4, SCR=1) |
| UART 波特率 | 9600 (IBRD=520, FBRD=53) |
| OLED 驱动 | SSD1306, 128×64, SPI 4线模式 |

### 文件结构

```
BSP/SPI0_OLED/
├── spi0_oled.c         # SSD1306 驱动
├── spi0_oled.h         # OLED API + 引脚宏
├── spi0_oledfont.h     # ASCII + 中文点阵字库
└── oledfont_bmp.h      # BMP 图片数据
```

### include 链

```
bsp.h ("../ti_msp_dl_config.h" → 提供 OLED_RES_PORT 等宏)
  └── SPI0_OLED/spi0_oled.h
        └── ../bsp.h (include guard 防重入)
```

### CS/DC/RES 电平约定

这三个控制引脚的电平由 **SSD1306 数据手册** 和 **SPI 协议** 规定，不是随意设置：

| 引脚 | 低电平 (0) | 高电平 (1) | 依据 |
|------|-----------|-----------|------|
| **CS** (片选) | 选中芯片，SPI 传输 | 释放芯片，总线空闲 | SPI 协议：片选低有效 |
| **DC** (数据/命令) | 写入的是**命令** (CMD) | 写入的是**数据** (DATA) | SSD1306 手册：D/C# 引脚 |
| **RES** (复位) | 复位 OLED 控制器 | 正常工作 | SSD1306 手册：复位低有效 |

`spi0_oled.c` 中 `OLED_WR_Byte()` 的实际操作流程：

```c
void OLED_WR_Byte(u8 dat, u8 cmd) {
    if (cmd) OLED_DC_Set();   // cmd=OLED_DATA(1) → DC拉高 → 接下来写数据
    else     OLED_DC_Clr();   // cmd=OLED_CMD(0)  → DC拉低 → 接下来写命令

    OLED_CS_Clr();             // CS拉低 → 选中OLED
    DL_SPI_transmitData8(SPI_0_INST, dat);  // 发送字节
    OLED_CS_Set();             // CS拉高 → 释放OLED
    OLED_DC_Set();             // DC恢复高电平（空闲状态）
}
```

`OLED_Init()` 中 RES 的复位时序：

```c
OLED_RST_Clr();   // RES拉低 → 复位开始
delay_ms(100);    // 保持低电平 ≥3μs (SSD1306要求)
OLED_RST_Set();   // RES拉高 → 复位结束，芯片重新初始化
delay_ms(100);    // 等待SSD1306内部启动完成
```

这些宏定义在 `spi0_oled.h` 中，实际 GPIO 操作由 `DL_GPIO_setPins/clearPins` 完成：

```c
#define OLED_CS_Clr()  DL_GPIO_clearPins(OLED_CS_PORT,  OLED_CS_PIN)
#define OLED_CS_Set()  DL_GPIO_setPins  (OLED_CS_PORT,  OLED_CS_PIN)
#define OLED_DC_Clr()  DL_GPIO_clearPins(OLED_DC_PORT,  OLED_DC_PIN)
#define OLED_DC_Set()  DL_GPIO_setPins  (OLED_DC_PORT,  OLED_DC_PIN)
#define OLED_RST_Clr() DL_GPIO_clearPins(OLED_RES_PORT, OLED_RES_PIN)
#define OLED_RST_Set() DL_GPIO_setPins  (OLED_RES_PORT, OLED_RES_PIN)
```
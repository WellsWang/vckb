# Vibe Coding Keyboard

## 简介

Vibe Coding Keyboard 是一个基于 ESP32-S3 的自定义键盘项目，支持 USB HID 键盘和鼠标功能。该键盘配备了一个 76x284 分辨率的 TFT-LCD 显示屏、9 个按键、1 个拨动开关和 1 个旋转编码器。通过配套的配置工具，可以灵活配置按键映射，支持多达 10 个键盘方案。

项目作者：Wells Wang (geek-logic.com)  
版本：0.1  
发布日期：2026/02/08  
许可证：GPL-3.0

## 功能特性

- **硬件核心**：ESP32-S3，具备原生 USB 接口
- **显示屏**：ST7789 驱动的 TFT-LCD，76x284 分辨率
- **输入设备**：
  - 9 个机械按键
  - 1 个拨动开关
  - 1 个旋转编码器（支持顺时针、逆时针和按下）
- **输出功能**：USB HID 键盘和鼠标
- **配置方案**：支持 10 个键盘映射方案，可切换激活
- **触发模式**：支持同时触发、顺序触发和粘滞触发
- **配置工具**：Python 桌面应用，支持实时配置和导入/导出配置

## 硬件要求

- ESP32-S3 开发板
- ST7789 TFT-LCD 显示屏 (76x284 分辨率)
- 9 个按键
- 1 个拨动开关
- 1 个旋转编码器
- 连接线和电源

### 引脚连接

#### LCD 显示屏 (ST7789)
- TFT_GND → GND
- TFT_VCC → 3.3V
- TFT_SCL → GPIO 12 (SCK)
- TFT_SDA → GPIO 11 (MOSI)
- TFT_RST → GPIO 9
- TFT_DC → GPIO 8
- TFT_CS → GPIO 10
- TFT_BL → GND

#### 按键和开关
- KEY0 → GPIO 3
- KEY1 → GPIO 4
- KEY2 → GPIO 5
- KEY3 → GPIO 6
- KEY4 → GPIO 7
- KEY5 → GPIO 14
- KEY6 → GPIO 15
- KEY7 → GPIO 16
- KEY8 → GPIO 17
- SW0 (拨动开关) → GPIO 38
- RT_BT (旋转编码器按钮) → GPIO 42
- RT_SA (旋转编码器 A) → GPIO 46
- RT_SB (旋转编码器 B) → GPIO 45

## 软件要求

- **固件**：
  - Arduino IDE
  - ESP32 板管理器
  - 所需库：Adafruit_GFX, Adafruit_ST7789, USB, USBHIDMouse, USBHIDKeyboard, EEPROM

- **配置工具**：
  - Python 3.x
  - Tkinter (通常随 Python 安装)
  - pyserial 库

## 安装和设置

### 固件烧录

1. 安装 Arduino IDE 和 ESP32 板管理器。
2. 安装所需库：Adafruit_GFX, Adafruit_ST7789, USBHID 等。
3. 打开 `src/Firmware/vckeyboard/vckeyboard.ino`。
4. 选择 ESP32-S3 板，设置 USB 模式为 USB-OTG。
5. 编译并上传固件到 ESP32-S3。

### 配置工具安装

1. 确保安装 Python 3.x。
2. 安装 pyserial：`pip install pyserial`
3. 运行 `src/Application/VCKBConfig.py`。

## 使用说明

1. 烧录固件后，键盘将作为 USB HID 设备连接到电脑。
2. 使用配置工具连接到键盘的串口（115200 波特率）。
3. 在配置工具中选择方案，设置按键映射。
4. 点击"写入"将配置保存到键盘的 EEPROM。
5. 键盘将根据配置响应按键输入。

### 远程配置模式

- 键盘上电时进入远程配置模式，可通过串口进行配置读写。
- 发送 0xFC 读取配置，0xFB 写入配置。

## 配置工具

`VCKBConfig.py` 是一个基于 Tkinter 的桌面应用，提供以下功能：

- 串口选择和连接
- 读取/写入键盘配置
- 导入/导出二进制配置文件
- 实时预览和编辑按键映射
- 支持多方案管理

### 按键映射格式

每个按键支持：
- 短名称（最多 10 字符）
- 触发模式：同时触发、顺序触发、粘滞触发
- 最多 5 个键值组合（基于 USB HID 键值表）

## 项目结构

```
vckb/
├── 3D Models/          # 3D 打印模型文件
├── src/
│   ├── Application/    # 配置工具
│   │   └── VCKBConfig.py
│   └── Firmware/       # 固件代码
│       └── vckeyboard/
│           └── vckeyboard.ino
└── README.md           # 本文件
```

## 许可证

本项目基于 GPL-3.0 开源协议发布。欢迎使用、修改和分发，但请保留原作者信息和开源协议声明。

## 作者

Wells Wang  
网站：geek-logic.com  
邮箱：（请联系作者获取）
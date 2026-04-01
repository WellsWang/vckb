/**************************************************************************
  Vibe Coding Keyboard
  ------------------------------------------------------------------------
  带有一个液晶显示屏的ESP32 S3 USB / 蓝牙自定义键盘

  液晶屏： 使用ST7789驱动的分辨率为76*284的TFT-LCD
  按键：
    拨动开关    x1
    按键        x9
    旋转编码器  x1
  
  硬件核心为 ESP32-S3，使用USB HID功能实现键盘和鼠标功能

  ------------------------------------------------------------------------
  
  作者： Wells Wang, geek-logic.com
  基于GPL-3.0开源协议发布，欢迎使用和修改，但请保留原作者信息和开源协议声明
  
  ------------------------------------------------------------------------
  
  版本 0.1 - 2026/02/08 初始版本，包含基本的显示和按键功能测试

 **************************************************************************/

///// EEPROM 设置
#include "EEPROM.h"
int addr = 0;
#define EEPROM_SIZE 2300     // EEPROM 大小，单位字节
#define EEPROM_VERSION 0x01 // EEPROM格式版本号，后续版本更新时可以通过修改此版本号来判断是否需要重新格式化EEPROM

///// LCD 设置
/**************************************************************************
LCD显示屏使用ST7789驱动，分辨率为76*284，连接方式为SPI接口
连接引脚：
  TFT_GND   - 连接到GND
  TFT_VCC   - 连接到3.3V
  TFT_SCL/K - 连接到数字引脚 12 (SCK)
  TFT_SDA   - 连接到数字引脚 11 (MOSI)
  TFT_RST   - 连接到数字引脚 9 (或设置为 -1 并连接到 Arduino RESET 引脚) 
  TFT_DC    - 连接到数字引脚 8 
  TFT_CS    - 连接到数字引脚 10
  TFT_BL    - 连接到GND
 **************************************************************************/
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <SPI.h>

#define TFT_CS        10
#define TFT_RST        9 // Or set to -1 and connect to Arduino RESET pin
#define TFT_DC         8
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST); // 液晶屏幕

///// ESP32-S3 USB模式设置
#ifndef ARDUINO_USB_MODE
#error This ESP32 SoC has no Native USB interface
#elif ARDUINO_USB_MODE == 1
#warning This sketch should be used when USB is in OTG mode
void setup() {}
void loop() {}
#else

  #include "USB.h"
  #include "USBHIDMouse.h"
  #include "USBHIDKeyboard.h"
  USBHIDMouse Mouse;
  USBHIDKeyboard Keyboard;

  //按键引脚设置
  const int KEY0 = 3;
  const int KEY1 = 4;
  const int KEY2 = 5;
  const int KEY3 = 6;
  const int KEY4 = 7;
  const int KEY5 = 14;
  const int KEY6 = 15;
  const int KEY7 = 16;
  const int KEY8 = 17;

  const int SW0 = 38;     // 拨动开关
  const int RT_BT = 42;   // 旋转编码器按钮
  const int RT_SA = 46;   // 旋转编码器A
  const int RT_SB = 45;   // 旋转编码器B
  
  const unsigned long DEBOUNCE_MS = 15; // 按键去抖时间，单位毫秒
  
  // 按键状态结构体
  struct Key {
    uint8_t pin;
    bool lastState;        // 上一次读取的原始电平
    unsigned long lastChange; 
    bool pressed;          // 稳定的按下状态
  };

  // 定义按键数组，包含9个按键，初始状态为未按下（HIGH）
  Key keys[9] = {
    {KEY0, HIGH, 0, false},
    {KEY1, HIGH, 0, false},
    {KEY2, HIGH, 0, false},
    {KEY3, HIGH, 0, false},
    {KEY4, HIGH, 0, false},
    {KEY5, HIGH, 0, false},
    {KEY6, HIGH, 0, false},
    {KEY7, HIGH, 0, false},
    {KEY8, HIGH, 0, false}
  };

  int key_history[3] = {-1, -1, -1};  // 按键历史记录

  // 键值映射结构体
  struct Key_Value {
    String name;
    byte value;
  };

  // 键值映射表，包含常用键盘按键和功能键
  Key_Value key_val[105] = {
    {"None", 0x00},
    {"`", 0x60},
    {"1", 0x31},
    {"2", 0x32},
    {"3", 0x33},
    {"4", 0x34},
    {"5", 0x35},
    {"6", 0x36},
    {"7", 0x37},
    {"8", 0x38},
    {"9", 0x39},
    {"0", 0x30},
    {"-", 0x2D},
    {"=", 0x3D},
    {"Space", 0x20},
    {"A", 0x61},
    {"B", 0x62},
    {"C", 0x63},
    {"D", 0x64},
    {"E", 0x65},
    {"F", 0x66},
    {"G", 0x67},
    {"H", 0x68},
    {"I", 0x69},
    {"J", 0x6A},
    {"K", 0x6B},
    {"L", 0x6C},
    {"M", 0x6D},
    {"N", 0x6E},
    {"O", 0x6F},
    {"P", 0x70},
    {"Q", 0x71},
    {"R", 0x72},
    {"S", 0x73},
    {"T", 0x74},
    {"U", 0x75},
    {"V", 0x76},
    {"W", 0x77},
    {"X", 0x78},
    {"Y", 0x79},
    {"Z", 0x7A},
    {"[", 0x5B},
    {"]", 0x5D},
    {"\\", 0x5C},
    {";", 0x3B},
    {"'", 0x27},
    {",", 0x2C},
    {".", 0x2E},
    {"/", 0x2F},
    {"Left Ctrl", 0x80},
    {"Left Shift", 0x81},
    {"Left Alt", 0x82},
    {"Left GUI", 0x83},
    {"Right Ctrl", 0x84},
    {"Right Shift", 0x85},
    {"Right Alt", 0x86},
    {"Right GUI", 0x87},
    {"Up Arrow", 0xDA},
    {"Down Arrow", 0xD9},
    {"Left Arrow", 0xD8},
    {"Right Arrow", 0xD7},
    {"Menu", 0xED},
    {"Backspace", 0xB2},
    {"Tab", 0xB3},
    {"Return", 0xB0},
    {"ESC", 0xB1},
    {"Insert", 0xD1},
    {"Delete", 0xD4},
    {"Page Up", 0xD3},
    {"Page Down", 0xD6},
    {"Home", 0xD2},
    {"End", 0xD5},
    {"Num Lock", 0xDB},
    {"Caps Lock", 0xC1},
    {"F1", 0xC2},
    {"F2", 0xC3},
    {"F3", 0xC4},
    {"F4", 0xC5},
    {"F5", 0xC6},
    {"F6", 0xC7},
    {"F7", 0xC8},
    {"F8", 0xC9},
    {"F9", 0xCA},
    {"F10", 0xCB},
    {"F11", 0xCC},
    {"F12", 0xCD},
    {"Print Screen", 0xCE},
    {"Scroll Lock", 0xCF},
    {"Pause", 0xD0},
    {"Keypad /", 0xDC},
    {"Keypad *", 0xDD},
    {"Keypad -", 0xDE},
    {"Keypad +", 0xDF},
    {"Keypad Enter", 0xE0},
    {"Keypad 1", 0xE1},
    {"Keypad 2", 0xE2},
    {"Keypad 3", 0xE3},
    {"Keypad 4", 0xE4},
    {"Keypad 5", 0xE5},
    {"Keypad 6", 0xE6},
    {"Keypad 7", 0xE7},
    {"Keypad 8", 0xE8},
    {"Keypad 9", 0xE9},
    {"Keypad 0", 0xEA},
    {"Keypad .", 0xEB}
  };

  // 旋转编码器状态结构体
  struct RotateEncoder  {
    int pinA;
    int pinB;
    int pinBtn;
    bool lastStateA;
    bool lastStateB;
    bool lastStateBtn;
    bool pressed;
    unsigned long lastChangeA;
    unsigned long lastChangeB;
    unsigned long lastPressedBtn;
    unsigned long lastReleasedBtn;
  };

  // 定义旋转编码器，初始状态为未按下（HIGH）
  RotateEncoder rotEnc = {RT_SA, RT_SB, RT_BT, HIGH, HIGH, HIGH, false, 0, 0, 0, 0};
  #define ROT_ENC_LONG_PRESS_MS 1000 // 旋转编码器按钮长按时间，单位milliseconds
  

  byte keymap[12][6]; // 12个按键设置，每个按键最多支持5个键值的组合，每个键值1字节，前面1字节表示同时激活或顺序按下
  String currentSchemeName = "Scheme 1";
  
  bool needDisplayRefresh = true;

  // 菜单项结构体，包含菜单名称、操作函数和父菜单ID
  struct MenuItem {
    String name;
    void (*action)();
    int parentId; // 父菜单ID，-1表示顶级菜单
    byte subMode; // 子功能的模式
                  // subMode的值可以根据需要定义不同的功能模式，例如：
                  // 0：普通USB模式，正常运行中
                  // 1: 蓝牙模式，和普通USB模式一样，仅激活了蓝牙连接
                  // 2: 网络设置模式，进入后显示网络IP地址等信息可以通过访问网页进行配置
                  // 3：菜单操作界面，显示菜单操作界面
                  // 11：激活方案界面，显示当前方案列表，可以选择要激活的方案
                  // 12：修改方案界面，显示当前方案列表，可以选择要修改的方案
                  // 13：蓝牙设置界面，显示蓝牙设置选项，可以选择要修改的蓝牙设置
                  // 14：网络设置界面，显示网络设置选项，可以选择要修改的网络设置
  };

  void switchScheme();        // 前向声明激活方案的函数，以便在菜单项中使用
  void modifySchemeName();    // 前向声明修改方案名称的函数，以便在菜单项中使用
  void setKeyShortName();     // 前向声明修改按键短名称的函数，以便在菜单项中使用
  void toggleActive();        // 前向声明切换激活方案的函数，以便在菜单项中使用
  void toggleTriggerMode();   // 前向声明切换触发模式的函数，以便在菜单中使用
  void selectKey();           // 前向声明选择按键值得函数，以便在菜单项中使用
  void remoteConfigMode();    // 前向声明远程配置模式函数，以便在菜单项中使用

  // 定义菜单项，包含顶级菜单和子菜单
  #define MENU_SIZE 41
  #define IDX_SCHEME_START 8
  #define IDX_KEY_START 21
  #define IDX_KEY_VALUE_START 36

  MenuItem menu[MENU_SIZE] ={                                 // ID
    {"Normal", nullptr, -1, 0},                               // 0
    {"Settings", nullptr, -1, 3},                             // 1
    {"Remote Config",  remoteConfigMode, -1, 2},              // 2
    {"Primary Scheme", switchScheme, 1, 11},                  // 3
    {"Secondary Scheme", switchScheme, 1, 12},                // 4
    {"Modify Scheme", nullptr, 1, 4},                         // 5
    {"Bluetooth Settings", [](){}, 1, 13},                    // 6
    {"Network Settings", [](){}, 1, 14},                      // 7
    {"Scheme 1", nullptr, 5, 3},                              // 8
    {"Scheme 2", nullptr, 5, 3},                              // 9  
    {"Scheme 3", nullptr, 5, 3},                              // 10
    {"Scheme 4", nullptr, 5, 3},                              // 11
    {"Scheme 5", nullptr, 5, 3},                              // 12
    {"Scheme 6", nullptr, 5, 3},                              // 13
    {"Scheme 7", nullptr, 5, 3},                              // 14
    {"Scheme 8", nullptr, 5, 3},                              // 15
    {"Scheme 9", nullptr, 5, 3},                              // 16
    {"Scheme 10", nullptr, 5, 3},                             // 17
    {"Scheme Name", modifySchemeName, 8, 21},                 // 18
    {"Active / Deactive", toggleActive, 8, 25},               // 19
    {"Key Mapping", nullptr, 8, 5},                           // 20
    {"Key 1", nullptr, 20, 3},                                // 21
    {"Key 2", nullptr, 20, 3},                                // 22
    {"Key 3", nullptr, 20, 3},                                // 23
    {"Key 4", nullptr, 20, 3},                                // 24
    {"Key 5", nullptr, 20, 3},                                // 25
    {"Key 6", nullptr, 20, 3},                                // 26
    {"Key 7", nullptr, 20, 3},                                // 27
    {"Key 8", nullptr, 20, 3},                                // 28
    {"Key 9", nullptr, 20, 3},                                // 29
    {"RotEnc Clockwise", nullptr, 20, 3},                     // 30
    {"RotEnc Counter Clockwise", nullptr, 20, 3},             // 31
    {"RotEnc Button", nullptr, 20, 3},                        // 32
    {"Modify Key Value", nullptr, 21, 3},                     // 33
    {"Set Trigger Mode", toggleTriggerMode, 21, 31},          // 34
    {"Set Key Name", setKeyShortName, 21, 32},                // 35
    {"Key Value 1", selectKey, 33, 35},                       // 36
    {"Key Value 2", selectKey, 33, 35},                       // 37
    {"Key Value 3", selectKey, 33, 35},                       // 38
    {"Key Value 4", selectKey, 33, 35},                       // 39
    {"Key Value 5", selectKey, 33, 35},                       // 40
  };

  int currentMenuId = 1; // 当前菜单ID，初始为0
  int prevMenuId = 0;   // 上一个菜单ID
  int nextMenuId = 0;   // 下一个菜单ID
  int currentMode = 0;     // 当前功能模式，初始为0

  byte currentSchemeId = 1; // 当前激活的方案ID，初始为1
  byte schemeId = 1;    // 当前正在修改的方案ID，初始为1
  byte nextSchemeId = 1;    // 下一个方案ID
  byte prevSchemeID = 1;
  byte activeSchemeId;
  int pos = 0; // 当前输入位置
  bool isActive; // 当前方案是否激活
  bool primary = true; // 是否为主方案，方案切换用
  byte keyId = 0;
  int keyIndex = 0; //当前键值索引
  #define KEY_INTERVAL 50   //顺序按键间隔时间ms
  #define KEY_SHORT_NAME_LENGTH 7 // 按键短名称字符串长度

  byte serialMode = 'N';      // 串口模式，N 未连接，W 等待链接，R 接收，T 发送
  unsigned long serialStart = 0;        // 串口开始发送接收时间
  int bytesCount = 0;         // 串口接收数据计数器
  char configData[EEPROM_SIZE];   // 配置数据
  #define SER_TIMEOUT 60      // 串口信息发送接收timeout时长，60秒

  struct Scheme {
    String name;
    bool active;
    byte keymap[12][6];
    String keyname[12];
  };
  Scheme schemes[10]; // 最多支持10组方案



  //////////////////// DATA 相关 ////////////////////
  
  // 格式化EEPROM
  void formatEEPROM() {
    /*
      EEPROM存储信息格式：
      地址范围      内容
      0             当前激活的键盘方案ID，1字节，默认值 0x01，即方案1
      1             键盘方案的总数量，1字节，默认值 0x0A , 10组方案
      2             设置顺序按下时的时间间隔，单位10毫秒，1字节，默认值 0x05，即50毫秒
      3             快速切换的第二套方案ID，1字节，默认值0x01，即方案1
      4-94          保留空间，未来存放蓝牙、网络等配置信息
      95            EEPROM格式版本号，1字节，默认值0x01，后续版本更新时可以通过修改此版本号来判断是否需要重新格式化EEPROM
      
      // 后续共10组96字节，每组表示一个方案的按键映射内容
      96-191        方案1的按键映射内容，96字节
        96字节内容格式：
          0         是否激活此方案，0不激活，1激活，1字节，默认值只有方案1为1，其余为0
          1-23      方案名称，23字节，最多支持23个字符的方案名称，超过部分会被截断，默认值为"Scheme {ID}"
          24-95     方案的按键映射内容，72字节，共9个按键加旋转编码器的两个方向和一个按键的设置（12个设置），每个按键6个字节
            每个按键的6字节内容格式：
              0         同时激活或顺序按下，0为同时激活，1为顺序按下，1字节，后续扩展可能增加更多的激活方式，默认值为0
              1-5       5个键值，每个1字节，表示按键按下时要发送的键值，最多支持5个键值的组合，超过部分会被忽略，键值参考USB HID键盘使用的键值表，0表示无按键，默认值为0

      192-287       方案2的按键映射内容，96字节
      ...
      960-1055       方案10的按键映射内容，96字节  
      1056-1099     保留空间
      1100-2299     1200字节按键短名称，10个按键配置方案，每个方案12个按键，每个按键10字节空间
        1100-1219   120个字节 方案一的按键短名称
          1100-1109 方案一的第一个按键的短名称
          ...
          1210-1219 方案二的第十二个按键的短名称
          ...
          ...
          ...
    */

    // 初始化EEPROM内容
    // 地址0：当前激活的键盘方案ID，默认值0x01（方案1）
    EEPROM.write(0, 0x01);
    
    // 地址1：键盘方案总数，默认值0x0A（10组方案）
    EEPROM.write(1, 0x0A);
    
    // 地址2：顺序按下时的时间间隔（单位10毫秒），默认值0x05（50毫秒）
    EEPROM.write(2, 0x05);

    // 地址3：快速切换的第二套方案ID，默认值0x01（方案1），两套方案相同表示不切换
    EEPROM.write(3, 0x01);
    
    // 地址4-95：保留空间，初始化为0
    for (int i = 4; i < 95; i++) {
      EEPROM.write(i, 0);
    }
    
    // 地址95：EEPROM格式版本号
    EEPROM.write(95, EEPROM_VERSION);

    // 地址96-1055：10组方案数据，每组96字节
    for (int scheme = 0; scheme < 10; scheme++) {
      int baseAddr = 96 + scheme * 96;
      
      // 第0字节：是否激活此方案（只有方案1激活）
      EEPROM.write(baseAddr, (scheme == 0) ? 1 : 0);
      
      // 第1-23字节：方案名称
      String schemeName = "Scheme " + String(scheme + 1);
      for (int j = 0; j < 23; j++) {
      EEPROM.write(baseAddr + 1 + j, (j < schemeName.length()) ? schemeName[j] : 0);
      }
      
      // 第24-95字节：按键映射内容（12个按键×6字节），初始化为0
      for (int i = 24; i < 96; i++)
        EEPROM.write(baseAddr + i, 0);
      
      // 初始化按键短名称
      for (int i = 0; i < 120; i++) 
        EEPROM.write(1100 + scheme * 120 + i, 0);
    }
    EEPROM.commit();
  }

  //读取当前按键方案名称
  String getSchemeName(byte id) {
    /*
      读取指定ID的方案名称
      参数：
        id：方案ID，范围1-10，对应EEPROM中地址96-1055的10组方案数据
      返回值：
        方案名称字符串，最多支持23个字符，超过部分会被截断
    */
    if (id < 1 || id > 10) {
      Serial.println("Invalid scheme ID");
      return "";
    }
    int baseAddr = id * 96;
    String schemeName = "";
    for (int i = 0; i < 23; i++) {
      char c = EEPROM.read(baseAddr + 1 + i);
      if (c == 0) break; // 遇到字符串结束符
      schemeName += c;
    }
    return schemeName;
  }

  //读取按键短名称

  String getKeyShortName(byte scheme_id, byte key_id){
    /*
      获取按键的短名称
      
      从EEPROM中读取指定方案和按键的短名称。如果未设置名称或名称为空，
      则返回默认名称 "KEYx"（x为按键ID）。
      
      参数：
      scheme_id 方案ID，用于定位EEPROM中不同方案的存储位置
      key_id 按键ID，用于定位该方案内的特定按键
      
      返回值：
      String 按键的短名称，长度最多为KEY_SHORT_NAME_LENGTH个字符，
            如果未设置则返回默认名称"KEYx"
    
      细节说明：
        - 基础地址：1100
        - 地址计算：baseAddr + scheme_id * 120 + key_id * 10 + i
        - 每个按键分配10字节空间存储短名称
        - 字符串以null终止符(0)或达到最大长度时结束
      
      注意： 确保scheme_id和key_id的有效性，避免越界访问EEPROM
    */
    int baseAddr = 1100;
    String short_name = "";
    for (int i = 0 ; i < 10 ; i++){
      char c = EEPROM.read(baseAddr + scheme_id * 120 + key_id * 10 + i);
      if (c==0 || i == KEY_SHORT_NAME_LENGTH) break; // 字符串结束或超过最大长度
      short_name += c;
    }
    if (short_name == "") short_name = "KEY" + String(key_id); //默认名称
    return short_name;
  }
  
  void getSchemeList() {
    /*
      获取所有方案的名称和激活状态，并存储在全局变量schemes数组中
      参数：
        无
      返回值：
        无，获取到的方案信息会存储在全局变量schemes数组中
    */
    for (byte id = 1; id <= 10; id++) {
      int baseAddr = id * 96;
      schemes[id - 1].active = (EEPROM.read(baseAddr) == 1);
      schemes[id - 1].name = getSchemeName(id);
      menu[IDX_SCHEME_START - 1 + id].name = schemes[id - 1].name; // 更新菜单项名称
      for (int i = 0; i < 12; i++) {
        for (int j = 0; j < 6; j++) {
          schemes[id - 1].keymap[i][j] = EEPROM.read(baseAddr + 24 + i * 6 + j);
        }
        schemes[id - 1].keyname[i] = getKeyShortName(id -1, i);
      }
    }
  }

  byte findActiveSchemeId(byte curId, bool direction){
    /*
      在EEPROM中查找下一个或上一个激活的方案ID
      参数：
        curId：当前方案ID，范围1-10
        direction：查找方向，true表示顺时针（下一个），false表示逆时针（上一个）
      返回值：
        下一个或上一个激活的方案ID，如果没有找到则返回当前方案ID 
    */
    int startAddr = curId * 96; // 当前方案的EEPROM地址
    int step = direction ? 96 : -96; // 步长，根据方向决定是向后还是向前查找
    int addr = startAddr; // 从当前方案开始查找
    do {
      addr += step; // 移动到下一个或上一个方案的地址
      if (addr < 96) {
        addr = 960; // 如果越过第一个方案，则跳转到最后一个方案
      } else if (addr > 960) {
        addr = 96; // 如果越过最后一个方案，则跳转到第一个方案
      }
      if (EEPROM.read(addr) == 1) { // 检查该方案是否激活
        return byte (addr / 96) ; // 返回找到的激活方案ID
      }
    } while (addr != startAddr); // 如果回到起始地址，说明没有找到其他激活的方案
    return curId; // 没有找到其他激活的方案，返回当前方案ID
  }

  void updateMenuParentID(int menu_id, int start_idx, int end_idx){
    /*
      更新菜单项的父菜单ID，将指定范围内的菜单项的父菜单ID设置为指定的菜单ID
      参数：
        menu_id：新的父菜单ID
        start_idx：起始索引
        end_idx：结束索引
      返回值：无
    */
    for (int i = start_idx; i < end_idx; i++) 
      menu[i].parentId = menu_id; // 将修改按键映射的菜单项的父菜单ID
  }

  // 更新按键历史记录
  void updateKeyHistory(int key_index){
    /*
      更新按键历史记录，将最新的按键索引添加到历史记录中
      参数：
        key_index：按键索引
      返回值：无
    */
    key_history[0] = key_history[1];
    key_history[1] = key_history[2];
    key_history[2] = key_index;
  }

  // 根据键值获取键号索引
  int getKeyCodeIndex(byte val){
    /*
      根据键值查找对应的键号索引
      参数：
        val：键值
      返回值：键号索引，如果未找到返回-1
    */
    for (int i = 0; i < 105; i++){
      if (key_val[i].value == val)
        return i;  
    }
    return -1;
  }


  //////////////////// UI相关 ////////////////////

  // 绘制菜单项
  void drawMenuItem(int selectedId) {
    /*
      绘制指定父菜单ID下的菜单项列表
      如果菜单项数目大于三项，则需要实现滚动显示功能，尽可能让选中项显示在屏幕中间，并且在选中项上方和下方各显示一个菜单项，如果选中项在顶部或底部，则优先显示选中项和其相邻的两个菜单项
      同时更新prevMenuId和nextMenuId变量，以便在用户按下旋转编码器时能够正确地切换菜单项。
      当选中项为第一项时，prevMenuId应该指向最后一项，nextMenuId指向第二项；当选中项为最后一项时，prevMenuId应该指向倒数第二项，nextMenuId指向第一项；其他情况下prevMenuId指向选中项的前一项，nextMenuId指向选中项的后一项。
      参数：
        parentId：父菜单ID，-1表示顶级菜单
        selectedId：当前选中的菜单项ID
      返回值：
        无，菜单项会直接绘制到屏幕上
    */
   
    // 找到所有属于该父菜单的菜单项
    int menuItems[MENU_SIZE];
    int menuCount = 0;
    for (int i = 0; i < MENU_SIZE; i++) {
      if (menu[i].parentId == menu[selectedId].parentId) {
        menuItems[menuCount++] = i;
      }
    }
    
    if (menuCount == 0) return;
    
    // 找到选中项在该父菜单中的索引
    int selectedIndex = -1;
    for (int i = 0; i < menuCount; i++) {
      if (menuItems[i] == selectedId) {
      selectedIndex = i;
      break;
      }
    }
    if (selectedIndex == -1) selectedIndex = 0;
    
    // 更新prevMenuId和nextMenuId
    currentMenuId = selectedId;
    prevMenuId = menuItems[(selectedIndex - 1 + menuCount) % menuCount];
    nextMenuId = menuItems[(selectedIndex + 1) % menuCount];
    
    // 确定显示的起始项索引
    int startIndex = (selectedIndex - 1 + menuCount) % menuCount;

    // 绘制菜单项（最多显示3项）
    tft.setTextSize(1);
    int displayCount = (menuCount > 3) ? 3 : menuCount;
    int i;
    for (i = 0; i < displayCount; i++) {
      int itemIndex = (startIndex + i) % menuCount;
      int itemId = menuItems[itemIndex];
      bool isSelected = (itemIndex == selectedIndex);
      
      tft.setTextColor(isSelected ? ST77XX_RED : ST77XX_WHITE);
      tft.fillRect(0, 20 + i * 16, tft.width(), 20, isSelected ? ST77XX_WHITE : ST77XX_BLACK);
      tft.setCursor(10, 24 + i * 16 );
      tft.print(menu[itemId].name);
    }

    for (; i < 3; i++) {
      tft.fillRect(0, 20 + i * 16, tft.width(), 20, ST77XX_BLACK);
    }

  }

  void drawSubMenu(int id){
    /*
      绘制指定ID的子菜单项
      参数：
        id：菜单项ID
      返回值：无
    */
    for (int i = 0; i < MENU_SIZE; i++) {
      if ( menu[i].parentId == id) {
        drawMenuItem(i);
        break;
      }
    }
  }

  void drawSaveButton(bool inverse){
    /*
      绘制保存按钮，根据inverse参数决定是否反白显示
      参数：
        inverse：是否反白显示，true表示反白显示，false表示正常显示
      返回值：无
    */
    tft.fillRect(240, 60, 40, 16, inverse ? ST77XX_WHITE : ST77XX_BLACK);
    if (!inverse) {
      tft.drawRect(240, 60, 40, 16, ST77XX_WHITE); // 画边框
    } 
    tft.setCursor(248, 64);
    tft.setTextColor(inverse ? ST77XX_RED : ST77XX_WHITE);
    tft.print("Save");
  }

  char updateCurrentText(char currentChar, int direction, bool inverse = true) {
    /*
      更新当前选择字符的显示，同时更新schemes中对应方案名称的字符，以便在修改方案名称时能够实时预览输入的效果
      根据pos（当前第几个字符）来确定屏幕上显示的位置，以及确定schemes中对应方案名称中的第几个字符，当前正在修改的字符反白显示。
      修改的字符为当前位置的下一个或前一个字符（ASCII码），可用于修改的ASCII字符范围为32-126，超过范围时循环回到起始或结束位置。
      参数：
        currentChar: 当前的字符
        direction：方向，1 表示当前字符下一个字符，-1 表示前一个字符, 0表示当前字符
      返回值：  变更后的字符，更新后的文本会直接显示在屏幕上
    */
    //char currentChar = currentSchemeName[pos]; // 获取当前字符

    if (currentChar < 32 || currentChar > 126) currentChar = 32;

    if (direction == 1) {
      currentChar++; // 向下一个字符移动
      if (currentChar > 126) currentChar = 32; // 如果超出范围，循环到起始字符
    } else if (direction == -1) {
      currentChar--; // 向前一个字符移动
      if (currentChar < 32) currentChar = 126; // 如果超出范围，循环到结束字符
    }

    //currentSchemeName[pos] = currentChar; // 更新currentSchemeName中的字符

    // 在屏幕上更新显示
    tft.fillRect(10 + pos * 6, 36, 6, 16, (inverse ? ST77XX_WHITE : ST77XX_BLACK));
    tft.setCursor(10 + pos * 6, 40);
    tft.setTextColor(inverse ? ST77XX_RED : ST77XX_WHITE);
    tft.print(currentChar);

    return currentChar;
  }

  void moveCursor(int max_length, String str, int direction) {
    /*
      移动光标位置，direction为1表示向右移动，-1表示向左移动
      光标位置由pos变量表示，范围0 - max_length，对应方案名称的max_length -1个字符位置
      当光标移动到最左或最右时，再次移动会循环回到另一端
    */
    if (pos != max_length)
      updateCurrentText(str[pos], 0, false); // 更新显示当前字符为未选中状态
    else
      drawSaveButton(false); // 如果当前光标在保存按钮上，则更新显示为未选中状态
    
    pos += direction;
    pos = (max_length + 1 + pos) % (max_length + 1);

    if (pos != max_length )
      updateCurrentText(str[pos], 0, true); // 更新显示当前字符为选中状态
    else
      drawSaveButton(true); // 如果当前光标在保存按钮上，则更新显示为选中状态
  }

  void updateTitle(String title){
    /*
      更新屏幕标题，根据当前模式设置背景颜色并显示标题
      参数：
        title：要显示的标题字符串
      返回值：无
    */
    tft.fillRect(0, 0, tft.width(), 20, (currentMode > 0) ? ST77XX_WHITE : ST77XX_GREEN);
    tft.setCursor(10, 2);
    tft.setTextColor(ST77XX_BLACK);
    tft.setTextSize(2);
    tft.print(title);
    tft.setTextSize(1);
  }

  // 显示触发的按键名称
  void displayKeyNames(byte key_index){
    /*
      显示指定按键索引对应的按键名称组合
      参数：
        key_index：按键索引
      返回值：无
    */
    bool first = true;
    String seperator;
    if (schemes[activeSchemeId - 1].keymap[key_index][0] == 0)
      seperator = " + ";
    else if (schemes[activeSchemeId - 1].keymap[key_index][0] == 1)
      seperator = " , ";
    // else
      seperator = " .. ";
    for (int i = 1; i <= 5; i++){
      if( schemes[activeSchemeId - 1].keymap[key_index][i] !=0 ) {
        if (!first) tft.print(seperator); // 多个按键间显示间隔符
        tft.print(key_val[getKeyCodeIndex(schemes[activeSchemeId - 1].keymap[key_index][i])].name);
        first = false;
      }
    }
    if (first) tft.print("...NOTHING...");
  }

  // 显示按键历史
  void displayKeyHistory(){
    /*
      显示最近按下的按键历史记录
      参数：无
      返回值：无
    */
    tft.fillRect(0, 20, tft.width(), tft.height() - 20, ST77XX_BLACK);
    tft.setTextColor(ST77XX_GREEN);
    for (int i = 0; i < 3; i++){
      if (key_history[i] != -1){
        tft.setCursor(10, 24 + i * 16 );
        tft.print(schemes[activeSchemeId - 1].keyname[key_history[i]]);
        tft.print(": ");
        displayKeyNames(key_history[i]);
      }
    }
  }

  /////////////////////////////////////////////////////

  //////////////////// 菜单功能相关 ////////////////////

  ////////// 修改方案名称 //////////
  
  void modifySchemeName() {
    /*
      显示修改方案名称的界面，允许用户输入新的方案名称并保存到EEPROM中
      方案名称最多支持23个字符，超过部分会被截断
    */
    pos = 0; // 当前输入位置
    currentSchemeName = getSchemeName(schemeId); // 获取当前方案名称
    //如果currentSchemeName长度不足23个字符，则补齐空格，以便在屏幕上显示固定长度的方案名称
    while (currentSchemeName.length() < 23) {
      currentSchemeName += " ";
    }
    
    tft.fillScreen(ST77XX_BLACK);
    updateTitle("Modify Scheme Name");
    tft.setCursor(10, 24);
    tft.setTextColor(ST77XX_WHITE);
    tft.print("Modify Scheme Name:");
    tft.setCursor(10, 40);
    tft.print(currentSchemeName);
    drawSaveButton(false); // 绘制保存按钮，初始为未选中状态
    updateCurrentText(currentSchemeName[pos], 0); // 显示第一个字符为当前选择字符
  }

  void saveSchemeName() {
    /*
      将当前的currentSchemeName保存到EEPROM中对应方案的名称位置
    */
    tft.fillRect(240, 60, 40, 16, ST77XX_BLACK);
    // 如果当前选择的是保存按钮，则保存修改后的方案名称到EEPROM中，并返回菜单界面
    String newName = currentSchemeName.substring(0, 23); // 获取修改后的方案名称，去掉末尾的空格
    newName.trim();

    int baseAddr = schemeId * 96;
    for (int i = 0; i < 23; i++) {
      EEPROM.write(baseAddr + 1 + i, (i < newName.length()) ? newName[i] : 0); // 将修改后的方案名称写入EEPROM中
    }
    EEPROM.commit();
    getSchemeList(); // 更新schemes数组中的方案信息，以便在菜单界面能够正确显示修改后的方案名称
  } 

  ////////// 修改按键短名称 //////////

  void setKeyShortName() {
    /*
      显示修改按键短名称的界面，允许用户输入新的短名称并保存到EEPROM中
      方案名称最多支持KEY_SHORT_NAME_LENGTH个字符，超过部分会被截断
    */
    pos = 0; // 当前输入位置
    String key_short_name = schemes[schemeId - 1].keyname[keyId]; // 获取当前方案名称
    //如果currentSchemeName长度不足23个字符，则补齐空格，以便在屏幕上显示固定长度的方案名称
    while (key_short_name.length() < KEY_SHORT_NAME_LENGTH) {
      key_short_name += " ";
    }
    
    tft.fillScreen(ST77XX_BLACK);
    updateTitle("Key" + String( keyId + 1 ) + " Short Name");
    tft.setCursor(10, 24);
    tft.setTextColor(ST77XX_WHITE);
    tft.print("Set Short Name:");
    tft.setCursor(10, 40);
    tft.print(key_short_name);
    drawSaveButton(false); // 绘制保存按钮，初始为未选中状态
    updateCurrentText(key_short_name[pos], 0); // 显示第一个字符为当前选择字符
  }

  void saveKeyShortName(byte scheme_id, byte key_id, String key_short_name) {
    /*
      将当前的按键短名称保存到EEPROM中对应方案的名称位置
    */
    tft.fillRect(240, 60, 40, 16, ST77XX_BLACK);
    // 如果当前选择的是保存按钮，则保存修改后的方案名称到EEPROM中，并返回菜单界面
    key_short_name = key_short_name.substring(0, KEY_SHORT_NAME_LENGTH); // 获取修改后的方案名称，去掉末尾的空格
    key_short_name.trim();

    int baseAddr = 1100 + scheme_id * 120 + key_id * 10;
    for (int i = 0; i < 10; i++) {
      EEPROM.write(baseAddr + i, (i < key_short_name.length()) ? key_short_name[i] : 0); // 将修改后的方案名称写入EEPROM中
    }
    EEPROM.commit();
    getSchemeList(); // 更新schemes数组中的方案信息，以便在菜单界面能够正确显示修改后的方案名称
  } 

  ////////// 修改激活方案 //////////

  void updateActiveScheme(){
    /*
      更新激活方案的显示，在屏幕上显示当前激活方案的名称
      参数：无
      返回值：无
    */
    tft.fillRect(0, 36, tft.width(), 16, ST77XX_WHITE);
    tft.setCursor(10, 40);
    tft.setTextColor(ST77XX_RED);
    tft.print(getSchemeName(currentSchemeId));
  }

  void switchScheme() {
  /*
    显示currentSchemeId对应的方案名称,
    在屏幕上只反白显示一行
  */
    tft.fillScreen(ST77XX_BLACK);
    updateTitle("Switch Scheme");
    tft.setCursor(10, 24);
    tft.setTextColor(ST77XX_WHITE);
    tft.print("Switch to : ");
    updateActiveScheme();
  }

  void saveActiveScheme() {
    /*
      将当前的currentSchemeId保存到EEPROM中地址0（Primary） 或地址3（Secondary），以便下次开机时能够自动加载上次激活的方案
    */
    EEPROM.write((currentMode == 11) ? 0 : 3, currentSchemeId); //currerntMode为11时保存Primary Scheme ID, 为12（非11）时 保存Secondary Scheme ID
    EEPROM.commit();
    if ((primary && currentMode == 11) || (!primary && currentMode == 12)) activeSchemeId = currentSchemeId;
  }


  ////////// 修改激活状态 //////////

  void drawActiveStatus () {
    /*
      绘制当前方案的激活状态，isActive为true表示激活，false表示不激活
      激活状态通过在屏幕上显示"Active"或"Inactive"来表示，同时使用不同的颜色区分
    */
    tft.fillRect(10, 36, 80, 16, ST77XX_BLACK);
    if (isActive) {
      tft.fillRect(10, 36, 80, 16, ST77XX_WHITE);
      tft.setTextColor(ST77XX_RED);
      tft.setCursor(32, 40);
      tft.print("Active");
    } else {
      tft.drawRect(10, 36, 80, 16, ST77XX_WHITE); // 画边框
      tft.setTextColor(ST77XX_WHITE);
      tft.setCursor(26, 40);
      tft.print("Inactive");
    }
  } 

  void toggleActive() {
    /*
      切换当前方案的激活状态，并保存到EEPROM中
       如果当前方案是激活的，则切换为不激活；如果当前方案是不激活的，则切换为激活
       同时更新schemes数组中对应方案的激活状态，以便在菜单界面能够正确显示当前方案的激活状态
    */
    tft.fillScreen(ST77XX_BLACK);
    updateTitle("Toggle Active Status");
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(10, 24);
    tft.print("Set to:");
    isActive = (EEPROM.read(schemeId * 96) == 1);
    drawActiveStatus(); // 更新显示当前方案的激活状态
  }
  
  void saveActiveStatus() {
    /*
      将当前方案的激活状态保存到EEPROM中对应方案的地址位置
    */
    EEPROM.write(schemeId * 96, isActive ? 1 : 0); // 将当前方案的激活状态写入EEPROM中
    EEPROM.commit();
    getSchemeList(); // 更新schemes数组中的方案信息，以便在菜单界面能够正确显示当前方案的激活状态
  }


  ////////// 修改触发模式 //////////

  void drawTriggerMode() {
    /*
      绘制当前方案的激活状态，0表示同时触发，1表示顺序触发，2表示粘滞顺序触发
      激活状态通过在屏幕上显示"Simultaneous"或"Sequential"、"Stick Seq"来表示，同时使用不同的颜色区分
    */
    if ( schemes[schemeId - 1].keymap[keyId][0] == 0) {
      tft.fillRect(10, 36, 80, 16, ST77XX_GREEN);
      tft.setTextColor(ST77XX_BLACK);
      tft.setCursor(14, 40);
      tft.print("Simultaneous");
    } else if ( schemes[schemeId - 1].keymap[keyId][0] == 1){
      tft.fillRect(10, 36, 80, 16, ST77XX_BLUE); // 画边框
      tft.setTextColor(ST77XX_WHITE);
      tft.setCursor(20, 40);
      tft.print("Sequential");
    } else {
      tft.fillRect(10, 36, 80, 16, ST77XX_RED); // 画边框
      tft.setTextColor(ST77XX_WHITE);
      tft.setCursor(20, 40);
      tft.print("Stick Seq"); 
    }
  } 

  void toggleTriggerMode() {
    /*
      切换当前按键的触发模式，并保存到EEPROM中
       如果当前方案是同时触发的，则切换为顺序触发；如果当前方案是顺序触发，则切换为同时触发
    */
    tft.fillScreen(ST77XX_BLACK);
    updateTitle("Toggle Tigger Mode");
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(10, 24);
    tft.print("Set to:");
    schemes[schemeId - 1].keymap[keyId][0] = EEPROM.read(schemeId * 96 + 24 + keyId * 6);
    drawTriggerMode(); // 更新显示当前按键的触发模式
  }

  void saveTriggerMode() {
    /*
      将当前按键的触发模式保存到EEPROM中对应方案的地址位置
    */
    EEPROM.write(schemeId * 96 + 24 + keyId * 6, schemes[schemeId - 1].keymap[keyId][0]); // 将当前按键的触发方式写入EEPROM中
    EEPROM.commit();
  }

  ////////// 选择按键键值 //////////

  void drawKeySelector(int key_code_index) {
    /*
      绘制当前组合键的键值
      参数：
      key_code_index: 当前选择的键值在key_val数组中的索引，如果为-1表示未选择任何键值
      返回值：无
    */
    if (key_code_index != -1) {
      tft.fillRect(10, 36, 80, 16, ST77XX_BLACK);
      tft.drawRect(10, 36, 80, 16, ST77XX_WHITE); // 画边框
      tft.setTextColor(ST77XX_WHITE);
      tft.setCursor(50 - key_val[key_code_index].name.length() * 3, 40);
      tft.print(key_val[key_code_index].name);
      schemes[schemeId - 1].keymap[keyId][currentMenuId - IDX_KEY_VALUE_START + 1] = key_val[key_code_index].value;
    } 
  }

  void selectKey() {
    /*
      显示选择键值的界面，允许用户选择要设置的键值
      参数：无
      返回值：无
    */
    tft.fillScreen(ST77XX_BLACK);
    updateTitle(menu[currentMenuId].name);
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(10, 24);
    tft.print("Select Key");
    schemes[schemeId - 1].keymap[keyId][currentMenuId - IDX_KEY_VALUE_START + 1] = EEPROM.read(schemeId * 96 + 24 + keyId * 6 + currentMenuId - IDX_KEY_VALUE_START + 1);
    drawKeySelector(getKeyCodeIndex(schemes[schemeId - 1].keymap[keyId][currentMenuId - IDX_KEY_VALUE_START + 1]));
  }
  
  void saveKeyValue() {
    /*
      将当前组合键的键值保存到EEPROM中对应方案的地址位置
    */
    EEPROM.write(schemeId * 96 + 24 + keyId * 6 + currentMenuId - IDX_KEY_VALUE_START + 1, schemes[schemeId - 1].keymap[keyId][currentMenuId - IDX_KEY_VALUE_START + 1]); // 将当前按键的触发方式写入EEPROM中
    EEPROM.commit();
  }

  ////////// 返回上一级菜单 //////////
  void backToMenu() {
    /*
      返回菜单界面，显示当前菜单项
    */
    if (menu[currentMenuId].parentId != -1) {
      // 找到当前菜单的父菜单的subMode，以便返回到父菜单的功能模式
      currentMode = menu[menu[currentMenuId].parentId].subMode; // 切换到父菜单的功能模式
    }
    else 
      currentMode = 3; // 切换到菜单操作模式

    getSchemeList(); 
    drawMenuItem(currentMenuId); // 刷新当前菜单项显示
  }

  //////////////////////////////////////////////////

  //////////////////// 按键相关 ////////////////////
  
  void checkKey(Key &k, void (*onPressed)()) {
    /*
      检查按键的状态，并调用相应的回调函数处理按键事件，包括去抖动处理和状态变化检测
      参数：
        k：要检查的Key对象，包含按键的引脚和状态信息
        onPressed：按键按下时的回调函数
      返回值：无，事件发生时会调用相应的回调函数
    */
    bool state = digitalRead(k.pin);
    if (state != k.lastState) {
      k.lastChange = millis();
      k.lastState = state;
    } else {
      // 状态稳定，判断去抖
      if ((millis() - k.lastChange) > DEBOUNCE_MS) {
        bool isPressedNow = (state == LOW);
        if (isPressedNow && !k.pressed) {
          // 按下
          k.pressed = true;
          onPressed();
          needDisplayRefresh = true;
        } else if (!isPressedNow && k.pressed) {
          // 释放
          k.pressed = false;
          needDisplayRefresh = true;
        }
      }
    }
  }

  void checkRotateEncoder(RotateEncoder &enc, void (*onRotateCW)(), void (*onRotateCCW)(), void (*onButtonPressed)(), void (*onButtonLongPressed)()) {
    /*
      检查旋转编码器的状态，包括旋转和按钮按下事件，并调用相应的回调函数
      参数：
        enc：要检查的RotateEncoder对象，包含旋转编码器的引脚和状态信息
        onRotateCW：旋转编码器顺时针旋转时的回调函数
        onRotateCCW：旋转编码器逆时针旋转时的回调函数
        onButtonPressed：旋转编码器按钮按下时的回调函数
        onButtonLongPressed：旋转编码器按钮长按时的回调函数
      返回值：无，事件发生时会调用相应的回调函数 
    */
    bool stateA = digitalRead(enc.pinA);
    bool stateB = digitalRead(enc.pinB);
    bool stateBtn = digitalRead(enc.pinBtn);

    // 检查旋转状态（A引脚）
    if (stateA != enc.lastStateA) {
      if (millis() - enc.lastChangeA > DEBOUNCE_MS) {
        enc.lastChangeA = millis();
        enc.lastStateA = stateA;
        if (stateA == LOW) {
          if (stateB == HIGH) {
            onRotateCW();
          } else {
            onRotateCCW();
          }
        }
      }
    }

    // 检查按钮状态
    if (stateBtn != enc.lastStateBtn) {
      if (stateBtn == LOW) {
        enc.lastPressedBtn = millis();
      } else {
        enc.lastReleasedBtn = millis();
      }
      enc.lastStateBtn = stateBtn;
      if (stateBtn == LOW)
        enc.pressed = true;
      else
        enc.pressed = false;
    }
    else if ((enc.lastReleasedBtn - enc.lastPressedBtn) > ROT_ENC_LONG_PRESS_MS && !enc.pressed) {
      onButtonLongPressed();
      enc.lastPressedBtn = 0; // 重置长按标志
      enc.lastReleasedBtn = 0; // 重置长按标志
    }
    else if ((enc.lastReleasedBtn - enc.lastPressedBtn) > DEBOUNCE_MS && !enc.pressed) {
      onButtonPressed();
      enc.lastPressedBtn = 0; // 重置长按标志
      enc.lastReleasedBtn = 0; // 重置长按标志
    }

  }

  // 顺时针旋转编码器
  void onRotateEnc_CW(){
    /*
      处理旋转编码器顺时针旋转的事件，根据当前模式执行不同的操作
      参数：无
      返回值：无
    */
    switch (currentMode)
    {
      case 0:
        TriggerKey(9);
        break;
      case 4:
      case 5:
        if (currentMode == 4) {
          updateMenuParentID(nextMenuId, IDX_KEY_START - 3, IDX_KEY_START); // 更新修改方案菜单项的父菜单ID为当前选中的方案菜单项ID，以便在修改方案名称、激活状态、按键映射等子菜单中能够正确地显示当前选中的方案信息
          schemeId = nextMenuId - IDX_SCHEME_START + 1; // 更新当前正在修改的方案ID，nextMenuId是下一个菜单项ID，减去IDX_SCHEME_START是因为方案菜单项ID从IDX_SCHEME_START开始，最后加1是因为方案ID从1开始
        } else if (currentMode == 5) {
          updateMenuParentID(nextMenuId,IDX_KEY_VALUE_START - 3, IDX_KEY_VALUE_START);
          keyId = nextMenuId - IDX_KEY_START; // 通过菜单ID获取按键ID
          updateKeyLED(true);          
        }
      case 3:
        drawMenuItem(nextMenuId);
        break;
      case 11:
      case 12:
        currentSchemeId = findActiveSchemeId(currentSchemeId, true);
        updateActiveScheme(); // 显示当前激活的方案名称
        break;
      case 21:    //选择要修改的字符，顺时针旋转编码器移动光标到下一个位置
        moveCursor(23, currentSchemeName, 1); // 移动光标到下一个位置
        break;  
      case 22:    //修改当前选择的字符，顺时针旋转编码器将当前字符修改为下一个字符
        currentSchemeName[pos] = updateCurrentText(currentSchemeName[pos], 1); // 更新当前选择字符为下一个字符，并更新显示
        break;
      case 25:    //在修改方案激活状态界面，顺时针旋转编码器切换激活状态
        isActive = !isActive; // 切换激活状态
        drawActiveStatus(); // 更新显示当前方案的激活状态
        break;
      case 31:    //修改触发模式
        schemes[schemeId - 1].keymap[keyId][0] = (schemes[schemeId - 1].keymap[keyId][0] + 1) % 3;
        drawTriggerMode();
        break;
      case 32:  // 修改短名称，选择要修改的字符，光标移到下一个位置
        moveCursor(KEY_SHORT_NAME_LENGTH, schemes[schemeId - 1].keyname[keyId], 1); // 移动光标到下一个位置
        break;  
      case 33:    //修改当前选择的字符，顺时针旋转编码器将当前字符修改为下一个字符
        schemes[schemeId - 1].keyname[keyId][pos] = updateCurrentText(schemes[schemeId - 1].keyname[keyId][pos], 1); // 更新当前选择字符为下一个字符，并更新显示
        break;
      case 35:   //选择键值
        keyIndex = (keyIndex + 106) % 105;
        drawKeySelector(keyIndex);
        break;
      default:
        break;   
    }
  }

  // 逆时针旋转编码器
  void onRotateEnc_CCW(){
    /*
      处理旋转编码器逆时针旋转的事件，根据当前模式执行不同的操作
      参数：无
      返回值：无
    */
    switch (currentMode)
    {
      case 0:
        TriggerKey(10);
        break;
      case 4:
      case 5:
        if (currentMode == 4 ) {
          updateMenuParentID(prevMenuId, IDX_KEY_START - 3, IDX_KEY_START); // 更新修改方案菜单项的父菜单ID为当前选中的方案菜单项ID，以便在修改方案名称、激活状态、按键映射等子菜单中能够正确地显示当前选中的方案信息
          schemeId = prevMenuId - IDX_SCHEME_START + 1; // 更新当前正在修改的方案ID，prevMenuId是上一个菜单项ID，减去IDX_SCHEME_START是因为方案菜单项ID从IDX_SCHEME_START开始，最后加1是因为方案ID从1开始
        } else if (currentMode == 5) {
          updateMenuParentID(prevMenuId, IDX_KEY_VALUE_START - 3, IDX_KEY_VALUE_START);
          keyId = prevMenuId - IDX_KEY_START;  // 通过菜单ID获取按键ID
          updateKeyLED(true);
        }
      case 3:
        drawMenuItem(prevMenuId);
        break;
      case 11:
      case 12:
        currentSchemeId = findActiveSchemeId(currentSchemeId, false);
        updateActiveScheme(); // 显示当前激活的方案名称
        break;
      case 21:    //选择要修改的字符，逆时针旋转编码器移动光标到前一个位置
        moveCursor(23, currentSchemeName, -1); // 移动光标到前一个位置
        break;
      case 22:    //修改当前选择的字符，逆时针旋转编码器将当前字符修改为前一个字符
        currentSchemeName[pos] = updateCurrentText(currentSchemeName[pos], -1); // 更新当前选择字符的显示，但不改变字符内容，用于在修改字符后刷新显示
        break;
      case 25:    //在修改方案激活状态界面，顺时针旋转编码器切换激活状态
        isActive = !isActive; // 切换激活状态
        drawActiveStatus(); // 更新显示当前方案的激活状态
        break;
      case 31:    //修改触发模式
        schemes[schemeId - 1].keymap[keyId][0] = (schemes[schemeId - 1].keymap[keyId][0] + 1) % 3;
        drawTriggerMode();
        break;
      case 32:  // 修改短名称，选择要修改的字符，光标移到下一个位置
        moveCursor(KEY_SHORT_NAME_LENGTH, schemes[schemeId - 1].keyname[keyId], -1); // 移动光标到下一个位置
        break;  
      case 33:    //修改当前选择的字符，顺时针旋转编码器将当前字符修改为下一个字符
        schemes[schemeId - 1].keyname[keyId][pos] = updateCurrentText(schemes[schemeId - 1].keyname[keyId][pos], -1); // 更新当前选择字符为下一个字符，并更新显示
        break;
      case 35:   //选择键值
        keyIndex = (keyIndex + 104) % 105;
        drawKeySelector(keyIndex);
        break;
      default:
        break;   
    }
  }

  // 旋转编码器按钮按下
  void onRotateEnc_ButtonPressed(){
    /*
      处理旋转编码器按钮按下的事件，根据当前模式执行不同的操作
      参数：无
      返回值：无
    */
    switch (currentMode)
    {
      case 0:
        TriggerKey(11);
        break;
      case 4:
      case 5:
        if (currentMode == 4) {
          //schemeId = 1;
          updateMenuParentID(schemeId + IDX_SCHEME_START - 1, IDX_KEY_START - 3, IDX_KEY_START); // 更新修改方案菜单项的父菜单ID为当前选中的方案菜单项ID，以便在修改方案名称、激活状态、按键映射等子菜单中能够正确地显示当前选中的方案信息
        } else if (currentMode == 5){
          updateMenuParentID(keyId + IDX_KEY_START, IDX_KEY_VALUE_START - 3, IDX_KEY_VALUE_START); //更新修改方案菜单项的父菜单ID为当前选中按键的ID
          updateKeyLED(true);
        }
      case 3:
        if (menu[currentMenuId].subMode !=  currentMode) {
          currentMode = menu[currentMenuId].subMode; // 切换到菜单项对应的功能模式
          if (menu[currentMenuId].action != nullptr) {
            menu[currentMenuId].action(); // 执行菜单项的操作函数
            needDisplayRefresh = false;
          } else
            drawSubMenu(currentMenuId);
        }
        else 
          drawSubMenu(currentMenuId);
        break;
      case 11:
      case 12:
        saveActiveScheme(); // 保存当前激活的方案到EEPROM
        backToMenu(); // 返回菜单界面
        break;
      case 21:  // 在修改方案名称界面，按下按钮修改当前选择的字符
        if (pos != 23) {
          updateCurrentText(currentSchemeName[pos], 0); // 更新当前选择字符为下一个字符，并更新显示
          currentMode = 22; // 切换到修改字符模式
        } else {
          saveSchemeName(); // 保存修改后的方案名称到EEPROM
          backToMenu(); // 返回菜单界面
        }
        break;
      case 22:  // 在修改方案名称界面，按下按钮保存修改的字符
        currentMode = 21; // 切换回移动光标模式
        break;
      case 25:  // 在修改方案激活状态界面，按下按钮保存修改的激活状态
        saveActiveStatus(); // 保存修改后的激活状态到EEPROM
        backToMenu(); // 返回菜单界面
        break;
      case 31:  // 修改触发模式
        saveTriggerMode();
        backToMenu();
        break;
      case 32:  // 在修改方案名称界面，按下按钮修改当前选择的字符
        if (pos != KEY_SHORT_NAME_LENGTH) {
          updateCurrentText(schemes[schemeId - 1].keyname[keyId][pos], 0); // 更新当前选择字符为下一个字符，并更新显示
          currentMode = 33; // 切换到修改字符模式
        } else {
          saveKeyShortName(schemeId - 1, keyId, schemes[schemeId - 1].keyname[keyId]); // 保存修改后的方案名称到EEPROM
          backToMenu(); // 返回菜单界面
        }
        break;
      case 33:  // 在修改方案名称界面，按下按钮保存修改的字符
        currentMode = 32; // 切换回移动光标模式
        break;
      case 35:  // 修改键值
        saveKeyValue();
        backToMenu();
        break;
      default:
        break;
    }
    Serial.print("Current Mode:");
    Serial.println(currentMode);
    if (needDisplayRefresh && currentMode != 0 ) {
      if (menu[currentMenuId].parentId != -1)
        updateTitle(menu[menu[currentMenuId].parentId].name);
      else
        updateTitle("Configuration");
    }
    needDisplayRefresh = true;
  }

  void onRotateEnc_ButtonLongPressed(){
    /*
      处理旋转编码器按钮长按的事件，根据当前模式执行不同的操作
      参数：无
      返回值：无
    */
    // 长按时执行当前菜单项的操作函数

    switch (currentMode)
    {
//    case 0:
//      backToMenu(); // 返回菜单界面
//      break;
    case 5:
      updateKeyLED(false);
    case 4:
    case 3:
      if (menu[currentMenuId].parentId != -1) {
        if (menu[currentMenuId].parentId != -1)
          currentMode = menu[menu[menu[currentMenuId].parentId].parentId].subMode; // 切换到父菜单的功能模式
        else
          currentMode = 3;
        drawMenuItem(menu[currentMenuId].parentId); // 刷新当前菜单项显示
      }
      else{
        currentMode = 0; // 返回普通模式
        tft.fillScreen(ST77XX_BLACK);
        updateTitle(getSchemeName(activeSchemeId));
        setPinMode(INPUT_PULLUP);
        needDisplayRefresh = false;
      }
      break;
//    case 11:
//      backToMenu(); // 返回菜单界面
//      break;
    case 2:
      serialMode = 'N';
    default:
      backToMenu(); // 返回菜单界面
      break;
    }
    Serial.print("Current Mode:");
    Serial.println(currentMode);
    if (needDisplayRefresh && currentMode != 0 ) {
      if (menu[currentMenuId].parentId != -1)
        updateTitle(menu[menu[currentMenuId].parentId].name);
      else
        updateTitle("Configuration");
    }
    needDisplayRefresh = true;
  }


  // 触发按键
  void TriggerKey(int keyId){
    /*
     触发指定按键的按下和释放操作。
      
     此函数根据当前激活方案（activeSchemeId）和按键ID（keyId），
     遍历对应按键的按键映射（keymap），依次按下所有配置的按键。
     如果keymap的第0位为1，则在按下后短暂延时并释放所有按键，
     并在释放后再延时KEY_INTERVAL毫秒。
     如果keymap的第0位为0，则在按下后短暂延时并释放所有按键。
     
     参数：keyId 要触发的按键ID。
    */
     
    byte k1, k2;

    if (schemes[activeSchemeId - 1].keymap[keyId][0] == 0){ // 如果是同时触发，最后同时释放按键
      for (int i = 1 ; i < 6 ; i++){ //触发组合键
        if (schemes[activeSchemeId - 1].keymap[keyId][i]) {
          Keyboard.press(schemes[activeSchemeId - 1].keymap[keyId][i]);
        }
      }
      delay(10);
      Keyboard.releaseAll();
    } else if (schemes[activeSchemeId - 1].keymap[keyId][0] == 1) {  // 顺序触发，挨个释放按键并增加按键间隔
      for (int i = 1 ; i < 6 ; i++){ //触发组合键
        if (schemes[activeSchemeId - 1].keymap[keyId][i]) {
          Keyboard.press(schemes[activeSchemeId - 1].keymap[keyId][i]);
        }
        delay(10);
        Keyboard.releaseAll();
        delay(KEY_INTERVAL);
      }
    } else if (schemes[activeSchemeId - 1].keymap[keyId][0] == 2) {  // 粘滞触发，挨个释放按键并相邻两个键有粘滞时间
      k1 = 0; 
      k2 = 0;  
      for (int i = 1 ; i < 6 ; i++){ //触发组合键
        if (schemes[activeSchemeId - 1].keymap[keyId][i]) {
          k1 = k2;
          k2 = schemes[activeSchemeId - 1].keymap[keyId][i];
        }
        if (k1) {
          Keyboard.press(k1);
          Keyboard.press(k2);
          delay(50);
          Keyboard.releaseAll();
        }
        Keyboard.press(k2);
        delay(50);
        Keyboard.releaseAll();
      }
    }

    updateKeyHistory(keyId);
    displayKeyHistory();

  }


  void quickSwitchScheme(){
    /*
      快速切换方案，根据拨动开关状态切换主方案或副方案
      参数：无
      返回值：无
    */
    // 读取当前激活的方案和方案名称
    activeSchemeId = (EEPROM.read(primary? 0 : 3) - 1 ) % 10 + 1;
    getSchemeList(); // 获取所有方案信息到schemes数组中
    getSchemeName(activeSchemeId);  
    if (currentMode == 0){
      updateTitle(getSchemeName(activeSchemeId));   
    }
  }

  ////////////////////////////////////////////////////
  
  //////////////////// 其他IO相关 ////////////////////

  // 更新按键LED状态灯
  void updateKeyLED(bool on){
    /*
      更新按键LED状态灯，根据on参数控制LED的亮灭
      参数：
        on：true表示点亮当前按键的LED，false表示熄灭所有LED
      返回值：无
    */
    for (int i=0; i < 9; i++){
      pinMode(keys[i].pin, OUTPUT);
      if (i == keyId && on)
        digitalWrite(keys[i].pin, LOW);
      else
        digitalWrite(keys[i].pin, HIGH);
    }
  }

  void setPinMode(int mode){
    /*
      设置所有按键引脚的模式
      参数：
        mode：引脚模式，如INPUT_PULLUP
      返回值：无
    */
    for(int i = 0; i < 9; i++) {
      pinMode(keys[i].pin, mode);
    }
  }



  /////////////////////////////////////////////////////

  //////////////////// 远程配置相关 ////////////////////

  void remoteConfigMode(){
    /*
      进入远程配置模式，显示提示信息等待PC端连接
      参数：无
      返回值：无
    */
    tft.fillScreen(ST77XX_BLACK);
    updateTitle(menu[currentMenuId].name);
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(10, 24);
    tft.print("Please open VCKBConfig app on PC");
    setSerialMode('W');
  }

  void sendConfigData(){
    /*
      发送配置数据到串口，用于远程配置
      参数：无
      返回值：无
    */
    byte b;
    for (int i = 0; i < EEPROM_SIZE; i++){
      b = EEPROM.read(i);
      Serial.write(b);
    }
  }

  void saveConfigData(){
    /*
      保存接收到的配置数据到EEPROM中
      参数：无
      返回值：无
    */
    for (int i = 0; i < EEPROM_SIZE; i++){
      EEPROM.write(i, configData[i]);
    }
    EEPROM.commit();
    quickSwitchScheme();   
  }

  void setSerialMode(byte serMode){
    /*
      设置串口模式并更新显示状态
      参数：
        serMode：串口模式 ('W'等待, 'R'接收, 'T'发送)
      返回值：无
    */
    int bgColor, textColor;
    String text;

    serialMode = serMode;
    switch (serialMode){
      case 'W':
        bgColor = ST77XX_WHITE;
        textColor = ST77XX_BLACK;
        text = "Waiting ...";
        break;
      case 'R':
        bgColor = ST77XX_GREEN;
        textColor = ST77XX_WHITE;
        text = "Receiving data ...";
        break;
      case 'T':
        bgColor = ST77XX_BLUE;
        textColor = ST77XX_WHITE;
        text = "Sending data ...";
        break;
    }
    tft.fillRect(10, 36, 200, 16, bgColor);
    tft.setTextColor(textColor);
    tft.setCursor(110 - text.length() * 3, 40);
    tft.print(text);
  }

  void checkSerial(){
    /*
      检查串口通信状态，根据当前串口模式处理接收或发送数据
      参数：无
      返回值：无
    */
    if (serialMode != 'N') {  // W,T,R
      switch (serialMode) {
        case 'W':  // wait 模式，接收命令
          if (Serial.available()){
            char rx_byte = Serial.read(); 
            tft.print(rx_byte);
            if (rx_byte == 0xFB) {   //接收模式
              setSerialMode('R');
              serialStart = millis();
              bytesCount = 0;
            } else if (rx_byte == 0xFC) {    //发送模式
              
              setSerialMode('T');
              serialStart = millis();
            }
          }
          break;
        case 'R':   // 接收模式
          if (millis() - serialStart > SER_TIMEOUT * 1000){  // 超时
            setSerialMode('W');
            Serial.write(0xFF);    //失败信息
            break;
          }
          if (Serial.available()){
            configData[bytesCount] = Serial.read(); 
            bytesCount++;
          }
          if (bytesCount == EEPROM_SIZE){
            saveConfigData();
            setSerialMode('W');
            Serial.write(0xFE);    //成功信息
          } else if (bytesCount > EEPROM_SIZE){
            setSerialMode('W');
            Serial.write(0xFF);    //失败，数据过大
          }
          break;
        case 'T':   // 发送模式
          if (millis() - serialStart > SER_TIMEOUT * 1000){  // 超时
            setSerialMode('W');
            Serial.write(0xFF);    //失败信息
            break;
          }
          sendConfigData();
          setSerialMode('W');
          break;

        default:
          break;
      }
    }
  }

  /////////////////////////////////////////////////////


  void setup(void) {
    /*
      Arduino setup函数，初始化硬件和软件
      参数：无
      返回值：无
    */
    delay(1000);
    Serial.begin(115200);
    Serial.println("start...");

    // 初始化EEPROM
    if (!EEPROM.begin(EEPROM_SIZE)) {
      Serial.println("failed to initialize EEPROM");
      delay(1000000);
    }
    // 检查EEPROM版本号，如果不匹配则格式化EEPROM
    if (EEPROM.read(95) != EEPROM_VERSION) {
      Serial.println("EEPROM version mismatch, formatting EEPROM...");
      formatEEPROM();
    } else {
      Serial.println("EEPROM version match, no need to format.");
    }
    
    //初始化按键输入引脚，使用内部上拉电阻
    setPinMode(INPUT_PULLUP);
    
    // 初始化切换开关
    pinMode(SW0, INPUT_PULLUP);

    // 初始化旋转编码器引脚
    pinMode(RT_BT, INPUT_PULLUP);
    pinMode(RT_SA, INPUT_PULLUP);
    pinMode(RT_SB, INPUT_PULLUP);

    Serial.print(F("Hello! ST77xx TFT Test"));
    tft.init(76, 284);          // 初始化TFT-LCD ST7789 76x284
    tft.setRotation(1);         // 设置屏幕显示方向
    tft.invertDisplay(false);
    tft.fillScreen(ST77XX_BLACK);

    // 初始化USB HID设备
    Mouse.begin();
    Keyboard.begin();
    USB.begin();

    Serial.println(F("Initialized"));

    uint16_t time = millis();

    delay(1000);

    // 获取当前方案
    primary = bool(digitalRead(SW0));
    quickSwitchScheme();
  }

  void loop() {
    /*
      Arduino主循环函数，不断检查按键状态和处理串口通信
      参数：无
      返回值：无
    */
    // 检查按键状态
    if (currentMode == 0) {
      checkKey(keys[0], [](){TriggerKey(0);});
      checkKey(keys[1], [](){TriggerKey(1);});
      checkKey(keys[2], [](){TriggerKey(2);});
      checkKey(keys[3], [](){TriggerKey(3);});
      checkKey(keys[4], [](){TriggerKey(4);});
      checkKey(keys[5], [](){TriggerKey(5);});
      checkKey(keys[6], [](){TriggerKey(6);});
      checkKey(keys[7], [](){TriggerKey(7);});
      checkKey(keys[8], [](){TriggerKey(8);});
    }

    checkRotateEncoder(rotEnc, 
      onRotateEnc_CW,
      onRotateEnc_CCW,
      onRotateEnc_ButtonPressed,
      onRotateEnc_ButtonLongPressed
    );

    bool sw = bool(digitalRead(SW0));
    if (sw != primary) {
      primary = sw;
      quickSwitchScheme();
    }

    checkSerial();


  }

#endif /* ARDUINO_USB_MODE */
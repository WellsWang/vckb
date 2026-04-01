#!/usr/bin/env python

"""
VibeCoding键盘配置工具
版本 1.0
作者： Wells Wang，geek-logic.com, 2026


"""

import tkinter as tk
from tkinter import filedialog, messagebox
import serial
import serial.tools.list_ports
import os
import time


EEPROM_VERSION = 1
EEPROM_SIZE = 2300

DEFAULT_KEY_INTERVAL = 50  # 默认按键触发间隔，单位毫秒
SCHEME_NUM = 10 # 方案数量

KEY_VAL = (
    ["None", 0x00],
    ["`", 0x60],
    ["1", 0x31],
    ["2", 0x32],
    ["3", 0x33],
    ["4", 0x34],
    ["5", 0x35],
    ["6", 0x36],
    ["7", 0x37],
    ["8", 0x38],
    ["9", 0x39],
    ["0", 0x30],
    ["-", 0x2D],
    ["=", 0x3D],
    ["Space", 0x20],
    ["A", 0x61],
    ["B", 0x62],
    ["C", 0x63],
    ["D", 0x64],
    ["E", 0x65],
    ["F", 0x66],
    ["G", 0x67],
    ["H", 0x68],
    ["I", 0x69],
    ["J", 0x6A],
    ["K", 0x6B],
    ["L", 0x6C],
    ["M", 0x6D],
    ["N", 0x6E],
    ["O", 0x6F],
    ["P", 0x70],
    ["Q", 0x71],
    ["R", 0x72],
    ["S", 0x73],
    ["T", 0x74],
    ["U", 0x75],
    ["V", 0x76],
    ["W", 0x77],
    ["X", 0x78],
    ["Y", 0x79],
    ["Z", 0x7A],
    ["[", 0x5B],
    ["]", 0x5D],
    ["\\", 0x5C],
    [";", 0x3B],
    ["'", 0x27],
    [",", 0x2C],
    [".", 0x2E],
    ["/", 0x2F],
    ["Left Ctrl", 0x80],
    ["Left Shift", 0x81],
    ["Left Alt", 0x82],
    ["Left GUI", 0x83],
    ["Right Ctrl", 0x84],
    ["Right Shift", 0x85],
    ["Right Alt", 0x86],
    ["Right GUI", 0x87],
    ["Up Arrow", 0xDA],
    ["Down Arrow", 0xD9],
    ["Left Arrow", 0xD8],
    ["Right Arrow", 0xD7],
    ["Menu", 0xED],
    ["Backspace", 0xB2],
    ["Tab", 0xB3],
    ["Return", 0xB0],
    ["ESC", 0xB1],
    ["Insert", 0xD1],
    ["Delete", 0xD4],
    ["Page Up", 0xD3],
    ["Page Down", 0xD6],
    ["Home", 0xD2],
    ["End", 0xD5],
    ["Num Lock", 0xDB],
    ["Caps Lock", 0xC1],
    ["F1", 0xC2],
    ["F2", 0xC3],
    ["F3", 0xC4],
    ["F4", 0xC5],
    ["F5", 0xC6],
    ["F6", 0xC7],
    ["F7", 0xC8],
    ["F8", 0xC9],
    ["F9", 0xCA],
    ["F10", 0xCB],
    ["F11", 0xCC],
    ["F12", 0xCD],
    ["Print Screen", 0xCE],
    ["Scroll Lock", 0xCF],
    ["Pause", 0xD0],
    ["Keypad /", 0xDC],
    ["Keypad *", 0xDD],
    ["Keypad -", 0xDE],
    ["Keypad +", 0xDF],
    ["Keypad Enter", 0xE0],
    ["Keypad 1", 0xE1],
    ["Keypad 2", 0xE2],
    ["Keypad 3", 0xE3],
    ["Keypad 4", 0xE4],
    ["Keypad 5", 0xE5],
    ["Keypad 6", 0xE6],
    ["Keypad 7", 0xE7],
    ["Keypad 8", 0xE8],
    ["Keypad 9", 0xE9],
    ["Keypad 0", 0xEA],
    ["Keypad .", 0xEB]
)


config_data = {
    "primary_scheme": 0,
    "secondary_scheme": 0,
    "schemes": [
        {
            "name": "",
            "activate": False,
            "keys": [
                {"short_name": "", "trigger": 0, "key_codes": [0 for _ in range(5)]} for _ in range(12)
            ]
        } for _ in range(SCHEME_NUM)
    ]
}

binary_data = [0] * EEPROM_SIZE  # EEPROM_SIZE字节 默认填充0x00
# EEPROM存储信息格式：
# 地址范围      内容
# 0             当前激活的键盘方案ID，1字节，默认值 0x01，即方案1
# 1             键盘方案的总数量，1字节，默认值 0x0A , 10组方案
# 2             设置顺序按下时的时间间隔，单位10毫秒，1字节，默认值 0x05，即50毫秒
# 3             快速切换的第二套方案ID，1字节，默认值0x01，即方案1
# 4-94          保留空间，未来存放蓝牙、网络等配置信息
# 95            EEPROM格式版本号，1字节，默认值0x01，后续版本更新时可以通过修改此版本号来判断是否需要重新格式化EEPROM
# 后续共10组96字节，每组表示一个方案的按键映射内容
# 96-191        方案1的按键映射内容，96字节
#   96字节内容格式：
#     0         是否激活此方案，0不激活，1激活，1字节，默认值只有方案1为1，其余为0
#     1-23      方案名称，23字节，最多支持23个字符的方案名称，超过部分会被截断，默认值为"Scheme {ID}"
#     24-95     方案的按键映射内容，72字节，共9个按键加旋转编码器的两个方向和一个按键的设置（12个设置），每个按键6个字节
#       每个按键的6字节内容格式：
#         0         同时激活或顺序按下，0为同时激活，1为顺序按下，1字节，后续扩展可能增加更多的激活方式，默认值为0
#         1-5       5个键值，每个1字节，表示按键按下时要发送的键值，最多支持5个键值的组合，超过部分会被忽略，键值参考USB HID键盘使用的键值表，0表示无按键，默认值为0
# 192-287       方案2的按键映射内容，96字节
# ...
# 960-1055      方案10的按键映射内容，96字节
# 1056-1099     保留空间
# 1100-2299     1200字节按键短名称，10个按键配置方案，每个方案12个按键，每个按键10字节空间
#   1100-1219   120个字节 方案一的按键短名称
#     1100-1109 方案一的第一个按键的短名称
#     ...
#     1210-1219 方案二的第十二个按键的短名称
#     ...


class VCKBConfigApp:
    """VibeCoding 键盘配置工具主应用类。

    功能：
    - 初始化配置数据
    - 创建Tkinter GUI
    - 读取/写入串口设备数据
    - 导入/导出 EEPROM 二进制配置文件
    """
    def __init__(self, root):

        self.init_data()  # 初始化数据     
        
        self.root = root
        self.root.title("Vibe Coding 键盘配置工具")
        self.rom1_path = None
        self.rom2_path = None
        self.error = False

        # 初始化变量
        self.port_var = tk.StringVar()
        self.main_scheme_var = tk.StringVar()
        self.sec_scheme_var = tk.StringVar()
        self.select_scheme_var = tk.StringVar()
        self.scheme_name_var = tk.StringVar()
        self.activate_var = tk.BooleanVar()
        self.key_short_name_vars = [tk.StringVar() for _ in range(12)]
        self.key_trigger_vars = [tk.StringVar(value="0") for _ in range(12)]
        self.key_key_vars = [[tk.StringVar() for _ in range(5)] for _ in range(12)]

        # 最上部一行：串口选择和按钮
        top_frame = tk.Frame(root)
        top_frame.pack(fill=tk.X, pady=10)
        
        # 左侧：选择串口
        tk.Label(top_frame, text="选择串口").pack(side=tk.LEFT, padx=5)
        self.port_menu = tk.OptionMenu(top_frame, self.port_var, *self.get_serial_ports())
        self.port_menu.pack(side=tk.LEFT)
        
        # 右侧：三个按钮
        button_frame = tk.Frame(top_frame)
        button_frame.pack(side=tk.RIGHT)
        self.read_button = tk.Button(button_frame, text="读取", command=self.receive_config_data)
        self.read_button.pack(side=tk.LEFT, padx=5)
        self.write_button = tk.Button(button_frame, text="写入", command=self.send_config_data)
        self.write_button.pack(side=tk.LEFT, padx=5)
        self.save_button = tk.Button(button_frame, text="导出", command=self.save_bin_file)
        self.save_button.pack(side=tk.LEFT, padx=5)
        self.load_button = tk.Button(button_frame, text="导入", command=self.load_bin_file)
        self.load_button.pack(side=tk.LEFT, padx=5)

        # 总体设置区块
        settings_frame = tk.Frame(root)
        settings_frame.pack(pady=10)
        tk.Label(settings_frame, text="主激活方案").grid(row=0, column=0, padx=5)
        self.main_scheme_menu = tk.OptionMenu(settings_frame, self.main_scheme_var, *self.get_active_scheme_names())
        self.main_scheme_menu.grid(row=0, column=1, padx=5)
        self.main_scheme_var.trace_add("write", self.on_main_scheme_change)
        tk.Label(settings_frame, text="次激活方案").grid(row=0, column=2, padx=5)
        self.sec_scheme_menu = tk.OptionMenu(settings_frame, self.sec_scheme_var, *self.get_active_scheme_names())
        self.sec_scheme_menu.grid(row=0, column=3, padx=5)
        self.sec_scheme_var.trace_add("write", self.on_sec_scheme_change)
        
        # 方案配置区域
        config_frame = tk.Frame(root)
        config_frame.pack(pady=10, fill=tk.BOTH, expand=True)
        
        # 第一行：选择方案、方案名称、激活
        first_row = tk.Frame(config_frame)
        first_row.pack(fill=tk.X, pady=5)
        tk.Label(first_row, text="选择方案").pack(side=tk.LEFT, padx=5)
        self.select_scheme_menu = tk.OptionMenu(first_row, self.select_scheme_var, *[i for i in range(1, SCHEME_NUM + 1)])
        self.select_scheme_var.trace_add("write", self.on_select_scheme_change)
        self.select_scheme_menu.pack(side=tk.LEFT, padx=5)
        tk.Label(first_row, text="方案名称").pack(side=tk.LEFT, padx=5)
        self.scheme_name_entry = tk.Entry(first_row, textvariable=self.scheme_name_var, width=25)
        self.scheme_name_entry.pack(side=tk.LEFT, padx=5)
        self.scheme_name_var.trace_add("write", self.on_scheme_name_change)
        tk.Label(first_row, text="激活").pack(side=tk.LEFT, padx=5)
        self.activate_check = tk.Checkbutton(first_row, variable=self.activate_var)
        self.activate_check.pack(side=tk.LEFT, padx=5)
        self.activate_var.trace_add("write", self.on_scheme_activate_change)
        
        # 按键设置：12行
        keys_frame = tk.Frame(config_frame)
        keys_frame.pack(fill=tk.BOTH, expand=True)
        for i in range(12):
            key_frame = tk.Frame(keys_frame)
            key_frame.pack(fill=tk.X, pady=2)
            if i == 9:
                tk.Label(key_frame, text=f"顺时针：").pack(side=tk.LEFT, padx=5)
            elif i == 10:
                tk.Label(key_frame, text=f"逆时针：").pack(side=tk.LEFT, padx=5)
            elif i == 11:
                tk.Label(key_frame, text=f"按　下：").pack(side=tk.LEFT, padx=5)
            else:
                tk.Label(key_frame, text=f"按键 {i+1}：").pack(side=tk.LEFT, padx=5)
            tk.Label(key_frame, text="短名称").pack(side=tk.LEFT, padx=5)
            short_name_entry = tk.Entry(key_frame, textvariable=self.key_short_name_vars[i], width=12)
            short_name_entry.pack(side=tk.LEFT, padx=5)
            self.key_short_name_vars[i].trace_add("write", lambda *args, idx=i: self.update_key_short_name(idx)) 
            simultaneous_radio = tk.Radiobutton(key_frame, variable=self.key_trigger_vars[i], value="0")
            simultaneous_radio.pack(side=tk.LEFT)
            tk.Label(key_frame, text="同时触发").pack(side=tk.LEFT, padx=5)
            sequential_radio = tk.Radiobutton(key_frame, variable=self.key_trigger_vars[i], value="1")
            sequential_radio.pack(side=tk.LEFT)
            self.key_trigger_vars[i].trace_add("write", lambda *args, idx=i: self.update_key_trigger(idx))
            tk.Label(key_frame, text="顺序触发").pack(side=tk.LEFT, padx=5)
            stick_seq_radio = tk.Radiobutton(key_frame, variable=self.key_trigger_vars[i], value="2")
            stick_seq_radio.pack(side=tk.LEFT)
            self.key_trigger_vars[i].trace_add("write", lambda *args, idx=i: self.update_key_trigger(idx))
            tk.Label(key_frame, text="粘滞触发").pack(side=tk.LEFT, padx=5)
            for j in range(5):
                key_menu = tk.OptionMenu(key_frame, self.key_key_vars[i][j], *[kv[0] for kv in KEY_VAL])
                key_menu.pack(side=tk.LEFT, padx=2)
                self.key_key_vars[i][j].trace_add("write", lambda *args, idx=i, subidx=j: self.update_key_key(idx, subidx))
        
   
        self.update_ui_from_config()


    ########################### DATA ############################
    def init_data(self):
        config_data["primary_scheme"] = 1
        config_data["secondary_scheme"] = 1
        config_data["key_interval"] = DEFAULT_KEY_INTERVAL
        for i in range(SCHEME_NUM):
            config_data["schemes"][i]["name"] = f"Scheme {i+1}"
            config_data["schemes"][i]["activate"] = (i == 0)  # 默认只有方案1激活
            for j in range(12):
                config_data["schemes"][i]["keys"][j]["short_name"] = f"Key {j+1}"
                config_data["schemes"][i]["keys"][j]["trigger"] = 0
                config_data["schemes"][i]["keys"][j]["key_codes"] = [0 for _ in range(5)]
        self.build_bin_data()

    def build_bin_data(self):
        binary_data[0] = config_data["primary_scheme"]
        binary_data[1] = SCHEME_NUM
        binary_data[2] = config_data["key_interval"] // 10  # 转换为10毫秒单位
        binary_data[3] = config_data["secondary_scheme"]
        binary_data[95] = EEPROM_VERSION
        for i in range(SCHEME_NUM):
            scheme = config_data["schemes"][i]
            offset = 96 + i * 96
            binary_data[offset] = 1 if scheme["activate"] else 0
            name_bytes = scheme["name"].encode('utf-8')[:23]
            name_bytes = name_bytes.strip(b' ').ljust(23, b'\x00')  # 不足23字节用0填充
            binary_data[offset + 1:offset + 1 + len(name_bytes)] = name_bytes
            for j in range(12):
                key_config = scheme["keys"][j]
                key_offset = offset + 24 + j * 6
                binary_data[key_offset] = key_config["trigger"]
                for k in range(5):
                    key_value = key_config["key_codes"][k]
                    binary_data[key_offset + 1 + k] = key_value
        offset = 1100
        for i in range(SCHEME_NUM):
            for j in range(12):
                short_name = config_data["schemes"][i]["keys"][j]["short_name"].encode('utf-8')[:10]
                short_name = short_name.strip(b' ').ljust(10, b'\x00')  # 不足10字节用0填充
                binary_data[offset:offset + 10] = short_name
                offset += 10

    def parse_bin_data(self, data):
        config_data["primary_scheme"] = data[0]
        config_data["key_interval"] = data[2] * 10  # 转换回毫秒单位
        config_data["secondary_scheme"] = data[3]
        for i in range(SCHEME_NUM):
            offset = 96 + i * 96
            config_data["schemes"][i]["activate"] = bool(data[offset])
            name_bytes = data[offset + 1:offset + 24]
            config_data["schemes"][i]["name"] = "".join(chr(b) for b in name_bytes).strip('\x00').replace('\x00', ' ')
            for j in range(12):
                key_offset = offset + 24 + j * 6
                config_data["schemes"][i]["keys"][j]["trigger"] = data[key_offset]
                config_data["schemes"][i]["keys"][j]["key_codes"] = list(data[key_offset + 1:key_offset + 6])
        offset = 1100
        for i in range(SCHEME_NUM):
            for j in range(12):
                short_name_bytes = data[offset:offset + 10]
                config_data["schemes"][i]["keys"][j]["short_name"] = "".join(chr(b) for b in short_name_bytes).strip('\x00').replace('\x00', ' ')
                offset += 10

    def get_active_scheme_names(self):
        active_scheme_names = []
        for i in range(SCHEME_NUM):
            if config_data["schemes"][i]["activate"]:
                active_scheme_names.append(str(i) + " - " + config_data["schemes"][i]["name"])
        return active_scheme_names
        

############################### 文件操作 ############################

    def save_bin_file(self):
        self.build_bin_data()
        path = filedialog.asksaveasfilename(defaultextension=".bin", filetypes=[("Binary files", "*.bin"), ("All files", "*.*")])
        if path:
            try:
                with open(path, 'wb') as f:
                    f.write("".join(chr(b) for b in binary_data).encode('latin-1'))  # 直接写入二进制数据
                messagebox.showinfo("成功", "配置已保存")
            except Exception as e:
                messagebox.showerror("错误", f"保存配置失败: {e}")
    
    def load_bin_file(self): 
        path = filedialog.askopenfilename(filetypes=[("BIN files", "*.bin"),("All files", "*.*")])
        if path:
            try:
                if os.path.getsize(path) != EEPROM_SIZE:
                    messagebox.showerror("错误", f"文件大小不正确，必须为{EEPROM_SIZE}字节")
                    return
                with open(path, 'rb') as f:
                    data = f.read()
                self.parse_bin_data(data)
                self.update_ui_from_config()
                messagebox.showinfo("成功", "配置已加载")

            except Exception as e:
                messagebox.showerror("错误", f"加载配置失败: {e}")


############################## UI 更新 ############################
    def update_ui_from_config(self):
        self.main_scheme_var.set(str(config_data["primary_scheme"] - 1) + " - " + config_data["schemes"][config_data["primary_scheme"] - 1]["name"])
        self.sec_scheme_var.set(str(config_data["secondary_scheme"] - 1) + " - " + config_data["schemes"][config_data["secondary_scheme"] - 1]["name"])
        self.update_scheme_ui(0)  # 默认显示方案1的配置


    def update_scheme_ui(self, scheme_id):
        self.select_scheme_var.set(scheme_id + 1)
        scheme = config_data["schemes"][scheme_id]
        self.scheme_name_var.set(scheme["name"])
        self.activate_var.set(scheme["activate"])
        for i in range(12):
            self.key_short_name_vars[i].set(scheme["keys"][i]["short_name"])
            self.key_trigger_vars[i].set(str(scheme["keys"][i]["trigger"]))
            for j in range(5):
                key_code = scheme["keys"][i]["key_codes"][j]
                key_name = next((kv[0] for kv in KEY_VAL if kv[1] == key_code), "None")
                self.key_key_vars[i][j].set(key_name)


##################################### UI事件处理 ############################
    def on_main_scheme_change(self,*args):
        try:
            select_id = int(self.main_scheme_var.get()[:self.main_scheme_var.get().find(" - ")])
        except Exception:
            select_id = 0
        config_data["primary_scheme"] = select_id + 1
    
    def on_sec_scheme_change(self,*args):
        try:
            select_id = int(self.sec_scheme_var.get()[:self.sec_scheme_var.get().find(" - ")])
        except Exception:
            select_id = 0
        config_data["secondary_scheme"] = select_id + 1

    def on_select_scheme_change(self,*args):
        try:
            scheme_id = int(self.select_scheme_var.get()) - 1
        except Exception:
            scheme_id = 0
        self.update_scheme_ui(scheme_id)

    def on_scheme_name_change(self, *args):
        try:
            scheme_id = int(self.select_scheme_var.get()) - 1
        except Exception:
            scheme_id = 0
        config_data["schemes"][scheme_id]["name"] = self.scheme_name_var.get()

    def on_scheme_activate_change(self, *args):
        try:
            scheme_id = int(self.select_scheme_var.get()) - 1
        except Exception:
            scheme_id = 0
        config_data["schemes"][scheme_id]["activate"] = self.activate_var.get()
        self.update_pri_sec_options()

    def update_pri_sec_options(self):
        active_names = self.get_active_scheme_names()

        menu = self.main_scheme_menu["menu"]
        menu.delete(0, "end")
        for name in active_names:
            menu.add_command(label=name, command=lambda value=name: self.main_scheme_var.set(value))
        if active_names:
            if self.main_scheme_var.get() not in active_names:
                self.main_scheme_var.set(active_names[0])
        else:
            self.main_scheme_var.set("")
        
        menu = self.sec_scheme_menu["menu"]
        menu.delete(0, "end")
        for name in active_names:
            menu.add_command(label=name, command=lambda value=name: self.sec_scheme_var.set(value))
        if active_names:
            if self.sec_scheme_var.get() not in active_names:
                self.sec_scheme_var.set(active_names[0])
        else:
            self.sec_scheme_var.set("")

        self.main_scheme_var.set(str(config_data["primary_scheme"] - 1) + " - " + config_data["schemes"][config_data["primary_scheme"] - 1]["name"])
        self.sec_scheme_var.set(str(config_data["secondary_scheme"] - 1) + " - " + config_data["schemes"][config_data["secondary_scheme"] - 1]["name"])


    def update_key_short_name(self, idx):
        try:
            scheme_id = int(self.select_scheme_var.get()) - 1
        except Exception:
            scheme_id = 0
        config_data["schemes"][scheme_id]["keys"][idx]["short_name"] = self.key_short_name_vars[idx].get()
    
    def update_key_trigger(self, idx):
        try:
            scheme_id = int(self.select_scheme_var.get()) - 1
        except Exception:
            scheme_id = 0
        config_data["schemes"][scheme_id]["keys"][idx]["trigger"] = int(self.key_trigger_vars[idx].get())

    def update_key_key(self, idx, subidx):
        try:
            scheme_id = int(self.select_scheme_var.get()) - 1
        except Exception:
            scheme_id = 0
        key_name = self.key_key_vars[idx][subidx].get()
        key_code = next((kv[1] for kv in KEY_VAL if kv[0] == key_name), 0)
        config_data["schemes"][scheme_id]["keys"][idx]["key_codes"][subidx] = key_code


########################### 串口处理 ############################


    def get_serial_ports(self):
        ports = [port.device for port in serial.tools.list_ports.comports()]
        return ports if ports else ["无可用串口"]


    def receive_config_data(self):
        if self.port_var.get() == "无可用串口" or not self.port_var.get():
            messagebox.showerror("错误", "没有可用的串口，请连接设备并选择正确的串口")
            return
        try:
            ser = serial.Serial(self.port_var.get(), 115200, timeout=1)
            if not ser.is_open:
                ser.open()
            ser.reset_input_buffer()  # 清空输入缓冲区  
            ser.reset_output_buffer()  # 清空输出缓冲区
            ser.flush()  # 确保缓冲区清空
            time.sleep(3)  # 等待串口稳定

            # 发送指令字符串
            ser.write(b'\xFC')  # 发送单字节指令0xFC表示读取配置
            ser.flush()  # 确保缓冲区清空
            time.sleep(0.5) 
            
            data = bytearray()
            timeout = time.time()
            while True and len(data) < EEPROM_SIZE:
                if ser.in_waiting:
                    chunk = ser.read(ser.in_waiting)
                    if chunk == b'\xFF':  # 0xFF表示设备返回错误
                        messagebox.showerror("错误", "设备返回错误")
                        if ser.is_open: ser.close()
                        return
                    else:
                        data.extend(chunk)
                time.sleep(0.01)
                if time.time() - timeout > 5:  # 超时30秒
                    break
            if len(data) == 0:
                messagebox.showerror("错误", "未收到设备数据，\n请确认设备已连接且进入Remote Config模式，并选择了正确的串口")
                if ser.is_open: ser.close()
                return
            if len(data) != EEPROM_SIZE:
                messagebox.showerror("错误", f"接收数据长度不正确，预期{EEPROM_SIZE}字节，实际{len(data)}字节")
                if ser.is_open: ser.close()
                return
            else:
                self.parse_bin_data(data)
                self.update_ui_from_config()
        except Exception as e:
            messagebox.showerror("错误", f"接收数据失败: {e}")
        finally:
            if ser.is_open:
                ser.close()

    def send_config_data(self):
        if self.port_var.get() == "无可用串口" or not self.port_var.get():
            messagebox.showerror("错误", "没有可用的串口，请连接设备并选择正确的串口")
            return
        self.build_bin_data()
        try:
            ser = serial.Serial(self.port_var.get(), 115200, timeout=1)
            if not ser.is_open:
                ser.open()
            ser.reset_input_buffer()  # 清空输入缓冲区  
            ser.reset_output_buffer()  # 清空输出缓冲区
            ser.flush()  # 确保缓冲区清空
            time.sleep(3)  # 等待串口稳定

            # 发送指令字符串
            ser.write(b'\xFB')  # 发送单字节指令0xFB表示写入配置
            ser.flush()  # 确保缓冲区清空
            time.sleep(0.5)

            # 发送配置数据
            ser.write("".join(chr(b) for b in binary_data).encode('latin-1'))  # 直接发送二进制数据
            ser.flush()  # 确保数据发送出去
            # 等待设备返回确认
            timeout = time.time()
            while True:
                if ser.in_waiting > 0:  # 检查是否有数据可读
                    response = ser.read(1)
                    if response == b'\xFE':  # 0xFE表示成功
                        messagebox.showinfo("成功", "配置已写入设备")
                    elif response == b'\xFF':  # 0xFF表示设备返回错误
                        messagebox.showerror("错误", "设备返回错误，写入失败")
                    else:
                        messagebox.showerror("错误", f"未知响应: {response}")
                    break
                if time.time() - timeout > 5:  # 超时5秒
                    messagebox.showerror("错误", "等待设备响应超时")
                    break
                time.sleep(0.01)
        except Exception as e:
            messagebox.showerror("错误", f"发送数据失败: {e}")
        finally:
            if ser.is_open:
                ser.close()

# 主程序入口
if __name__ == "__main__":
    root = tk.Tk()
    app = VCKBConfigApp(root)
    root.mainloop()
# SWD 协议实现

## 文件说明

- `SW_DP.c` - SWD 协议底层实现
- `SW_DP.h` - SWD 协议接口定义
- `dap_handler.c` - DAP 命令处理(需要集成 SWD 函数)

## 实现状态

### ✅ 已完成

- [x] SWD 基础读写函数 (`swd_write`, `swd_read`)
- [x] SWD 传输函数 (`SWD_Transfer`)
- [x] SWD 序列传输 (`SWD_Sequence`)
- [x] 线复位序列 (`PORT_SWD_SETUP`)
- [x] 配置函数 (`SWD_Configure`, `SWD_SetIdleCycles`)

### ⏳ 待集成

需要在 `dap_handler.c` 中集成以下功能:

1. **DAP_SWJ_Sequence** 命令 - 调用 `SWJ_Sequence()`
2. **DAP_Transfer** 命令 - 调用 `SWD_Transfer()`
3. **DAP_TransferBlock** 命令 - 批量调用 `SWD_Transfer()`
4. **DAP_SWD_Configure** 命令 - 调用 `SWD_Configure()`
5. **DAP_Connect** 命令 - 调用 `PORT_SWD_SETUP()`

## 编译集成

在 `CMakeLists.txt` 中添加:

```cmake
idf_component_register(
    SRCS 
        "dap/dap_handler.c"
        "dap/SW_DP.c"          # 添加这一行
        # ... 其他源文件
    INCLUDE_DIRS 
        "include"
        "dap"                   # 添加这一行
        # ... 其他包含目录
)
```

## 测试步骤

1. **编译固件**
   ```bash
   cd /path/to/project
   idf.py build
   ```

2. **烧录固件**
   ```bash
   idf.py flash monitor
   ```

3. **连接硬件**
   ```
   ESP32-S3 GPIO1 (SWCLK) → STM32 PA14
   ESP32-S3 GPIO2 (SWDIO) → STM32 PA13
   ESP32-S3 GPIO3 (nRESET) → STM32 NRST
   ESP32-S3 GND → STM32 GND
   ```

4. **在 Keil 中测试**
   - 打开 Keil MDK
   - Project → Options → Debug
   - 选择 CMSIS-DAP
   - 点击 Settings
   - 应该能看到设备并读取芯片 ID

## 参考资料

- 基于 [free-dap](https://github.com/ataradov/free-dap) 项目实现
- ARM CMSIS-DAP 规范
- SWD 协议规范

## 下一步

完成 `dap_handler.c` 中的命令集成后,固件就可以正常工作了。

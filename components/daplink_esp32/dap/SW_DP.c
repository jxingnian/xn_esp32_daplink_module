/**
 * @file SW_DP.c
 * @brief SWD 协议底层实现 (基于 free-dap)
 * @author 星年
 * @date 2025-12-08
 * @note 参考 free-dap 项目实现
 */

#include "DAP_config.h"
#include "esp_rom_sys.h"
#include <string.h>

// DAP Transfer 请求位定义
#define DAP_TRANSFER_APnDP        (1 << 0)
#define DAP_TRANSFER_RnW          (1 << 1)
#define DAP_TRANSFER_A2           (1 << 2)
#define DAP_TRANSFER_A3           (1 << 3)

// DAP Transfer 应答
#define DAP_TRANSFER_OK           (1 << 0)
#define DAP_TRANSFER_WAIT         (1 << 1)
#define DAP_TRANSFER_FAULT        (1 << 2)
#define DAP_TRANSFER_ERROR        (1 << 3)
#define DAP_TRANSFER_INVALID      0

// SWD 配置
static int swd_turnaround = 1;  // Turnaround 周期数
static int idle_cycles = 0;     // Idle 周期数
static bool data_phase = true;  // 数据阶段使能

/**
 * @brief 计算奇偶校验位
 */
static inline uint32_t parity_u32(uint32_t value) {
    value ^= value >> 16;
    value ^= value >> 8;
    value ^= value >> 4;
    value &= 0x0f;
    return (0x6996 >> value) & 1;
}

/**
 * @brief SWD 写入多个位
 */
static void swd_write(uint32_t value, int size) {
    for (int i = 0; i < size; i++) {
        if (value & 1) {
            PIN_SWDIO_SET();
        } else {
            PIN_SWDIO_CLR();
        }
        PIN_SWCLK_CLR();
        esp_rom_delay_us(1);
        PIN_SWCLK_SET();
        esp_rom_delay_us(1);
        value >>= 1;
    }
}

/**
 * @brief SWD 读取多个位
 */
static uint32_t swd_read(int size) {
    uint32_t value = 0;
    uint32_t bit;
    
    for (int i = 0; i < size; i++) {
        PIN_SWCLK_CLR();
        esp_rom_delay_us(1);
        bit = PIN_SWDIO_IN();
        PIN_SWCLK_SET();
        esp_rom_delay_us(1);
        value |= (bit << i);
    }
    
    return value;
}

/**
 * @brief 运行时钟周期(不传输数据)
 */
static void swd_run_clock(int cycles) {
    while (cycles--) {
        PIN_SWCLK_CLR();
        esp_rom_delay_us(1);
        PIN_SWCLK_SET();
        esp_rom_delay_us(1);
    }
}

/**
 * @brief SWD 传输函数 (基于 free-dap 实现)
 * @param req 请求位 (APnDP, RnW, A2, A3)
 * @param data 数据指针
 * @return ACK 响应
 */
int SWD_Transfer(int req, uint32_t *data) {
    uint32_t value;
    int ack = 0;
    
    // 只保留有效位
    req &= (DAP_TRANSFER_APnDP | DAP_TRANSFER_RnW | DAP_TRANSFER_A2 | DAP_TRANSFER_A3);
    
    // 1. 发送请求 (8 bits): Start(1) + APnDP(1) + RnW(1) + A[2:3](2) + Parity(1) + Stop(1) + Park(1)
    PIN_SWDIO_OUT_ENABLE();
    swd_write(0x81 | (parity_u32(req) << 5) | (req << 1), 8);
    
    // 2. Turnaround
    PIN_SWDIO_OUT_DISABLE();
    swd_run_clock(swd_turnaround);
    
    // 3. 读取 ACK (3 bits)
    ack = swd_read(3);
    
    if (DAP_TRANSFER_OK == ack) {
        // 4. 数据传输
        if (req & DAP_TRANSFER_RnW) {
            // 读操作
            value = swd_read(32);
            
            // 校验奇偶位
            if (parity_u32(value) != swd_read(1)) {
                ack = DAP_TRANSFER_ERROR;
            }
            
            if (data) {
                *data = value;
            }
            
            // Turnaround
            swd_run_clock(swd_turnaround);
            PIN_SWDIO_OUT_ENABLE();
            
        } else {
            // 写操作
            // Turnaround
            swd_run_clock(swd_turnaround);
            PIN_SWDIO_OUT_ENABLE();
            
            // 写数据 + 奇偶位
            swd_write(*data, 32);
            swd_write(parity_u32(*data), 1);
        }
        
        // Idle cycles
        PIN_SWDIO_CLR();
        swd_run_clock(idle_cycles);
        
    } else if (DAP_TRANSFER_WAIT == ack || DAP_TRANSFER_FAULT == ack) {
        // WAIT 或 FAULT 响应
        if (data_phase && (req & DAP_TRANSFER_RnW)) {
            swd_run_clock(32 + 1);  // 跳过数据 + 奇偶位
        }
        
        swd_run_clock(swd_turnaround);
        PIN_SWDIO_OUT_ENABLE();
        
        if (data_phase && (0 == (req & DAP_TRANSFER_RnW))) {
            PIN_SWDIO_CLR();
            swd_run_clock(32 + 1);  // 跳过数据 + 奇偶位
        }
        
    } else {
        // 无效响应
        swd_run_clock(swd_turnaround + 32 + 1);
    }
    
    // 结束，设置 SWDIO 为高
    PIN_SWDIO_SET();
    
    return ack;
}

/**
 * @brief SWD 序列传输
 * @param count 位数
 * @param data 数据
 */
void SWD_Sequence(uint32_t count, const uint8_t *data) {
    uint32_t n;
    
    PIN_SWDIO_OUT_ENABLE();
    
    while (count) {
        n = (count > 8) ? 8 : count;
        swd_write(*data, n);
        data++;
        count -= n;
    }
}

/**
 * @brief 发送 JTAG 到 SWD 切换序列
 */
void SWJ_Sequence(uint32_t count, const uint8_t *data) {
    SWD_Sequence(count, data);
}

/**
 * @brief SWD 线复位序列
 */
void PORT_SWD_SETUP(void) {
    uint8_t data[8];
    
    // 1. 发送至少 50 个时钟周期的高电平 (复位线路)
    memset(data, 0xFF, sizeof(data));
    SWD_Sequence(51, data);
    
    // 2. 发送 JTAG-to-SWD 切换序列: 0xE79E
    data[0] = 0x9E;
    data[1] = 0xE7;
    SWD_Sequence(16, data);
    
    // 3. 再发送至少 50 个时钟周期的高电平
    memset(data, 0xFF, sizeof(data));
    SWD_Sequence(51, data);
    
    // 4. 发送 idle 周期
    memset(data, 0x00, sizeof(data));
    SWD_Sequence(8, data);
}

/**
 * @brief 配置 SWD 参数
 */
void SWD_Configure(uint8_t turnaround_cycles, uint8_t data_phase_enable) {
    swd_turnaround = turnaround_cycles;
    data_phase = data_phase_enable ? true : false;
}

/**
 * @brief 设置 Idle 周期数
 */
void SWD_SetIdleCycles(uint8_t cycles) {
    idle_cycles = cycles;
}

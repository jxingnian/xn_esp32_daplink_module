/*
 * @file spi_switch.c
 * @brief SPI/GPIO模式切换 - ESP32-S3 + ESP-IDF 5.5 专用
 * @author windowsair (原作者), 星年 (移植简化)
 */

#include "sdkconfig.h"
#include <stdbool.h>
#include "cmsis_compiler.h"
#include "spi_switch.h"
#include "gpio_common.h"

#define DAP_SPI GPSPI2

typedef enum {
    SPI_40MHz_DIV = 2,
} spi_clk_div_t;

/**
 * @brief 初始化 SPI 用于 SWD 通信
 */
void DAP_SPI_Init(void)
{
    periph_ll_enable_clk_clear_rst(PERIPH_SPI2_MODULE);

    // 停止 GPIO 驱动，避免 SPI 时序问题
    gpio_ll_set_level(&GPIO, GPIO_NUM_9, 0);
    gpio_ll_set_level(&GPIO, GPIO_NUM_8, 0);

    // 设置为 GPIO 功能
    gpio_ll_func_sel(&GPIO, GPIO_NUM_9, PIN_FUNC_GPIO);
    gpio_ll_func_sel(&GPIO, GPIO_NUM_8, PIN_FUNC_GPIO);

    GPIO.func_out_sel_cfg[GPIO_NUM_8].oen_sel = 0;
    GPIO.func_out_sel_cfg[GPIO_NUM_9].oen_sel = 0;

    // 不使用 DMA
    DAP_SPI.user.usr_conf_nxt = 0;
    DAP_SPI.slave.usr_conf = 0;
    DAP_SPI.dma_conf.dma_rx_ena = 0;
    DAP_SPI.dma_conf.dma_tx_ena = 0;

    // 设置为主机模式
    DAP_SPI.slave.slave_mode = false;

    // 使用全部 64 字节缓冲区
    DAP_SPI.user.usr_mosi_highpart = false;
    DAP_SPI.user.usr_miso_highpart = false;

    // 禁用 CS 引脚
    DAP_SPI.user.cs_setup = false;
    DAP_SPI.user.cs_hold = false;

    // 禁用 CS 信号
    DAP_SPI.misc.cs0_dis = 1;
    DAP_SPI.misc.cs1_dis = 1;
    DAP_SPI.misc.cs2_dis = 1;
    DAP_SPI.misc.cs3_dis = 1;
    DAP_SPI.misc.cs4_dis = 1;
    DAP_SPI.misc.cs5_dis = 1;

    // 半双工传输
    DAP_SPI.user.doutdin = false;

    // 设置数据位顺序 (SWD 使用 LSB)
    DAP_SPI.ctrl.wr_bit_order = 1;
    DAP_SPI.ctrl.rd_bit_order = 1;

    // 不使用 dummy
    DAP_SPI.user.usr_dummy = 0;

    // 设置 SPI 时钟: 40MHz 50% 占空比
    DAP_SPI.clock.clk_equ_sysclk = false;
    DAP_SPI.clock.clkdiv_pre = 0;
    DAP_SPI.clock.clkcnt_n = SPI_40MHz_DIV - 1;
    DAP_SPI.clock.clkcnt_h = SPI_40MHz_DIV / 2 - 1;
    DAP_SPI.clock.clkcnt_l = SPI_40MHz_DIV - 1;

    // MISO 延迟设置
    DAP_SPI.user.rsck_i_edge = true;
    DAP_SPI.din_mode.din0_mode = 0;
    DAP_SPI.din_mode.din1_mode = 0;
    DAP_SPI.din_mode.din2_mode = 0;
    DAP_SPI.din_mode.din3_mode = 0;
    DAP_SPI.din_num.din0_num = 0;
    DAP_SPI.din_num.din1_num = 0;
    DAP_SPI.din_num.din2_num = 0;
    DAP_SPI.din_num.din3_num = 0;

    // 设置时钟极性和相位 CPOL = 1, CPHA = 0
    DAP_SPI.misc.ck_idle_edge = 1;
    DAP_SPI.user.ck_out_edge = 0;

    // 使能 SPI 时钟
    DAP_SPI.clk_gate.clk_en = 1;
    DAP_SPI.clk_gate.mst_clk_active = 1;
    DAP_SPI.clk_gate.mst_clk_sel = 1;

    // 不使用命令和地址
    DAP_SPI.user.usr_command = 0;
    DAP_SPI.user.usr_addr = 0;
}

/**
 * @brief 切换到 GPIO 模式
 */
__FORCEINLINE void DAP_SPI_Deinit(void)
{
    gpio_ll_func_sel(&GPIO, GPIO_NUM_9, PIN_FUNC_GPIO);
    gpio_ll_func_sel(&GPIO, GPIO_NUM_8, PIN_FUNC_GPIO);

    // SWCLK 输出
    gpio_ll_output_enable(&GPIO, GPIO_NUM_9);

    // SWDIO 输出和输入
    gpio_ll_output_enable(&GPIO, GPIO_NUM_8);
    gpio_ll_input_enable(&GPIO, GPIO_NUM_8);
}

/**
 * @brief 获取 SPI 控制权
 */
__FORCEINLINE void DAP_SPI_Acquire(void)
{
    gpio_ll_func_sel(&GPIO, GPIO_NUM_9, PIN_FUNC_GPIO);
}

/**
 * @brief 释放 SPI 控制权
 */
__FORCEINLINE void DAP_SPI_Release(void)
{
    gpio_ll_func_sel(&GPIO, GPIO_NUM_9, PIN_FUNC_GPIO);
}

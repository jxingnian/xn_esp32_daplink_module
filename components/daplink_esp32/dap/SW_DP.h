/**
 * @file SW_DP.h
 * @brief SWD 协议接口定义
 * @author 星年
 * @date 2025-12-08
 */

#ifndef SW_DP_H
#define SW_DP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief SWD 传输函数
 * @param req 请求位 (APnDP, RnW, A2, A3)
 * @param data 数据指针
 * @return ACK 响应
 */
int SWD_Transfer(int req, uint32_t *data);

/**
 * @brief SWD 序列传输
 * @param count 位数
 * @param data 数据
 */
void SWD_Sequence(uint32_t count, const uint8_t *data);

/**
 * @brief 发送 JTAG 到 SWD 切换序列
 */
void SWJ_Sequence(uint32_t count, const uint8_t *data);

/**
 * @brief SWD 线复位序列
 */
void PORT_SWD_SETUP(void);

/**
 * @brief 配置 SWD 参数
 */
void SWD_Configure(uint8_t turnaround_cycles, uint8_t data_phase_enable);

/**
 * @brief 设置 Idle 周期数
 */
void SWD_SetIdleCycles(uint8_t cycles);

#ifdef __cplusplus
}
#endif

#endif /* SW_DP_H */

/*
 * @Author: 星年 && jixingnian@gmail.com
 * @Date: 2025-12-08 21:36:56
 * @LastEditors: xingnian jixingnian@gmail.com
 * @LastEditTime: 2025-12-08 21:39:01
 * @FilePath: \todo-xn_esp32_daplink_module\s3_daplink_usb\components\DAP\source\dap_utility.c
 * @Description: DAP工具函数实现，包含奇偶校验查找表
 * 
 * Copyright (c) 2025 by ${git_name_email}, All Rights Reserved. 
 */
#include "dap_utility.h"

/**
 * @brief 奇偶校验字节查找表
 * 
 * 该表用于快速计算一个字节(0-255)中1的个数的奇偶性
 * - 如果字节中1的个数为偶数，对应值为0
 * - 如果字节中1的个数为奇数，对应值为1
 * 
 * 使用宏递归展开生成256个元素的查找表：
 * - P2(n): 生成2位的奇偶校验，4个元素
 * - P4(n): 生成4位的奇偶校验，16个元素
 * - P6(n): 生成6位的奇偶校验，64个元素
 * - 最终展开生成完整的256个元素
 * 
 * 原理：
 * n^1 表示翻转最低位的奇偶性
 * 通过递归组合，每增加2位就扩展4倍元素
 * 
 * 使用示例：
 * uint8_t parity = kParityByteTable[data]; // 获取data的奇偶校验位
 */
const uint8_t kParityByteTable[256] =
{
    /* P2(n): 处理2位，生成4个元素 [n, n^1, n^1, n] */
    #define P2(n) n, n^1, n^1, n
    
    /* P4(n): 处理4位，调用P2生成16个元素 */
    #define P4(n) P2(n), P2(n^1), P2(n^1), P2(n)
    
    /* P6(n): 处理6位，调用P4生成64个元素 */
    #define P6(n) P4(n), P4(n^1), P4(n^1), P4(n)

    /* 最终展开：处理8位，调用P6生成256个元素 */
    P6(0), P6(1), P6(1), P6(0)
};

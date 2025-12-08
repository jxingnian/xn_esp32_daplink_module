/**
 * @file spi_op.c
 * @author windowsair
 * @brief Using SPI for common transfer operations
 * @change: 2020-11-25 first version
 *          2021-2-11 Support SWD sequence
 *          2021-3-10 Support 3-wire SPI
 *          2022-9-15 Support ESP32C3
 *          2024-6-9  Fix DAP_SPI_WriteBits issue
 * @version 0.5
 * @date 2024-6-9
 *
 * @copyright MIT License
 *
 */
#include "sdkconfig.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "dap_configuration.h"

#include "cmsis_compiler.h"
#include "spi_op.h"
#include "spi_switch.h"
#include "gpio_common.h"

/**
 * @brief ESP32-S3专用SPI外设定义
 *        GPSPI2是ESP32-S3的通用SPI控制器2
 */
#define DAP_SPI GPSPI2

/**
 * @brief 设置MOSI(主机输出从机输入)数据位长度
 * @param x 要发送的位数
 * @note ms_data_bitlen寄存器控制SPI传输的数据长度
 */
#define SET_MOSI_BIT_LEN(x) DAP_SPI.ms_dlen.ms_data_bitlen = x

/**
 * @brief 设置MISO(主机输入从机输出)数据位长度
 * @param x 要接收的位数
 * @note 与SET_MOSI_BIT_LEN使用相同的寄存器,因为ESP32-S3的SPI使用统一的数据长度寄存器
 */
#define SET_MISO_BIT_LEN(x) DAP_SPI.ms_dlen.ms_data_bitlen = x

/**
 * @brief 启动SPI传输并等待完成
 * @note 执行流程:
 *       1. 设置update位为1,触发配置更新
 *       2. 等待update位自动清零,表示配置已应用
 *       3. 设置usr位为1,启动用户定义的SPI传输
 *       4. 等待usr位自动清零,表示传输完成
 */
#define START_AND_WAIT_SPI_TRANSMISSION_DONE() \
    do {                                       \
        DAP_SPI.cmd.update = 1;                \
        while (DAP_SPI.cmd.update) continue;   \
        DAP_SPI.cmd.usr = 1;                   \
        while (DAP_SPI.cmd.usr) continue;      \
    } while(0)

/**
 * @brief 计算整数除法并向上取整
 * @param A 被除数
 * @param B 除数
 * @return 向上取整后的商
 * @note 用于计算给定位数需要多少字节来存储
 *       例如: 9位需要2字节, 8位需要1字节
 */
__STATIC_FORCEINLINE int div_round_up(int A, int B)
{
    return (A + B - 1) / B;
}


/**
 * @brief 写入指定数量的位数据(LSB优先,小端序)
 * @param count 要写入的位数(最多64位,不做长度检查)
 * @param buf 数据缓冲区指针
 * @note 注意: 不检查指针有效性,调用者必须确保指针有效
 *       此函数用于SWD协议中发送任意位数的数据
 */
void DAP_SPI_WriteBits(const uint8_t count, const uint8_t *buf)
{
    uint32_t data[16];  // 临时数据缓冲区,用于对齐数据
    int nbytes, i;

    // 禁用命令阶段(不发送命令字节)
    DAP_SPI.user.usr_command = 0;
    // 禁用地址阶段(不发送地址字节)
    DAP_SPI.user.usr_addr = 0;

    // 启用MOSI(数据输出),表示有数据要发送
    DAP_SPI.user.usr_mosi = 1;
    // 禁用MISO(数据输入),此操作只发送不接收
    DAP_SPI.user.usr_miso = 0;
    
    // 设置要发送的位数(寄存器值为实际位数-1)
    SET_MOSI_BIT_LEN(count - 1);

    // 计算需要多少字节来容纳count位数据
    nbytes = div_round_up(count, 8);
    // 将数据从buf复制到对齐的临时缓冲区
    memcpy(data, buf, nbytes);

    // 将数据写入SPI数据缓冲区寄存器
    // ESP32-S3的SPI有16个32位数据缓冲区寄存器
    for (i = 0; i < nbytes; i++) {
        DAP_SPI.data_buf[i] = data[i];
    }

    // 启动传输并等待完成
    START_AND_WAIT_SPI_TRANSMISSION_DONE();
}



/**
 * @brief 读取指定数量的位数据(LSB优先,小端序)
 * @param count 要读取的位数(最多64位,不做长度检查)
 * @param buf 数据缓冲区指针,用于存储读取的数据
 * @note 注意: 不检查指针有效性,调用者必须确保指针有效
 *       此函数用于SWD协议中接收任意位数的数据
 */
void DAP_SPI_ReadBits(const uint8_t count, uint8_t *buf) {
    int i;
    uint32_t data_buf[2];  // 临时缓冲区,最多存储64位数据

    // 将32位数组转换为字节指针,便于按字节访问
    uint8_t * pData = (uint8_t *)data_buf;

    // 禁用MOSI(数据输出),此操作只接收不发送
    DAP_SPI.user.usr_mosi = 0;
    // 启用MISO(数据输入),表示要接收数据
    DAP_SPI.user.usr_miso = 1;

#if (USE_SPI_SIO == 1)
    // 启用SIO模式(单线双向模式)
    // 在SWD协议中,数据线是双向的,需要使用SIO模式
    DAP_SPI.user.sio = true;
#endif

    // 设置要接收的位数(寄存器值为实际位数-1)
    SET_MISO_BIT_LEN(count - 1U);

    // 启动传输并等待完成
    START_AND_WAIT_SPI_TRANSMISSION_DONE();

#if (USE_SPI_SIO == 1)
    // 传输完成后关闭SIO模式
    DAP_SPI.user.sio = false;
#endif

    // 从SPI数据缓冲区寄存器读取接收到的数据
    data_buf[0] = DAP_SPI.data_buf[0];
    data_buf[1] = DAP_SPI.data_buf[1];

    // 将数据从临时缓冲区复制到输出缓冲区
    for (i = 0; i < div_round_up(count, 8); i++)
    {
        buf[i] = pData[i];
    }
    // 对最后一个字节应用掩码,清除多余的位
    // 例如: 如果count=10,则最后一个字节只有2位有效
    buf[i-1] = buf[i-1] & ((2 >> (count % 8)) - 1);
}

#if defined CONFIG_IDF_TARGET_ESP8266 || defined CONFIG_IDF_TARGET_ESP32
/**
 * @brief SWD协议步骤1: 发送数据包请求头并接收ACK响应
 * @param packetHeaderData 从主机发送的8位数据包头
 *        包含: Start(1) + APnDP(1) + RnW(1) + A[2:3](2) + Parity(1) + Stop(1) + Park(1)
 * @param ack 指向存储目标设备ACK响应的指针(3位)
 * @param TrnAfterACK ACK后的转换周期数
 * @note 此版本适用于ESP8266和ESP32
 *       使用MOSI发送头部,MISO接收ACK
 */
__FORCEINLINE void DAP_SPI_Send_Header(const uint8_t packetHeaderData, uint8_t *ack, uint8_t TrnAfterACK)
{
    uint32_t dataBuf;

    // 启用MOSI用于发送数据包头
    DAP_SPI.user.usr_mosi = 1;
    // 设置发送8位数据(寄存器值为实际位数-1)
    SET_MOSI_BIT_LEN(8 - 1);

    // 启用MISO用于接收ACK响应
    DAP_SPI.user.usr_miso = 1;

#if (USE_SPI_SIO == 1)
    // 启用单线双向模式
    DAP_SPI.user.sio = true;
#endif

    // 设置MISO接收位数:
    // 1位Trn(ACK前的转换周期) + 3位ACK + TrnAfterACK - 1(寄存器规定)
    SET_MISO_BIT_LEN(1U + 3U + TrnAfterACK - 1U);

    // 将数据包头复制到数据缓冲区寄存器
    // 其他字节填充0
    DAP_SPI.data_buf[0] = (packetHeaderData << 0) | (0U << 8) | (0U << 16) | (0U << 24);

    // 启动传输并等待完成
    START_AND_WAIT_SPI_TRANSMISSION_DONE();

#if (USE_SPI_SIO == 1)
    // 传输完成后关闭SIO模式
    DAP_SPI.user.sio = false;
#endif

    // 读取接收到的数据
    dataBuf = DAP_SPI.data_buf[0];
    // 提取ACK位(跳过第1位Trn,取接下来的3位)
    *ack = (dataBuf >> 1) & 0b111;
} // defined CONFIG_IDF_TARGET_ESP8266 || defined CONFIG_IDF_TARGET_ESP32

#elif defined CONFIG_IDF_TARGET_ESP32C3 || defined CONFIG_IDF_TARGET_ESP32S3
/**
 * @brief SWD协议步骤1: 发送数据包请求头并接收ACK响应
 * @param packetHeaderData 从主机发送的8位数据包头
 *        包含: Start(1) + APnDP(1) + RnW(1) + A[2:3](2) + Parity(1) + Stop(1) + Park(1)
 * @param ack 指向存储目标设备ACK响应的指针(3位)
 * @param TrnAfterACK ACK后的转换周期数
 * @note 此版本适用于ESP32C3和ESP32S3
 *       使用命令阶段发送头部(更高效),MISO接收ACK
 */
__FORCEINLINE void DAP_SPI_Send_Header(const uint8_t packetHeaderData, uint8_t *ack, uint8_t TrnAfterACK)
{
    uint32_t dataBuf;

    // 禁用MOSI数据阶段(使用命令阶段代替)
    DAP_SPI.user.usr_mosi = 0;
    // 启用命令阶段,用于发送数据包头
    DAP_SPI.user.usr_command = 1;
    // 启用MISO用于接收ACK响应
    DAP_SPI.user.usr_miso = 1;

    // 设置命令阶段:
    // 8位数据包头 + 1位Trn(ACK前的转换周期) - 1(寄存器规定)
    DAP_SPI.user2.usr_command_bitlen = 8U + 1U - 1U;
    // 设置命令值为数据包头
    DAP_SPI.user2.usr_command_value = packetHeaderData;


#if (USE_SPI_SIO == 1)
    // 启用单线双向模式
    DAP_SPI.user.sio = true;
#endif

    // 设置MISO接收位数:
    // 3位ACK + TrnAfterACK - 1(寄存器规定)
    // 注意: Trn已在命令阶段处理,这里只需接收ACK
    SET_MISO_BIT_LEN(3U + TrnAfterACK - 1U);

    // 启动传输并等待完成
    START_AND_WAIT_SPI_TRANSMISSION_DONE();

#if (USE_SPI_SIO == 1)
    // 传输完成后关闭SIO模式
    DAP_SPI.user.sio = false;
#endif

    // 禁用命令阶段,恢复默认状态
    DAP_SPI.user.usr_command = 0;

    // 读取接收到的数据
    dataBuf = DAP_SPI.data_buf[0];
    // 提取ACK位(直接取低3位,因为Trn已在命令阶段处理)
    *ack = dataBuf & 0b111;
}
#endif


/**
 * @brief SWD协议步骤2: 读取数据(用于读操作)
 * @param resData 指向存储从目标设备读取的32位数据的指针
 * @param resParity 指向存储奇偶校验位的指针
 * @note 读取序列: 1位Trn(结束) + 32位数据 + 1位奇偶校验
 *       数据以LSB优先方式传输
 *       此函数在DAP_SPI_Send_Header之后调用,用于完成SWD读事务
 */
__FORCEINLINE void DAP_SPI_Read_Data(uint32_t *resData, uint8_t *resParity)
{
    // 使用64位缓冲区存储接收的数据(32位数据 + 1位奇偶校验)
    volatile uint64_t dataBuf;
    uint32_t *pU32Data = (uint32_t *)&dataBuf;

    // 禁用MOSI(数据输出),此操作只接收不发送
    DAP_SPI.user.usr_mosi = 0;
    // 启用MISO(数据输入),用于接收目标设备的数据
    DAP_SPI.user.usr_miso = 1;

#if (USE_SPI_SIO == 1)
    // 启用单线双向模式,SWD协议使用双向数据线
    DAP_SPI.user.sio = true;
#endif

    // 设置接收位数: 1位Trn(结束转换周期) + 32位数据 + 1位奇偶校验 - 1(寄存器规定)
    SET_MISO_BIT_LEN(1U + 32U + 1U - 1U);

    // 启动传输并等待完成
    START_AND_WAIT_SPI_TRANSMISSION_DONE();

#if (USE_SPI_SIO == 1)
    // 传输完成后关闭SIO模式
    DAP_SPI.user.sio = false;
#endif

    // 从SPI数据缓冲区读取接收到的数据
    pU32Data[0] = DAP_SPI.data_buf[0];
    pU32Data[1] = DAP_SPI.data_buf[1];

    // 提取32位响应数据(跳过Trn位,从bit0开始)
    *resData = (dataBuf >> 0U) & 0xFFFFFFFFU;
    // 提取1位奇偶校验位(位于32位数据之后)
    *resParity = (dataBuf >> (0U + 32U)) & 1U;
}

#if defined CONFIG_IDF_TARGET_ESP8266 || defined CONFIG_IDF_TARGET_ESP32
/**
 * @brief SWD协议步骤2: 写入数据(用于写操作)
 * @param data 要写入目标设备的32位数据
 * @param parity 数据的奇偶校验位
 * @note 写入序列: 32位数据 + 1位奇偶校验
 *       数据以LSB优先方式传输
 *       此版本适用于ESP8266和ESP32
 */
__FORCEINLINE void DAP_SPI_Write_Data(uint32_t data, uint8_t parity)
{
    // 启用MOSI(数据输出),用于发送数据
    DAP_SPI.user.usr_mosi = 1;
    // 禁用MISO(数据输入),此操作只发送不接收
    DAP_SPI.user.usr_miso = 0;

    // 设置发送位数: 32位数据 + 1位奇偶校验 - 1(寄存器规定)
    SET_MOSI_BIT_LEN(32U + 1U - 1U);

    // 将数据复制到SPI数据缓冲区寄存器
    DAP_SPI.data_buf[0] = data;      // 32位数据
    DAP_SPI.data_buf[1] = parity;    // 1位奇偶校验

    // 启动传输并等待完成
    START_AND_WAIT_SPI_TRANSMISSION_DONE();
}
#elif defined CONFIG_IDF_TARGET_ESP32C3 || defined CONFIG_IDF_TARGET_ESP32S3
/**
 * @brief SWD协议步骤2: 写入数据(用于写操作)
 * @param data 要写入目标设备的32位数据
 * @param parity 数据的奇偶校验位
 * @note 写入序列: 32位数据 + 1位奇偶校验 + 1位额外位
 *       数据以LSB优先方式传输
 *       此版本适用于ESP32C3和ESP32S3
 *       ESP32C3/S3无法正确发送33位数据,需要额外发送1位
 *       该额外位不会被目标设备识别为起始位
 */
__FORCEINLINE void DAP_SPI_Write_Data(uint32_t data, uint8_t parity)
{
    // 启用MOSI(数据输出),用于发送数据
    DAP_SPI.user.usr_mosi = 1;
    // 禁用MISO(数据输入),此操作只发送不接收
    DAP_SPI.user.usr_miso = 0;

    // 设置发送位数: 32位数据 + 1位奇偶校验 + 1位额外位 - 1(寄存器规定)
    // 额外位用于解决ESP32C3/S3无法正确发送33位数据的问题
    SET_MOSI_BIT_LEN(32U + 1U + 1U - 1U);
    
    // 将数据复制到SPI数据缓冲区寄存器
    DAP_SPI.data_buf[0] = data;
    // 奇偶校验位放在最低位,额外位为0(确保不被识别为起始位)
    DAP_SPI.data_buf[1] = parity == 0 ? 0b00 : 0b01;

    // 启动传输并等待完成
    START_AND_WAIT_SPI_TRANSMISSION_DONE();
}
#endif


#if defined CONFIG_IDF_TARGET_ESP8266 || defined CONFIG_IDF_TARGET_ESP32 || defined CONFIG_IDF_TARGET_ESP32S3
/**
 * @brief 生成指定数量的时钟周期
 * @param num 要生成的时钟周期数
 * @note 通过发送全0数据来生成空闲时钟周期
 *       用于SWD协议中的线路复位或空闲周期生成
 *       此版本适用于ESP8266、ESP32和ESP32S3
 *       TODO: 生成单个时钟可能需要较长时间
 */
__FORCEINLINE void DAP_SPI_Generate_Cycle(uint8_t num)
{
    // 启用MOSI(数据输出),通过发送数据来生成时钟
    DAP_SPI.user.usr_mosi = 1;
    // 禁用MISO(数据输入),不需要接收数据
    DAP_SPI.user.usr_miso = 0;
    
    // 设置发送位数(寄存器值为实际位数-1)
    SET_MOSI_BIT_LEN(num - 1U);

    // 发送全0数据,仅用于生成时钟信号
    DAP_SPI.data_buf[0] = 0x00000000U;

    // 启动传输并等待完成
    START_AND_WAIT_SPI_TRANSMISSION_DONE();
}
#elif defined CONFIG_IDF_TARGET_ESP32C3
/**
 * @brief 生成指定数量的时钟周期
 * @param num 要生成的时钟周期数
 * @note ESP32C3无法正确发送单个位,因此使用读操作代替
 *       读操作同样会产生时钟信号
 *       此版本仅适用于ESP32C3
 *       TODO: 生成单个时钟可能需要较长时间
 */
__FORCEINLINE void DAP_SPI_Generate_Cycle(uint8_t num)
{
    // 禁用MOSI(数据输出)
    DAP_SPI.user.usr_mosi = 0;
    // 启用MISO(数据输入),通过读操作来生成时钟
    DAP_SPI.user.usr_miso = 1;

    // 设置接收位数(寄存器值为实际位数-1)
    // ESP32C3无法发送单个位,使用读操作代替发送操作
    SET_MISO_BIT_LEN(num - 1U);

    // 启动传输并等待完成
    START_AND_WAIT_SPI_TRANSMISSION_DONE();
}
#endif

#if defined CONFIG_IDF_TARGET_ESP32 || defined CONFIG_IDF_TARGET_ESP32C3 || defined CONFIG_IDF_TARGET_ESP32S3
/**
 * @brief 快速生成1个时钟周期
 * @note 通过释放并重新获取SPI总线来快速生成单个时钟
 *       比DAP_SPI_Generate_Cycle(1)更高效
 *       此版本适用于ESP32、ESP32C3和ESP32S3
 */
__FORCEINLINE void DAP_SPI_Fast_Cycle()
{
    // 释放SPI总线(产生时钟上升沿)
    DAP_SPI_Release();
    // 重新获取SPI总线(产生时钟下降沿)
    DAP_SPI_Acquire();
}
#endif

/**
 * @brief 生成协议错误恢复周期(读操作后)
 * @note 当SWD读操作发生协议错误时调用此函数
 *       发送32位数据 + 1位来清除错误状态
 *       所有位都设置为1(高电平)
 *       用于将SWD总线恢复到已知状态
 */
__FORCEINLINE void DAP_SPI_Protocol_Error_Read()
{
    // 启用MOSI(数据输出),发送错误恢复数据
    DAP_SPI.user.usr_mosi = 1;
    // 禁用MISO(数据输入),不需要接收数据
    DAP_SPI.user.usr_miso = 0;
    
    // 设置发送位数: 32位忽略数据 + 1位 - 1(寄存器规定)
    SET_MOSI_BIT_LEN(32U + 1U - 1);

    // 发送全1数据,用于错误恢复
    DAP_SPI.data_buf[0] = 0xFFFFFFFFU;
    DAP_SPI.data_buf[1] = 0xFFFFFFFFU;

    // 启动传输并等待完成
    START_AND_WAIT_SPI_TRANSMISSION_DONE();
}


/**
 * @brief 生成协议错误恢复周期(写操作后)
 * @note 当SWD写操作发生协议错误时调用此函数
 *       发送1位Trn + 32位数据 + 1位来清除错误状态
 *       所有位都设置为1(高电平)
 *       用于将SWD总线恢复到已知状态
 *       与DAP_SPI_Protocol_Error_Read的区别是多了1位Trn
 */
__FORCEINLINE void DAP_SPI_Protocol_Error_Write()
{
    // 启用MOSI(数据输出),发送错误恢复数据
    DAP_SPI.user.usr_mosi = 1;
    // 禁用MISO(数据输入),不需要接收数据
    DAP_SPI.user.usr_miso = 0;
    
    // 设置发送位数: 1位Trn + 32位忽略数据 + 1位 - 1(寄存器规定)
    SET_MOSI_BIT_LEN(1U + 32U + 1U - 1);

    // 发送全1数据,用于错误恢复
    DAP_SPI.data_buf[0] = 0xFFFFFFFFU;
    DAP_SPI.data_buf[1] = 0xFFFFFFFFU;

    // 启动传输并等待完成
    START_AND_WAIT_SPI_TRANSMISSION_DONE();
}

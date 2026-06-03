#ifndef __BSP_VOICE_H__
#define __BSP_VOICE_H__

#include "ti_msp_dl_config.h" // 确保引入了 TI 官方宏定义

// ---- 语音模块设备信息 ----
#define VOICE_MODULE_ADDR   (0x34)  // 从机地址
#define REG_VOICE_RESULT    (0x64)  // 语音识别结果寄存器

// ---- 软件 I2C 引脚控制宏定义 ----
// 1. 设置 SDA 为输出模式 (使用 SysConfig 生成的 IOMUX 宏)
#define SDA_OUT()   {                                                       \
                        DL_GPIO_initDigitalOutput(I2C_SDA_IOMUX);    \
                        DL_GPIO_setPins(I2C_PORT, I2C_SDA_PIN);      \
                        DL_GPIO_enableOutput(I2C_PORT, I2C_SDA_PIN); \
                    }

// 2. 设置 SDA 为输入模式
#define SDA_IN()    { DL_GPIO_initDigitalInput(I2C_SDA_IOMUX); }

// 3. 获取 SDA 引脚的当前电平
#define SDA_GET()   ( ( ( DL_GPIO_readPins(I2C_PORT, I2C_SDA_PIN) & I2C_SDA_PIN ) > 0 ) ? 1 : 0 )

// 4. 控制 SDA 和 SCL 引脚的电平输出 (1为高电平，0为低电平)
#define SDA(x)      ( (x) ? (DL_GPIO_setPins(I2C_PORT, I2C_SDA_PIN)) : (DL_GPIO_clearPins(I2C_PORT, I2C_SDA_PIN)) )
#define SCL(x)      ( (x) ? (DL_GPIO_setPins(I2C_PORT, I2C_SCL_PIN)) : (DL_GPIO_clearPins(I2C_PORT, I2C_SCL_PIN)) )

// 微秒延时定义 (如果主频是32MHz，此处可用 SDK 自带的 delay_cycles)
#define delay_us(x)  delay_cycles((x) * 32) 

// ---- 底层 I2C 时序函数声明 ----
void IIC_Start(void);
void IIC_Stop(void);
void IIC_Send_Ack(uint8_t ack);
uint8_t IIC_Wait_Ack(void);
void IIC_Write(uint8_t data);
uint8_t IIC_Read(void);

// ---- 上层业务函数声明 ----
/**
 * @brief  从 I2C 语音识别模块读取当前识别到的关键词 ID
 * @param  无
 * @retval 0x00 代表未识别到；其余值（如0x1C, 0x21, 0x09）为识别到的ID
 */
uint8_t Voice_Module_ReadID(void);

#endif /* __BSP_VOICE_H__ */
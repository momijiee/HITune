#include "bsp_voice.h"

/******************************************************************
 * 函 数 名 称：IIC_Start
 * 函 数 说 明：IIC起始信号
 * 函 数 形 参：无
 * 函 数 返 回：无
 ******************************************************************/
void IIC_Start(void)
{
    SDA_OUT();

    SCL(0);
    SDA(1);
    SCL(1);

    delay_us(5);

    SDA(0);
    delay_us(5);
    SCL(0);
    delay_us(5);
}

/******************************************************************
 * 函 数 名 称：IIC_Stop
 * 函 数 说 明：IIC停止信号
 * 函 数 形 参：无
 * 函 数 返 回：无
 ******************************************************************/
void IIC_Stop(void)
{
    SDA_OUT();

    SCL(0);
    SDA(0);

    SCL(1);
    delay_us(5);
    SDA(1);
    delay_us(5);
}

/******************************************************************
 * 函 数 名 称：IIC_Send_Ack
 * 函 数 说 明：主机发送应答
 * 函 数 形 参：0应答  1非应答
 * 函 数 返 回：无
 ******************************************************************/
void IIC_Send_Ack(uint8_t ack)
{
    SDA_OUT();
    SCL(0);
    SDA(0);
    delay_us(5);
    if(!ack) SDA(0);
    else     SDA(1);
    SCL(1);
    delay_us(5);
    SCL(0);
    SDA(1);
}

/******************************************************************
 * 函 数 名 称：IIC_Wait_Ack
 * 函 数 说 明：等待从机应答
 * 函 数 形 参：无
 * 函 数 返 回：1=无应答   0=有应答
 ******************************************************************/
uint8_t IIC_Wait_Ack(void)
{
    uint8_t ack_flag = 10;
    SDA_IN();
    SDA(1);
    delay_us(5);
    SCL(1);
    delay_us(5);
    
    while( (SDA_GET() == 1) && (ack_flag) )
    {
        ack_flag--;
        delay_us(5);
    }

    if( ack_flag <= 0 )
    {
        IIC_Stop();
        return 1;
    }
    else
    {
        SCL(0);
        SDA_OUT();
    }
    return 0;
}

/******************************************************************
 * 函 数 名 称：IIC_Write
 * 函 数 说 明：IIC写一个字节
 * 函 数 形 参：data 写入的数据
 * 函 数 返 回：无
 ******************************************************************/
void IIC_Write(uint8_t data)
{
    int i = 0;
    SDA_OUT();
    SCL(0); // 拉低时钟开始数据传输

    for( i = 0; i < 8; i++ )
    {
        SDA( (data & 0x80) >> 7 );
        delay_us(2);
        data <<= 1;
        delay_us(6);
        SCL(1);
        delay_us(4);
        SCL(0);
        delay_us(4);
    }
}

/******************************************************************
 * 函 数 名 称：IIC_Read
 * 函 数 说 明：IIC读1个字节
 * 函 数 形 参：无
 * 函 数 返 回：读出的1个字节数据
 ******************************************************************/
uint8_t IIC_Read(void)
{
    uint8_t i, receive = 0;
    SDA_IN(); // SDA设置为输入
    for(i = 0; i < 8; i++ )
    {
        SCL(0);
        delay_us(5);
        SCL(1);
        delay_us(5);
        receive <<= 1;
        if( SDA_GET() )
        {
            receive |= 0x01;
        }
        delay_us(5);
    }
    return receive;
}

/******************************************************************
 * 函 数 名 称：Voice_Module_ReadID
 * 函 数 说 明：从语音识别模块读取当前识别到的关键词 ID
 * 函 数 形 参：无
 * 函 数 返 回：0x00 代表未识别到；其余值为识别到的ID
 ******************************************************************/
uint8_t Voice_Module_ReadID(void)
{
    uint8_t voice_id = 0x00;

    // ---- 步骤 1：向从机告知我们要读取的寄存器地址 ----
    IIC_Start();
    IIC_Write(VOICE_MODULE_ADDR << 1); // 发送从机写地址 (0x34 << 1 = 0x68)
    if(IIC_Wait_Ack() == 1) {
        return 0x00; // 模块无应答，直接返回
    }

    IIC_Write(REG_VOICE_RESULT);       // 发送识别结果寄存器地址 (0x64)
    if(IIC_Wait_Ack() == 1) {
        return 0x00;
    }

    // ---- 步骤 2：切换为读模式，获取 1 字节的数据 ----
    IIC_Start();                       // 重新发送起始信号 (Restart)
    IIC_Write((VOICE_MODULE_ADDR << 1) | 1); // 发送从机读地址 (0x69)
    if(IIC_Wait_Ack() == 1) {
        return 0x00;
    }

    voice_id = IIC_Read();             // 读取 1 个字节的识别结果
    IIC_Send_Ack(1);                   // 读完最后一个字节，给从机发送 NACK (非应答)
    IIC_Stop();                        // 结束通信

    return voice_id;
}
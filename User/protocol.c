#include "main.h"

uint8_t COUNT; //数据帧计数器

extern UART_HandleTypeDef huart1;

uint8_t checkmode=M_FRAME_CHECK_SUM; //定义校验方式

extern uint8_t UART_RX_BUF[UART_RX_LEN];
extern __IO uint16_t UART_RX_STA;

//拆包一帧数据
//将数据分别解析到结构体中，
//这样在后面处理的时候就可以非常方便的解析各个有效数据
m_result my_unpack_frame(m_frame_typedef *fx)
{
    uint16_t rxchkval=0;                //接收到的校验值
    uint16_t calchkval=0;            //计算得到的校验值
    uint8_t datalen=0;                 //有效数据长度
    datalen=UART_RX_STA & 0X7FFF;
    
    if(datalen<5) //当帧长度小于5的时候(地址1位，功能码1位，帧序号1位，数据长度1位，校验码1位)认为这一帧数据有问题，直接返回帧格式错误。
    {
        UART_RX_STA=0;
        return MR_FRAME_FORMAT_ERR;  
    }
    switch(checkmode)
    {
        case M_FRAME_CHECK_SUM:                            //校验和
            calchkval=mc_check_sum(UART_RX_BUF,datalen-1);
            rxchkval=UART_RX_BUF[datalen-1];
            break;
        case M_FRAME_CHECK_XOR:                            //异或校验
            calchkval=mc_check_xor(UART_RX_BUF,datalen-1);
            rxchkval=UART_RX_BUF[datalen-1];
            break;
        case M_FRAME_CHECK_CRC8:                        //CRC8校验
            calchkval=mc_check_crc8(UART_RX_BUF,datalen-1);
            rxchkval=UART_RX_BUF[datalen-1];
            break;
        case M_FRAME_CHECK_CRC16:                        //CRC16校验
            calchkval=mc_check_crc16(UART_RX_BUF,datalen-2);
            rxchkval=((uint16_t)UART_RX_BUF[datalen-2]<<8)+UART_RX_BUF[datalen-1];
            break;
    }
    
    if(calchkval==rxchkval)        //校验正常
    {
        fx->address=UART_RX_BUF[0];
        fx->function=UART_RX_BUF[1];
        fx->count=UART_RX_BUF[2];
        fx->datalen=UART_RX_BUF[3];
        if(fx->datalen)
        {
            for(datalen=0;datalen<fx->datalen;datalen++)
            {
                fx->data[datalen]=UART_RX_BUF[4+datalen];        //拷贝数据
            }
        }
        fx->chkval=rxchkval;    //记录校验值
    }else 
    {
        UART_RX_STA=0;
        return MR_FRAME_CHECK_ERR;
    }
    UART_RX_STA=0;      //应当在拆包完成之后再清楚标志位
    return MR_OK;
}

//打包一帧数据，并发送
//fx：指向需要打包的帧
void my_packsend_frame(m_frame_typedef *fx)
{
    uint16_t i;
    uint16_t calchkval=0;            //计算得到的校验值
    uint16_t framelen=0;                //打包后的帧长度
    uint8_t sendbuf[UART_RX_LEN];                //发送缓冲区

    if(checkmode==M_FRAME_CHECK_CRC16)framelen=6+fx->datalen;
    else framelen=5+fx->datalen;
    sendbuf[0]=fx->address;
    sendbuf[1]=fx->function;
    sendbuf[2]=fx->count;
    sendbuf[3]=fx->datalen;
    for(i=0;i<fx->datalen;i++)
    {
        sendbuf[4+i]=fx->data[i];
    }
    switch(checkmode)
    {
        case M_FRAME_CHECK_SUM:                            //校验和
            calchkval=mc_check_sum(sendbuf,fx->datalen+4);
            break;
        case M_FRAME_CHECK_XOR:                            //异或校验
            calchkval=mc_check_xor(sendbuf,fx->datalen+4);
            break;
        case M_FRAME_CHECK_CRC8:                        //CRC8校验
            calchkval=mc_check_crc8(sendbuf,fx->datalen+4);
            break;
        case M_FRAME_CHECK_CRC16:                        //CRC16校验
            calchkval=mc_check_crc16(sendbuf,fx->datalen+4);
            break;
    }

    if(checkmode==M_FRAME_CHECK_CRC16)        //如果是CRC16,则有2个字节的CRC
    {
        sendbuf[4+fx->datalen]=(calchkval>>8)&0XFF;     //高字节在前
        sendbuf[5+fx->datalen]=calchkval&0XFF;            //低字节在后
    }else sendbuf[4+fx->datalen]=calchkval&0XFF;
    HAL_UART_Transmit_DMA(&huart1, sendbuf, framelen);     //DMA发送这一帧数据
}

m_result my_deal_frame(m_frame_typedef *fx)
{
    if(fx->address == MY_ADDRESS)
    {
        switch(fx->function)
        {
            case 1:
            {
                My_Func_1();
            }break;
            case 2:
            {
                //My_Func_2(fx);
            }break;
            case 3:
            {
                //My_Func_3(fx);
            }break;
            case 4:
            {
                //My_Func_4(fx);
            }break;
            case 5:
            {
                //My_Func_5(fx);
            }break;
            case 6:
            {
                //My_Func_6(fx);
            }break;
            case 7:
            {
                //My_Func_7(fx);
            }break;
            case 8:
            {
                //My_Func_8(fx);
            }break;
            default:
                return MR_FUNC_ERR;
        }
    }return MR_OK;
}

void My_Func_1(void)
{
    m_frame_typedef txbuff;
    txbuff.address=MY_ADDRESS;      //放入地址码
    txbuff.function=1;          //功能码返回
    txbuff.count=(COUNT++)%255; //帧序号加一
    txbuff.datalen=3;           //随便发送3位数据
    txbuff.data[0]=0x01;
    txbuff.data[1]=0x02;
    txbuff.data[2]=0x03;
    my_packsend_frame(&txbuff); //将数据打包并发送
}

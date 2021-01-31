#ifndef __PROTOCOL_H
#define __PROTOCOL_H
#include "main.h"

#define MY_ADDRESS 1    //本机地址
#define UART_RX_LEN 1024

//校验方式宏定义
#define M_FRAME_CHECK_SUM	0		//校验和
#define M_FRAME_CHECK_XOR	1		//异或校验
#define M_FRAME_CHECK_CRC8	2		//CRC8校验
#define M_FRAME_CHECK_CRC16	3		//CRC16校验

//返回结果：错误类型定义
typedef enum
{
	MR_OK=0,						//正常
	MR_FRAME_FORMAT_ERR = 1,		//帧格式错误	 
	MR_FRAME_CHECK_ERR = 2,			//校验值错位 
	MR_FUNC_ERR = 3,				//内存错误 
	MR_TIMEOUT = 4,				//通信超时
}m_result;

//帧格式定义
__packed typedef struct
{ 
	//uint8_t head[2];					//帧头
	uint8_t address;					//设备地址：1~255
	uint8_t function;				//功能码，0~255
	uint8_t count;						//帧编号
	uint8_t datalen;					//有效数据长度
	uint8_t data[UART_RX_LEN];	//数据存储区
	uint16_t chkval;					//校验值 
	//uint8_t tail;						//帧尾
}m_frame_typedef;

//extern m_protocol_dev_typedef m_ctrl_dev;			//定义帧
extern uint8_t COUNT; //数据帧计数器

m_result my_unpack_frame(m_frame_typedef *fx);
void my_packsend_frame(m_frame_typedef *fx);
m_result my_deal_frame(m_frame_typedef *fx);
void My_Func_1(void);//功能码1

#endif

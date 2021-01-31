# USART1
---
title: 单片机：第五篇 一个简单的基础通信协议的设计与实现
banner_img: 'https://img-ldb.oss-accelerate.aliyuncs.com/'
index_img: 'https://img-ldb.oss-accelerate.aliyuncs.com/'
tags:
  - 单片机
  - STM32
  - 通信协议
date: 2021-01-30 15:16:37
category: 学习
---
不同设备之间的通信，都需要设计自己的通信协议。为了保证设备与设备之间的数据的稳定传输，通信协议的设计需要考虑很多的问题。当然应对不同的应用场景，可以有针对性的设计不同的通信协议。
<!-- more -->
## 一种常见的通信协议格式

这是一种我们比较常见的通信协议格式
| 帧头 | 地址位 | 功能位|帧序号 | 数据长度 | 数据内容 | 校验位 | 帧尾 |
|:----: | :----: | :----: |:----: | :----: | :----: | :----: | :----: |
| 1/2字节 | 1字节 |1字节 | 2字节 | 2字节 |n字节 |1/2字节 |1/2字节 |

而为了应对不同的情况，可以依照情况做删改，例如减少帧头和帧尾，减少帧序号等等。

而本篇实现的通信协议如下，这里将几个部分都做了，实际中可能并不需要这么冗余的帧，可以按需求适当删改：

| 地址位 |功能位 | 帧序号 | 数据长度 | 数据内容 | 校验位 |
| :----: |:----: | :----: |:----: | :----: | :----: |
| 1字节 | 1字节 | 1字节 | 1字节 |n字节 |1字节 |

本篇例程使用的开发板是STM32F103VET6，应用工具是[MDK-ARM v5.33](http://armkeil.blob.core.windows.net/eval/MDK533.EXE)，[STM32CubeMX V6.1.1](https://www.st.com/content/st_com/en/products/development-tools/software-development-tools/stm32-software-development-tools/stm32-configurators-and-code-generators/stm32cubemx.html)
注:STM32CubeMX需要安装[JAVA环境(JRE)](https://javadl.oracle.com/webapps/download/AutoDL?BundleId=244068_89d678f2be164786b292527658ca1605)。

## 搭建串口收发环境

参考：<https://blog.csdn.net/u014470361/article/details/79206352#comments>
使用串口1，DMA方式收发数据

注：DMA，全称为：Direct Memory Access，即直接存储器访问。DMA 传输方式无需 CPU 直接控制传输，也没有中断处理方式那样保留现场和恢复现场的过程，通过硬件为 RAM 与 I/O 设备开辟一条直接传送数据的通路，能使 CPU 的效率大为提高。

### 配置STM32CubeMX

打开STM32CubeMX，File->New Project->Start Project

![20210130162818-2021-01-30](https://img-ldb.oss-accelerate.aliyuncs.com//pingo/20210130162818-2021-01-30.png)

RCC->打开外部时钟

![20210130162940-2021-01-30](https://img-ldb.oss-accelerate.aliyuncs.com//pingo/20210130162940-2021-01-30.png)

USART1->Asynchronous 异步通信

![20210130163105-2021-01-30](https://img-ldb.oss-accelerate.aliyuncs.com//pingo/20210130163105-2021-01-30.png)

下面NVIC Settings->Enabled 使能串口中断

![20210130163235-2021-01-30](https://img-ldb.oss-accelerate.aliyuncs.com//pingo/20210130163235-2021-01-30.png)

还是下面DMA Setthing->ADD->USART1_RX/USART_TX->Priority 使能DMA收发模式，高优先级

![20210130163530-2021-01-30](https://img-ldb.oss-accelerate.aliyuncs.com//pingo/20210130163530-2021-01-30.png)

SYS->Dubug-Serial Wire 启用调试引脚，因为我使用ST-Link进行调试，不使能调试引脚的话没法调试。

![20210130163854-2021-01-30](https://img-ldb.oss-accelerate.aliyuncs.com//pingo/20210130163854-2021-01-30.png)

上面的Clock Configuration时钟配置可以忽略，使用默认8MHz即可，然后是第三个选项Project Manager->Project Name设置工程名->Project Location设置工程路径，然后选择IDE->MDK-ARM
注意工程名和路径都不要出现中文字符

![20210130164447-2021-01-30](https://img-ldb.oss-accelerate.aliyuncs.com//pingo/20210130164447-2021-01-30.png)

最后点击GENERATE CODE生成工程文件，如果失败的话，可以尝试更换JAVA环境。

### 添加USART部分代码

在main.h宏定义一个最大接收字节数1024

``` c
#define UART_RX_LEN 1024
```

打开工程，并在main.c中添加部分代码

定义接收数组，接收数据长度以及标识。UART_RX_STA的0-14位存储数据长度，第15位表示接收状态。

``` c
  /* USER CODE BEGIN PV */
  uint8_t UART_RX_BUF[UART_RX_LEN];
  __IO uint16_t UART_RX_STA;
  /* USER CODE END PV */
```

注意位置DMA初始化需在```MX_USART1_UART_Init();```串口初始化之后。

``` c
  /* USER CODE BEGIN 2 */
  HAL_UART_Receive_DMA(&huart1, UART_RX_BUF, UART_RX_LEN);  // 启动DMA接收
  __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);              // 使能空闲中断
  /* USER CODE END 2 */
```

在while循环中添加DMA发送指令，将接收到的数据发送回去

``` c
if(UART_RX_STA & 0X8000)
{
  HAL_UART_Transmit_DMA(&huart1, UART_RX_BUF, UART_RX_STA & 0X7FFF);    // 将接收到的数据发送回去
  UART_RX_STA = 0;
}
/* USER CODE END WHILE */
```

打开stm32f1xx_it.c文件添加代码

``` c
/* USER CODE BEGIN PD */
extern uint8_t UART_RX_BUF[UART_RX_LEN];
extern __IO uint16_t UART_RX_STA;
/* USER CODE END PD */
```

拉到底，找到USART1中断。修改如下

``` c
void USART1_IRQHandler(void)
{
  /* USER CODE BEGIN USART1_IRQn 0 */
    if(__HAL_UART_GET_FLAG(&huart1, UART_FLAG_IDLE) != RESET&&(UART_RX_STA&0x8000==0))  // 空闲中断标记被置位，并且拆包完成
    {
      __HAL_UART_CLEAR_IDLEFLAG(&huart1);  // 清楚中断标记
      HAL_UART_DMAStop(&huart1);           // 停止DMA接收
      UART_RX_STA = UART_RX_LEN - __HAL_DMA_GET_COUNTER(huart1.hdmarx);  // 总数据量减去未接收到的数据量为已经接收到的数据量
      UART_RX_BUF[UART_RX_STA] = 0;  // 添加结束符
      UART_RX_STA |= 0X8000;         // 标记接收结束
      HAL_UART_Receive_DMA(&huart1, UART_RX_BUF, UART_RX_LEN);  // 重新启动DMA接收
  }
  /* USER CODE END USART1_IRQn 0 */
  HAL_UART_IRQHandler(&huart1);
  /* USER CODE BEGIN USART1_IRQn 1 */

  /* USER CODE END USART1_IRQn 1 */
}
```

编译->生成->```- 0 Error(s), 0 Warning(s).```下载，烧录，打开串口调试助手->波特率115200->随便发送几个字节，查看接收
![20210130171150-2021-01-30](https://img-ldb.oss-accelerate.aliyuncs.com//pingo/20210130171150-2021-01-30.png)

到此，一个基本的串口DMA收发环境就搭建好了。下面就是通信协议的内容了。

## 通信协议的实现

新建一个protocol.h文件

``` c
#ifndef PROTOCOL_H
#define PROTOCOL_H
#include "main.h"

#define MY_ADDRESS 1    //本机地址

//校验方式宏定义
#define M_FRAME_CHECK_SUM    0        //校验和
#define M_FRAME_CHECK_XOR    1        //异或校验
#define M_FRAME_CHECK_CRC8    2        //CRC8校验
#define M_FRAME_CHECK_CRC16    3        //CRC16校验

//返回结果：错误类型定义
typedef enum
{
    MR_OK=0,                //正常
    MR_FRAME_FORMAT_ERR = 1,        //帧格式错误
    MR_FRAME_CHECK_ERR = 2,            //校验值错位
    MR_FUNC_ERR = 3,            //内存错误
    MR_TIMEOUT = 4,                //通信超时
}m_result;

//帧格式定义
__packed typedef struct
{
    //u8 head[2];                //帧头
    u8 address;                //设备地址：1~255
    u8 function;                //功能码，0~255
    u8 count;                    //帧编号
    u8 datalen;                //有效数据长度
    u8 data[UART_RX_LEN];                    //数据存储区
    u16 chkval;                //校验值
    //u8 tail;                //帧尾
}m_frame_typedef;

extern m_protocol_dev_typedef m_ctrl_dev;            //定义帧
extern u8 COUNT;                            //数据帧计数器

void my_packsend_frame(m_frame_typedef *fx);
m_result my_unpack_frame(m_frame_typedef *fx);
m_result my_deal_frame(m_frame_typedef *fx);

void My_Func_1(void);                     //功能码1

#endif

```

再建一个protocol.c文件

``` c
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


```

然后和校验、或校验、CRC8和CRC16校验的代码就不贴了，可以点击本文末尾的链接查看。

最后打开main.c将void main函数修改如下：

``` c
int main(void)
{
  /* USER CODE BEGIN 1 */
    m_frame_typedef fx;
    m_result res;
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  HAL_UART_Receive_DMA(&huart1, UART_RX_BUF, UART_RX_LEN);  // 启动DMA接收
  __HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);              // 使能空闲中断
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    if(UART_RX_STA & 0X8000)      //是否接收到数据
    {
        res=my_unpack_frame(&fx);   //拆解一帧数据
        if(res==MR_OK)  //拆解完成
        {
            my_deal_frame(&fx); //解析这帧数据
        }
        UART_RX_STA = 0;    //清空标志和长度
    }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}
```

可以编译运行一下，如果有错误可以查看一下头文件是否完整，左侧是否将你的新文件添加进来了。还有记得在main.h的适当位置include你的protocol.h和check.h文件。

最后的运行结果就是这个样子的->(勾选16进制发送与接收)
再编辑一条0x01的功能码命令，例如
| 地址位 |功能位 | 帧序号 | 数据长度 | 数据内容 | 校验位 |
| :----: |:----: | :----: |:----: | :----: | :----: |
| 01 | 01 | 01 | 01 |00 |04 |

![20210131231459-2021-01-31](https://img-ldb.oss-cn-hangzhou.aliyuncs.com/pinGo/20210131231459-2021-01-31.png)

一个简易的通信协议的设计就完成了，一般需要注意以下几个点，就是一般接收到一帧数据之后，将数据的各个部分都分别拆解到结构体中，这样可以非常方便的做后面的处理，同样打包一帧数据也是如此，只需要将结构体的各个部分写好，然后将数据帧结构体提交给发送函数就可以了，基本的思路就是这样的。

我将整个工程贴在Github上，可以点击这里查看

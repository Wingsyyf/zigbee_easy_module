/**************************************************************************************************
  Filename:       SampleApp.h
  Revised:        $Date: 2007-10-27 17:22:23 -0700 (Sat, 27 Oct 2007) $
  Revision:       $Revision: 15795 $

  Description:    This file contains the Sample Application definitions.


  Copyright 2007 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product.  Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED 揂S IS?WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, 
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE, 
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com. 
**************************************************************************************************/

#ifndef SAMPLEAPP_H
#define SAMPLEAPP_H

#ifdef __cplusplus
extern "C"
{
#endif

/*********************************************************************
 * INCLUDES
 */
#include "ZComDef.h"

/*********************************************************************
 * CONSTANTS
 */

// These constants are only for example and should be changed to the
// device's needs
#define SAMPLEAPP_ENDPOINT           20

#define SAMPLEAPP_PROFID             0x0F08
#define SAMPLEAPP_DEVICEID           0x0001
#define SAMPLEAPP_DEVICE_VERSION     0
#define SAMPLEAPP_FLAGS              0

#define SAMPLEAPP_MAX_CLUSTERS       2
#define SAMPLEAPP_CLUSTERID          1
  
//终端路由器重启判断  
#define RESET_TRY_TIMES              40
  
//协调器重启等待回复时间(ms)
#define CO_WAIT_TIME                 10000
//协调器重启延时(s)
#define CO_RESET_TIME                3600
  
//定义串口CMD 
/************************************************************************************
#define SEND_PANID                  0xF1  //读取PANID
#define SEND_CHANNEL                0xF2  //读取信道
#define SEND_SHORTADDR              0xFA  //读取短地址
#define SEND_MACADDR                0xFB  //读取MAC地址
#define SEND_DEVTYPE                0xFC  //读取设备类型
#define SEND_DATA                   0xFD  //外发数据
#define SEND_NWKSTD                 0xFE  //读取网络状态
#define SEND_RESET                  0xFF  //复位
  
#define CHANGE_PANID                0xE1  //更改PANID
//#define CHANGE_CHANNEL              0xE2  //更改信道  
  
#define CNTDEVNUM                   0xD1  //统计入网设备数量
************************************************************************************/   
#define SEND_PANID                  0xFA  //读取PANID
#define SEND_CHANNEL                0xFB  //读取信道
#define SEND_SHORTADDR              0xFC  //读取短地址
#define SEND_MACADDR                0xFD  //读取MAC地址
#define SEND_DEVTYPE                0xFE  //读取设备类型
#define SEND_NWKSTD                 0xFF  //读取网络状态
        
#define SEND_DATA                   0xF1  //外发数据(协调器为广播，节点为点播至协调器)
#define SEND_DATA_P2P               0xF2  //点播数据(协调器专有)

#define CHANGE_PANID                0xE1  //更改PANID
#define CHANGE_CHANNEL              0xE2  //更改信道
  
#define CNTDEVNUM                   0xD1  //统计入网设备数量(*****)
#define SEND_BACK                   0xD2  //打开(协调器接收数据回传到发送端）
#define SEND_BACK_CANCEL            0xD3  //关闭(协调器接收数据回传到发送端）
        
#define SEND_RESET                  0xDF  //复位

/*********************************************************************
 * 
 */

/*********************************************************************
 * FUNCTIONS
 */
typedef struct RFTXBUF{
  uint8 type[3];
  uint8 myNWK[2];
  uint8 myMAC[16];
}RFTX;

/*typedef struct
{
  osal_event_hdr_t  hdr;
  uint8             *msg;
} sendBackData_t;
*/
/*
 * Task Initialization for the Generic Application
 */
extern void SampleApp_Init( uint8 task_id );

/*
 * Task Event Processor for the Generic Application
 */
extern UINT16 SampleApp_ProcessEvent( uint8 task_id, uint16 events );

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* SAMPLEAPP_H */

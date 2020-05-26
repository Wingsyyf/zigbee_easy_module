/**************************************************************************************************
  Filename:       SampleApp.c
  Revised:        $Date: 2009-03-18 15:56:27 -0700 (Wed, 18 Mar 2009) $
  Revision:       $Revision: 19453 $

  Description:    Sample Application (no Profile).


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

/*********************************************************************
  This application isn't intended to do anything useful, it is
  intended to be a simple example of an application's structure.

  This application sends it's messages either as broadcast or
  broadcast filtered group messages.  The other (more normal)
  message addressing is unicast.  Most of the other sample
  applications are written to support the unicast message model.

  Key control:
    SW1:  Sends a flash command to all devices in Group 1.
    SW2:  Adds/Removes (toggles) this device in and out
          of Group 1.  This will enable and disable the
          reception of the flash command.
*********************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include "OSAL.h"
#include "ZGlobals.h"
#include "AF.h"
#include "aps_groups.h"
#include "ZDApp.h"
#include "sapi.h"
#include "MT_SYS.h"
#include "OSAL_Nv.h"

#include "SampleApp.h"
#include "SampleAppHw.h"

#include "OnBoard.h"

/* HAL */
#include "hal_lcd.h"
#include "hal_led.h"
#include "hal_key.h"
#include "hal_uart.h"

//定义事件
#define HEART_BEAT_EVENT 0x01
#define RESET_EVENT 0x02
#define P2P_SEND_EVT 0x03

#define SAMPLEAPP_RESET_EVT 0x0F

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * TYPEDEFS
 */
uint8 uart_rdata[40];         //数据暂存器，最多能够缓存40个字节
uint8 uart_rdlenrightflag=0; //数据长度校验位正确标志
uint8 uart_rdsuccessflag=0;  //成功接收到数据信息标志位
uint8 uart_rd_enableflag=0;   //接受状态标记
uint8 uart_rdlen=0;          //有用信息的数据长度
uint8 uart_rdlentemp=0;      //用来记录已读数据长度
uint8 reset_flag=0;
uint8 send_rdata[128];
uint8 num[8];
uint8 num1[8];
uint16 mypanid;
/*********************************************************************
 * GLOBAL VARIABLES
 */

// This list should be filled with Application specific Cluster IDs.
const cId_t SampleApp_ClusterList[SAMPLEAPP_MAX_CLUSTERS] =
{
  SAMPLEAPP_CLUSTERID
};

const SimpleDescriptionFormat_t SampleApp_SimpleDesc =
{
  SAMPLEAPP_ENDPOINT,              //  int Endpoint;
  SAMPLEAPP_PROFID,                //  uint16 AppProfId[2];
  SAMPLEAPP_DEVICEID,              //  uint16 AppDeviceId[2];
  SAMPLEAPP_DEVICE_VERSION,        //  int   AppDevVer:4;
  SAMPLEAPP_FLAGS,                 //  int   AppFlags:4;
  SAMPLEAPP_MAX_CLUSTERS,          //  uint8  AppNumInClusters;
  (cId_t *)SampleApp_ClusterList,  //  uint8 *pAppInClusterList;
  0,                               //  uint8  AppNumInClusters;
  (cId_t *)NULL                    //  uint8 *pAppInClusterList;
};

// This is the Endpoint/Interface description.  It is defined here, but
// filled-in in SampleApp_Init().  Another way to go would be to fill
// in the structure here and make it a "const" (in code space).  The
// way it's defined in this sample app it is define in RAM.
endPointDesc_t SampleApp_epDesc;

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
uint8 SampleApp_TaskID;   
uint8 SampleApp_TransID;  
devStates_t SampleApp_NwkState;  //保存节点状态
/*********************************************************************
 * LOCAL FUNCTIONS
 */
void Uart_Read( uint8 port, uint8 event );
void Uart_Write(uint8 cmd,uint8 port,uint8 *buf, uint16 len);
void ShowInfo(void);
void To_string(uint8 * dest, char * src, uint8 length);
void SampleApp_MessageMSGCB( afIncomingMSGPacket_t *pkt );
/*********************************************************************
 * NETWORK LAYER CALLBACKS
 */

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      SampleApp_Init
 *
 * @brief   Initialization function for the Generic App Task.
 *          This is called during initialization and should contain
 *          any application specific initialization (ie. hardware
 *          initialization/setup, table initialization, power up
 *          notificaiton ... ).
 *
 * @param   task_id - the ID assigned by OSAL.  This ID should be
 *                    used to send messages and set timers.
 *
 * @return  none
 */
void SampleApp_Init( uint8 task_id )
{
  SampleApp_TaskID = task_id;
  SampleApp_TransID = 0;

  // Fill out the endpoint description.
  SampleApp_epDesc.endPoint = SAMPLEAPP_ENDPOINT;
  SampleApp_epDesc.task_id = &SampleApp_TaskID;
  SampleApp_epDesc.simpleDesc
            = (SimpleDescriptionFormat_t *)&SampleApp_SimpleDesc;
  SampleApp_epDesc.latencyReq = noLatencyReqs;

  // Register the endpoint description with the AF
  afRegister( &SampleApp_epDesc );
  halUARTCfg_t uartConfig;
  //------------------------配置串口---------------------------------
  uartConfig.configured=TRUE;
  uartConfig.baudRate=HAL_UART_BR_115200;
  uartConfig.flowControl=FALSE;
  uartConfig.callBackFunc=Uart_Read;
  HalUARTOpen(0,&uartConfig);
  HalUARTWrite(0,"uartinit",8); 
}

/*********************************************************************
 * @fn      SampleApp_ProcessEvent
 *
 * @brief   Generic Application Task event processor.  This function
 *          is called to process all events for the task.  Events
 *          include timers, messages and any other user defined events.
 *
 * @param   task_id  - The OSAL assigned task ID.
 * @param   events - events to process.  This is a bit map and can
 *                   contain more than one event.
 *
 * @return  none
 */
uint16 SampleApp_ProcessEvent( uint8 task_id, uint16 events )
{
  afIncomingMSGPacket_t *MSGpkt;
  (void)task_id;  // Intentionally unreferenced parameter 

  afDataConfirm_t *afDataConfirm;
  byte sentEP;
  ZStatus_t sentStatus;
  byte sentTransID;  
  
  if ( events & SYS_EVENT_MSG )
  {
    MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( SampleApp_TaskID );
    while ( MSGpkt )
    {
      
      switch ( MSGpkt->hdr.event )
      {
        case ZDO_STATE_CHANGE://网络状态改变事件
          SampleApp_NwkState = (devStates_t)(MSGpkt->hdr.status);
          
          if(SampleApp_NwkState==DEV_INIT)
          {
            HalUARTWrite(0,"init",4);
           
          }
          if(SampleApp_NwkState==DEV_NWK_DISC)
          {
              HalUARTWrite(0,"disc",4);
              reset_flag++;
              if(reset_flag==RESET_TRY_TIMES){
                  osal_set_event(SampleApp_TaskID,RESET_EVENT);
              } 
          }
          if(SampleApp_NwkState==DEV_END_DEVICE||SampleApp_NwkState==DEV_ROUTER)
          {
              HalUARTWrite(0,"join",4);
              osal_set_event(SampleApp_TaskID,HEART_BEAT_EVENT);
          }
          break;
          
          case AF_INCOMING_MSG_CMD://接收到数据的事件
              SampleApp_MessageMSGCB( MSGpkt );
          break;

          case AF_DATA_CONFIRM_CMD:      
            afDataConfirm = (afDataConfirm_t *)MSGpkt;
            sentEP = afDataConfirm->endpoint;
            sentStatus = afDataConfirm->hdr.status;
            sentTransID = afDataConfirm->transID;
            (void)sentEP;
            (void)sentTransID;

            if ( sentStatus != ZSuccess ){
              HalUARTWrite(0,"not ok",6);
            }
            if(sentStatus == ZSuccess){
              HalUARTWrite(0,"ok",2);
            }
          break;
          
        default:
          break;
      }    
      // 释放内存
      osal_msg_deallocate( (uint8 *)MSGpkt );
      // Next - if one is available
      MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( SampleApp_TaskID );
    }
    // 释放事件
    return (events ^ SYS_EVENT_MSG);
  }
  if(events & RESET_EVENT)//重启事件（入网失败时重启）
  {   
    if(SampleApp_NwkState==DEV_INIT||SampleApp_NwkState==DEV_NWK_DISC){
      reset_flag=0;
      SystemReset();
    }
    else{
      return (events ^ RESET_EVENT);//释放重启事件
    }
  }
 
  if(events & HEART_BEAT_EVENT)
  {
    ShowInfo();
    osal_start_timerEx( SampleApp_TaskID, HEART_BEAT_EVENT,(3000 + (osal_rand() & 0x00FF)) );
    //osal_start_timerEx(SampleApp_TaskID,SEND_DATA_EVENT,3000);//定时函数，2000ms
    return (events ^ HEART_BEAT_EVENT);
  }
  
  if(events & SAMPLEAPP_RESET_EVT)//常规重启事件
  {
    SystemReset();
  }
  
  return 0;
}


/*********************************************************************
 * LOCAL FUNCTIONS
 */
/*********************************************************************
 * @fn      Uart_Read
 *
 * @brief   解析串口数据
 *
 * @param   
 *
 * @return  none
 */
void Uart_Read(uint8 port, uint8 event)
{
	uint8  ch,check_temp;
	uart_rdlenrightflag=0;          
	uart_rdsuccessflag=0;           
	uart_rd_enableflag=0;           
	uart_rdlen=0;               
	uart_rdlentemp=0;
        
        uint8 mychannel;
        uint8 PAN[2];
        uint8 SHORT[2];
        //uint16 shorttemp;
        //uint8 brodata[4];

  while (Hal_UART_RxBufLen(0)){
        HalUARTRead(0,&ch,1);
        if(ch==0xac){
                uart_rd_enableflag=1;
                uart_rdlentemp++;
        }
	else if(uart_rd_enableflag==1&&uart_rdlentemp==1){
		uart_rdlen=ch;
		uart_rdlentemp++;
	}
	else if(uart_rd_enableflag==1&&uart_rdlentemp>=2&&uart_rdlentemp<=uart_rdlen+1){
		uart_rdata[uart_rdlentemp-2]=ch;
		uart_rdlentemp++;
	}
	else if(uart_rd_enableflag==1&&uart_rdlentemp==uart_rdlen+2){
		check_temp = ~uart_rdlen;
		if(ch==check_temp){
			uart_rdlenrightflag=1;
                        uart_rdlentemp++;
		}
		else{
			uart_rdlenrightflag=0;          
			uart_rdsuccessflag=0;           
			uart_rd_enableflag=0;           
			uart_rdlen=0;               
			uart_rdlentemp=0;		
		}
        }
	else if(uart_rd_enableflag==1&&uart_rdlentemp==uart_rdlen+3){
		if(( ch==0xef )&&( uart_rdlenrightflag==1 )){
			uart_rdsuccessflag=1;
                        uart_rdlentemp=0;
		}
		else{
			uart_rdlenrightflag=0;          
			uart_rdsuccessflag=0;           
			uart_rd_enableflag=0;           
			uart_rdlen=0;               
			uart_rdlentemp=0;
		}
	}
        else{
                         
                uart_rdsuccessflag=0;           
                uart_rd_enableflag=0;           
                uart_rdlen=0;               
                uart_rdlentemp=0;
        }
  }
  
  if(uart_rdsuccessflag==1){     
        uart_rdsuccessflag=0;
        afAddrType_t my_DstAddr;
        my_DstAddr.addrMode=(afAddrMode_t)Addr16Bit;
        my_DstAddr.endPoint=SAMPLEAPP_ENDPOINT;
        my_DstAddr.addr.shortAddr=0x0000;
        
        RFTX rftx;
	uint16 nwk;
	nwk=NLME_GetShortAddr();
	To_string(rftx.myNWK,(uint8 *)&nwk,2);
        To_string(rftx.myMAC,NLME_GetExtAddr(),8);
        
        //自定协议 AC LEN CMD DATA ~LEN EF
         
/************************************************************************************
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
  
#define CNTDEVNUM                   0xD1  //统计入网设备数量(暂未实现)
#define SEND_BACK                   0xD2  //打开(协调器接收数据回传到发送端）
#define SEND_BACK_CANCEL            0xD3  //关闭(协调器接收数据回传到发送端）
        
#define SEND_RESET                  0xDF  //复位
************************************************************************************/  
        
     switch(uart_rdata[0]){     
/**********************************************************************************
 *****************************读取设备信息*****************************************
            
           SEND_PANID                  0xFA  //读取PANID
           SEND_CHANNEL                0xFB  //读取信道
           SEND_SHORTADDR              0xFC  //读取短地址
           SEND_MACADDR                0xFD  //读取MAC地址
           SEND_DEVTYPE                0xFE  //读取设备类型
           SEND_NWKSTD                 0xFF  //读取网络状态
            
 *****************  UART back data: 0xac len cmd * ~len 0xef   ********************
 **********************************************************************************
*/ 
          case SEND_PANID:
                osal_memcpy(&mypanid,&_NIB.nwkPanId,sizeof(uint16));
                PAN[1]=0xFF&mypanid;
                PAN[0]=0xFF&(mypanid>>8);
                Uart_Write(0xFA,0,PAN,2);
            break;  
            
          case SEND_CHANNEL:
                Uart_Write(0xFB,0,&_NIB.nwkLogicalChannel,1);
            break;  
            
          case SEND_SHORTADDR:
                //osal_memcpy(&shorttemp,&rftx.myNWK,sizeof(uint16));
                SHORT[1]=0xFF&nwk;
                SHORT[0]=0xFF&(nwk>>8);
                Uart_Write(0xFC,0,SHORT,2);
            break;
            
          case SEND_MACADDR:
                Uart_Write(0xFD,0,rftx.myMAC,16);
            break;
            
          case SEND_DEVTYPE:
                if(DEVICE_LOGICAL_TYPE==00){
                    Uart_Write(0xFE,0,"COR",3);
                }
                if(DEVICE_LOGICAL_TYPE==01){
                    Uart_Write(0xFE,0,"ROU",3);
                }
                if(DEVICE_LOGICAL_TYPE==02){
                    Uart_Write(0xFE,0,"END",3);
                }
             break;
          case SEND_NWKSTD:
                if(SampleApp_NwkState==DEV_NWK_DISC){
                    Uart_Write(0xFF,0,"discovery",9);
                }
                else if(SampleApp_NwkState==DEV_NWK_JOINING){
                    Uart_Write(0xFF,0,"join",4);
                }
                else if(SampleApp_NwkState==DEV_NWK_REJOIN){
                    Uart_Write(0xFF,0,"rejoin",6);
                }
                else if(SampleApp_NwkState==DEV_NWK_ORPHAN){
                    Uart_Write(0xFF,0,"orphan",6);
                }
                else if(SampleApp_NwkState==DEV_END_DEVICE){
                    Uart_Write(0xFF,0,"device online as end",20);
                }
                else if(SampleApp_NwkState==DEV_ROUTER){
                    Uart_Write(0xFF,0,"device online as rou",20);
                }
                else if(SampleApp_NwkState==DEV_COORD_STARTING){
                    Uart_Write(0xFF,0,"bulid net",9);
                }
                else if(SampleApp_NwkState==DEV_ZB_COORD){
                    Uart_Write(0xFF,0,"net ready",9);
                }
            break;
/**********************************************************************************
 *****************************发送数据命令*****************************************
            
            SEND_DATA                   0xF1  //外发数据(协调器为广播，节点为点播至协调器)
            SEND_DATA_P2P               0xF2  //点播数据(协调器专有)
            
 *****************  UART back data: 0xac len cmd * ~len 0xef   ********************
 **********************************************************************************
*/  
          case SEND_DATA:  
                AF_DataRequest(&my_DstAddr,
                     &SampleApp_epDesc,
                     SAMPLEAPP_CLUSTERID,
                     uart_rdlen-1,
                     &uart_rdata[1],
                     &SampleApp_TransID,
                     AF_ACK_REQUEST,
                     AF_DEFAULT_RADIUS);          
                Uart_Write(0xF1,0,&uart_rdata[1],uart_rdlen-1);
            break;
/**********************************************************************************
 *****************************更改配置命令*****************************************
            
            CHANGE_PANID                0xE1  //更改PANID
            CHANGE_CHANNEL              0xE2  //更改信道
            
 *****************  UART back data: 0xac len cmd * ~len 0xef   ********************
 **********************************************************************************
*/            
          case CHANGE_PANID:
                 mypanid=(uart_rdata[1]<<8) + uart_rdata[2];
                 osal_memcpy(&_NIB.nwkPanId,&mypanid,sizeof(uint16));
                 NLME_UpdateNV(0x01);
                 osal_nv_write(ZCD_NV_PANID, 0, osal_nv_item_len( ZCD_NV_PANID ), &zgConfigPANID);
                 //osal_set_event(SampleApp_TaskID,CHANGE_PANID_EVT);
                 osal_start_timerEx( SampleApp_TaskID,SAMPLEAPP_RESET_EVT,1000 ); 
            break; 
        case CHANGE_CHANNEL:
                mychannel=uart_rdata[1];
                if(mychannel<0x0B||mychannel>0x1A){
                    HalUARTWrite(0,"wrong channel",13);
                }
                else{
                     _NIB.nwkLogicalChannel = mychannel;
                    NLME_UpdateNV(0x01);
                    osal_nv_write(ZCD_NV_CHANLIST, 0, osal_nv_item_len( ZCD_NV_CHANLIST ), &zgConfigPANID);
                    osal_start_timerEx( SampleApp_TaskID,
                                  SAMPLEAPP_RESET_EVT,
                                  1000 );
                }
            break;
/**********************************************************************************
 *****************************设备相关命令*****************************************
            
            CNTDEVNUM                   0xD1  //统计入网设备数量(*****)
            SEND_BACK                   0xD2  //打开(协调器接收数据回传到发送端）
            SEND_BACK_CANCEL            0xD3  //关闭(协调器接收数据回传到发送端）
            
 *****************  UART back data: 0xac len cmd * ~len 0xef   ********************
 **********************************************************************************
*/                   
     
                            
               
/**********************************************************************************
 *****************************复位命令*********************************************
            
            SEND_RESET                  0xDF  //复位
            
 *****************  UART back data: 0xac len cmd * ~len 0xef   ********************
 **********************************************************************************
*/                 
          case SEND_RESET:
                SystemReset();
            break;

         default:
            break;
        }
  }
}
/*********************************************************************
 * @fn      Uart_Write
 *
 * @brief   AC LEN CMD&DATA ~LEN EF
 *
* @param   cmd:控制指令，port:串口，buf:串口待发送buf，len:有效数据长度（待发送数据） 
 *
 * @return  none
 */

void Uart_Write(uint8 cmd,uint8 port,uint8 *buf, uint16 len)
{   
    uint8 uartbuf[256];
    uartbuf[0]=0xac; 
    uartbuf[1]=len+1;
    uartbuf[2]=cmd;
    int i;
    for(i=0;i<len;i++){
      uartbuf[i+3]=buf[i];
    }
    uartbuf[len+3]=~uartbuf[1];
    uartbuf[len+4]=0xef;   
    HalUARTWrite(port,&uartbuf[0],len+5);

}

/*********************************************************************
 * @fn      To_string
 *
 * @brief   
 *
 * @param   none
 *
 * @return  none
 */
void To_string(uint8 * dest, char * src, uint8 length)
{
    uint8 *xad;
    uint8 i=0;
    uint8 ch;
    xad=src+length-1;
    for(i=0;i<length;i++,xad--)
    {
        ch=(*xad>>4)&0x0F;
        dest[i<<1]=ch+((ch<10)?'0':'7');
        ch=*xad&0x0F;
        dest[(i<<1)+1]=ch+((ch<10)?'0':'7');                    
    }
}

/*********************************************************************
 * @fn      ShowInfo
 *
 * @brief   
 *
 * @param   none
 *
 * @return  none
 */
void ShowInfo(void)
{
    RFTX rftx;
    uint16 nwk;   
    num[3]++;
    if(num[3]==10){
        num[2]++;
        num[3]=0;
        if(num[2]==10){
          num[1]++;
          num[2]=0;
          if(num[1]==10){
            num[0]++;
            num[1]=0;
          }
        }
    }
    //HalUARTWrite(0,num,8);
    for(int i=0;i<8;i++){
      switch(num[i]){
        case 0:
          num1[i]='0';
          break;
        case 1:
          num1[i]='1';
          break;
        case 2:
          num1[i]='2';
          break;
        case 3:
          num1[i]='3';
          break;
        case 4:
          num1[i]='4';
          break;
         case 5:
          num1[i]='5';
          break;
         case 6:
          num1[i]='6';
          break;
         case 7:
          num1[i]='7';
          break;
         case 8:
          num1[i]='8';
          break;
         case 9:
          num1[i]='9';
          break;
         default:
          break;
      }
    }
    if(SampleApp_NwkState==DEV_END_DEVICE)
    {
      osal_memcpy(rftx.type,"E13",3);
    }
    if(SampleApp_NwkState==DEV_ROUTER)
    {
      osal_memcpy(rftx.type,"ROU",3);
    }
    nwk=NLME_GetShortAddr();
    To_string(rftx.myNWK,(uint8 *)&nwk,2);
    //To_string(rftx.myMAC,NLME_GetExtAddr(),8);
    //nwk=NLME_GetCoordShortAddr();
    //To_string(rftx.pNWK,(uint8 *)&nwk,2);
    osal_memcpy(rftx.myMAC,num1,4);
    afAddrType_t my_DstAddr;
    my_DstAddr.addrMode=(afAddrMode_t)Addr16Bit;
    my_DstAddr.endPoint=SAMPLEAPP_ENDPOINT;
    my_DstAddr.addr.shortAddr=0x0000;
    AF_DataRequest(&my_DstAddr,
                   &SampleApp_epDesc,
                   SAMPLEAPP_CLUSTERID,
                   12,
                   (uint8 *)&rftx,
                   &SampleApp_TransID,
                   AF_ACK_REQUEST,
                   AF_DEFAULT_RADIUS);
    
    
       /*HalUARTWrite(0,"type:",5);
       HalUARTWrite(0,rftx.type,3);
       HalUARTWrite(0,"myNWK:",6);
       HalUARTWrite(0,rftx.myNWK,4);
       HalUARTWrite(0,"pNWK:",5);
       HalUARTWrite(0,rftx.pNWK,4);*/      
  /*RFTX rftx;
    uint16 nwk;
    

    if(SampleApp_NwkState==DEV_END_DEVICE){
      osal_memcpy(rftx.type,"END",3);
    }
    if(SampleApp_NwkState==DEV_ROUTER){
      osal_memcpy(rftx.type,"ROU",3);
    }
    nwk=NLME_GetShortAddr();//获取本节点的网络地址
    //mnwk = NLME_GetExtAddr();
    To_string(rftx.myNWK,(uint8 *)&nwk,2);//把本节点的网络地址存储在myNWK
    //To_string(rftx.myNWK,(char *)NLME_GetExtAddr(),8);

    nwk=NLME_GetCoordShortAddr();//获取父节点的网络地址
    To_string(rftx.pNWK,(uint8 *)&nwk,2);//父节点的网络地址存储在pNWK
    
    HalUARTWrite(0,"type:",5);
    HalUARTWrite(0,rftx.type,3);
     HalUARTWrite(0,"myNWK:",6);
    HalUARTWrite(0,rftx.myNWK,2);
    HalUARTWrite(0,"pNWK:",5);
    HalUARTWrite(0,rftx.pNWK,2);
    
    afAddrType_t my_DstAddr;
    my_DstAddr.addrMode = (afAddrMode_t)Addr16Bit;//设置发送模式为点播
    my_DstAddr.endPoint = SAMPLEAPP_ENDPOINT;//初始化端口号
    my_DstAddr.addr.shortAddr = 0x0000;//指定协调器的网络地址（指定发给协调器）
    AF_DataRequest(&my_DstAddr,
                   &SampleApp_epDesc,
                   SAMPLEAPP_CLUSTERID,
                   11,//长度
                   (uint8 *)&rftx,//发送的数据
                   &SampleApp_TransID,
                   AF_DISCV_ROUTE,
                   AF_DEFAULT_RADIUS); */
}


/*********************************************************************
 * @fn      SampleApp_MessageMSGCB
 *
 * @brief   Data message processor callback.  This function processes
 *          any incoming data - probably from other devices.  So, based
 *          on cluster ID, perform the intended action.
 *
 * @param   none
 *
 * @return  none
 */
void SampleApp_MessageMSGCB( afIncomingMSGPacket_t *pkt )
{ 
  //uint8 i,len;
  uint8 buffer[4];
  uint8 *sendbuf="YES";

  uint8 mychannel;
  
  switch ( pkt->clusterId )
  {
    //int i;
    //uint16 len;
    case SAMPLEAPP_CLUSTERID: 
      osal_memcpy(buffer,pkt->cmd.Data,pkt->cmd.DataLength);
      
      //接收到协调器发来确认指令，返回YES
      if((buffer[0]=='C')||(buffer[1]=='F')||(buffer[2]=='M')){
        afAddrType_t my_DstAddr;
        my_DstAddr.addrMode=(afAddrMode_t)Addr16Bit;
        my_DstAddr.endPoint=SAMPLEAPP_ENDPOINT;
        my_DstAddr.addr.shortAddr=0x0000;
        AF_DataRequest(&my_DstAddr,
                       &SampleApp_epDesc,
                       SAMPLEAPP_CLUSTERID,
                       3,
                       sendbuf,
                       &SampleApp_TransID,
                       AF_DISCV_ROUTE,
                       AF_DEFAULT_RADIUS);
      }
      
      //接收到协调器广播的改变PANID命令，设置定时器1000ms后进入重启事件
      if((buffer[0]==0xE1)&&(buffer[3]==0xFF)){
          mypanid=(buffer[1]<<8) + buffer[2];
          
          //osal_memcpy(&_NIB.nwkPanId,&mypanid,sizeof(uint16));
          
          _NIB.nwkPanId = mypanid;
          NLME_UpdateNV(0x01);
          osal_nv_write(ZCD_NV_PANID, 0, osal_nv_item_len( ZCD_NV_PANID ), &zgConfigPANID);
          osal_start_timerEx( SampleApp_TaskID,
                        SAMPLEAPP_RESET_EVT,
                        1000 ); 
      }
      
      //收到协调器统计在网数量命令，发送0xD1+短地址给协调器
        RFTX rftx;
	uint16 nwk;
	nwk=NLME_GetShortAddr();
	To_string(rftx.myNWK,(uint8 *)&nwk,2);
        
      if((buffer[0]==0xD1)&&(buffer[1]==0xD2)&&(buffer[2]==0xD3)&&(buffer[3]==0xFF)){
        uint8 tem_buf[5];
        tem_buf[0]=0xD1;
        for(int i=0;i<5;i++){
            tem_buf[i+1]=rftx.myNWK[i];
        }
          afAddrType_t my_DstAddr;
          my_DstAddr.addrMode=(afAddrMode_t)Addr16Bit;
          my_DstAddr.endPoint=SAMPLEAPP_ENDPOINT;
          my_DstAddr.addr.shortAddr=0x0000;
          
          AF_DataRequest(&my_DstAddr,
                         &SampleApp_epDesc,
                         SAMPLEAPP_CLUSTERID,
                         5,
                         tem_buf,
                         &SampleApp_TransID,
                         AF_DISCV_ROUTE,
                         AF_DEFAULT_RADIUS);
          HalUARTWrite(0,tem_buf,5);
      }
      HalUARTWrite(0,buffer,pkt->cmd.DataLength);
      //打印出广播接收到的数据
      //HalUARTWrite(0,"Get Date:",9);
      //HalUARTWrite(0,buffer,pkt->cmd.DataLength);
       if((buffer[0]==0xFE)&&(buffer[3]==0xFF)){
          mychannel=buffer[1];
          if(mychannel<0x0B||mychannel>0x1A){
                    HalUARTWrite(0,"wrong channel",13);
          }
          else{
              HalUARTWrite(0,&mychannel,1);
              _NIB.nwkLogicalChannel = mychannel;
              NLME_UpdateNV(0x01);
              osal_nv_write(ZCD_NV_CHANLIST, 0, osal_nv_item_len( ZCD_NV_CHANLIST ), &zgConfigPANID);
              osal_start_timerEx( SampleApp_TaskID,
                            SAMPLEAPP_RESET_EVT,
                            1000 );
          }
      }
      
        
      //HalUARTWrite(0,"Get Date:",9); // 提示收到数据
      //HalUARTWrite(0,buffer,4);
      //HalUARTWrite(0,"\t\n",2);
      /*len=pkt->cmd.Data[0]; 
      HalUARTWrite(0,"Get Date:",9); // 提示收到数据
      for(i=0;i<len;i++) 
        HalUARTWrite(0,&pkt->cmd.Data[i+1],1);//发给PC机显示 
      HalUARTWrite(0,"\t\n",2); // 回车换行 */
      
      break;

  }
}


/*********************************************************************
*********************************************************************/


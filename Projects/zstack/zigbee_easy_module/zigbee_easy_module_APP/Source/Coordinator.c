/**************************************************************************************************
  Filename:       Coordinator.c
  Revised:        $Date: 2020.02.14 $

  Description:    Sample Application (no Profile).

*********************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include "OSAL.h"
#include "AF.h"
#include "ZDApp.h"
#include "ZDObject.h"
#include "ZDProfile.h"
#include <String.h>
#include <hal_types.h>
#include "sapi.h"
#include "MT_SYS.h"
#include "OSAL_Nv.h"
#include "SampleApp.h"
#include "DebugTrace.h"
#include "OnBoard.h"

/* HAL */
#include "hal_lcd.h"
#include "hal_led.h"
#include "hal_key.h"
#include "hal_uart.h"

#define CHILD_NUM_EVENT      0x01
#define CO_RESET_EVENT       0x02
#define CHANGE_PANID_EVT     0x03

#define SAMPLEAPP_RESET_EVT  0x0F
/*********************************************************************
 * GLOBAL VARIABLES
 */
uint8 uart_rdata[128];         //数据暂存器，最多能够缓存40个字节
uint8 uart_rdlenrightflag=0; //数据长度校验位正确标志
uint8 uart_rdsuccessflag=0;  //成功接收到数据信息标志位
uint8 uart_rd_enableflag=0;   //接受状态标记
uint8 uart_rdlen=0;          //有用信息的数据长度
uint8 uart_rdlentemp=0;      //用来记录已读数据长度

uint8 child_num=0; 
uint16 time_cnt=0;

uint16 mypanid;
uint8 tmpshortaddr[4];
uint8 cntnum=0;

uint8 sendback_flag=0;

//sendBackData_t  *send_pMsg;
// This list should be filled with Application specific Cluster IDs.
const cId_t SampleApp_ClusterList[SAMPLEAPP_MAX_CLUSTERS] =
{
  SAMPLEAPP_CLUSTERID
};

//描述zigbee节点设备（设备描述符）
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

//********************************************************************
endPointDesc_t SampleApp_epDesc;//节点描述符
uint8 SampleApp_TaskID;   //任务优先级
uint8 SampleApp_TransID;  // 数据发送序列号
devStates_t SampleApp_NwkState;

/*********************************************************************
 * LOCAL FUNCTIONS
 */
void SampleApp_MessageMSGCB( afIncomingMSGPacket_t *pckt );
void SampleApp_SendTheMessage( void );
void Uart_Read( uint8 port, uint8 event );
void Uart_Write(uint8 cmd,uint8 port,uint8 *buf, uint16 len);
void ShowInfo(void);
void To_string(uint8 * dest, char * src, uint8 length);
void node_confirm(void);
void Send_data_back( afIncomingMSGPacket_t *pkt ,uint8* tmpbuffer);
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
  halUARTCfg_t uartConfig;
  SampleApp_TaskID                   = task_id;
  SampleApp_TransID                  = 0;
  // Fill out the endpoint description.
  SampleApp_epDesc.endPoint          = SAMPLEAPP_ENDPOINT;
  SampleApp_epDesc.task_id           = &SampleApp_TaskID;
  SampleApp_epDesc.simpleDesc        = (SimpleDescriptionFormat_t *)&SampleApp_SimpleDesc;
  SampleApp_epDesc.latencyReq        = noLatencyReqs;
  // Register the endpoint description with the AF
  afRegister( &SampleApp_epDesc );
  //------------------------配置串口---------------------------------
  uartConfig.configured=TRUE;
  uartConfig.baudRate=HAL_UART_BR_115200;
  uartConfig.flowControl=FALSE;
  uartConfig.callBackFunc=Uart_Read;
  HalUARTOpen(0,&uartConfig);
  HalUARTWrite(0,"UartInit OK\n", sizeof("UartInit OK\n"));//串口发送
  //Uart_Write("uartinit",sizeof("uartinit")-1);
  //-----------------------------------------------------------------
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
  if ( events & SYS_EVENT_MSG )
  {
    MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( SampleApp_TaskID );
    
    while ( MSGpkt )
    {
      switch ( MSGpkt->hdr.event )
      {
       case AF_INCOMING_MSG_CMD:
          SampleApp_MessageMSGCB( MSGpkt );
          break;
          
       case ZDO_STATE_CHANGE:
           SampleApp_NwkState = (devStates_t)(MSGpkt->hdr.status);
           if(SampleApp_NwkState==DEV_COORD_STARTING)
          {
            HalUARTWrite(0,"cost",4);
          }
          if(SampleApp_NwkState==DEV_ZB_COORD)
          {
            HalUARTWrite(0,"zbco",4);
            osal_set_event(SampleApp_TaskID,CHILD_NUM_EVENT);
          }
          break;
		  
        default:
          break;
      }
      osal_msg_deallocate( (uint8 *)MSGpkt );
      MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( SampleApp_TaskID );
    }
    return (events ^ SYS_EVENT_MSG);
  }
  if(events & CHILD_NUM_EVENT)
  {
    
    if(time_cnt==CO_RESET_TIME){
      node_confirm();
    }
    time_cnt++;
    osal_start_timerEx(SampleApp_TaskID,CHILD_NUM_EVENT,1000);//定时函数
    return (events ^ CHILD_NUM_EVENT);
  }
  if(events & CO_RESET_EVENT)
  {
    
    if(child_num==0){
      SystemReset();
    }
    child_num=0;
    return (events ^ CO_RESET_EVENT);
  }
  if(events & SAMPLEAPP_RESET_EVT)
  {
    SystemReset();
  }
  
  if(events & CHANGE_PANID_EVT)
  {
        _NIB.nwkPanId = mypanid;
        NLME_UpdateNV(0x01);
        osal_nv_write(ZCD_NV_PANID, 0, osal_nv_item_len( ZCD_NV_PANID ), &zgConfigPANID);
        osal_start_timerEx( SampleApp_TaskID,
                      SAMPLEAPP_RESET_EVT,
                      2000 );
        return (events ^ CHANGE_PANID_EVT);
  }       
  return 0;
}

/*********************************************************************
 * LOCAL FUNCTIONS
 */
/*********************************************************************
 * @fn      Uart_Read
 *
 * @brief   
 *
 * @param   none
 *
 * @return  none
 */
void Uart_Read(uint8 port, uint8 event)
{
//**********串口协议解析用的变量************************************	
  uint8  ch,check_temp;
  uart_rdlenrightflag=0;          
  uart_rdsuccessflag=0;           
  uart_rd_enableflag=0;           
  uart_rdlen=0;               
  uart_rdlentemp=0;
		
//**********发送数据用到的变量**************************************	
  afAddrType_t my_DstAddr;//发送函数结构体初始化
		
//*****************临时变量*****************************************		
  RFTX rftx;//读取设备状态结构体		
  uint8 PAN_ID[2];//PANID temp
  uint8 SHORT[2];//SHORTADDR temp
  uint16 SHORTADDR_temp;//SHORTADDR temp
  uint8 Change_panid[4];//CHANGEPANID temp
  uint8 brodata[4];
  uint8 mychannel;

  while (Hal_UART_RxBufLen(0))
  {
        HalUARTRead(0,&ch,1);
        if(ch==0xac)
        {
			uart_rd_enableflag=1;
			uart_rdlentemp++;
        }
	else if(uart_rd_enableflag==1&&uart_rdlentemp==1)
	{
		uart_rdlen=ch;
		uart_rdlentemp++;
	}
	else if(uart_rd_enableflag==1&&uart_rdlentemp>=2&&uart_rdlentemp<=uart_rdlen+1)
	{
		uart_rdata[uart_rdlentemp-2]=ch;
		uart_rdlentemp++;
	}
	else if(uart_rd_enableflag==1&&uart_rdlentemp==uart_rdlen+2)
	{
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
          
          SHORTADDR_temp=NLME_GetShortAddr();
          To_string(rftx.myNWK,(uint8 *)&SHORTADDR_temp,2);
          To_string(rftx.myMAC,NLME_GetExtAddr(),8);
          
/************************************************************************************
**************************自定协议AC LEN CMD DATA ~LEN EF***************************
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
  
#define CNTDEVNUM                   0xD1  //统计入网设备数量(未实现)
        
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
                PAN_ID[1]=0xFF&mypanid;
                PAN_ID[0]=0xFF&(mypanid>>8);
                Uart_Write(0xFA,0,PAN_ID,2);
            break;  
            
          case SEND_CHANNEL:
		Uart_Write(0xFB,0,&_NIB.nwkLogicalChannel,1);
            break;
            
          case SEND_SHORTADDR:
                SHORT[1]=0xFF&SHORTADDR_temp;
                SHORT[0]=0xFF&(SHORTADDR_temp>>8);
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
            {0xac len [0xF2 SHORTADDR(2) DATA] ~len 0xef}
         
 *****************  UART back data: 0xac len cmd * ~len 0xef   ********************
 **********************************************************************************
*/         
          case SEND_DATA:
              my_DstAddr.addrMode = (afAddrMode_t)AddrBroadcast;//设置发送模式为广播
              my_DstAddr.endPoint = SAMPLEAPP_ENDPOINT;//初始化端口号
              my_DstAddr.addr.shortAddr=0xFFFF;
              AF_DataRequest(&my_DstAddr,
                            &SampleApp_epDesc,
                            SAMPLEAPP_CLUSTERID,
                            uart_rdlen-1,
                            &uart_rdata[1],
                            &SampleApp_TransID,
                            AF_DISCV_ROUTE,
                            AF_DEFAULT_RADIUS);          
              Uart_Write(0xF1,0,&uart_rdata[1],uart_rdlen-1);
            break; 
            
         case SEND_DATA_P2P:
              my_DstAddr.addrMode = (afAddrMode_t)Addr16Bit;//设置发送模式为点播
              my_DstAddr.endPoint = SAMPLEAPP_ENDPOINT;//初始化端口号
              uint16 myaddr_short;
              myaddr_short=(uart_rdata[1]<<8) + uart_rdata[2];
              my_DstAddr.addr.shortAddr=myaddr_short;
              
              AF_DataRequest(&my_DstAddr,
                   &SampleApp_epDesc,
                   SAMPLEAPP_CLUSTERID,
                   uart_rdlen-3,
                   &uart_rdata[3],
                   &SampleApp_TransID,
                   AF_DISCV_ROUTE,
                   AF_DEFAULT_RADIUS);          
              //Uart_Write(0xF2,0,&uart_rdata[1],uart_rdlen-1);//调试用
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
               Change_panid[0]=0xE1;
               Change_panid[1]=uart_rdata[1];
               Change_panid[2]=uart_rdata[2];
               Change_panid[3]=0xFF;
                my_DstAddr.addrMode = (afAddrMode_t)AddrBroadcast;//设置发送模式为广播
                my_DstAddr.endPoint = SAMPLEAPP_ENDPOINT;//初始化端口号
                my_DstAddr.addr.shortAddr=0xFFFF;
               AF_DataRequest(&my_DstAddr,
                   &SampleApp_epDesc,
                   SAMPLEAPP_CLUSTERID,
                   4,
                   &Change_panid[0],
                   &SampleApp_TransID,
                   AF_DISCV_ROUTE,
                   AF_DEFAULT_RADIUS);
                osal_start_timerEx( SampleApp_TaskID,
                      CHANGE_PANID_EVT,
                      3000 );
            break; 
            
         case CHANGE_CHANNEL:

              mychannel=uart_rdata[1];
              
              if(mychannel<0x0B||mychannel>0x1A){
              HalUARTWrite(0,"wrong channel",13);
              }
              else{
              brodata[0]=0xFE;
              brodata[1]=uart_rdata[1];
              brodata[2]=0;
              brodata[3]=0xFF;
              my_DstAddr.addrMode = (afAddrMode_t)AddrBroadcast;//设置发送模式为广播
              my_DstAddr.endPoint = SAMPLEAPP_ENDPOINT;//初始化端口号
              my_DstAddr.addr.shortAddr=0xFFFF;
              //zb_WriteConfiguration(ZCD_NV_PANID, sizeof(uint16),  &mychannel);
              //HalUARTWrite(0,&uart_rdata[1],1);
              AF_DataRequest(&my_DstAddr,
                              &SampleApp_epDesc,
                              SAMPLEAPP_CLUSTERID,
                              4,
                              &brodata[0],
                              &SampleApp_TransID,
                              AF_DISCV_ROUTE,
                              AF_DEFAULT_RADIUS);
              
              _NIB.nwkLogicalChannel = mychannel;
              NLME_UpdateNV(0x01);
              osal_nv_write(ZCD_NV_CHANLIST, 0, osal_nv_item_len( ZCD_NV_CHANLIST ), &zgConfigPANID);
              osal_start_timerEx( SampleApp_TaskID,
                    SAMPLEAPP_RESET_EVT,
                    1000 );
              
              /*zb_WriteConfiguration(ZCD_NV_PANID, sizeof(uint16),  &pan_id) ;
              zb_SystemReset();
            
              ZDP_MgmtNwkUpdateReq( zAddrType_t *dstAddr,
              uint32 ChannelMask,
              uint8 ScanDuration,
              uint8 ScanCount,
              uint8 NwkUpdateId,
              uint16 NwkManagerAddr )
            
              MgmtNwkUpdateReqFormat_t req;
              req.DstAddr=0xFFFF;
              req.DstAddrMode=AddrBroadcast;
              req.ChannelMask[0]=BREAK_UINT32(newChanList,0);
              req.ChannelMask[1]=BREAK_UINT32(newChanList,1);
              req.ChannelMask[2]=BREAK_UINT32(newChanList,2);
              req.ChannelMask[3]=BREAK_UINT32(newChanList,3);
              req.ScanDuration=0xFE;      //Request is to change Channel
              req.ScanCount=0;                //don't care
              req.NwkManagerAddr=0;       //don't care
              zdoMgmtNwkUpdateReq(&req);*/
            
              
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
        case CNTDEVNUM:
             Change_panid[0]=0xD1;
             Change_panid[1]=0xD2;
             Change_panid[2]=0xD3;
             Change_panid[3]=0xFF;
             AF_DataRequest(&my_DstAddr,
                            &SampleApp_epDesc,
                            SAMPLEAPP_CLUSTERID,
                            4,
                            &Change_panid[0],
                            &SampleApp_TransID,
                            AF_DISCV_ROUTE,
                            AF_DEFAULT_RADIUS);
              //HalUARTWrite(0,Change_panid,4);
              //osal_set_event(SampleApp_TaskID,CNT_NUM_EVT);
          break;
            
        case SEND_BACK:
            sendback_flag=1;
          break; 
        
        case SEND_BACK_CANCEL:
            sendback_flag=0;
          break;
                            
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
  //afIncomingMSGPacket_t *MSGpkt;
  //MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( SampleApp_TaskID );
  //SampleApp_NwkState = (devStates_t)(MSGpkt->hdr.status);
}
/*********************************************************************
 * @fn      Uart_Write
 *
 * @brief   
 *
 * @param   none
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
    for(i=0;i<length;i++,xad--){
        ch=(*xad>>4)&0x0F;
        dest[i<<1]=ch+((ch<10)?'0':'7');
        ch=*xad&0x0F;
        dest[(i<<1)+1]=ch+((ch<10)?'0':'7');                    
    }
}

/*********************************************************************
 * @fn      node_confirm
 *
 * @brief   
 *
 * @param   none
 *
 * @return  none
 */
void node_confirm(void){
    uint8 *buf="CFM";
    afAddrType_t my_DstAddr;
    my_DstAddr.addrMode = (afAddrMode_t)AddrBroadcast;//设置发送模式为广播
    my_DstAddr.endPoint = SAMPLEAPP_ENDPOINT;//初始化端口号
    my_DstAddr.addr.shortAddr=0xFFFF;
    AF_DataRequest(&my_DstAddr,
                   &SampleApp_epDesc,
                   SAMPLEAPP_CLUSTERID,
                   3,
                   buf,
                   &SampleApp_TransID,
                   AF_DISCV_ROUTE,
                   AF_DEFAULT_RADIUS);
    time_cnt=0;
    osal_start_timerEx(SampleApp_TaskID,CO_RESET_EVENT,CO_WAIT_TIME);
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
    uint8 buffer[50];
    switch ( pkt->clusterId ){
            case SAMPLEAPP_CLUSTERID: 
            osal_memcpy(buffer,pkt->cmd.Data,pkt->cmd.DataLength);
            
            if((buffer[0]=='Y')||(buffer[1]=='E')||(buffer[2]=='S')){
                    child_num++;
            }

            if(buffer[0]==0xD1){
                    uint8 tmpbbuf[4];
                    for(int i=0;i<4;i++){
                            tmpbbuf[i]=buffer[i+1];
                    }
                    if(strcmp(tmpshortaddr,tmpbbuf)!=0){
                            cntnum++;
                    }
                    for(int i=0;i<4;i++){
                            tmpshortaddr[i]=buffer[i+1];
                    }
            }
                    HalUARTWrite(0,buffer,pkt->cmd.DataLength);
            break;   
    }
}



/*********************************************************************
*********************************************************************/
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

#include "SampleApp.h"
#include "DebugTrace.h"

#include "OnBoard.h"

/* HAL */
#include "hal_lcd.h"
#include "hal_led.h"
#include "hal_key.h"
#include "hal_uart.h"

/*********************************************************************
 * GLOBAL VARIABLES
 */

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

/*********************************************************************
 * LOCAL FUNCTIONS
 */
void SampleApp_MessageMSGCB( afIncomingMSGPacket_t *pckt );
void SampleApp_SendTheMessage( void );

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
  uartConfig.callBackFunc=NULL;
  HalUARTOpen(0,&uartConfig);
  HalUARTWrite(0,"UartInit OK\n", sizeof("UartInit OK\n"));//串口发送
  HalUARTWrite(0,"UartInit2 OK\n", sizeof("UartInit2 OK\n"));//串口发送
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
        // Received when a messages is received (OTA) for this endpoint
        case AF_INCOMING_MSG_CMD:
          SampleApp_MessageMSGCB( MSGpkt );
          break;
        default:
          break;
      }
      // Release the memory
      osal_msg_deallocate( (uint8 *)MSGpkt );
      // Next - if one is available
      MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( SampleApp_TaskID );
    }
    // return unprocessed events
    return (events ^ SYS_EVENT_MSG);
  }
  // Discard unknown events
  return 0;
}

/*********************************************************************
 * Event Generation Functions
 */
/*********************************************************************
 * LOCAL FUNCTIONS
 */



/*********************************************************************
 * @fn      SampleApp_MessageMSGCB
 *
 *
 * @param   none
 *
 * @return  none
 */
void SampleApp_MessageMSGCB( afIncomingMSGPacket_t *pkt )
{ 
  //uint8 i,len;
  uint8 buffer[10];
  switch ( pkt->clusterId )
  {
    case SAMPLEAPP_CLUSTERID: //如果是串口透传的信息
      osal_memcpy(buffer,pkt->cmd.Data,10);
      HalUARTWrite(0,buffer,10);
      /*len=pkt->cmd.Data[0]; 
      HalUARTWrite(0,"Get Date:",9); // 提示收到数据
      for(i=0;i<len;i++) 
        HalUARTWrite(0,&pkt->cmd.Data[i+1],1);//发给PC机显示 
      HalUARTWrite(0,"\t\n",2); // 回车换行 
      */
      break;

  }
}

/*********************************************************************
*********************************************************************/

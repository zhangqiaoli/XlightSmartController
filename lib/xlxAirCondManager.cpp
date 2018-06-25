#include "xlxAirCondManager.h"
#include "xlSmartController.h"
#include "xlxConfig.h"
#include "xlxLogger.h"
xlAirCondManager theACManager;

xlAirCondManager::xlAirCondManager()
{
  m_isChanged = FALSE;
}
xlAirCondManager::~xlAirCondManager()
{

}
void xlAirCondManager::InitACNode(ACDev_t& acNode)
{
  memset(&acNode,0x00,sizeof(ACDev_t));
  uint32_t now = Time.now();
  acNode.m_nSSLastMsgTime = now;
  acNode.m_nCMLastMsgTime = now;
  acNode.m_nTotalEQ = 0;
  acNode.m_nLastTotalEQ =0;
  acNode.m_todayStart.nTotalEQ = 0;
  acNode.m_todayStart.timestamp = now;
  acNode.m_todayStart.bEQReset = 0;
  acNode.m_todayEnd.nTotalEQ = 0;
  acNode.m_todayEnd.timestamp = now;
  acNode.m_todayEnd.bEQReset = 1;
  acNode.m_nEndDay = Time.format(now, "%Y%m%d").toInt();
}

bool xlAirCondManager::UpdateACCurrentByNodeid(uint8_t nodeid,uint16_t ec)
{
  Serial.printlnf("UpdateACCurrentByNodeid update nodeid:%d,ec:%d",nodeid,ec);
  ACDev_t acNode;
  memset(&acNode,0x00,sizeof(ACDev_t));
  acNode.nid = nodeid;
  if(m_lstACDev.get(&acNode) >=0)
  { // old node,need update
    uint32_t now = Time.now();
    acNode.m_nElecCurrent = ec;
    acNode.m_nSSLastMsgTime = now;
    acNode.m_nEndDay = Time.format(now, "%Y%m%d").toInt();
  }
  else
  {// not found,need add
    InitACNode(acNode);
    acNode.nid = nodeid;
    acNode.m_nElecCurrent = ec;
  }
  PublishACEC(acNode);
  return TRUE;
}

bool xlAirCondManager::UpdateACByNodeid(uint8_t nodeid,uint16_t ec,uint32_t eq/* = 0*/,uint16_t eqindex/* = 0*/,uint8_t eqreset/* = 0*/)
{
  Serial.printlnf("UpdateACByNodeid update nodeid:%d,ec:%d,eq:%d,index:%d,eqreset:%d",nodeid,ec,eq,eqindex,eqreset);
  ACDev_t acNode;
  memset(&acNode,0x00,sizeof(ACDev_t));
  acNode.nid = nodeid;
  if(m_lstACDev.get(&acNode) >=0)
  { // old node,need update
    uint32_t now = Time.now();
    acNode.m_nElecCurrent = ec;
    uint32_t lasttime = acNode.m_nSSLastMsgTime;
    acNode.m_nSSLastMsgTime = now;
    uint32_t nLastDay = acNode.m_nEndDay;
    acNode.m_nEndDay = Time.format(now, "%Y%m%d").toInt();
    if(eqindex == acNode.m_lastEQIndex)
    { // same msg,ignore
      Serial.printlnf("same msg,ignore");
      return TRUE;
    }
    acNode.m_lastEQIndex = eqindex;
    bool updateLastTotal = FALSE;
    if(eqreset == 1)
    {// total eq was reset to 0,need accumulate
      if(acNode.m_todayEnd.bEQReset == 0 || (acNode.m_todayEnd.bEQReset == 1 && now - acNode.m_todayEnd.timestamp > 3600))
      { // reset more than 1hour
        updateLastTotal = TRUE;
      }
    }
    else if(eq < acNode.m_nTotalEQ)
    { // need reset lasttotal
      LOGE(LOGTAG_MSG,"warning:smartsocket total eq:%.2f less than p1 total eq:%.2f!",eq/100.0,acNode.m_nTotalEQ/100.0);
    }
    if(updateLastTotal == TRUE)
    {
      LOGN(LOGTAG_MSG,"Reset,before update lasttotal=%.2f,total=%.2f",acNode.m_nLastTotalEQ/100.0,acNode.m_nTotalEQ/100.0);
      acNode.m_nLastTotalEQ = acNode.m_nTotalEQ;
    }
    Serial.printlnf("before total:%.2f,lasttotal:%.2f",acNode.m_nTotalEQ/100.0,acNode.m_nLastTotalEQ/100.0);
    acNode.m_nTotalEQ = acNode.m_nLastTotalEQ + eq;
    Serial.printlnf("after total:%.2f,lasttotal:%.2f",acNode.m_nTotalEQ/100.0,acNode.m_nLastTotalEQ/100.0);

    if(acNode.m_nEndDay > nLastDay)
    {  ////////////////////update day eq////////////////////
      uint8_t notrundays = (now - lasttime)/(3600*24);
      if(notrundays >= HISTORYEQDAYS)
      { // history data invalid,clear
        LOGW(LOGTAG_MSG,"now=%d,last=%d,need clear history data",now,lasttime);
        memset(acNode.m_arrHistoryEQ,0x00,HISTORYEQDAYS*sizeof(uint32_t));
      }
      else
      {
        uint8_t nowWeekday = Time.weekday(now);
        uint8_t lastWeekday = Time.weekday(lasttime);
        uint8_t intervalday = (nowWeekday+7-lastWeekday)%7;
        if(intervalday==1 && (now - lasttime)<=300 )
        {
          //normal new day,update m_arrHistoryEQ
          acNode.m_arrHistoryEQ[0] = acNode.m_nTotalEQ - acNode.m_todayStart.nTotalEQ;
        }
        else
        {
          acNode.m_arrHistoryEQ[0] = acNode.m_todayEnd.nTotalEQ - acNode.m_todayStart.nTotalEQ;
        }
        for(int8_t i=HISTORYEQDAYS-1;i>=0;i--)
        {
          if(i >= intervalday)
          {
            acNode.m_arrHistoryEQ[i] = acNode.m_arrHistoryEQ[i-intervalday];
          }
          else
          {
            acNode.m_arrHistoryEQ[i] = 0;
          }
        }
      }
      acNode.m_todayStart.timestamp = now;
      acNode.m_todayStart.nTotalEQ = acNode.m_nTotalEQ;
      acNode.m_todayStart.bEQReset = eqreset;
      acNode.m_todayEnd.timestamp = now;
      acNode.m_todayEnd.nTotalEQ = acNode.m_nTotalEQ;
      acNode.m_todayEnd.bEQReset = eqreset;
      acNode.m_arrHistoryEQ[0] = 0;
    }
    else
    {
      if(acNode.m_nEndDay == nLastDay)
      {
        acNode.m_todayEnd.timestamp = now;
        acNode.m_todayEnd.nTotalEQ = acNode.m_nTotalEQ;
        acNode.m_todayEnd.bEQReset = eqreset;
        acNode.m_arrHistoryEQ[0] = acNode.m_todayEnd.nTotalEQ - acNode.m_todayStart.nTotalEQ;
        Serial.printlnf("toadyeq:%d",acNode.m_arrHistoryEQ[0]);
      }
      else
      {
        LOGE(LOGTAG_MSG,"now=%d,last=%d,time has occur error!",now,lasttime);
      }
    }
    ////////////////////update eq end////////////////
  }
  else
  {// not found,need add
    InitACNode(acNode);
    acNode.nid = nodeid;
    acNode.m_nElecCurrent = ec;
    acNode.m_todayStart.bEQReset = eqreset;
    acNode.m_todayEnd.bEQReset = eqreset;
    acNode.m_todayStart.nTotalEQ = eq;
    acNode.m_todayEnd.nTotalEQ = eq;
    acNode.m_nTotalEQ = eq;
    acNode.m_nLastTotalEQ = 0;
  }
  m_lstACDev.add(&acNode);
  Serial.printlnf("UpdateACByNodeid nodeid:%d,sid:%d,ec:%d,eq:%d,ss:%d,mc:%d,onoff:%d,mode:%d,temp:%d,fan:%d,endday:%d",acNode.nid,\
               acNode.sid,acNode.m_nElecCurrent,acNode.m_arrHistoryEQ[0],acNode.m_nSSLastMsgTime,acNode.m_nCMLastMsgTime,\
               acNode.m_sLastOp.onoff,acNode.m_sLastOp.mode,acNode.m_sLastOp.temp,acNode.m_sLastOp.fanlevel,\
               acNode.m_nEndDay);
  m_isChanged = TRUE;
  PublishACInfo(acNode);

  if(((acNode.m_nTotalEQ >> 16)^0xFFFFFFFFFFFF) == 0 )
  {
    LOGW(LOGTAG_MSG, "Total EQ need clear to 0");
    acNode.m_nTotalEQ = 0;
    acNode.m_nLastTotalEQ = 0;
  }
  return TRUE;
}


bool xlAirCondManager::UpdateACStatusByNodeid(uint8_t nodeid,uint8_t onoff,uint8_t mode,uint8_t temp,uint8_t fanlevel)
{
  bool statusChanged = FALSE;
  ACDev_t acNode;
  memset(&acNode,0x00,sizeof(ACDev_t));
  acNode.nid = nodeid;
  uint32_t now = Time.now();
  if(m_lstACDev.get(&acNode) >=0)
  { // old node,need update
    acNode.m_nCMLastMsgTime = now;
    if(acNode.m_sLastOp.onoff != onoff)
    {
      acNode.m_sLastOp.onoff = onoff;
      statusChanged = TRUE;
    }
    if(acNode.m_sLastOp.mode != mode)
    {
      acNode.m_sLastOp.mode = mode;
      statusChanged = TRUE;
    }
    if(acNode.m_sLastOp.temp != temp)
    {
      acNode.m_sLastOp.temp = temp;
      statusChanged = TRUE;
    }
    if(acNode.m_sLastOp.fanlevel != fanlevel)
    {
      acNode.m_sLastOp.fanlevel = fanlevel;
      statusChanged = TRUE;
    }
    Serial.printlnf("UpdateACStatusByNodeid update nodeid:%d",nodeid);
  }
  else
  {// not found,need add
    acNode.m_nSSLastMsgTime = now;
    acNode.m_nCMLastMsgTime = now;
    acNode.m_sLastOp.onoff = onoff;
    acNode.m_sLastOp.mode = mode;
    acNode.m_sLastOp.temp = temp;
    acNode.m_sLastOp.fanlevel = fanlevel;
    acNode.m_nEndDay = Time.format(now, "%Y%m%d").toInt();
    statusChanged = TRUE;
  }
  m_lstACDev.add(&acNode);
  Serial.printlnf("UpdateACStatusByNodeid nodeid:%d,sid:%d,ec:%d,ss:%d,mc:%d,onoff:%d,mode:%d,temp:%d,fan:%d,endday:%d",acNode.nid,\
               acNode.sid,acNode.m_nElecCurrent,acNode.m_nSSLastMsgTime,acNode.m_nCMLastMsgTime,\
               acNode.m_sLastOp.onoff,acNode.m_sLastOp.mode,acNode.m_sLastOp.temp,acNode.m_sLastOp.fanlevel,\
               acNode.m_nEndDay);
  m_isChanged = TRUE;
  if(statusChanged)
  {
    PublishACStatus(acNode);
  }
  return TRUE;
}

bool xlAirCondManager::UpdateACStatusByNodeid(uint8_t nodeid,AirConditioningStauts_t& acStatus)
{
  bool statusChanged = FALSE;
  ACDev_t acNode;
  memset(&acNode,0x00,sizeof(ACDev_t));
  acNode.nid = nodeid;
  uint32_t now = Time.now();
  if(m_lstACDev.get(&acNode) >=0)
  { // old node,need update
    acNode.m_nCMLastMsgTime = now;
    if(acNode.m_sLastOp.onoff != acStatus.onoff || acNode.m_sLastOp.mode != acStatus.mode\
       || acNode.m_sLastOp.temp != acStatus.temp || acNode.m_sLastOp.fanlevel != acStatus.fanlevel)
    {
      acNode.m_sLastOp = acStatus;
      statusChanged = TRUE;
    }
  }
  else
  {// not found,need add
    acNode.m_nSSLastMsgTime = now;
    acNode.m_nCMLastMsgTime = now;
    acNode.m_nEndDay = Time.format(now, "%Y%m%d").toInt();
    acNode.m_sLastOp = acStatus;
    statusChanged = TRUE;
  }
  m_lstACDev.add(&acNode);
  m_isChanged = TRUE;
  if(statusChanged)
  {
    PublishACStatus(acNode);
  }
  return TRUE;
}

bool xlAirCondManager::PublishACEC(ACDev_t& acNode)
{
  String strTemp = String::format("{'nd':%d,'ec':%.2f}",acNode.nid,acNode.m_nElecCurrent/100.0);
  theSys.PublishACDeviceStatus(strTemp.c_str());
  return TRUE;
}

bool xlAirCondManager::PublishACStatus(ACDev_t& acNode)
{
  String strTemp = String::format("{'nd':%d,'dt':[%d,%d,%d,%d]}",acNode.nid,acNode.m_sLastOp.onoff,\
                           acNode.m_sLastOp.mode,acNode.m_sLastOp.temp,acNode.m_sLastOp.fanlevel);
  theSys.PublishACDeviceStatus(strTemp.c_str());
  return TRUE;
}

bool xlAirCondManager::PublishACInfo(ACDev_t& acNode,bool bAllDays/*=FALSE*/)
{
  String strTemp = "";
  String strformat = "{'nd':%d,'ec':%.2f,'total':%.2f,'dt':[%d,%d,%d,%d],'cd':%d,'days':%d,";
  uint8_t days = 1;
  if(bAllDays)
  {
    days = HISTORYEQDAYS;
  }
  strformat += "'eq':[";
  for(uint8_t i=0;i<days;i++)
  {
    strformat += String(acNode.m_arrHistoryEQ[i]/100.0,2);
    if(i != days-1)
    {
      strformat += ",";
    }
    else
    {
      strformat += "]}";
    }
  }
  strTemp = String::format(strformat,acNode.nid,acNode.m_nElecCurrent/100.0,acNode.m_nTotalEQ/100.0,acNode.m_sLastOp.onoff,\
                           acNode.m_sLastOp.mode,acNode.m_sLastOp.temp,acNode.m_sLastOp.fanlevel,acNode.m_nEndDay,days);
	theSys.PublishACDeviceStatus(strTemp.c_str());
	return TRUE;
}

bool xlAirCondManager::PublishHistoryInfo()
{
  for(uint8_t i = 0;i<m_lstACDev.count();i++)
  {
    Serial.printlnf("PublishHistoryInfo nodeid:%d,sid:%d,ec:%.2f,ss:%d,mc:%d,onoff:%d,mode:%d,temp:%d,fan:%d,endday:%d",m_lstACDev._pItems[i].nid,\
                 m_lstACDev._pItems[i].sid,m_lstACDev._pItems[i].m_nElecCurrent/100.0,m_lstACDev._pItems[i].m_nSSLastMsgTime,m_lstACDev._pItems[i].m_nCMLastMsgTime,\
                 m_lstACDev._pItems[i].m_sLastOp.onoff,m_lstACDev._pItems[i].m_sLastOp.mode,m_lstACDev._pItems[i].m_sLastOp.temp,m_lstACDev._pItems[i].m_sLastOp.fanlevel,\
                 m_lstACDev._pItems[i].m_nEndDay);
    if( !IS_NOT_AC_NODEID(m_lstACDev._pItems[i].nid))
    {
      PublishACInfo(m_lstACDev._pItems[i],TRUE);
    }
  }
}

bool xlAirCondManager::PublishTimeoutAlarm(uint8_t *arrNodeid,uint8_t size,uint8_t type)
{
  StaticJsonBuffer<255> jsonBuffer;

  JsonObject& root = jsonBuffer.createObject();
  root["name"] = "Offline";
  root["devname"] = "";
  root["tag"] = type;

  JsonArray& arrnd = root.createNestedArray("nd");
  for(int i = 0 ;i<size;i++ )
  {
    arrnd.add(*(arrNodeid+i));
  }
  char msg[255];
  memset(msg,0x00,sizeof(msg));
  root.printTo(msg,255);
  theSys.PublishAlarm(msg);
}

void xlAirCondManager::ProcessCheck()
{
  uint32_t now  = Time.now();
  uint32_t nLastDay = Time.format(now, "%Y%m%d").toInt();
  uint8_t arrSSNode[MAX_AC_DEVICE_PER_CONTROLLER];
  uint8_t ssndSize = 0;
  uint8_t arrMCNode[MAX_AC_DEVICE_PER_CONTROLLER];
  uint8_t mcndSize = 0;
  memset(arrSSNode,0x00,sizeof(arrSSNode));
  memset(arrMCNode,0x00,sizeof(arrMCNode));
  for(uint8_t i = 0;i<m_lstACDev.count();i++)
  {
    Serial.printlnf("ProcessCheck nodeid:%d,sid:%d,ec:%d,ss:%d,mc:%d,onoff:%d,mode:%d,temp:%d,fan:%d,endday:%d",m_lstACDev._pItems[i].nid,\
                 m_lstACDev._pItems[i].sid,m_lstACDev._pItems[i].m_nElecCurrent,m_lstACDev._pItems[i].m_nSSLastMsgTime,m_lstACDev._pItems[i].m_nCMLastMsgTime,\
                 m_lstACDev._pItems[i].m_sLastOp.onoff,m_lstACDev._pItems[i].m_sLastOp.mode,m_lstACDev._pItems[i].m_sLastOp.temp,m_lstACDev._pItems[i].m_sLastOp.fanlevel,\
                 m_lstACDev._pItems[i].m_nEndDay);
    if( !IS_NOT_AC_NODEID(m_lstACDev._pItems[i].nid) && now - m_lstACDev._pItems[i].m_nSSLastMsgTime < 3600*24*HISTORYEQDAYS)
    {
      // TODO timeout check
      if(now - m_lstACDev._pItems[i].m_nSSLastMsgTime > 180)
      { // 3min timeout
        arrSSNode[ssndSize++] = m_lstACDev._pItems[i].nid;
        Serial.printlnf("SS node:%d,timeout",m_lstACDev._pItems[i].nid);
      }
      if(now - m_lstACDev._pItems[i].m_nCMLastMsgTime > 180)
      { // 3min timeout
        arrMCNode[mcndSize++] = m_lstACDev._pItems[i].nid;
        Serial.printlnf("MC node:%d,timeout",m_lstACDev._pItems[i].nid);
      }
      //  day eq array check
      bool bNeedpublish = FALSE;
      if(nLastDay > m_lstACDev._pItems[i].m_nEndDay)
      { // day eq array need update
        m_lstACDev._pItems[i].m_nEndDay = nLastDay;
        m_lstACDev._pItems[i].m_arrHistoryEQ[0] = m_lstACDev._pItems[i].m_todayEnd.nTotalEQ - m_lstACDev._pItems[i].m_todayStart.nTotalEQ;
        m_lstACDev._pItems[i].m_todayStart.nTotalEQ = m_lstACDev._pItems[i].m_todayEnd.nTotalEQ;
        for(int8_t j = HISTORYEQDAYS-1;j>=0;j--)
        {
          if(j>0)
          {
            if(m_lstACDev._pItems[i].m_arrHistoryEQ[j] != 0) bNeedpublish = TRUE;
            m_lstACDev._pItems[i].m_arrHistoryEQ[j] = m_lstACDev._pItems[i].m_arrHistoryEQ[j-1];
          }
          else
          {
            m_lstACDev._pItems[i].m_arrHistoryEQ[0] = 0;
          }
        }
        m_isChanged = TRUE;
      }
      if(bNeedpublish)
      { // eq array data isn't all 0,need publish
        PublishACInfo(m_lstACDev._pItems[i],TRUE);
      }
    }
    else
    {
      LOGW(LOGTAG_MSG, "ProcessCheck check not ac node,what?");
    }
  }
  if(mcndSize > 0)
  {
    PublishTimeoutAlarm(arrMCNode,mcndSize,2);
  }
  if(ssndSize > 0)
  {
      PublishTimeoutAlarm(arrSSNode,ssndSize,3);
  }
  SaveACNode();
}


bool xlAirCondManager::LoadACNode()
{
  bool bReadmain = TRUE;
  ACDev_t ACNodeArray[MAX_AC_DEVICE_PER_CONTROLLER];
  if (sizeof(ACDev_t)*MAX_NODE_PER_CONTROLLER >= MEM_ACLIST_LEN)
  {
    if (theConfig.getP1Flash()->read<ACDev_t[MAX_AC_DEVICE_PER_CONTROLLER]>(ACNodeArray, MEM_ACLIST_OFFSET))
    {
      for (int i = 0; i < MAX_AC_DEVICE_PER_CONTROLLER; i++)
      {
        if( !IS_NOT_AC_NODEID(ACNodeArray[i].nid))
        {
          if (m_lstACDev.add(&ACNodeArray[i]) < 0)
          {
            LOGW(LOGTAG_MSG, "AC node row %d failed to load from flash", i);
            bReadmain = FALSE;
            break;
          }
          else
          {
            LOGW(LOGTAG_MSG, "AC  node row %d success to load from flash-nodeid=%d", i,ACNodeArray[i].nid);
          }
        }
      }
    }
    else
    {
      LOGW(LOGTAG_MSG, "Failed to read the ac node list from flash.");
      bReadmain = FALSE;
    }
  }
  else
  {
    LOGW(LOGTAG_MSG, "Failed to load ac node list, too large.");
    bReadmain = FALSE;
  }
  if(!bReadmain)
  { //read from backup
    if (sizeof(ACDev_t)*MAX_NODE_PER_CONTROLLER >= MEM_ACLIST_BACKUP_LEN)
    {
      if (theConfig.getP1Flash()->read<ACDev_t[MAX_AC_DEVICE_PER_CONTROLLER]>(ACNodeArray, MEM_ACLIST_BACKUP_OFFSET))
      {
        for (int i = 0; i < MAX_AC_DEVICE_PER_CONTROLLER; i++)
        {
          if( !IS_NOT_AC_NODEID(ACNodeArray[i].nid))
          {
            if (m_lstACDev.add(&ACNodeArray[i]) < 0)
            {
              LOGW(LOGTAG_MSG, "AC backup node row %d failed to load from flash", i);
              return FALSE;
            }
            else
            {
              LOGW(LOGTAG_MSG, "AC  backup node row %d success to load from flash-nodeid=%d", i,ACNodeArray[i].nid);
            }
          }
        }
      }
      else
      {
        LOGW(LOGTAG_MSG, "Failed to read the ac backup node list from flash.");
        return FALSE;
      }
    }
    else
    {
      LOGW(LOGTAG_MSG, "Failed to load ac backup node list, too large.");
      return FALSE;
    }
  }
  return TRUE;
}

bool xlAirCondManager::SaveACNode()
{
  bool ret = FALSE;
  if( m_isChanged ) {
    m_isChanged = FALSE;
    ACDev_t lv_buf[MAX_AC_DEVICE_PER_CONTROLLER];
    memset(lv_buf, 0x00, sizeof(lv_buf));
    memcpy(lv_buf, m_lstACDev._pItems, sizeof(ACDev_t) * m_lstACDev.count());
    uint8_t attemps = 0;
    while(++attemps <=3 )
    {
      if(theConfig.getP1Flash()->write<ACDev_t[MAX_AC_DEVICE_PER_CONTROLLER]>(lv_buf, MEM_ACLIST_OFFSET))
      {
        LOGN(LOGTAG_MSG, "write ac nodelist success!");
        ret = TRUE;
        break;
      }
      else
      {
        LOGW(LOGTAG_MSG,"write ac nodelist failed! tried=%d",attemps);
      }
    }
    attemps = 0;
    while(++attemps <=3 )
    {
      if(theConfig.getP1Flash()->write<ACDev_t[MAX_AC_DEVICE_PER_CONTROLLER]>(lv_buf, MEM_ACLIST_BACKUP_OFFSET))
      {
        LOGN(LOGTAG_MSG, "write ac backup nodelist success!");
        ret = TRUE;
        break;
      }
      else
      {
        LOGW(LOGTAG_MSG,"write ac backup nodelist failed! tried=%d",attemps);
      }
    }
  }
  return ret;
}

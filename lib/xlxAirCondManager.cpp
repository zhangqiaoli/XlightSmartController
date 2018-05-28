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

bool xlAirCondManager::UpdateACByNodeid(uint8_t nodeid,uint16_t ec,bool bOnlyec/*=FALSE*/,uint16_t eq/* = 0*/)
{
  ACDev_t acNode;
  memset(&acNode,0x00,sizeof(ACDev_t));
  acNode.nid = nodeid;
  uint32_t now = Time.now();
  if(m_lstACDev.get(&acNode) >=0)
  { // old node,need update
    acNode.m_nElecCurrent = ec;
    acNode.m_nSSLastMsgTime = now;
    uint32_t nLastDay = acNode.m_nEndDay;
    acNode.m_nEndDay = Time.format(now, "%Y%m%d").toInt();
    Serial.printlnf("now is %d",acNode.m_nEndDay);
    ////////////////////update eq////////////////////
    if(acNode.m_nEndDay > nLastDay)
    { // new day,update m_arrHistoryEQ
      Serial.printlnf("Update day,now is %d",acNode.m_nEndDay);
      for(int i=HISTORYEQDAYS-1;i>=0;i++)
      {
        if(i>0)
        {
          acNode.m_arrHistoryEQ[i] = acNode.m_arrHistoryEQ[i-1];
        }
        else
        {
          acNode.m_arrHistoryEQ[0] = 0;
          acNode.m_arrHistoryEQ[0] += eq;
        }
      }
    }
    else
    {
      acNode.m_arrHistoryEQ[0] += eq;
    }
    ////////////////////update eq end////////////////
  }
  else
  {// not found,need add
    acNode.m_nElecCurrent = ec;
    acNode.m_nSSLastMsgTime = now;
    acNode.m_nCMLastMsgTime = now;
    acNode.m_nEndDay = Time.format(now, "%Y%m%d").toInt();
  }
  m_lstACDev.add(&acNode);
  m_isChanged = TRUE;
  if(bOnlyec)
  {
    PublishACEC(acNode);
  }
  else
  {
    PublishACInfo(acNode);
  }
  return TRUE;
}


bool xlAirCondManager::UpdateACStatusByNodeid(uint8_t nodeid,uint8_t onoff,uint8_t mode,uint8_t temp,uint8_t fanlevel)
{
  ACDev_t acNode;
  memset(&acNode,0x00,sizeof(ACDev_t));
  acNode.nid = nodeid;
  uint32_t now = Time.now();
  if(m_lstACDev.get(&acNode) >=0)
  { // old node,need update
    acNode.m_nCMLastMsgTime = now;
    acNode.m_sLastOp.onoff = onoff;
    acNode.m_sLastOp.mode = mode;
    acNode.m_sLastOp.temp = temp;
    acNode.m_sLastOp.fanlevel = fanlevel;
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
  }
  m_lstACDev.add(&acNode);
  Serial.printlnf("UpdateACStatusByNodeid nodeid:%d,sid:%d,ec:%d,ss:%d,mc:%d,onoff:%d,mode:%d,temp:%d,fan:%d,endday:%d",acNode.nid,\
               acNode.sid,acNode.m_nElecCurrent,acNode.m_nSSLastMsgTime,acNode.m_nCMLastMsgTime,\
               acNode.m_sLastOp.onoff,acNode.m_sLastOp.mode,acNode.m_sLastOp.temp,acNode.m_sLastOp.fanlevel,\
               acNode.m_nEndDay);
  m_isChanged = TRUE;
  PublishACStatus(acNode);
  return TRUE;
}

bool xlAirCondManager::UpdateACStatusByNodeid(uint8_t nodeid,AirConditioningStauts_t& acStatus)
{
  ACDev_t acNode;
  memset(&acNode,0x00,sizeof(ACDev_t));
  acNode.nid = nodeid;
  uint32_t now = Time.now();
  if(m_lstACDev.get(&acNode) >=0)
  { // old node,need update
    acNode.m_nCMLastMsgTime = now;
    acNode.m_sLastOp = acStatus;
  }
  else
  {// not found,need add
    acNode.m_nSSLastMsgTime = now;
    acNode.m_nCMLastMsgTime = now;
    acNode.m_nEndDay = Time.format(now, "%Y%m%d").toInt();
    acNode.m_sLastOp = acStatus;
  }
  m_lstACDev.add(&acNode);
  m_isChanged = TRUE;
  PublishACStatus(acNode);
  return TRUE;
}

bool xlAirCondManager::PublishACEC(ACDev_t& acNode)
{
  String strTemp = String::format("{'nd':%d,'ec':%d}",acNode.nid,acNode.m_nElecCurrent);
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
  String strformat = "{'nd':%d,'ec':%d,'dt':[%d,%d,%d,%d],'days':%d,";
  uint8_t days = 1;
  if(bAllDays)
  {
    days = HISTORYEQDAYS;
  }
  strformat += "'eq':[";
  for(uint8_t i=0;i<days;i++)
  {
    strformat += String(acNode.m_arrHistoryEQ[i]);
    if(i != HISTORYEQDAYS-1)
    {
      strformat += ",";
    }
    else
    {
      strformat += "]}";
    }
  }
  strTemp = String::format(strformat,acNode.nid,acNode.m_nElecCurrent,acNode.m_sLastOp.onoff,\
                           acNode.m_sLastOp.mode,acNode.m_sLastOp.temp,acNode.m_sLastOp.fanlevel,days);
	theSys.PublishACDeviceStatus(strTemp.c_str());
	return TRUE;
}

void xlAirCondManager::ProcessCheck()
{
  uint32_t now  = Time.now();
  uint32_t nLastDay = Time.format(now, "%Y%m%d").toInt();
  for(uint8_t i = 0;i<m_lstACDev.count();i++)
  {
    Serial.printf("nodeid:%d,sid:%d,ec:%d,ss:%d,mc:%d,onoff:%d,mode:%d,temp:%d,fan:%d,endday:%d",m_lstACDev._pItems[i].nid,\
                 m_lstACDev._pItems[i].sid,m_lstACDev._pItems[i].m_nElecCurrent,m_lstACDev._pItems[i].m_nSSLastMsgTime,m_lstACDev._pItems[i].m_nCMLastMsgTime,\
                 m_lstACDev._pItems[i].m_sLastOp.onoff,m_lstACDev._pItems[i].m_sLastOp.mode,m_lstACDev._pItems[i].m_sLastOp.temp,m_lstACDev._pItems[i].m_sLastOp.fanlevel,\
                 m_lstACDev._pItems[i].m_nEndDay);
    if( !IS_NOT_AC_NODEID(m_lstACDev._pItems[i].nid) && now - m_lstACDev._pItems[i].m_nSSLastMsgTime < 3600*HISTORYEQDAYS)
    {
      // TODO timeout check
      //  day eq array check
      bool bNeedpublish = FALSE;
      if(nLastDay > m_lstACDev._pItems[i].m_nEndDay)
      { // day eq array need update
        for(int i=HISTORYEQDAYS-1;i>=0;i++)
        {
          if(i>0)
          {
            if(m_lstACDev._pItems[i].m_arrHistoryEQ[i] != 0) bNeedpublish = TRUE;
            m_lstACDev._pItems[i].m_arrHistoryEQ[i] = m_lstACDev._pItems[i].m_arrHistoryEQ[i-1];
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
  SaveACNode();
}


bool xlAirCondManager::LoadACNode()
{
  bool bReadmain = TRUE;
  ACDev_t ACNodeArray[MAX_AC_DEVICE_PER_CONTROLLER];
  if (sizeof(ACDev_t)*MAX_NODE_PER_CONTROLLER <= MEM_ACLIST_LEN)
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
    if (sizeof(ACDev_t)*MAX_NODE_PER_CONTROLLER <= MEM_ACLIST_BACKUP_LEN)
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

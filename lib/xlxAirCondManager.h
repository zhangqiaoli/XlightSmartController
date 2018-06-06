//  xlxAirConditioning.h - Xlight SmartController airconditioning struct
#ifndef xlxAirCondManager_h
#define xlxAirCondManager_h

#include "application.h"
#include "OrderedList.h"

#define HISTORYEQDAYS 3
#define MAX_AC_DEVICE_PER_CONTROLLER   8

#define RTE_DEV_KEEP_ALIVE  60

typedef struct
{
  uint8_t onoff;
  uint8_t mode;
  uint8_t temp;
  uint8_t fanlevel;
} AirConditioningStauts_t;


typedef struct
	__attribute__((packed))
{
  // nodeid
  uint8_t nid;
  // subid
  uint8_t sid;
  // electricity,need divide 100
  int16_t m_nElecCurrent;
  // aircondition last operate status
  AirConditioningStauts_t m_sLastOp;
  // Smart socket last msg time
  uint32_t m_nSSLastMsgTime;
  // control module last msg time
  uint32_t m_nCMLastMsgTime;
  // total eq
  uint64_t m_nTotalEQ;
  uint16_t m_arrHistoryEQ[HISTORYEQDAYS];
  uint32_t m_nEndDay;
  // air smartsocket last eq msg index for distinct
  uint16_t m_lastEQIndex;
}ACDev_t;

// airconditioning device List Class
class ACDevListClass : public OrderdList<ACDev_t>
{
public:

  ACDevListClass(uint8_t maxl = 8, bool desc = false, uint8_t initlen = 4) : OrderdList(maxl, desc, initlen) {};
  virtual int compare(ACDev_t _first, ACDev_t _second) {
    if( _first.nid > _second.nid ) {
      return 1;
    } else if( _first.nid < _second.nid ) {
      return -1;
    } else {
      return 0;
    }
  };
};

class xlAirCondManager
{
private:
  ACDevListClass m_lstACDev;
  bool m_isChanged;
public:
  xlAirCondManager();
  ~xlAirCondManager();

  // nodeid search functions
  //ListNode<xlAirConditioning> *SearchAC(UC nodeid);
  bool UpdateACByNodeid(uint8_t nodeid,uint16_t ec,bool bOnlyec=FALSE,uint16_t eq = 0,uint16_t eqindex = 0);

  bool UpdateACStatusByNodeid(uint8_t nodeid,AirConditioningStauts_t& acStatus);
  bool UpdateACStatusByNodeid(uint8_t nodeid,uint8_t onoff,uint8_t mode,uint8_t temp,uint8_t fanlevel);
  bool PublishACInfo(ACDev_t& acNode,bool bAllDays=FALSE);
  bool PublishACEC(ACDev_t& acNode);
  bool PublishACStatus(ACDev_t& acNode);
  bool PublishHistoryInfo();
  bool PublishTimeoutAlarm(uint8_t *arrNodeid,uint8_t size,uint8_t type);

  void ProcessCheck();

public:
  bool LoadACNode();
  bool SaveACNode();

};

extern xlAirCondManager theACManager;

#endif /* xlxAirCondManager_h */

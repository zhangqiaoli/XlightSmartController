//  xlSmartController.h - Xlight SmartController project scale definitions header

#ifndef xlSmartController_h
#define xlSmartController_h

#include "xliCommon.h"
#include "xlxCloudObj.h"
#include "xlxInterfaceRF24.h"
#include "xlxConfig.h"
#include "xlxChain.h"
#include "xliMemoryMap.h"

//------------------------------------------------------------------
// Xlight Command Queue Structures
//------------------------------------------------------------------

//ToDo: Create command queue


class SmartControllerClass;           // forward reference

//------------------------------------------------------------------
// Smart Controller Class
//------------------------------------------------------------------
class SmartControllerClass : public CloudObjClass
{
private:
  BOOL m_isRF;
  BOOL m_isBLE;
  BOOL m_isLAN;
  BOOL m_isWAN;

public:
  SmartControllerClass();
  void Init();
  void InitRadio();
  void InitNetwork();
  void InitPins();
  void InitSensors();
  void InitCloudObj();

  BOOL Start();
  String GetSysID();
  UC GetStatus();
  void SetStatus(UC st);

  BOOL CheckRF();
  BOOL CheckNetwork();
  BOOL CheckBLE();
  BOOL SelfCheck(UL ms);
  BOOL IsRFGood();
  BOOL IsBLEGood();
  BOOL IsLANGood();
  BOOL IsWANGood();

  // Process all kinds of commands
  void ProcessCommands();
  void CollectData(UC tick);

  // Device Control Functions
  int DevSoftSwitch(BOOL sw, UC dev = 0);
  int DevChangeColor();					//ToDo: Params

  // High speed system timer process
  void FastProcess();

  // Cloud interface implementation
  int CldSetTimeZone(String tzStr);
  int CldPowerSwitch(String swStr);
  int CldJSONCommand(String jsonData);

  // Parsing Functions
  bool ParseRows(JsonObject& data, int index);

  // Cloud Interface Action Types
  bool Change_Rule(RuleRow_t row);
  bool Change_Schedule(ScheduleRow_t row);

  bool Change_Scenario(ScenarioRow_t row);				//ToDo: Params
  bool Change_DeviceStatus(DevStatus_t row);				//ToDo: Params
  bool Change_Sensor();					//ToDo: Params

  // Action loop to create alarms from rules
  void ReadNewRules(UL ms);

  //Creation of alarms
  bool CreateAlarm(ScheduleRow_t row);

  //Alarm Triggered Actions
  void AlarmTimerTriggered();

  //LinkedLists (Working memory tables)
  DevStatus_t DevStatus_row;
  ChainClass<ScheduleRow_t> Schedule_table = ChainClass<ScheduleRow_t>(PRE_FLASH_MAX_TABLE_SIZE);
  ChainClass<ScenarioRow_t> Scenario_table = ChainClass<ScenarioRow_t>(PRE_FLASH_MAX_TABLE_SIZE);
  ChainClass<RuleRow_t> Rule_table = ChainClass<RuleRow_t>((int)( MEM_RULES_LEN / sizeof(RuleRow_t) )); // 65536/5 = 13107

protected:
  // Communication Interfaces
  RF24InterfaceClass m_cmRF24;
};

//------------------------------------------------------------------
// Function & Class Helper
//------------------------------------------------------------------
extern SmartControllerClass theSys;

#endif /* xlSmartController_h */

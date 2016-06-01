//  xlSmartController.h - Xlight SmartController project scale definitions header

#ifndef xlSmartController_h
#define xlSmartController_h

#include "xliCommon.h"
#include "xlxCloudObj.h"
#include "xlxInterfaceRF24.h"

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

  // High speed system timer process
  void FastProcess();

  // Cloud interface implementation
  int CldSetTimeZone(String tzStr);
  int CldPowerSwitch(String swStr);
  int CldJSONCommand(String jsonData);

  void AlarmTimerTriggered(int SCTindex);

protected:
  // Communication Interfaces
  RF24InterfaceClass m_cmRF24;
};

//------------------------------------------------------------------
// Function & Class Helper
//------------------------------------------------------------------
extern SmartControllerClass theSys;

#endif /* xlSmartController_h */

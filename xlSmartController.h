//  xlSmartController.h - Xlight SmartController project scale definitions header

#ifndef xlSmartController_h
#define xlSmartController_h

#include "application.h"
#include "xliCommon.h"

class SmartControllerClass;           // forward reference

//------------------------------------------------------------------
// Smart Controller Class
//------------------------------------------------------------------
class SmartControllerClass
{
private:
  UC m_Status;
  String m_SysID;
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

  BOOL Start();
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
};

//------------------------------------------------------------------
// Function & Class Helper
//------------------------------------------------------------------
extern SmartControllerClass theSys;

#endif /* xlSmartController_h */

//  xliCloudObj.h - Xlight cloud variables and functions definitions header

#ifndef xliCloudObj_h
#define xliCloudObj_h

#include "xliCommon.h"
#include "ArduinoJson.h"

// Comment it off if we don't use Particle public cloud
/// Notes:
/// Currently, Particle public cloud only supports 10 variables and 4 functions.
/// Therefore, they are better used for debugging and monitoring.
/// In general situation, we should use publish/subscribe or MQTT.
#define USE_PARTICLE_CLOUD

// Notes: Variable name max length is 12 characters long.
// Cloud variables
#define CLV_SysID               "sysID"           // Can also be a Particle Object
#define CLV_TimeZone            "timeZone"        // Can also be a Particle Object
#define CLV_DevStatus           "devStatus"       // Can also be a Particle Object
#define CLV_JSONData            "jsonData"        // Can also be a Particle Object
#define CLV_LastMessage         "lastMsg"         // Can also be a Particle Object
#define CLV_SenTemperatur       "senTemp"
#define CLV_SenHumidity         "senHumid"
#define CLV_SenBrightness       "senBright"

// Notes: The length of the funcKey is limited to a max of 12 characters.
// Cloud functions
#define CLF_SetTimeZone         "SetTimeZone"     // Can also be a Particle Object
#define CLF_PowerSwitch         "PowerSwitch"     // Can also be a Particle Object
#define CLF_JSONCommand         "JSONCommand"     // Can also be a Particle Object
#define CLF_SetHue              "SetHue"
#define CLF_SetCurTime          "SetCurTime"


//------------------------------------------------------------------
// Xlight CloudObj Class
//------------------------------------------------------------------
class CloudObjClass
{
public:
  // Variables should be defined as public
  String m_SysID;
  int m_devStatus;
  String m_tzString;
  String m_jsonData;
  String m_lastMsg;

  float m_temperature;
  float m_humidity;
  uint16_t m_brightness;

public:
  CloudObjClass();

  virtual int CldSetTimeZone(String tzStr) = 0;
  virtual int CldPowerSwitch(String swStr) = 0;
  virtual int CldJSONCommand(String jsonData) = 0;

  BOOL UpdateTemperature(float value);
  BOOL UpdateHumidity(float value);
  BOOL UpdateBrigntness(uint16_t value);
  void UpdateJSONData();

protected:
  void InitCloudObj();

  StaticJsonBuffer<SENSORDATA_JSON_SIZE> m_jBuf;
  JsonObject *m_jpRoot;
  JsonObject *m_jpData;
};

#endif /* xliCloudObj_h */

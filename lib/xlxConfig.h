//  xlxConfig.h - Xlight Configuration Reader & Writer

#ifndef xlxConfig_h
#define xlxConfig_h

#include "xliCommon.h"
#include "xliMemoryMap.h"
#include "TimeAlarms.h"

// Change it only if Config_t structure is updated
#define VERSION_CONFIG_DATA   1

// Xlight Application Identification
#define XLA_ORGANIZATION          "xlight.ca"               // Default value. Read from EEPROM
#define XLA_PRODUCT_NAME          "XController"             // Default value. Read from EEPROM
#define XLA_AUTHORIZATION         "use-token-auth"
#define XLA_TOKEN                 "your-access-token"       // Can update online

//Row State Flags for Sync between Cloud, Flash, and Working Memory
enum OP_FLAG {DELETE, POST, PUT, GET};
enum FLASH_FLAG {UNSAVED, SAVED};
enum RUN_FLAG {UNEXECUTED, EXECUTED};

//------------------------------------------------------------------
// Xlight Configuration Data Structures
//------------------------------------------------------------------
typedef struct
{
  US id;                                    // timezone id
  SHORT offset;                             // offser in minutes
  UC dst                      :1;           // daylight saving time flag
} Timezone_t;

typedef struct
{
  UC State                    :4;           // Component state
  UC CW                       :8;           // Brightness of cold white
  UC WW                       :8;           // Brightness of warm white
  UC R                        :8;           // Brightness of red
  UC G                        :8;           // Brightness of green
  UC B                        :8;           // Brightness of blue
} Hue_t;

typedef struct
{
  UC version                  :4;           // Data version, other than 0xFF
  US sensorBitmap             :16;          // Sensor enable bitmap
  UC indBrightness            :4;           // Indicator of brightness
  UC typeMainDevice           :8;           // Type of the main lamp
  UC numDevices               :8;           // Number of devices
  Timezone_t timeZone;                      // Time zone
  char Organization[24];                    // Organization name
  char ProductName[24];                     // Product name
  char Token[64];                           // Token
} Config_t;

//------------------------------------------------------------------
// Xlight Device Status Table Structures
//------------------------------------------------------------------
typedef struct //max 64 bytes
{
  OP_FLAG op_flag : 2;
  FLASH_FLAG flash_flag : 1;
  RUN_FLAG run_flag : 1;
  UC id;                                    // ID, 1 based
  UC type;                                  // Type of lamp
  Hue_t ring1;
  Hue_t ring2;
  Hue_t ring3;
} DevStatus_t;

//------------------------------------------------------------------
// Xlight Schedule Table Structures
//------------------------------------------------------------------

typedef struct //Schedule Table
{
  OP_FLAG op_flag			: 2;
  FLASH_FLAG flash_flag		: 1;
  RUN_FLAG run_flag			: 1;
	UC uid				          : 8;
	UC weekdays			        : 7;	  //values: 1-7
	BOOL isRepeat		        : 1;	  //values: 0-1
	UC hour				          : 5;    //values: 0-23
	UC min				          : 6;    //values: 0-59
	AlarmId alarm_id	      : 8;
} ScheduleRow_t;

#define SCT_ROW_SIZE	sizeof(ScheduleRow_t)
#define MAX_SCT_ROWS	(int)(MEM_SCHEDULE_LEN/sizeof(SCT_ROW_SIZE))

//------------------------------------------------------------------
// Xlight Rule Table Structures
//------------------------------------------------------------------

typedef struct
{
	OP_FLAG op_flag : 2;
	FLASH_FLAG flash_flag : 1;
	RUN_FLAG run_flag : 1;
	UC uid                   : 8;
	UC SCT_uid               : 8;
	UC alarm_id              : 8;
	UC SNT_uid               : 8;
	UC notif_uid             : 8;
} RuleRow_t;

#define RT_ROW_SIZE 	sizeof(RuleRow_t)
#define MAX_RT_ROWS 	(int)(MEM_RULES_LEN/sizeof(RT_ROW_SIZE))

//------------------------------------------------------------------
// Xlight Scenerio Table Structures
//------------------------------------------------------------------

typedef struct
{
	OP_FLAG op_flag : 2;
	FLASH_FLAG flash_flag : 1;
	RUN_FLAG run_flag : 1;
	UC uid			            : 8;
	Hue_t ring1;
	Hue_t ring2;
	Hue_t ring3;
	UC filter		            : 8;
} ScenarioRow_t;

#define SNT_ROW_SIZE	sizeof(ScenarioRow_t)
#define MAX_SNT_ROWS	(int)(MEM_SCENARIOS_LEN/sizeof(SNT_ROW_SIZE))

//------------------------------------------------------------------
// Xlight Configuration Class
//------------------------------------------------------------------
class ConfigClass
{
private:
  BOOL m_isLoaded;
  BOOL m_isChanged;         // Config Change Flag
  BOOL m_isDSTChanged;      // Device Status Table Change Flag
  BOOL m_isSCTChanged;      // Schedule Table Change Flag
  BOOL m_isRTChanged;		    // Rules Table Change Flag
  BOOL m_isSNTChanged;	 	  // Scenerio Table Change Flag

  Config_t m_config;

public:
  ConfigClass();
  void InitConfig();

  BOOL LoadConfig();
  BOOL SaveConfig();
  BOOL IsConfigLoaded();

  BOOL IsConfigChanged();
  void SetConfigChanged(BOOL flag);

  BOOL IsDSTChanged();
  void SetDSTChanged(BOOL flag);

  BOOL IsSCTChanged();
  void SetSCTChanged(BOOL flag);

  BOOL IsRTChanged();
  void SetRTChanged(BOOL flag);

  BOOL IsSNTChanged();
  void SetSNTChanged(BOOL flag);

  UC GetVersion();
  BOOL SetVersion(UC ver);

  US GetTimeZoneID();
  BOOL SetTimeZoneID(US tz);

  UC GetDaylightSaving();
  BOOL SetDaylightSaving(UC flag);

  SHORT GetTimeZoneOffset();
  SHORT GetTimeZoneDSTOffset();
  BOOL SetTimeZoneOffset(SHORT offset);
  String GetTimeZoneJSON();

  String GetOrganization();
  void SetOrganization(const char *strName);

  String GetProductName();
  void SetProductName(const char *strName);

  String GetToken();
  void SetToken(const char *strName);

  BOOL IsSensorEnabled(sensors_t sr);
  void SetSensorEnabled(sensors_t sr, BOOL sw = true);

  UC GetBrightIndicator();
  BOOL SetBrightIndicator(UC level);

  UC GetMainDeviceType();
  BOOL SetMainDeviceType(UC type);

  UC GetNumDevices();
  BOOL SetNumDevices(UC num);
};

//------------------------------------------------------------------
// Function & Class Helper
//------------------------------------------------------------------
extern ConfigClass theConfig;

#endif /* xlxConfig_h */

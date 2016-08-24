//  xlxConfig.h - Xlight Configuration Reader & Writer

#ifndef xlxConfig_h
#define xlxConfig_h

#include "xliCommon.h"
#include "xliMemoryMap.h"
#include "TimeAlarms.h"
#include "OrderedList.h"
#include "flashee-eeprom.h"

/*Note: if any of these structures are modified, the following print functions may need updating:
 - ConfigClass::print_config()
 - SmartControllerClass::print_devStatus_table()
 - SmartControllerClass::print_schedule_table()
 - SmartControllerClass::print_scenario_table()
 - SmartControllerClass::print_rule_table()
*/

#define PACK //MSVS intellisense doesn't work when structs are packed
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
#ifdef PACK
	__attribute__((packed))
#endif
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
  UC version                  :8;           // Data version, other than 0xFF
  US sensorBitmap             :16;          // Sensor enable bitmap
  UC indBrightness            :4;           // Indicator of brightness
  UC typeMainDevice           :8;           // Type of the main lamp
  UC numDevices               :8;           // Number of devices
  Timezone_t timeZone;                      // Time zone
  char Organization[24];                    // Organization name
  char ProductName[24];                     // Product name
  char Token[64];                           // Token
  BOOL enableCloudSerialCmd;                // Whether enable cloud serial command
  UC numNodes;                              // Number of Nodes (include device, remote control, etc.)
} Config_t;

//------------------------------------------------------------------
// Xlight Device Status Table Structures
//------------------------------------------------------------------
typedef struct
#ifdef PACK
	__attribute__((packed))
#endif
{
  OP_FLAG op_flag : 2;
  FLASH_FLAG flash_flag : 1;
  RUN_FLAG run_flag : 1;
  UC uid;						   // required
  UC node_id;                      // Node ID (for RF communication), 1 based
  UC type;                         // Type of lamp
  Hue_t ring1;
  Hue_t ring2;
  Hue_t ring3;
} DevStatusRow_t;

#define DST_ROW_SIZE sizeof(DevStatusRow_t)

//------------------------------------------------------------------
// Xlight Schedule Table Structures
//------------------------------------------------------------------
typedef struct
#ifdef PACK
	__attribute__((packed))
#endif
{
  OP_FLAG op_flag			: 2;
  FLASH_FLAG flash_flag		: 1;
  RUN_FLAG run_flag			: 1;
	UC uid			        : 8;
	UC weekdays			    : 7;	  //values: 0-7
	BOOL isRepeat		    : 1;	  //values: 0-1
	UC hour				    : 5;    //values: 0-23
	UC minute				: 6;    //values: 0-59
	AlarmId alarm_id	    : 8;
} ScheduleRow_t;

#define SCT_ROW_SIZE	sizeof(ScheduleRow_t)
#define MAX_SCT_ROWS	(int)(MEM_SCHEDULE_LEN / SCT_ROW_SIZE)

//------------------------------------------------------------------
// Xlight NodeID List
//------------------------------------------------------------------
typedef struct    // Exact 12 bytes
	__attribute__((packed))
{
	UC nid;
	UC reserved;
  UC identify[6];
  UL recentActive;
} NodeIdRow_t;

//------------------------------------------------------------------
// Xlight Rule Table Structures
//------------------------------------------------------------------

typedef struct
#ifdef PACK
	__attribute__((packed))
#endif
{
	OP_FLAG op_flag			 : 2;
	FLASH_FLAG flash_flag	 : 1;
	RUN_FLAG run_flag		 : 1;
	UC uid                   : 8;
	UC node_id				 : 8;
	UC SCT_uid               : 8;
	UC SNT_uid               : 8;
	UC notif_uid             : 8;
  // ToDo: add other trigger conditions, e.g. sensor data
} RuleRow_t;

#define RT_ROW_SIZE 	sizeof(RuleRow_t)
#define MAX_RT_ROWS		128

//------------------------------------------------------------------
// Xlight Scenerio Table Structures
//------------------------------------------------------------------

typedef struct
#ifdef PACK
	__attribute__((packed))
#endif
{
	OP_FLAG op_flag				: 2;
	FLASH_FLAG flash_flag		: 1;
	RUN_FLAG run_flag			: 1;
	UC uid			            : 8;
	Hue_t ring1;
	Hue_t ring2;
	Hue_t ring3;
	UC filter		            : 8;
} ScenarioRow_t;

#define SNT_ROW_SIZE	sizeof(ScenarioRow_t)
#define MAX_SNT_ROWS	128

// Node List Class
class NodeListClass : public OrderdList<NodeIdRow_t>
{
public:
  bool m_isChanged;

  NodeListClass(uint8_t maxl = 64, bool desc = false, uint8_t initlen = 8) : OrderdList(maxl, desc, initlen) {
    m_isChanged = false; };
  virtual int compare(NodeIdRow_t _first, NodeIdRow_t _second) {
    if( _first.nid > _second.nid ) {
      return 1;
    } else if( _first.nid < _second.nid ) {
      return -1;
    } else {
      return 0;
    }
  };
  int getMemSize();
  int getFlashSize();
  bool loadList();
  bool saveList();
  void showList();
  UC requestNodeID(char type, UC identify[6]);
};

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
  BOOL m_isNIDChanged;	 	  // Node ID List Change Flag

  Config_t m_config;
  Flashee::FlashDevice* P1Flash;

public:
  ConfigClass();
  void InitConfig();
  void InitDevStatus();

  // write to P1 using spark-flashee-eeprom
  BOOL MemWriteScenarioRow(ScenarioRow_t row, uint32_t address);
  BOOL MemReadScenarioRow(ScenarioRow_t &row, uint32_t address);

  BOOL LoadConfig();
  BOOL SaveConfig();
  BOOL IsConfigLoaded();

  BOOL LoadDeviceStatus();
  BOOL SaveDeviceStatus();

  BOOL SaveScheduleTable();
  BOOL SaveScenarioTable();

  BOOL LoadRuleTable();
  BOOL SaveRuleTable();

  BOOL LoadNodeIDList();
  BOOL SaveNodeIDList();

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

  BOOL IsNIDChanged();
  void SetNIDChanged(BOOL flag);

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

  BOOL IsCloudSerialEnabled();
  void SetCloudSerialEnabled(BOOL sw = true);

  BOOL IsSensorEnabled(sensors_t sr);
  void SetSensorEnabled(sensors_t sr, BOOL sw = true);

  UC GetBrightIndicator();
  BOOL SetBrightIndicator(UC level);

  UC GetMainDeviceType();
  BOOL SetMainDeviceType(UC type);

  UC GetNumDevices();
  BOOL SetNumDevices(UC num);

  UC GetNumNodes();
  BOOL SetNumNodes(UC num);

  NodeListClass lstNodes;
};

//------------------------------------------------------------------
// Function & Class Helper
//------------------------------------------------------------------
extern ConfigClass theConfig;

#endif /* xlxConfig_h */

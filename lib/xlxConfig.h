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

#define CURRENT_DEVICE              (theConfig.GetMainDeviceID())
#define CURRENT_SUBDEVICE           (theConfig.GetSubDeviceID())
#define IS_CURRENT_DEVICE(nid)      ((nid) == CURRENT_DEVICE || CURRENT_DEVICE == NODEID_DUMMY)

// Maximum items in AST scenario table
#define MAX_ASR_SNT_ITEMS           16

// Maximum items in Hardware Key Map
#define MAX_KEY_MAP_ITEMS           4
#define MAX_NUM_BUTTONS             4
#define MAX_BTN_OP_TYPE             2

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
  UC nid;                                   // NodeID
  UC subID;                                 // SubID
} HardKeyMap_t;

typedef struct
{
  UC action;                                // Type of action
  UC keyMap;                                // Button Key Map: 8 bits for each button, one bit corresponds to one relay key
} Button_Action_t;

typedef struct
#ifdef PACK
	__attribute__((packed))
#endif
{
  UC State                    :1;           // Component state
  UC BR                       :7;           // Brightness of white [0..100]
  US CCT                      :16;          // CCT (warm or cold) [2700..6500]
  UC R                        :8;           // Brightness of red
  UC G                        :8;           // Brightness of green
  UC B                        :8;           // Brightness of blue
  UC L1                       :8;           // Length of thread 1
  UC L2                       :8;           // Length of thread 2
  UC L3                       :8;           // Length of thread 3
} Hue_t;

#if VERSION_CONFIG_DATA > 24
typedef struct
{
  // Static & status parameters
  UC version                  :8;           // Data version, other than 0xFF
  UC indBrightness            :8;           // Indicator of brightness
  UC relay_key_value;
  char Organization[20];                    // Organization name
  char ProductName[20];                     // Product name

  // Configurable parameters
  UC mainDevID;                             // NodeID for main device
  UC subDevID;                              // SubID for main device
  UC typeMainDevice           :8;           // Type of the main lamp
  UC rfChannel;                             // RF Channel: [0..127]
  UC rfPowerLevel             :2;           // RF Power Level 0..3
  UC rfDataRate               :2;           // RF Data Rate [0..2], 0 for 1Mbps, or 1 for 2Mbps, 2 for 250kbs
  BOOL enableCloudSerialCmd   :1;           // Whether enable cloud serial command
  BOOL enableDailyTimeSync    :1;           // Whether enable daily time synchronization
  BOOL enableSpeaker          :1;           // Whether enable speaker
  BOOL fixedNID               :1;           // Whether fixed Node ID
  UC bcMsgRtpTimes            :4;           // Broadcast message repeat times
  UC ndMsgRtpTimes            :4;           // Node message repeat times
  BOOL stWiFi                 :1;           // Wi-Fi status: On / Off
  BOOL enHWSwitch             :1;           // Whether use Hardware Switch as default
  UC hwsObj                   :3;           // Hardware Switch Object
  UC useCloud                 :2;           // How to depend on the Cloud
  BOOL disableWiFi            :1;           // Disable Wi-Fi Chip
  US sensorBitmap             :16;          // Sensor enable bitmap
  UC numDevices               :8;           // Number of devices
  UC numNodes;                              // Number of Nodes (include device, remote control, etc.)
  Timezone_t timeZone;                      // Time zone
  US maxBaseNetworkDuration;
  UC tmLoopKC;                              // Loop Keycode timeout
  BOOL disableLamp            :1;           // if disable lamp
  UC reserved                 :7;           // reserved
  UC Reserved_UC1[1];
  char bleName[24];
  char blePin[6];
  char pptAccessCode[8];
  UC asrSNT[MAX_ASR_SNT_ITEMS];
  HardKeyMap_t keyMap[MAX_KEY_MAP_ITEMS];
  Button_Action_t btnAction[MAX_NUM_BUTTONS][MAX_BTN_OP_TYPE];  // 0: press, 1: long press
} Config_t;

#else
typedef struct
{
  UC version                  :8;           // Data version, other than 0xFF
  US sensorBitmap             :16;          // Sensor enable bitmap
  UC indBrightness            :8;           // Indicator of brightness
  UC typeMainDevice           :8;           // Type of the main lamp
  UC numDevices               :8;           // Number of devices
  Timezone_t timeZone;                      // Time zone
  char Organization[20];                    // Organization name
  UC bcMsgRtpTimes            :4;           // Broadcast message repeat times
  UC ndMsgRtpTimes            :4;           // Node message repeat times
  UC tmLoopKC;                              // Loop Keycode timeout
  UC rfChannel;                             // RF Channel: [0..127]
  UC Reserved_UC1;
  char ProductName[20];                     // Product name
  UC subDevID;                              // SubID for main device
  UC relay_key_value;
  UC Reserved_UC2[2];
  char Token[64];                           // Token
  BOOL enableCloudSerialCmd   :1;           // Whether enable cloud serial command
  BOOL enableDailyTimeSync    :1;           // Whether enable daily time synchronization
  BOOL enableSpeaker          :1;           // Whether enable speaker
  BOOL fixedNID               :1;           // Whether fixed Node ID
  UC rfDataRate               :2;           // RF Data Rate [0..2], 0 for 1Mbps, or 1 for 2Mbps, 2 for 250kbs
  BOOL disableWiFi            :1;           // Disable Wi-Fi Chip
  BOOL Reserved_bool          :1;           // Reserved for boolean flags
  UC numNodes;                              // Number of Nodes (include device, remote control, etc.)
  UC rfPowerLevel             :2;           // RF Power Level 0..3
  BOOL stWiFi                 :1;           // Wi-Fi status: On / Off
  BOOL enHWSwitch             :1;           // Whether use Hardware Switch as default
  UC hwsObj                   :3;           // Hardware Switch Object
  UC Reserved1                :1;           // Reserved bits
  US maxBaseNetworkDuration;
  UC useCloud;                              // How to depend on the Cloud
  UC mainDevID;                             // NodeID for main device
  char bleName[24];
  char blePin[6];
  char pptAccessCode[8];
  UC asrSNT[MAX_ASR_SNT_ITEMS];
  HardKeyMap_t keyMap[MAX_KEY_MAP_ITEMS];
  Button_Action_t btnAction[MAX_NUM_BUTTONS][MAX_BTN_OP_TYPE];  // 0: press, 1: long press
} Config_t;

#endif

//------------------------------------------------------------------
// Xlight Device Status Table Structures
//------------------------------------------------------------------
typedef struct
#ifdef PACK
	__attribute__((packed))
#endif
{
  OP_FLAG op_flag         : 2;
  FLASH_FLAG flash_flag   : 1;
  RUN_FLAG run_flag       : 1;
  UC uid;						   // required
  UC node_id;          // RF nodeID
  UC present              :1;  // 0 - not present; 1 - present
  UC reserved             :3;
  UC filter               :4;
  UC type;                         // Type of lamp
  US token;
  Hue_t ring[MAX_RING_NUM];
} DevStatusRow_t;

#define DST_ROW_SIZE sizeof(DevStatusRow_t)

typedef struct
#ifdef PACK
	__attribute__((packed))
#endif
{
  UC node_id;                       // RF nodeID
  UC present              :1;       // 0 - not present; 1 - present
  UC reserved             :7;
  UC type;                         // Type of remote
  US token;
} RemoteStatus_t;

//------------------------------------------------------------------
// Xlight Schedule Table Structures
//------------------------------------------------------------------
typedef struct
#ifdef PACK
	__attribute__((packed))
#endif
{
  OP_FLAG op_flag			    : 2;
  FLASH_FLAG flash_flag		: 1;
  RUN_FLAG run_flag			  : 1;
	UC uid			            : 8;
	UC weekdays			        : 7;	  //values: 0-7
	BOOL isRepeat		        : 1;	  //values: 0-1
	UC hour				          : 5;    //values: 0-23
	UC minute				        : 6;    //values: 0-59
	AlarmId alarm_id	      : 8;
} ScheduleRow_t;

#define SCT_ROW_SIZE	sizeof(ScheduleRow_t)
#define MAX_SCT_ROWS	(int)(MEM_SCHEDULE_LEN / SCT_ROW_SIZE)

//------------------------------------------------------------------
// Xlight NodeID List
//------------------------------------------------------------------
#define LEN_NODE_IDENTITY     8
typedef struct    // Exact 16 bytes
	__attribute__((packed))
{
	UC nid;
	UC device;       // Associated device (node id)
  UC identity[LEN_NODE_IDENTITY];
  UL recentActive;
  UC sid;         // SubID
  UC cfgPos;      // Config storage position
} NodeIdRow_t;

inline BOOL isIdentityEmpty(UC *pId, UC nLen = LEN_NODE_IDENTITY)
{
  for( int i = 0; i < nLen; i++ ) { if(pId[i] > 0) return FALSE; }
  return TRUE;
};

inline void copyIdentity(UC *pId, uint64_t *pData)
{
  UC nLen = min(LEN_NODE_IDENTITY, sizeof(uint64_t));
  memcpy(pId, pData, nLen);
};

inline void resetIdentity(UC *pId, UC nLen = LEN_NODE_IDENTITY)
{ memset(pId, 0x00, nLen); };

inline BOOL isIdentityEqual(UC *pId1, UC *pId2, UC nLen = LEN_NODE_IDENTITY)
{
  for( int i = 0; i < nLen; i++ ) { if(pId1[i] != pId2[i]) return false; }
  return true;
};

inline BOOL isIdentityEqual(UC *pId1, uint64_t *pData)
{
  UC nLen = min(LEN_NODE_IDENTITY, sizeof(uint64_t));
  UC pId2[LEN_NODE_IDENTITY]; copyIdentity(pId2, pData);
  return isIdentityEqual(pId1, pId2, nLen);
};

//------------------------------------------------------------------
// Xlight Rule Table Structures
//------------------------------------------------------------------
typedef struct
#ifdef PACK
	__attribute__((packed))
#endif
{
	UC enabled               : 1;    // Whether the condition is enabled
  UC sr_scope              : 3;    // Sensor scope
  UC symbol                : 4;    // Sensor logic symbols
  UC connector             : 2;    // Condition logic symbols
	UC sr_id                 : 4;    // Sensor ID
  US sr_value1;
  US sr_value2;
} Condition_t;

typedef struct
#ifdef PACK
	__attribute__((packed))
#endif
{
	OP_FLAG op_flag			     : 2;
	FLASH_FLAG flash_flag	   : 1;
	RUN_FLAG run_flag		     : 1;
	UC uid                   : 8;
	UC node_id				       : 8;
	UC SCT_uid               : 8;
	UC SNT_uid               : 8;
	UC notif_uid             : 8;
  // Once rule triggered, whether repeatly check
  UC tmr_int               : 1; // Whether enable timer
  UC tmr_started           : 1; // Whether timer started
  US tmr_span;             // Timer span in minutes
  UL tmr_tic_start;        // Timer started tick
  // Other trigger conditions, e.g. sensor data
  Condition_t actCond[MAX_CONDITION_PER_RULE];
} RuleRow_t;

#define RT_ROW_SIZE 	sizeof(RuleRow_t)
#define MAX_RT_ROWS		64

//------------------------------------------------------------------
// Xlight Scenerio Table Structures
//------------------------------------------------------------------

typedef struct
#ifdef PACK
	__attribute__((packed))
#endif
{
	OP_FLAG op_flag				  : 2;
	FLASH_FLAG flash_flag		: 1;
	RUN_FLAG run_flag			  : 1;
	UC uid			            : 8;
  UC sw                   : 2; // Main Switch: Switch value for set power command
	Hue_t ring[MAX_RING_NUM];
	UC filter		            : 4;
  UC reserved             : 4;
} ScenarioRow_t;

#define SNT_ROW_SIZE	sizeof(ScenarioRow_t)
#define MAX_SNT_ROWS	64

//------------------------------------------------------------------
// Xlight Node Config Table Structures
//------------------------------------------------------------------
typedef struct
{
  OP_FLAG op_flag			    : 2;
  FLASH_FLAG flash_flag		: 1;
  RUN_FLAG run_flag			  : 1;
	UC uid			            : 8;
  UL cid;                 // Config ID
  UC type			            : 8;
  UC len			            : 8;
  UC reserved[8];
} NodeConfig_head_t;

typedef struct
{
  NodeConfig_head_t header;
  UC data[240];
} NodeConfig_t;

#define NCT_HEAD_SIZE	    sizeof(NodeConfig_head_t)
#define NCT_ROW_SIZE	    sizeof(NodeConfig_t)
#define MAX_NCT_ROWS	    (int)(MEM_NODECONFIG_LEN / NCT_ROW_SIZE)

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
  void showList(BOOL toCloud = false, UC nid = 0);
  void publishNode(NodeIdRow_t _node);
  UC requestNodeID(UC preferID, char type, uint64_t identity);
  BOOL clearNodeId(UC nodeID);

protected:
  UC getAvailableNodeId(UC preferID, UC defaultID, UC minID, UC maxID, uint64_t identity);
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
  UL m_lastTimeSync;

  Config_t m_config;
  Flashee::FlashDevice* P1Flash;

  void UpdateTimeZone();
  void DoTimeSync();

public:
  ConfigClass();
  void InitConfig();
  BOOL InitDevStatus(UC nodeID);
  Flashee::FlashDevice* getP1Flash()
  {
	  return P1Flash;
  }

  // write to P1 using spark-flashee-eeprom
  BOOL MemWriteScenarioRow(ScenarioRow_t row, uint32_t address);
  BOOL MemReadScenarioRow(ScenarioRow_t &row, uint32_t address);

  BOOL LoadConfig();
  BOOL SaveConfig();
  BOOL IsConfigLoaded();

  BOOL IsValidConfig();
  BOOL LoadBackupConfig();
  BOOL SaveBackupConfig();

  BOOL LoadDeviceStatus();
  BOOL SaveDeviceStatus();

  BOOL SaveScheduleTable();
  BOOL SaveScenarioTable();

  BOOL LoadRuleTable();
  BOOL SaveRuleTable();

  BOOL LoadNodeIDList();
  BOOL SaveNodeIDList();
  BOOL LoadBackupNodeList();

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
  BOOL SetVersion(UC ver,BOOL force=false);

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

#if VERSION_CONFIG_DATA <= 24
  String GetToken();
  void SetToken(const char *strName);
#endif

  String GetBLEName();
  void SetBLEName(const char *strName);

  String GetBLEPin();
  void SetBLEPin(const char *strPin);

  String GetPPTAccessCode();
  void SetPPTAccessCode(const char *strPin);
  BOOL CheckPPTAccessCode(const char *strPin);

  BOOL IsCloudSerialEnabled();
  void SetCloudSerialEnabled(BOOL sw = true);

  BOOL IsSpeakerEnabled();
  void SetSpeakerEnabled(BOOL sw = true);

  BOOL IsFixedNID();
  void SetFixedNID(BOOL sw = true);

  BOOL IsDailyTimeSyncEnabled();
  void SetDailyTimeSyncEnabled(BOOL sw = true);
  BOOL CloudTimeSync(BOOL _force = true);

  US GetSensorBitmap();
  void SetSensorBitmap(US bits);
  BOOL IsSensorEnabled(sensors_t sr);
  void SetSensorEnabled(sensors_t sr, BOOL sw = true);

  UC GetBrightIndicator();
  BOOL SetBrightIndicator(UC level);

  UC GetMainDeviceID();
  BOOL SetMainDeviceID(UC devID);
  UC GetSubDeviceID();
  BOOL SetSubDeviceID(UC devID);

  UC GetMainDeviceType();
  BOOL SetMainDeviceType(UC type);

  UC GetRemoteNodeDevice(UC remoteID);
  BOOL SetRemoteNodeDevice(UC remoteID, US devID);

  UC GetNumDevices();
  BOOL SetNumDevices(UC num);

  UC GetNumNodes();
  BOOL SetNumNodes(UC num);

  US GetMaxBaseNetworkDur();
  BOOL SetMaxBaseNetworkDur(US dur);

  BOOL GetDisableWiFi();
  BOOL SetDisableWiFi(BOOL _st);
  BOOL GetDisableLamp();
  BOOL SetDisableLamp(BOOL _st);

  UC GetUseCloud();
  BOOL SetUseCloud(UC opt);

  BOOL GetWiFiStatus();
  BOOL SetWiFiStatus(BOOL _st);

  BOOL GetHardwareSwitch();
  BOOL SetHardwareSwitch(BOOL _sw);

  UC GetRelayKeyObj();
  BOOL SetRelayKeyObj(UC _value);

  UC GetTimeLoopKC();
  BOOL SetTimeLoopKC(UC _value);

  UC GetRFChannel();
  BOOL SetRFChannel(UC channel);

  UC GetRFDataRate();
  BOOL SetRFDataRate(UC dataRate);

  UC GetRFPowerLevel();
  BOOL SetRFPowerLevel(UC level);

  UC GetBcMsgRptTimes();
  BOOL SetBcMsgRptTimes(UC _times);

  UC GetNdMsgRptTimes();
  BOOL SetNdMsgRptTimes(UC _times);

  UC GetASR_SNT(const UC _code);
  BOOL SetASR_SNT(const UC _code, const UC _snt = 0);
  void showASRSNT();

  UC GetRelayKeys();
  BOOL SetRelayKeys(const UC _keys);
  UC GetRelayKey(const UC _code);
  BOOL SetRelayKey(const UC _code, const UC _on);

  UC GetKeyMapItem(const UC _key, UC *_subID = NULL);
  BOOL SetKeyMapItem(const UC _key, const UC _nid, const UC _subID = 0);
  UC SearchKeyMapItem(const UC _nid, const UC _subID = 0);
  BOOL IsKeyMapItemAvalaible(const UC _code);
  bool IsKeyMatchedItem(const UC _code, const UC _nid, const UC _subID = 0);
  void showKeyMap();
  UC GetKeyOnNum();

  BOOL SetExtBtnAction(const UC _btn, const UC _opt, const UC _act, const UC _keymap);
  BOOL ExecuteBtnAction(const UC _btn, const UC _opt);
  void showButtonActions();

  NodeListClass lstNodes;
  RemoteStatus_t m_stMainRemote;
};

//------------------------------------------------------------------
// Function & Class Helper
//------------------------------------------------------------------
extern ConfigClass theConfig;

#endif /* xlxConfig_h */

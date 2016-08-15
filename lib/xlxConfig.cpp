/**
 * xlxConfig.cpp - Xlight Config library for loading & saving configuration
 *
 * Created by Baoshi Sun <bs.sun@datatellit.com>
 * Copyright (C) 2015-2016 DTIT
 * Full contributor list:
 *
 * Documentation:
 * Support Forum:
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 *******************************
 *
 * REVISION HISTORY
 * Version 1.0 - Created by Baoshi Sun <bs.sun@datatellit.com>
 *
 * DESCRIPTION
 * 1. configurations are stored in the emulated EEPROM and P1 external Flash
 * 1.1 The Photon and Electron have 2047 bytes of emulated EEPROM
 *     with address space from 0x0000 to 0x07FF.
 * 1.2 The P1 has 1M bytes of external Flash, but can only be accessed via
 *     3rd party API currently.
 * 2. Use EEPROM class (high level API) to access the emulated EEPROM.
 * 3. Use spark-flashee-eeprom (low level 3rd party API) to access P1 external Flash.
 * 4. Please refer to xliMemoryMap.h for memory allocation.
 *
 * ToDo:
 * 1. Move default config values to header as global #define's
 * 2.
 * 3.
**/

#include "xlxConfig.h"
#include "xliPinMap.h"
#include "xlxLogger.h"
#include "xliMemoryMap.h"
#include "xlSmartController.h"

using namespace Flashee;

// the one and only instance of ConfigClass
ConfigClass theConfig = ConfigClass();

//------------------------------------------------------------------
// Xlight Config Class
//------------------------------------------------------------------
ConfigClass::ConfigClass()
{
	P1Flash = Devices::createWearLevelErase();

  m_isLoaded = false;
  m_isChanged = false;
  m_isDSTChanged = false;
  m_isSCTChanged = false;
  m_isRTChanged = false;
  m_isSNTChanged = false;
	m_isNIDChanged = false;
  InitConfig();
}

void ConfigClass::InitConfig()
{
  memset(&m_config, 0x00, sizeof(m_config));
  m_config.version = VERSION_CONFIG_DATA;
  m_config.timeZone.id = 90;              // Toronto
  m_config.timeZone.offset = -300;        // -5 hours
  m_config.timeZone.dst = 1;              // 1 or 0
  m_config.sensorBitmap = 7;
  strcpy(m_config.Organization, XLA_ORGANIZATION);
  strcpy(m_config.ProductName, XLA_PRODUCT_NAME);
  strcpy(m_config.Token, XLA_TOKEN);
  m_config.indBrightness = 0;
  m_config.typeMainDevice = 1;
  m_config.numDevices = 1;
	m_config.numNodes = 2;	// One main device( the smart lamp) and one remote control
	m_config.enableCloudSerialCmd = false;
}

void ConfigClass::InitDevStatus()
{
	Hue_t whiteHue;
	whiteHue.B = 0;
	whiteHue.G = 0;
	whiteHue.R = 0;
	whiteHue.CW = 0xFF;
	whiteHue.WW = 0xFF;
	whiteHue.State = 1; // 1 for on, 0 for off

	theSys.DevStatus_row.op_flag = (OP_FLAG)1;
	theSys.DevStatus_row.flash_flag = (FLASH_FLAG)1;
	theSys.DevStatus_row.run_flag = (RUN_FLAG)1;

	theSys.DevStatus_row.ring1 = whiteHue;
	theSys.DevStatus_row.ring2 = whiteHue;
	theSys.DevStatus_row.ring3 = whiteHue;

	//todo: are these 2 default status values correct?
	theSys.DevStatus_row.id = 1;
	theSys.DevStatus_row.type = 1;
}

BOOL ConfigClass::MemWriteScenarioRow(ScenarioRow_t row, uint32_t address)
{
#ifdef MCU_TYPE_P1
	return P1Flash->read<ScenarioRow_t>(row, address);
#else
	return false;
#endif
}

BOOL ConfigClass::MemReadScenarioRow(ScenarioRow_t &row, uint32_t address)
{
#ifdef MCU_TYPE_P1
	return P1Flash->write<ScenarioRow_t>(row, address);
#else
	return false;
#endif
}

BOOL ConfigClass::LoadConfig()
{
  // Load System Configuration
  if( sizeof(Config_t) <= MEM_CONFIG_LEN )
  {
    EEPROM.get(MEM_CONFIG_OFFSET, m_config);
    if( m_config.version == 0xFF
      || m_config.timeZone.id == 0
      || m_config.timeZone.id > 500
      || m_config.timeZone.dst > 1
      || m_config.timeZone.offset < -780
      || m_config.timeZone.offset > 780
      || m_config.numDevices > MAX_DEVICE_PER_CONTROLLER
			|| m_config.numNodes > MAX_NODE_PER_CONTROLLER
      || m_config.typeMainDevice == devtypUnknown
      || m_config.typeMainDevice >= devtypDummy )
    {
      InitConfig();
      m_isChanged = true;
      LOGW(LOGTAG_MSG, F("Sysconfig is empty, use default settings."));
      SaveConfig();
    }
    else
    {
      LOGI(LOGTAG_MSG, F("Sysconfig loaded."));
    }
    m_isLoaded = true;
    m_isChanged = false;
  } else {
    LOGE(LOGTAG_MSG, F("Failed to load Sysconfig, too large."));
  }

  // Load Device Status
	LoadDeviceStatus();

  // We don't load Schedule Table directly
  // We don't load Scenario Table directly

  // Load Rules
	LoadRuleTable();

	// Load NodeID List
	LoadNodeIDList();

  return m_isLoaded;
}

BOOL ConfigClass::SaveConfig()
{
  if( m_isChanged )
  {
    EEPROM.put(MEM_CONFIG_OFFSET, m_config);
    m_isChanged = false;
    LOGI(LOGTAG_MSG, F("Sysconfig saved."));
  }

	// Save Device Status
	SaveDeviceStatus();

	// Save Schedule Table
	SaveScheduleTable();

	// Save Rule Table
	SaveRuleTable();

	// Save Scenario Table
	SaveScenarioTable();

	// Save NodeID List
	SaveNodeIDList();

  return true;
}

BOOL ConfigClass::IsConfigLoaded()
{
  return m_isLoaded;
}

BOOL ConfigClass::IsConfigChanged()
{
  return m_isChanged;
}

void ConfigClass::SetConfigChanged(BOOL flag)
{
	m_isChanged = flag;
}

BOOL ConfigClass::IsDSTChanged()
{
  return m_isDSTChanged;
}

void ConfigClass::SetDSTChanged(BOOL flag)
{
	m_isDSTChanged = flag;
}

BOOL ConfigClass::IsSCTChanged()
{
  return m_isSCTChanged;
}

void ConfigClass::SetSCTChanged(BOOL flag)
{
	m_isSCTChanged = flag;
}

BOOL ConfigClass::IsRTChanged()
{
	return m_isRTChanged;
}

void ConfigClass::SetRTChanged(BOOL flag)
{
	m_isRTChanged = flag;
}

BOOL ConfigClass::IsSNTChanged()
{
	return m_isSNTChanged;
}

void ConfigClass::SetSNTChanged(BOOL flag)
{
	m_isSNTChanged = flag;
}

BOOL ConfigClass::IsNIDChanged()
{
	return m_isNIDChanged;
}

void ConfigClass::SetNIDChanged(BOOL flag)
{
	m_isNIDChanged = flag;
}

UC ConfigClass::GetVersion()
{
  return m_config.version;
}

BOOL ConfigClass::SetVersion(UC ver)
{
  if( ver != m_config.version )
  {
    m_config.version = ver;
    m_isChanged = true;
  }
}

US ConfigClass::GetTimeZoneID()
{
  return m_config.timeZone.id;
}

BOOL ConfigClass::SetTimeZoneID(US tz)
{
  if( tz == 0 || tz > 500 )
    return false;

  if( tz != m_config.timeZone.id )
  {
    m_config.timeZone.id = tz;
    m_isChanged = true;
    theSys.m_tzString = GetTimeZoneJSON();
  }
  return true;
}

UC ConfigClass::GetDaylightSaving()
{
  return m_config.timeZone.dst;
}

BOOL ConfigClass::SetDaylightSaving(UC flag)
{
  if( flag > 1 )
    return false;

  if( flag != m_config.timeZone.dst )
  {
    m_config.timeZone.dst = flag;
    m_isChanged = true;
    theSys.m_tzString = GetTimeZoneJSON();
  }
  return true;
}

SHORT ConfigClass::GetTimeZoneOffset()
{
  return m_config.timeZone.offset;
}

SHORT ConfigClass::GetTimeZoneDSTOffset()
{
  return (m_config.timeZone.offset + m_config.timeZone.dst * 60);
}

BOOL ConfigClass::SetTimeZoneOffset(SHORT offset)
{
  if( offset >= -780 && offset <= 780)
  {
    if( offset != m_config.timeZone.offset )
    {
      m_config.timeZone.offset = offset;
      m_isChanged = true;
      theSys.m_tzString = GetTimeZoneJSON();
    }
    return true;
  }
  return false;
}

String ConfigClass::GetTimeZoneJSON()
{
  String jsonStr = String::format("{\"id\":%d,\"offset\":%d,\"dst\":%d}",
      GetTimeZoneID(), GetTimeZoneOffset(), GetDaylightSaving());
  return jsonStr;
}

String ConfigClass::GetOrganization()
{
  String strName = m_config.Organization;
  return strName;
}

void ConfigClass::SetOrganization(const char *strName)
{
  strncpy(m_config.Organization, strName, sizeof(m_config.Organization) - 1);
  m_isChanged = true;
}

String ConfigClass::GetProductName()
{
  String strName = m_config.ProductName;
  return strName;
}

void ConfigClass::SetProductName(const char *strName)
{
  strncpy(m_config.ProductName, strName, sizeof(m_config.ProductName) - 1);
  m_isChanged = true;
}

String ConfigClass::GetToken()
{
  String strName = m_config.Token;
  return strName;
}

void ConfigClass::SetToken(const char *strName)
{
  strncpy(m_config.Token, strName, sizeof(m_config.Token) - 1);
  m_isChanged = true;
}

BOOL ConfigClass::ConfigClass::IsCloudSerialEnabled()
{
	return m_config.enableCloudSerialCmd;
}

void ConfigClass::SetCloudSerialEnabled(BOOL sw)
{
	if( sw != m_config.enableCloudSerialCmd ) {
		m_config.enableCloudSerialCmd = sw;
		m_isChanged = true;
	}
}

BOOL ConfigClass::IsSensorEnabled(sensors_t sr)
{
    return(BITTEST(m_config.sensorBitmap, sr));
}

void ConfigClass::SetSensorEnabled(sensors_t sr, BOOL sw)
{
  US bits = m_config.sensorBitmap;
  if (sw) {
    bits = BITSET(m_config.sensorBitmap, sr);
  } else {
    bits = BITUNSET(m_config.sensorBitmap, sr);
  }
  if( bits != m_config.sensorBitmap )
  {
    m_config.sensorBitmap = bits;
    m_isChanged = true;
  }
}

UC ConfigClass::GetBrightIndicator()
{
  return m_config.indBrightness;
}

BOOL ConfigClass::SetBrightIndicator(UC level)
{
  if( level != m_config.indBrightness )
  {
    m_config.indBrightness = level;
    m_isChanged = true;
    return true;
  }

  return false;
}

UC ConfigClass::GetMainDeviceType()
{
  return m_config.typeMainDevice;
}

BOOL ConfigClass::SetMainDeviceType(UC type)
{
  if( type > devtypUnknown && type < devtypDummy )
  {
    if( type != m_config.typeMainDevice )
    {
      m_config.typeMainDevice = type;
      m_isChanged = true;
    }
    return true;
  }
  return false;
}


UC ConfigClass::GetNumDevices()
{
  return m_config.numDevices;
}

BOOL ConfigClass::SetNumDevices(UC num)
{
  if( m_config.numDevices <= MAX_DEVICE_PER_CONTROLLER )
  {
    if( num != m_config.numDevices )
    {
      m_config.numDevices = num;
      m_isChanged = true;
    }
    return true;
  }
  return false;
}

UC ConfigClass::GetNumNodes()
{
	return m_config.numNodes;
}

BOOL ConfigClass::SetNumNodes(UC num)
{
	if( m_config.numNodes <= MAX_NODE_PER_CONTROLLER )
  {
    if( num != m_config.numNodes )
    {
      m_config.numNodes = num;
      m_isChanged = true;
    }
    return true;
  }
  return false;
}

// Load Device Status
BOOL ConfigClass::LoadDeviceStatus()
{
  if( sizeof(DevStatus_t) <= MEM_DEVICE_STATUS_LEN )
  {
		EEPROM.get(MEM_DEVICE_STATUS_OFFSET, theSys.DevStatus_row);
		//check row values / error cases
		if (theSys.DevStatus_row.op_flag != (OP_FLAG)1
			|| theSys.DevStatus_row.flash_flag != (FLASH_FLAG)1
			|| theSys.DevStatus_row.run_flag != (RUN_FLAG)1 )
		{
			//no devstatus saved, give default values
			InitDevStatus();
			m_isDSTChanged = true;
			LOGW(LOGTAG_MSG, F("Corrupt Device Status Table, using default status."));
			SaveConfig();
		}
		else
		{
			LOGD(LOGTAG_MSG, F("Device status table loaded."));
		}
    m_isDSTChanged = false;
  }
  else
  {
    LOGW(LOGTAG_MSG, F("Failed to load device status table, too large."));
  }

	return true;
}

// Save Device Status
BOOL ConfigClass::SaveDeviceStatus()
{
  if( m_isDSTChanged )
  {
	  DevStatus_t tmpRow = theSys.DevStatus_row; //copy of data to write to flash

	  //change flags to 111 to indicate flash row is occupied
	  tmpRow.op_flag = (OP_FLAG)1;
	  tmpRow.flash_flag = (FLASH_FLAG)1;
	  tmpRow.run_flag = (RUN_FLAG)1;

	  if (sizeof(tmpRow) <= MEM_DEVICE_STATUS_LEN)
	  {
		  EEPROM.put(MEM_DEVICE_STATUS_OFFSET, tmpRow); //write to flash
		  theSys.DevStatus_row.flash_flag = SAVED; //toggle flash flag

		  m_isDSTChanged = false;
		  LOGD(LOGTAG_MSG, F("Device status table saved."));
			return true;
	  }
	  else
	  {
		  LOGE(LOGTAG_MSG, F("Unable to write Device Status to flash, out of memory bounds"));
	  }
  }

	return false;
}

// Save Schedule Table
BOOL ConfigClass::SaveScheduleTable()
{
  if( m_isSCTChanged )
  {
	  bool success_flag = true;

	  ListNode<ScheduleRow_t> *rowptr = theSys.Schedule_table.getRoot();
	  while (rowptr != NULL)
	  {
		  if (rowptr->data.run_flag == EXECUTED && rowptr->data.flash_flag == UNSAVED)
		  {
			  ScheduleRow_t tmpRow = rowptr->data; //copy of data to write to flash

			  switch (rowptr->data.op_flag)
			  {
				case DELETE:
					//change flags to 000 to indicate flash row is empty
					tmpRow.op_flag = (OP_FLAG)0;
					tmpRow.flash_flag = (FLASH_FLAG)0;
					tmpRow.run_flag = (RUN_FLAG)0;
					break;

				case PUT:
				case POST:
				case GET:
					//change flags to 111 to indicate flash row is occupied
					tmpRow.op_flag = (OP_FLAG)1;
					tmpRow.flash_flag = (FLASH_FLAG)1;
					tmpRow.run_flag = (RUN_FLAG)1;
					break;
			  }

			  //write tmpRow to flash
			  int row_index = rowptr->data.uid;
			  if (row_index < MAX_SCT_ROWS)
			  {
				  EEPROM.put(MEM_SCHEDULE_OFFSET + row_index*SCT_ROW_SIZE, tmpRow); //write to flash

				  rowptr->data.flash_flag = SAVED; //toggle flash flag
			  }
			  else
			  {
				  LOGE(LOGTAG_MSG, F("Error, cannot write Schedule row %d to flash, out of memory bounds"), row_index);
				  success_flag = false;
			  }
		  }
		  rowptr = rowptr->next;
	  }

	  if (success_flag)
	  {
		  m_isSCTChanged = false;
		  LOGD(LOGTAG_MSG, F("Schedule table saved."));
			return true;
	  }
	  else
	  {
		  LOGE(LOGTAG_MSG, F("Unable to write 1 or more Schedule table rows to flash"));
	  }
  }

	return false;
}

// Save Scenario Table
BOOL ConfigClass::SaveScenarioTable()
{
  if (m_isSNTChanged)
  {
	  bool success_flag = true;

	  ListNode<ScenarioRow_t> *rowptr = theSys.Scenario_table.getRoot();
	  while (rowptr != NULL)
	  {
		  if (rowptr->data.run_flag == EXECUTED && rowptr->data.flash_flag == UNSAVED)
		  {
			  ScenarioRow_t tmpRow = rowptr->data; //copy of data to write to p1

			  switch (rowptr->data.op_flag)
			  {
			  case DELETE:
				  //change flags to indicate flash row is empty
				  tmpRow.op_flag = (OP_FLAG)0;
				  tmpRow.flash_flag = (FLASH_FLAG)0;
				  tmpRow.run_flag = (RUN_FLAG)0;
				  break;
			  case PUT:
			  case POST:
			  case GET:
				  //change flags to 111 to indicate flash row is occupied
				  tmpRow.op_flag = (OP_FLAG)1;
				  tmpRow.flash_flag = (FLASH_FLAG)1;
				  tmpRow.run_flag = (RUN_FLAG)1;
				  break;
			  }

			  //write tmpRow to p1
			  int row_index = rowptr->data.uid;
			  if (row_index < MAX_SNT_ROWS)
			  {
#ifdef MCU_TYPE_P1
				  P1Flash->write<ScenarioRow_t>(tmpRow, MEM_SCENARIOS_OFFSET + row_index*SNT_ROW_SIZE);
#endif
				  rowptr->data.flash_flag = SAVED; //toggle flash flag
			  }
			  else
			  {
				  LOGE(LOGTAG_MSG, F("Error, cannot write Scenario row %d to flash, out of memory bounds"), row_index);
				  success_flag = false;
			  }
		  }
		  rowptr = rowptr->next;
	  }

	  if (success_flag)
	  {
		  m_isSNTChanged = false;
		  LOGD(LOGTAG_MSG, F("Scenario table saved."));
			return true;
	  }
	  else
	  {
		  LOGE(LOGTAG_MSG, F("Unable to write 1 or more Scenario table rows to flash"));
	  }

	  m_isSNTChanged = false;
	  LOGD(LOGTAG_MSG, F("Scenerio table saved."));
  }

	return false;
}

// Load Rules from P1 Flash
BOOL ConfigClass::LoadRuleTable()
{
#ifdef MCU_TYPE_P1
  RuleRow_t RuleArray[MAX_RT_ROWS];
  if (RT_ROW_SIZE*MAX_RT_ROWS <= MEM_RULES_LEN)
  {
	  if (P1Flash->read<RuleRow_t[MAX_RT_ROWS]>(RuleArray, MEM_RULES_OFFSET))
	  {
		  for (int i = 0; i < MAX_RT_ROWS; i++) //interate through RuleArray for non-empty rows
		  {
			  if (RuleArray[i].op_flag == (OP_FLAG)1
				  && RuleArray[i].flash_flag == (FLASH_FLAG)1
				  && RuleArray[i].run_flag == (RUN_FLAG)1)
			  {
				  //change flags to be written into working memory chain
				  RuleArray[i].op_flag = POST;
				  RuleArray[i].run_flag = UNEXECUTED;
				  RuleArray[i].flash_flag = SAVED;		//Already know it exists in flash
				  if (!theSys.Rule_table.add(RuleArray[i])) //add non-empty row to working memory chain
				  {
					  LOGW(LOGTAG_MSG, F("Rule row %d failed to load from flash"), i);
				  }
			  }
			  //else: row is either empty or trash; do nothing
		  }

		  m_isRTChanged = true; //allow ReadNewRules() to run
		  theSys.ReadNewRules(); //acts on the Rules rules newly loaded from flash
		  m_isRTChanged = false; //since we are not calling SaveConfig(), change flag to false again
	  }
	  else
	  {
		  LOGW(LOGTAG_MSG, F("Failed to read the rule table from flash."));
	  }
  }
  else
  {
	  LOGW(LOGTAG_MSG, F("Failed to load rule table, too large."));
  }
#endif

	return true;
}

// Save Rule Table
BOOL ConfigClass::SaveRuleTable()
{
	if ( m_isRTChanged )
	{
		bool success_flag = true;

		ListNode<RuleRow_t> *rowptr = theSys.Rule_table.getRoot();
		while (rowptr != NULL)
		{
			if (rowptr->data.run_flag == EXECUTED && rowptr->data.flash_flag == UNSAVED)
			{
				RuleRow_t tmpRow = rowptr->data; //copy of data to write to p1

				switch (rowptr->data.op_flag)
				{
				case DELETE:
					//change flags to indicate flash row is empty
					tmpRow.op_flag = (OP_FLAG)0;
					tmpRow.flash_flag = (FLASH_FLAG)0;
					tmpRow.run_flag = (RUN_FLAG)0;
					break;
				case PUT:
				case POST:
				case GET:
					//change flags to 111 to indicate flash row is occupied
					tmpRow.op_flag = (OP_FLAG)1;
					tmpRow.flash_flag = (FLASH_FLAG)1;
					tmpRow.run_flag = (RUN_FLAG)1;
					break;
				}

				//write tmpRow to p1
				int row_index = rowptr->data.uid;
				if (row_index < MAX_RT_ROWS)
				{
	#ifdef MCU_TYPE_P1
					P1Flash->write<RuleRow_t>(tmpRow, MEM_RULES_OFFSET + row_index*RT_ROW_SIZE);
	#endif
					rowptr->data.flash_flag = SAVED; //toggle flash flag
				}
				else
				{
					LOGE(LOGTAG_MSG, F("Error, cannot write Schedule row %d to flash, out of memory bounds"), row_index);
					success_flag = false;
				}
			}
			rowptr = rowptr->next;
		}

		if (success_flag)
		{
			m_isRTChanged = false;
			LOGD(LOGTAG_MSG, F("Rule table saved."));
			return true;
		}
		else
		{
			LOGE(LOGTAG_MSG, F("Unable to write 1 or more Rule table rows to flash"));
		}
	}

	return false;
}

// Load NodeID List
BOOL ConfigClass::LoadNodeIDList()
{
	// ToDo:

	return true;
}

// Save NodeID List
BOOL ConfigClass::SaveNodeIDList()
{
	if ( m_isNIDChanged )
	{
		// ToDo: save Node ID list
	}

	return false;
}

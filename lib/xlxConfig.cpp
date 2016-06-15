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
#include "xlxLogger.h"
#include "xliMemoryMap.h"
#include "xlSmartController.h"

// the one and only instance of ConfigClass
ConfigClass theConfig = ConfigClass();

//------------------------------------------------------------------
// Xlight Config Class
//------------------------------------------------------------------
ConfigClass::ConfigClass()
{
  m_isLoaded = false;
  m_isChanged = false;
  m_isDSTChanged = false;
  m_isSCTChanged = false;
  m_isRTChanged = false;
  m_isSNTChanged = false;
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
}

BOOL ConfigClass::LoadConfig()
{
  // Load System Configuration
  if( EEPROM.length() >= MEM_CONFIG_OFFSET + MEM_CONFIG_LEN )
  {
    EEPROM.get(MEM_CONFIG_OFFSET, m_config);
    if( m_config.version == 0xFF
      || m_config.timeZone.id == 0
      || m_config.timeZone.id > 500
      || m_config.timeZone.dst > 1
      || m_config.timeZone.offset < -780
      || m_config.timeZone.offset > 780
      || m_config.numDevices > MAX_DEVICE_PER_CONTROLLER
      || m_config.typeMainDevice == devtypUnknown
      || m_config.typeMainDevice >= devtypDummy )
    {
      InitConfig();
      m_isChanged = true;
      LOGW(LOGTAG_MSG, "Sysconfig is empty, use default settings.");
      SaveConfig();
    }
    else
    {
      LOGI(LOGTAG_MSG, "Sysconfig loaded.");
    }
    m_isLoaded = true;
    m_isChanged = false;
  } else {
    LOGE(LOGTAG_MSG, "Failed to load Sysconfig.");
  }

  // ToDo: load device status
  if( EEPROM.length() >= MEM_DEVICE_STATUS_OFFSET + MEM_DEVICE_STATUS_LEN )
  {
	//load from memory
	if (0//check row values
		// ToDo: add error test cases
		)
	{
		//InitDevStatus();
		//m_isDSTChanged = true;
		//LOGW(LOGTAG_MSG, "Corrupt Device Status Table, using default status");
		//SaveConfig();
	}
	else
	{
		LOGD(LOGTAG_MSG, "Device status table loaded.");
	}
    m_isDSTChanged = false;
  } else {
    LOGW(LOGTAG_MSG, "Failed to load device status table.");
  }

  // ToDo: load Schedule
  if( EEPROM.length() >= MEM_SCHEDULE_OFFSET + MEM_SCHEDULE_LEN )
  {
    //...

	//ToDo: load schedule, create alarms for all state_flag = full rows

    m_isSCTChanged = false;
    LOGD(LOGTAG_MSG, "Schedule table loaded.");
  } else {
    LOGW(LOGTAG_MSG, "Failed to load schedule table.");
  }

  // ToDo: load Rules from P1 Flash, change m_isRTChanged to false
  // Use PI Flash access library
  // MEM_RULES_OFFSET

  // ToDo: load Scenerios from P1 Flash, change m_isSNTChanged to false
  // Use PI Flash access library
  // MEM_SCENARIOS_OFFSET

  return m_isLoaded;
}

BOOL ConfigClass::SaveConfig()
{
  if( m_isChanged )
  {
    EEPROM.put(MEM_CONFIG_OFFSET, m_config);
    m_isChanged = false;
    LOGI(LOGTAG_MSG, "Sysconfig saved.");
  }

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
		  LOGD(LOGTAG_MSG, "Device status table saved.");
	  }
	  else 
	  {
		  LOGE(LOGTAG_MSG, "Unable to write Device Status to flash, out of memory bounds");
	  }
  }

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
					//change flags to 111 to indicate flash row is occupied
					tmpRow.op_flag = (OP_FLAG)1;
					tmpRow.flash_flag = (FLASH_FLAG)1;
					tmpRow.run_flag = (RUN_FLAG)1;
					break;
			  }

			  //write tmpRow to flash
			  int row_index = rowptr->data.uid;
			  if (row_index < MAX_SCT_ROWS && sizeof(tmpRow) <= SCT_ROW_SIZE)
			  {
				  EEPROM.put(MEM_SCHEDULE_OFFSET + row_index*SCT_ROW_SIZE, tmpRow); //write to flash

				  rowptr->data.flash_flag = SAVED; //toggle flash flag
			  }
			  else
			  {
				  LOGE(LOGTAG_MSG, "Error, cannot write Schedule row %d to flash, out of memory bounds", row_index);
				  success_flag = false;
			  }
		  }
		  rowptr = rowptr->next;
	  }
	  
	  if (success_flag)
	  {
		  m_isSCTChanged = false;
		  LOGD(LOGTAG_MSG, "Schedule table saved.");
	  }
	  else
	  {
		  LOGE(LOGTAG_MSG, "Unable to write 1 or more Schedule table rows to flash");
	  }
  }

  if ( m_isRTChanged )
  {
	  //ToDo: Write RuleTable_queue to memory
	  // iterate through table, find flag= EXECUTED rows
	  // change all 3 flags in the row to 1 if its add/update, change flash flags to 0 if its delete
	  // Write row to flash if add/update
	  // change original row flag to SAVED
	  // MEM_RULES_OFFSET
	  // Keep all of Rule Table in working memory

	  m_isRTChanged = false;
	  LOGD(LOGTAG_MSG, "Rule table saved.");
  }

  if (m_isSNTChanged)
  {
	  //ToDo: Write ScenerioTable_queue to memory (indexs are taken care of by cloud input)
	  // iterate through table, find flag= EXECUTED rows
	  // change all 3 flags in the row to 1 if its add/update, change flash flags to 0 if its delete
	  // Write row to flash if add/update
	  // change original row flag to SAVED
	  // MEM_RULES_OFFSET

	  m_isRTChanged = false;
	  LOGD(LOGTAG_MSG, "Scenerio table saved.");
  }

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

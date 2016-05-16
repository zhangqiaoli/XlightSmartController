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
    //...

    m_isDSTChanged = false;
    LOGD(LOGTAG_MSG, "Device status table loaded.");
  } else {
    LOGW(LOGTAG_MSG, "Failed to load device status table.");
  }

  // ToDo: load Schedule
  if( EEPROM.length() >= MEM_SCHEDULE_OFFSET + MEM_SCHEDULE_LEN )
  {
    //...

    m_isSCTChanged = false;
    LOGD(LOGTAG_MSG, "Schedule table loaded.");
  } else {
    LOGW(LOGTAG_MSG, "Failed to load schedule table.");
  }

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
    //ToDo:

    m_isDSTChanged = false;
    LOGD(LOGTAG_MSG, "Device status table saved.");
  }

  if( m_isSCTChanged )
  {
	//ToDo: Read SCT table, add/delete changed alarm/timer entries
	
	//ToDo: create new alarms/timers (tie to SmartControllerClass::AlarmTimerTriggered() function)
	//ToDo: create updateAlarm() func with param of only info to be used to create the alarm/timer

    m_isSCTChanged = false;
    LOGD(LOGTAG_MSG, "Schedule table saved.");
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

BOOL ConfigClass::IsDSTChanged()
{
  return m_isDSTChanged;
}

BOOL ConfigClass::IsSCTChanged()
{
  return m_isSCTChanged;
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

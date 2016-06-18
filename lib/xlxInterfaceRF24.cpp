/**
 * xlxInterfaceRF24.cpp - Xlight Xlight interface for RF2.4 communication
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
 *
 * ToDo:
 * 1.
**/

#include "xlxInterfaceRF24.h"
#include "xlxLogger.h"

//------------------------------------------------------------------
// Xlight RF24Interface Class
//------------------------------------------------------------------
RF24InterfaceClass::RF24InterfaceClass()
{
  m_DataQueue.SetMaxLength(BUFFER_OVERFLOW);
}

void RF24InterfaceClass::ResetInterface()
{
  // Clear Buffer Data
	m_DataQueue.Remove();
}

// Triggered when data received
void RF24InterfaceClass::OnReceive(UC *data, US len)
{
	if( m_DataQueue.Append(data, len) <= 0 )
	{
    ResetInterface();
	}
}

// Check message buffer, split messages and process message one by one
void RF24InterfaceClass::CheckMessageBuffer()
{
  uint8_t len;
  char data[MAX_MESSAGE_LENGTH];

  while( m_DataQueue.Length() >= HEADER_SIZE ) {
    // Extract one message
    UC *buf = m_DataQueue.GetBuffer();
    for( len = 0; len < m_DataQueue.Length(); len++) {
      if( buf[len] == '\n' ) {
        // Copy message
        memset(data, 0x00, MAX_MESSAGE_LENGTH);
        memcpy(data, buf, len - 1);
        // Process message
        ProcessMessage(data, len - 1);
        // Remove processed message
        m_DataQueue.Remove(len);
        len = 0;
        break;
      }
    }

    if( len >= m_DataQueue.Length() )
      break;
  }
}

// Send message to RF2.4 module
BOOL RF24InterfaceClass::SendMessage(const MyMessage &message)
{
  char data[MAX_MESSAGE_LENGTH];
  MyMessage msg = message;
  if( msg.getSerialString(data) ) {
    // ToDo: send data

    return true;
  }
  return false;
}

// Process message received from RF2.4 module
BOOL RF24InterfaceClass::ProcessMessage(char *data, uint8_t length)
{
  MyMessage msg;
  if( parse(msg, data) ) {
    LOGI(LOGTAG_MSG, "RF2.4 msg: ", data);
    // ToDo: process msg

    return true;
  }
  return false;
}

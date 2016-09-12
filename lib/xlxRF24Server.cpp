/**
 * xlxRF24Server.cpp - Xlight RF2.4 server library based on via RF2.4 module
 * for communication with instances of xlxRF24Client
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
 * Dependancy
 * 1. Particle RF24 library, refer to
 *    https://github.com/stewarthou/Particle-RF24
 *    http://tmrh20.github.io/RF24/annotated.html
 *
 * DESCRIPTION
 * 1. Mysensors protocol compatible and extended
 * 2. Address algorithm
 * 3. PipePool (0 AdminPipe, Read & Write; 1-5 pipe pool, Read)
 * 4. Session manager, optional address shifting
 * 5.
 *
 * ToDo:
 * 1. Two pipes collaboration for security: divide a message into two parts
 * 2. Apply security layer, e.g. AES, encryption, etc.
 *
**/

#include "xlxRF24Server.h"
#include "xliPinMap.h"
#include "xlSmartController.h"
#include "xlxLogger.h"

#include "MyParserSerial.h"

//------------------------------------------------------------------
// the one and only instance of RF24ServerClass
RF24ServerClass theRadio(PIN_RF24_CE, PIN_RF24_CS);
MyMessage msg;
MyParserSerial msgParser;
UC *msgData = (UC *)&(msg.msg);

RF24ServerClass::RF24ServerClass(uint8_t ce, uint8_t cs, uint8_t paLevel)
	:	MyTransportNRF24(ce, cs, paLevel)
{
	_times = 0;
	_succ = 0;
	_received = 0;
}

bool RF24ServerClass::ServerBegin()
{
  // Initialize RF module
	if( !init() ) {
    LOGC(LOGTAG_MSG, F("RF24 module is not valid!"));
		return false;
	}

  // Set role to Controller or Gateway
	SetRole_Gateway();
  return true;
}

// Make NetworkID with the right 4 bytes of device MAC address
uint64_t RF24ServerClass::GetNetworkID(bool _full)
{
  uint64_t netID = 0;

  byte mac[6];
  WiFi.macAddress(mac);
	int i = (_full ? 0 : 2);
  for (; i<6; i++) {
    netID += mac[i];
		if( _full && i == 5 ) break;
    netID <<= 8;
  }

  return netID;
}

bool RF24ServerClass::ChangeNodeID(const uint8_t bNodeID)
{
	if( bNodeID == getAddress() ) {
			return true;
	}

	if( bNodeID == 0 ) {
		SetRole_Gateway();
	} else {
		setAddress(bNodeID, RF24_BASE_RADIO_ID);
	}

	return true;
}

void RF24ServerClass::SetRole_Gateway()
{
	uint64_t lv_networkID = GetNetworkID();
	setAddress(GATEWAY_ADDRESS, lv_networkID);
}

bool RF24ServerClass::ProcessSend(String &strMsg, MyMessage &my_msg)
{
	bool sentOK = false;
	bool bMsgReady = false;
	int iValue;
	float fValue;
	char strBuffer[64];

	int nPos = strMsg.indexOf(':');
	uint8_t lv_nNodeID;
	uint8_t lv_nMsgID;
	String lv_sPayload;
	if (nPos > 0) {
		// Extract NodeID & MessageID
		lv_nNodeID = (uint8_t)(strMsg.substring(0, nPos).toInt());
		lv_nMsgID = (uint8_t)(strMsg.substring(nPos + 1).toInt());
		int nPos2 = strMsg.indexOf(nPos + 1, ':');
		if (nPos2 > 0) {
			lv_nMsgID = (uint8_t)(strMsg.substring(nPos + 1, nPos2).toInt());
			lv_sPayload = strMsg.substring(nPos2 + 1);
		} else {
			lv_nMsgID = (uint8_t)(strMsg.substring(nPos + 1).toInt());
			lv_sPayload = "";
		}
	}
	else {
		// Parse serial message
		lv_nMsgID = 0;
	}

	switch (lv_nMsgID)
	{
	case 0: // Free style
		iValue = min(strMsg.length(), 63);
		strncpy(strBuffer, strMsg.c_str(), iValue);
		strBuffer[iValue] = 0;
		// Serail format to MySensors message structure
		bMsgReady = msgParser.parse(msg, strBuffer);
		if (bMsgReady) {
			SERIAL("Now sending message...");
		}
		break;

	case 1:   // Request new node ID
		if (getAddress() == GATEWAY_ADDRESS) {
			SERIAL_LN("Controller can not request node ID\n\r");
		}
		else {
			UC deviceType = (getAddress() == 3 ? NODE_TYP_REMOTE : NODE_TYP_LAMP);
			ChangeNodeID(AUTO);
			msg.build(AUTO, BASESERVICE_ADDRESS, deviceType, C_INTERNAL, I_ID_REQUEST, false);
			msg.set(GetNetworkID(true));		// identity: could be MAC, serial id, etc
			bMsgReady = true;
			SERIAL("Now sending request node id message...");
		}
		break;

	case 2:   // Lamp present, req ack
		{
			UC lampType = (UC)devtypWRing3;
			msg.build(getAddress(), lv_nNodeID, lampType, C_PRESENTATION, S_LIGHT, true);
			msg.set(GetNetworkID(true));
			bMsgReady = true;
			SERIAL("Now sending lamp present message...");
		}
		break;

	case 3:   // Temperature sensor present with sensor id 1, req no ack
		msg.build(getAddress(), lv_nNodeID, 1, C_PRESENTATION, S_TEMP, false);
		msg.set("");
		bMsgReady = true;
		SERIAL("Now sending DHT11 present message...");
		break;

	case 4:   // Temperature set to 23.5, req no ack
		msg.build(getAddress(), lv_nNodeID, 1, C_SET, V_TEMP, false);
		fValue = 23.5;
		msg.set(fValue, 2);
		bMsgReady = true;
		SERIAL("Now sending set temperature message...");
		break;

	case 5:   // Humidity set to 45, req no ack
		msg.build(getAddress(), lv_nNodeID, 1, C_SET, V_HUM, false);
		iValue = 45;
		msg.set(iValue);
		bMsgReady = true;
		SERIAL("Now sending set humidity message...");
		break;
	}

	if (bMsgReady) {
		SERIAL("to %d...", msg.getDestination());
		sentOK = ProcessSend();
		my_msg = msg;
		SERIAL_LN(sentOK ? "OK" : "failed");
	}

	return sentOK;
}

bool RF24ServerClass::ProcessSend(String &strMsg)
{
	MyMessage tempMsg;
	return ProcessSend(strMsg, tempMsg);
}

// ToDo: add message to queue instead of sending out directly
bool RF24ServerClass::ProcessSend(MyMessage *pMsg)
{
	if( !pMsg ) { pMsg = &msg; }

	// Determine the receiver addresse
	_times++;
	if( send(pMsg->getDestination(), *pMsg) )
	{
		_succ++;
		return true;
	}

	return false;
}

bool RF24ServerClass::ProcessReceive()
{
	if( !isValid() ) return false;

	bool msgReady = false;
  bool sentOK = false;
  uint8_t to = 0;
  uint8_t pipe;
	UC replyTo;
	UC msgType;
	UC transTo;
  if (!available(&to, &pipe))
    return false;

  uint8_t len = receive(msgData);
  if( len < HEADER_SIZE )
  {
    LOGW(LOGTAG_MSG, "got corrupt dynamic payload!");
    return false;
  } else if( len > MAX_MESSAGE_LENGTH )
  {
    LOGW(LOGTAG_MSG, "message length exceeded: %d", len);
    return false;
  }

  char strDisplay[SENSORDATA_JSON_SIZE];
  _received++;
	msgType = msg.getType();
  LOGD(LOGTAG_MSG, "Received from pipe %d msg-len=%d, from:%d to:%d dest:%d cmd:%d type:%d sensor:%d payl-len:%d",
        pipe, len, msg.getSender(), to, msg.getDestination(), msg.getCommand(),
        msgType, msg.getSensor(), msg.getLength());
	/*
  memset(strDisplay, 0x00, sizeof(strDisplay));
  msg.getJsonString(strDisplay);
  SERIAL_LN("  JSON: %s, len: %d", strDisplay, strlen(strDisplay));
  memset(strDisplay, 0x00, sizeof(strDisplay));
  SERIAL_LN("  Serial: %s, len: %d", msg.getSerialString(strDisplay), strlen(strDisplay));
	*/

  switch( msg.getCommand() )
  {
    case C_INTERNAL:
      if( msgType == I_ID_REQUEST && msg.getSender() == AUTO ) {
        // On ID Request message:
        /// Get new ID
				char cNodeType = (char)msg.getSensor();
				uint64_t nIdentity = msg.getUInt64();
        UC newID = theConfig.lstNodes.requestNodeID(cNodeType, nIdentity);
        replyTo = msg.getSender();
        /// Send response message
        msg.build(getAddress(), replyTo, newID, C_INTERNAL, I_ID_RESPONSE, false, true);
				if( newID > 0 ) {
	        msg.set(getMyNetworkID());
	        LOGN(LOGTAG_EVENT, "Allocated NodeID:%d type:%c to %s", newID, cNodeType, PrintUint64(strDisplay, nIdentity));
				} else {
					LOGW(LOGTAG_MSG, "Failed to allocate NodeID type:%c to %s", cNodeType, PrintUint64(strDisplay, nIdentity));
				}
				msgReady = true;
      } else if( msgType == I_ID_RESPONSE && getAddress() == AUTO ) {
				// Device/client got nodeID from Controller
				uint8_t lv_nodeID = msg.getSensor();
        if( lv_nodeID == NODEID_GATEWAY || lv_nodeID == NODEID_DUMMY ) {
          LOGE(LOGTAG_MSG, F("Failed to get NodeID"));
        } else {
					uint64_t lv_networkID = msg.getUInt64();
          LOGI(LOGTAG_DATA, "Get NodeId: %d, networkId: %s", lv_nodeID, PrintUint64(strDisplay, lv_networkID));
          setAddress(lv_nodeID, lv_networkID);
        }
      }
      break;

		case C_PRESENTATION:
			if( msgType == S_LIGHT || msgType == S_DIMMER ) {
				US token;
				if( !msg.isAck() ) {
					// Presentation message: appear of Smart Lamp
					// Verify credential, return token if true, and change device status
					UC lv_nNodeID = msg.getSender();
					UC lampType = msg.getSensor();
					uint64_t nIdentity = msg.getUInt64();
					token = theSys.VerifyDevicePresence(lv_nNodeID, lampType, nIdentity);
					if( token ) {
						// return token
						// Notes: lampType & S_LIGHT are not necessary
						replyTo = msg.getSender();
		        msg.build(getAddress(), replyTo, lampType, C_PRESENTATION, msgType, false, true);
						msg.set((unsigned int)token);
						msgReady = true;
					} else {
						LOGW(LOGTAG_MSG, "Unqualitied device connect attemp received");
					}
				} else {
					// Device/client got Response to Presentation message, ready to work
					token = msg.getUInt();
					LOGN(LOGTAG_MSG, "Node:%d got Presentation Response with token:%d", getAddress(), token);
				}
			}
			break;

		case C_REQ:
			// ToDo: verify token
			if( msgType == V_PERCENTAGE ) {
				// Lamp on/of
				replyTo = msg.getSender();
				transTo = msg.getSensor();
				if( transTo > 0 ) {
					bool bIsAck = msg.isAck();

					// Transfer message
					msg.build(getAddress(), transTo, replyTo, C_REQ, V_PERCENTAGE, !bIsAck, bIsAck);
					msgReady = true;
				}
			}
			break;

		case C_SET:
			// ToDo: verify token
			if( msgType == V_STATUS || msgType == V_PERCENTAGE ) {
				// Lamp on/of
				replyTo = msg.getSender();
				transTo = msg.getSensor();
				if( transTo > 0 ) {
					UC bOnOff;
					String sPayload;
					if( msgType == V_STATUS ) {
						bOnOff = msg.getByte();
					} else if( msgType == V_PERCENTAGE ) {
						sPayload = msg.getString();
					}
					bool bIsAck = msg.isAck();

					// Transfer message
					msg.build(getAddress(), transTo, replyTo, C_SET, msgType, !bIsAck, bIsAck);
					if( msgType == V_STATUS ) {
						msg.set(bOnOff);
					} else if( msgType == V_PERCENTAGE ) {
						msg.set(sPayload.c_str());
					}
					msgReady = true;
				}
			}
			break;

    default:
      break;
  }

	// Send reply message
	if( msgReady ) {
		_times++;
		sentOK = send(replyTo, msg, pipe);
		if( sentOK ) _succ++;
	}

  return true;
}

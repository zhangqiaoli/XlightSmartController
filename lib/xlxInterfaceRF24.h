//  xlxInterfaceRF24.h - Xlight interface for RF2.4 communication header

#ifndef xlxInterfaceRF24_h
#define xlxInterfaceRF24_h

#include "xliCommon.h"
#include "DataQueue.h"
#include "MyMessage.h"
#include "MyParserSerial.h"

#define BUFFER_OVERFLOW (MAX_MESSAGE_LENGTH * 10)      // buffer size for 10 messages

//------------------------------------------------------------------
// Xlight RF24Interface Class
//------------------------------------------------------------------
class RF24InterfaceClass : public MyParserSerial
{
public:
  RF24InterfaceClass();
  void ResetInterface();

  inline MyMessage& build (MyMessage &msg, uint8_t destination, uint8_t sensor, uint8_t command, uint8_t type, bool enableAck) {
  	msg.destination = destination;
  	msg.sender = GATEWAY_ADDRESS;
  	msg.sensor = sensor;
  	msg.type = type;
  	mSetCommand(msg,command);
  	mSetRequestAck(msg,enableAck);
  	mSetAck(msg,false);
  	return msg;
  };

  void OnReceive(UC *data, US len);
  void CheckMessageBuffer();
  BOOL SendMessage(const MyMessage &message);
  BOOL ProcessMessage(char *data, uint8_t length);

protected:
  CDataQueue	m_DataQueue;
};

#endif /* xlxInterfaceRF24_h */

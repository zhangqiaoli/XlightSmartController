//  xlxRF24Server.h - Xlight RF2.4 server library

#ifndef xlxRF24Server_h
#define xlxRF24Server_h

#include "MyTransportNRF24.h"

// RF24 Server class
class RF24ServerClass : public MyTransportNRF24
{
public:
  RF24ServerClass(uint8_t ce=RF24_CE_PIN, uint8_t cs=RF24_CS_PIN, uint8_t paLevel=RF24_PA_LEVEL);

  bool ServerBegin();
  uint64_t GetNetworkID();
  void SetRole_Gateway();
  bool ChangeNodeID(const uint8_t bNodeID);
  bool ProcessSend(String &strMsg);
  bool ProcessReceive();
  uint8_t GetNextAvailableNodeId();

  unsigned long _times;
  unsigned long _succ;
  unsigned long _received;
};

//------------------------------------------------------------------
// Function & Class Helper
//------------------------------------------------------------------
extern RF24ServerClass theRadio;

#endif /* xlxRF24Server_h */

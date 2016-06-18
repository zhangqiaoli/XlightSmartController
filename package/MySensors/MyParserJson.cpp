/**
 * MyParserJson.cpp - Xlight extention for MySensors message lib
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
 * Parse Json string into message object
 *
 * ToDo:
 * 1.
**/

#include "MyParser.h"
#include "MyParserJson.h"
#include "ArduinoJson.h"

MyParserJson::MyParserJson() : MyParser() {}

bool MyParserJson::parse(MyMessage &message, char *inputString) {
	const char *str, *p, *value=NULL;
	uint8_t bvalue[MAX_PAYLOAD];
	uint8_t blen = 0;
	int i = 0;
	uint8_t command = 0;
	uint8_t ack = 0;

	// Extract command data coming with the JSON string
  StaticJsonBuffer<MAX_MESSAGE_LENGTH*2> jsonBuf;
  JsonObject& root = jsonBuf.parseObject((char *)inputString);
  if( !root.success() )
    return false;
  if( root.size() < 5 )  // Check for invalid input
    return false;

  message.destination = atoi(root["node-id"]);
  message.sensor = atoi(root["sensor-id"]);
  command = atoi(root["msg-type"]);
  mSetCommand(message, command);
  ack = atoi(root["ack"]);
  message.type = atoi(root["sub-type"]);

  // Payload
  str = root["payload"].asString();
  if (command == C_STREAM) {
    blen = 0;
    uint8_t val;
    while (*str) {
      val = h2i(*str++) << 4;
      val += h2i(*str++);
      bvalue[blen] = val;
      blen++;
    }
  } else {
    value = str;
  }

	message.sender = GATEWAY_ADDRESS;
	message.last = GATEWAY_ADDRESS;
  mSetRequestAck(message, ack?1:0);
  mSetAck(message, false);
	if (command == C_STREAM)
		message.set(bvalue, blen);
	else
		message.set(value);
	return true;
}

uint8_t MyParserJson::h2i(char c) {
	uint8_t i = 0;
	if (c <= '9')
		i += c - '0';
	else if (c >= 'a')
		i += c - 'a' + 10;
	else
		i += c - 'A' + 10;
	return i;
}

/**
 * xlxSerialConsole.cpp - Xlight Device Management Console via Serial Port
 * This module offers monitoring, testing, device control and setting features
 * in both interactive mode and command line mode.
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
 * 1. Instruction output
 * 2. Select mode: Interactive mode or Command line
 * 3. In interactive mode, navigate menu and follow the screen instruction
 * 4. In command line mode, input the command and press enter. Need user manual
 * 5.
 *
 * Command category:
 * - do: execute command or function, e.g. control the lights, send a message, etc.
 * - help: show help information.
 * - ping: ping an IP or domain name
 * - send: send testing message
 * - set: log level, flags, system settings, communication parameters, etc.
 * - show: system info, status, variable, statistics, etc.
 * - sys: system level operation, like reset, recover, safe mode, etc.
 * - test: generate test message or simulate user operation
 *
 * ToDo:
 * 1.
 *
**/

#include "xlxSerialConsole.h"
#include "xlSmartController.h"
#include "xliConfig.h"
#include "xlxConfig.h"
#include "xlxLogger.h"
#include "xlxPanel.h"
#include "xlxRF24Server.h"
#include "xlxASRInterface.h"
#include "xlxBLEInterface.h"

//------------------------------------------------------------------
// the one and only instance of SerialConsoleClass
SerialConsoleClass theConsole;
String gstrWiFi_SSID;
String gstrWiFi_Password;
uint8_t gintWiFi_Auth;
uint8_t gintWiFi_Cipher;

//------------------------------------------------------------------
// Global Callback Function Helper
bool gc_nop(const char *cmd) { return true; }
bool gc_doHelp(const char *cmd) { return theConsole.doHelp(cmd); }
bool gc_doCheck(const char *cmd) { return theConsole.doCheck(cmd); }
bool gc_doShow(const char *cmd) { return theConsole.doShow(cmd); }
bool gc_doPing(const char *cmd) { return theConsole.doPing(cmd); }
bool gc_doAction(const char *cmd) { return theConsole.doAction(cmd); }
bool gc_doTest(const char *cmd) { return theConsole.doTest(cmd); }
bool gc_doSend(const char *cmd) { return theConsole.doSend(cmd); }
bool gc_doSet(const char *cmd) { return theConsole.doSet(cmd); }
bool gc_doSys(const char *cmd) { return theConsole.doSys(cmd); }
bool gc_doSysSub(const char *cmd) { return theConsole.doSysSub(cmd); }
bool gc_doSysSetupWiFi(const char *cmd) { return theConsole.SetupWiFi(cmd); }
bool gc_doSysSetWiFiCredential(const char *cmd) { return theConsole.SetWiFiCredential(cmd); }

//------------------------------------------------------------------
// State Machine
/// State
typedef enum
{
  consoleRoot = 0,
  consoleHelp,
  consoleCheck,
  consoleShow,
  consolePing,
  consoleDo,
  consoleTest,
  consoleSend,
  consoleSet,
  consoleSys,

  consoleSetupWiFi = 51,
  consoleWF_YesNo,
  consoleWF_GetSSID,
  consoleWF_GetPassword,
  consoleWF_GetAUTH,
  consoleWF_Cipher,
  consoleWF_Confirm,

  consoleDummy = 255
} consoleState_t;

/// Matrix: actual definition for command/handler array
const StateMachine_t fsmMain[] = {
  // Current-State    Next-State      Event-String        Function
  {consoleRoot,       consoleRoot,    "?",                gc_doHelp},
  {consoleRoot,       consoleRoot,    "help",             gc_doHelp},
  {consoleRoot,       consoleRoot,    "check",            gc_doCheck},
  {consoleRoot,       consoleRoot,    "show",             gc_doShow},
  {consoleRoot,       consoleRoot,    "ping",             gc_doPing},
  {consoleRoot,       consoleRoot,    "do",               gc_doAction},
  {consoleRoot,       consoleRoot,    "test",             gc_doTest},
  {consoleRoot,       consoleRoot,    "send",             gc_doSend},
  {consoleRoot,       consoleRoot,    "set",              gc_doSet},
  {consoleRoot,       consoleSys,     "sys",              gc_doSys},
  /// Menu default
  {consoleRoot,       consoleRoot,    "",                 gc_doHelp},

  /// Shared function
  {consoleSys,        consoleRoot,    "reset",            gc_doSysSub},
  {consoleSys,        consoleRoot,    "safe",             gc_doSysSub},
  {consoleSys,        consoleRoot,    "dfu",              gc_doSysSub},
  {consoleSys,        consoleRoot,    "update",           gc_doSysSub},
  {consoleSys,        consoleRoot,    "sync",             gc_doSysSub},
  {consoleSys,        consoleRoot,    "clear",            gc_doSysSub},
  {consoleSys,        consoleRoot,    "base",             gc_doSysSub},
  {consoleSys,        consoleRoot,    "private",          gc_doSysSub},
  {consoleSys,        consoleRoot,    "serial",           gc_doSysSub},
  /// Workflow
  {consoleSys,        consoleWF_YesNo,   "setup",         gc_doSysSetupWiFi},
  /// Menu default
  {consoleSys,        consoleRoot,    "",                 gc_doHelp},

  {consoleWF_YesNo,   consoleWF_GetSSID, "yes",           gc_doSysSetupWiFi},
  {consoleWF_YesNo,   consoleWF_GetSSID, "y",             gc_doSysSetupWiFi},
  {consoleWF_YesNo,   consoleRoot,    "no",               gc_nop},
  {consoleWF_YesNo,   consoleRoot,    "n",                gc_nop},
  {consoleWF_YesNo,   consoleRoot,    "",                 gc_nop},
  {consoleWF_GetSSID, consoleWF_GetPassword, "",          gc_doSysSetupWiFi},
  {consoleWF_GetPassword, consoleWF_GetAUTH, "",          gc_doSysSetupWiFi},
  {consoleWF_GetAUTH, consoleWF_Cipher,  "0",             gc_doSysSetupWiFi},
  {consoleWF_GetAUTH, consoleWF_Cipher,  "1",             gc_doSysSetupWiFi},
  {consoleWF_GetAUTH, consoleWF_Cipher,  "2",             gc_doSysSetupWiFi},
  {consoleWF_GetAUTH, consoleWF_Cipher,  "3",             gc_doSysSetupWiFi},
  {consoleWF_GetAUTH, consoleRoot,    "q",                gc_nop},
  {consoleWF_Cipher,  consoleWF_Confirm, "0",             gc_doSysSetupWiFi},
  {consoleWF_Cipher,  consoleWF_Confirm, "1",             gc_doSysSetupWiFi},
  {consoleWF_Cipher,  consoleWF_Confirm, "2",             gc_doSysSetupWiFi},
  {consoleWF_Cipher,  consoleWF_Confirm, "3",             gc_doSysSetupWiFi},
  {consoleWF_Cipher,  consoleRoot,    "q",                gc_nop},
  {consoleWF_Confirm, consoleRoot,    "yes",              gc_doSysSetWiFiCredential},
  {consoleWF_Confirm, consoleRoot,    "y",                gc_doSysSetWiFiCredential},
  {consoleWF_Confirm, consoleRoot,    "no",               gc_nop},
  {consoleWF_Confirm, consoleRoot,    "n",                gc_nop},
  {consoleWF_Confirm, consoleRoot,    "",                 gc_nop},

  /// System default
  {consoleDummy,      consoleRoot,    "",                 gc_doHelp}
};

const char *strAuthMethods[4] = {"None", "WEP", "WPA", "WPA2"};
const char *strCipherMethods[4] = {"None", "AES", "TKIP", "AES_TKIP"};

//------------------------------------------------------------------
// Xlight SerialConsole Class
//------------------------------------------------------------------
SerialConsoleClass::SerialConsoleClass()
{
  isInCloudCommand = false;
}

void SerialConsoleClass::Init()
{
  SetStateMachine(fsmMain, sizeof(fsmMain) / sizeof(StateMachine_t), consoleRoot);
  addDefaultHandler(NULL);    // Use virtual callback function
}

bool SerialConsoleClass::processCommand()
{
  bool retVal = readSerial();
  if( !retVal ) {
    SERIAL_LN("Unknown command or incorrect arguments\n\r");
  }

  return retVal;
}

bool SerialConsoleClass::callbackCommand(const char *cmd)
{
  IF_SERIAL_DEBUG(SERIAL_LN("do callback %s", cmd));

  return true;
}

bool SerialConsoleClass::callbackDefault(const char *cmd)
{
  return doHelp(cmd);
}

//--------------------------------------------------
// Command Functions
//--------------------------------------------------
bool SerialConsoleClass::showThisHelp(String &strTopic)
{
#ifndef SYS_RELEASE

  if(strTopic.equals("check")) {
    SERIAL_LN("--- Command: check <object> ---");
    SERIAL_LN("To check component status, where <object> could be:");
    SERIAL_LN("   ble:   check BLE module availability");
    SERIAL_LN("   flash: check flash space");
    SERIAL_LN("   rf:    check RF availability");
    SERIAL_LN("   wifi:  check Wi-Fi module status");
    SERIAL_LN("   wlan:  check internet status");
    SERIAL_LN("e.g. check rf\n\r");
    //CloudOutput("check ble|flash|rf|wifi|wlan");
  } else if(strTopic.equals("show")) {
    SERIAL_LN("--- Command: show <object> ---");
    SERIAL_LN("To show value or summary information, where <object> could be:");
    SERIAL_LN("   ble:     show BLE summary");
    SERIAL_LN("   debug:   show debug channel and level");
    SERIAL_LN("   flag:    show system flags");
    SERIAL_LN("   net:     show network summary");
    SERIAL_LN("   node:    show node summary");
    SERIAL_LN("   button:  show button (knob) status");
    SERIAL_LN("   nlist:   show NodeID list");
    SERIAL_LN("   rf:      print RF details");
    SERIAL_LN("   time:    show current time and time zone");
    SERIAL_LN("   var:     show system variables");
    SERIAL_LN("   table:   show working memory tables");
    SERIAL_LN("   device:  show functional devices");
    SERIAL_LN("   remote:  show remotes");
    SERIAL_LN("   asrsnt:  show ASR command scenario table");
    SERIAL_LN("   keymap:  show hardware key map table");
    SERIAL_LN("   extbtn:  show extended button table");
    SERIAL_LN("   version: show firmware version");
    SERIAL_LN("e.g. show rf\n\r");
    //CloudOutput("show ble|debug|dev|flag|net|node|rf|time|var|table|version");
  } else if(strTopic.equals("ping")) {
    SERIAL_LN("--- Command: ping <address> ---");
    SERIAL_LN("To ping an IP or domain name, default address is 8.8.8.8");
#if XLIGHT_EDITION_ID == XLIGHT_CLASSROOM_EDITION
    SERIAL_LN("e.g. ping www.baidu.com");
#else
	SERIAL_LN("e.g. ping www.google.com");
#endif
    SERIAL_LN("e.g. ping 192.168.0.1\n\r");
    //CloudOutput("ping <address>");
  } else if(strTopic.equals("do")) {
    SERIAL_LN("--- Command: do <action parameters> ---");
    SERIAL_LN("To execute action, e.g. turn on the lamp");
    SERIAL_LN("e.g. do on");
    SERIAL_LN("e.g. do off");
    SERIAL_LN("e.g. do color R,G,B\n\r");
    //CloudOutput("do on|off|color");
  } else if(strTopic.equals("test")) {
    SERIAL_LN("--- Command: test <action parameters> ---");
    SERIAL_LN("To perform testing, where <action> could be:");
    SERIAL_LN("   ledring [0:5]: check brightness LED indicator");
    SERIAL_LN("   ledrgb: check status RGB LED");
    SERIAL_LN("   ping <ip address>: ping ip address");
    SERIAL_LN("   send <NodeId:MessageId[:Payload]>: send test message to node");
    SERIAL_LN("   send <message>: send MySensors format message");
    SERIAL_LN("   keymap <key> <0:1>: set relay key on/off");
    SERIAL_LN("   ble <message>: send message via BLE");
    SERIAL_LN("   asr <cmd>: send command to ASR module\n\r");
    //CloudOutput("test ping|send|ble|asr");
  } else if(strTopic.equals("send")) {
    SERIAL_LN("--- Command: send <message> or <NodeId:MessageId[:Payload]> ---");
    SERIAL_LN("To send testing message");
    SERIAL_LN("e.g. send 0:1");
    SERIAL_LN("e.g. send 0;1;0;0;6;");
    SERIAL_LN("e.g. send 0;1;1;0;0;23.5\n\r");
    //CloudOutput("send <msg> or <NodeId:MsgId[:Pload]>");
  } else if(strTopic.equals("set")) {
    char *sObj = next();
    if( sObj ) {
      if (wal_strnicmp(sObj, "flag", 4) == 0) {
        SERIAL_LN("--- Command: set flag <flag name> [0|1] ---");
        SERIAL_LN("<flag name>: csc, cdts, fnid, hwsw");
        SERIAL_LN("e.g. set flag csc [0|1]");
        SERIAL_LN("     , enable or disable Cloud Serial Command");
        SERIAL_LN("e.g. set flag cdts [0|1]");
        SERIAL_LN("     , enable or disable Cloud Daily Time Sync");
        SERIAL_LN("e.g. set flag fnid [0|1]");
        SERIAL_LN("     , enable or disable Fixed NodeID");
        SERIAL_LN("e.g. set flag hwsw [0|1]");
        SERIAL_LN("     , default use hardware switch");
        //CloudOutput("set flag csc|cdts|fnid|hwsw");
      } else if (wal_strnicmp(sObj, "var", 3) == 0) {
        SERIAL_LN("--- Command: set var <var name> <value> ---");
        SERIAL_LN("<var name>: senmap, devst, bmrt, nmrt, rfch, rfpl, rfdr");
        SERIAL_LN("e.g. set var senmap 23");
        SERIAL_LN("     , set Sensor Bitmap to 0x17");
        SERIAL_LN("e.g. set var devst 5");
        SERIAL_LN("     , set Device Status to sleep mode");
        SERIAL_LN("e.g. set var bmrt [0..15]");
        SERIAL_LN("     , set Bcast Msg Repeat Times");
        SERIAL_LN("e.g. set var nmrt [0..15]");
        SERIAL_LN("     , set Node Msg Repeat Times");
        SERIAL_LN("e.g. set var rfch [0..127]");
        SERIAL_LN("     , set RF Channel");
        SERIAL_LN("e.g. set var rfpl [0..3]");
        SERIAL_LN("     , set RF Power Level to min(0), low(1), high(2) or max(3)");
        SERIAL_LN("e.g. set var rfdr [0..2]");
        SERIAL_LN("     , set RF Datarate to 1MBPS(0), 2MBPS(1) or 250KBPS(2)");
        //CloudOutput("set var senmap|devst|rfch|rfpl|rfdr");
      } else if (wal_strnicmp(sObj, "spkr", 4) == 0) {
        SERIAL_LN("--- Command: set spkr [0|1] ---");
        SERIAL_LN("To disable or enable speaker");
        //CloudOutput("set spkr 0|1");
      } else if (wal_strnicmp(sObj, "cloud", 5) == 0) {
        SERIAL_LN("--- Command: set cloud [0|1|2] ---");
        SERIAL_LN("To disable, enable or require cloud");
        //CloudOutput("set cloud 0|1|2");
      }
    } else {
      SERIAL_LN("--- Command: set <object value> ---");
      SERIAL_LN("To change config");
      SERIAL_LN("e.g. set tz -5");
      SERIAL_LN("e.g. set dst [0|1]");
      SERIAL_LN("     , set daylight saving time");
      SERIAL_LN("e.g. set time sync");
      SERIAL_LN("     , synchronize time with Cloud");
      SERIAL_LN("e.g. set time hh:mm:ss");
      SERIAL_LN("e.g. set date YYYY-MM-DD");
      SERIAL_LN("e.g. set nodeid [0..250]");
      SERIAL_LN("e.g. set base [0|1]");
      SERIAL_LN("     , to enable or disable base network");
      SERIAL_LN("e.g. set maxebn <duration>");
      SERIAL_LN("     , to set maximum base network enable duration");
      SERIAL_LN("e.g. set spkr [0|1]");
      SERIAL_LN("     , to enable or disable speaker");
      SERIAL_LN("set flag <flag name> [0|1]");
      SERIAL_LN("     , to set system flag value, use '? set flag' for detail");
      SERIAL_LN("set var <var name> <value>");
      SERIAL_LN("     , to set value of variable, use '? set var' for detail");
      SERIAL_LN("e.g. set cloud [0|1|2]");
      SERIAL_LN("     , cloud option disable|enable|must");
      SERIAL_LN("e.g. set maindev <nodeid>");
      SERIAL_LN("     , to change the main device");
      SERIAL_LN("e.g. set subid <subNID>");
      SERIAL_LN("     , to change the sub NID");
      SERIAL_LN("e.g. set remote <nodeid device>");
      SERIAL_LN("     , to assign device to remote");
      SERIAL_LN("e.g. set blename <BLEName>");
      SERIAL_LN("     , to change BLE SSID");
      SERIAL_LN("e.g. set blepin <BLEPin>");
      SERIAL_LN("     , to change BLE Pin");
      SERIAL_LN("e.g. set loopkc [0..%d]", MAX_KEY_MAP_ITEMS);
      SERIAL_LN("     , to set button loop keycode");
      SERIAL_LN("e.g. set kcto [0..255]");
      SERIAL_LN("     , to set loop keycode timeout");
      SERIAL_LN("e.g. set hwsobj [0|1|2]");
      SERIAL_LN("     , to set hardware switch object type");
      SERIAL_LN("e.g. set pptpin <PPTPin>");
      SERIAL_LN("     , to change PPT access code");
      SERIAL_LN("e.g. set asrsnt <code scenario_id>");
      SERIAL_LN("     , to set scenario for ASR command");
      SERIAL_LN("e.g. set keymap <key nid subID>");
      SERIAL_LN("     , to set keymap item");
      SERIAL_LN("e.g. set extbtn <button operation action keymap>");
      SERIAL_LN("     , to define extbtn action");
      SERIAL_LN("e.g. set debug [log:level]");
      SERIAL_LN("     , where log is [serial|flash|syslog|cloud|all");
      SERIAL_LN("     and level is [none|alter|critical|error|warn|notice|info|debug]\n\r");
      //CloudOutput("set tz|dst|nodeid|base|spkr|flag|var|cloud|maindev|subid|debug|blename|blepin");
    }
  } else if(strTopic.equals("sys")) {
    SERIAL_LN("--- Command: sys <mode> ---");
    SERIAL_LN("To control the system status, where <mode> could be:");
    SERIAL_LN("   base <duration>: switch to base network and accept new device for <duration=60> seconds");
    SERIAL_LN("   private: switch to private network");
    SERIAL_LN("   reset:   reset the system");
    SERIAL_LN("   safe:    enter safe/recover mode");
    SERIAL_LN("   setup:   setup Wi-Fi crediential");
    SERIAL_LN("   dfu:     enter DFU mode");
    SERIAL_LN("   listen:  enter listening mode");
    SERIAL_LN("   update:  update firmware");
    SERIAL_LN("   serial reset: reset serial port");
    SERIAL_LN("   sync <object>: object synchronize with Cloud");
    SERIAL_LN("   clear <object>: clear object, such as nodeid");
    SERIAL_LN("e.g. sys sync time");
    SERIAL_LN("e.g. sys clear nodeid 1");
    SERIAL_LN("e.g. sys clear credentials");
    SERIAL_LN("e.g. sys reset\n\r");
    //CloudOutput("sys base|private|reset|safe|setup|dfu|update|serial|sync|clear");
  } else {
    SERIAL_LN("Available Commands:");
    SERIAL_LN("    check, show, ping, do, test, send, set, sys, help or ?");
    SERIAL_LN("Use 'help <command>' for more information\n\r");
    //CloudOutput("check, show, ping, do, test, send, set, sys, help or ?");
  }
#else
  SERIAL_LN("Available Commands:");
  SERIAL_LN("    check, show, ping, do, test, send, set, sys, help or ?\n\r");
#endif

  return true;
}

bool SerialConsoleClass::doHelp(const char *cmd)
{
  IF_SERIAL_DEBUG(SERIAL_LN("doHelp(%s)\n\r", cmd));

  String strTopic = "";
  char *sTopic = next();
  if( sTopic ) {
    // Get Menu Position according to Topic
    strTopic = sTopic;
  } else if(cmd) {
    strTopic = cmd;
  } else {
    sTopic = first();
    if( sTopic )
      strTopic = sTopic;
  }
  strTopic.toLowerCase();

  return showThisHelp(strTopic);
}

// check - Check commands
bool SerialConsoleClass::doCheck(const char *cmd)
{
  IF_SERIAL_DEBUG(SERIAL_LN("doCheck(%s)\n\r", cmd));

  bool retVal = true;

  char *sTopic = next();
  if( sTopic ) {
    if (wal_strnicmp(sTopic, "rf", 2) == 0) {
      SERIAL_LN("**RF module is %s Received %lu", theRadio.isValid() ? "available." : "not available!", theRadio._received);
      float succ_r = 0;
      if( theRadio._times > 0 ) {
        succ_r = (float)theRadio._succ * 100 / theRadio._times;
        SERIAL_LN("  Sent %lu out of %lu, Succ-rate %.2f%%",
            theRadio._succ, theRadio._times, succ_r);
      }
      CloudOutput("c_rf:%d, succ_r:%.2f", theRadio.isValid(), succ_r);
    } else if (wal_strnicmp(sTopic, "wifi", 4) == 0) {
      if( !theConfig.GetDisableWiFi() ) {
        SERIAL("**Wi-Fi module is %s, ", (WiFi.ready() ? "ready" : "not ready!"));
        int lv_RSSI = WiFi.RSSI();
        if( lv_RSSI < 0 ) {
          SERIAL_LN("RSSI=%ddB\n\r", WiFi.RSSI());
        } else if( lv_RSSI == 1 ) {
          SERIAL_LN("Wi-Fi chip error\n\r");
        } else {
          SERIAL_LN("time-out\n\r");
        }
        CloudOutput("c_wifi:%d-%ddB", WiFi.ready(), lv_RSSI);
      } else {
        SERIAL("**Wi-Fi module is disabled\n\r");
      }
    } else if (wal_strnicmp(sTopic, "wlan", 4) == 0) {
      if( !theConfig.GetDisableWiFi() ) {
#if XLIGHT_EDITION_ID == XLIGHT_CLASSROOM_EDITION
		  SERIAL_LN("**Resolving IP for www.baidu.com...%s\n\r", (WiFi.resolve("www.baidu.com") ? "OK" : "failed!"));
#else
		  SERIAL_LN("**Resolving IP for www.google.com...%s\n\r", (WiFi.resolve("www.google.com") ? "OK" : "failed!"));
#endif
        
      } else {
        SERIAL("**Wi-Fi module is disabled\n\r");
      }
      //CloudOutput("c_wlan:1");
    } else if (wal_strnicmp(sTopic, "flash", 5) == 0) {
      SERIAL_LN("** Free memory: %lu bytes, total EEPROM space: %lu bytes\n\r", System.freeMemory(), EEPROM.length());
      CloudOutput("c_flash:%lu-%lu", System.freeMemory(), EEPROM.length());
    } else if (wal_strnicmp(sTopic, "ble", 3) == 0) {
      // Send test command and check received message
#ifndef DISABLE_BLE
      theBLE.config();
#else
      SERIAL_LN("** Not support BLE Module on this device **");
      CloudOutput("No BLE module");
#endif
      //SERIAL_LN("** BLE Module is %s **", theBLE.isGood() ? "good" : "error");
    } else {
      retVal = false;
    }
  } else {
    retVal = false;
  }

  return retVal;
}

// show - Show commands: config, status, variable, statistic data, etc.
bool SerialConsoleClass::doShow(const char *cmd)
{
  IF_SERIAL_DEBUG(SERIAL_LN("doShow(%s)\n\r", cmd));

  bool retVal = true;
  char strDisplay[64];

  char *sTopic = next();
  if( sTopic ) {
    if (wal_strnicmp(sTopic, "net", 3) == 0) {
      SERIAL_LN("** Network Summary **");
      SERIAL_LN("  Current RF NetworkID: %s", PrintUint64(strDisplay, theRadio.getCurrentNetworkID()));
      SERIAL_LN("  Private RF NetworkID: %s", PrintUint64(strDisplay, theRadio.getMyNetworkID()));
      SERIAL_LN("  Base RF Network is %s", (theRadio.isBaseNetworkEnabled() ? "enabled" : "disabled"));
      uint8_t mac[6];
      //WiFi.macAddress(mac);
	    theSys.GetMac(mac);
      SERIAL_LN("  MAC address: %s", PrintMacAddress(strDisplay, mac));
      if( !theConfig.GetDisableWiFi() ) {
        if( WiFi.ready() ) {
          SERIAL("  IP Address: ");
          TheSerial.println(WiFi.localIP());
          SERIAL("  Subnet Mask: ");
          TheSerial.println(WiFi.subnetMask());
          SERIAL("  Gateway IP: ");
          TheSerial.println(WiFi.gatewayIP());
          SERIAL_LN("  SSID: %s", WiFi.SSID());
          SERIAL_LN("  Cloud %s", Particle.connected() ? "connected" : "disconnected");
        }
      }
      SERIAL_LN("");
      //CloudOutput("RF NetworkID %s, MAC %s, SSID %s",
      //    PrintUint64(strDisplay, theRadio.getCurrentNetworkID()),
      //    PrintMacAddress(strDisplay, mac),
      //    WiFi.SSID());
	  } else if (wal_strnicmp(sTopic, "node", 4) == 0) {
      uint8_t lv_NodeID = theRadio.getAddress();
      SERIAL_LN("**NodeID: %d (%s), Status: %d", lv_NodeID, (lv_NodeID==GATEWAY_ADDRESS ? "Gateway" : (lv_NodeID==AUTO ? "AUTO" : "Node")), theSys.GetStatus());
      SERIAL_LN("  Product Info: %s-%s-%d", theConfig.GetOrganization().c_str(), theConfig.GetProductName().c_str(), theConfig.GetVersion());
      SERIAL_LN("  System Info: %s-%s\n\r", theSys.GetSysID().c_str(), theSys.GetSysVersion().c_str());
      CloudOutput("s_node:%d-%d", lv_NodeID, theSys.GetStatus());
    } else if (wal_strnicmp(sTopic, "button", 6) == 0) {
      SERIAL_LN("Knob status - Dimmer:%d, Button:%d, CCT Flag:%d\n\r",  thePanel.GetDimmerValue(), thePanel.GetButtonStatus(), thePanel.GetCCTFlag());
      CloudOutput("s_button:%d-%d-%d", thePanel.GetDimmerValue(), thePanel.GetButtonStatus(), thePanel.GetCCTFlag());
    } else if (wal_strnicmp(sTopic, "nlist", 5) == 0) {
      SERIAL_LN("**Node List count:%d, size:%d", theConfig.lstNodes.count(), theConfig.lstNodes.size());
      theConfig.lstNodes.showList();
      CloudOutput("s_nlist:%d-%d", theConfig.lstNodes.count(), theConfig.lstNodes.size());
    } else if (wal_strnicmp(sTopic, "asrsnt", 6) == 0) {
      theConfig.showASRSNT();
    } else if (wal_strnicmp(sTopic, "keymap", 6) == 0) {
      SERIAL_LN("HW switch object type: %d, loopkc: %d", theConfig.GetRelayKeyObj(), theSys.GetLoopKeyCode());
      theConfig.showKeyMap();
    } else if (wal_strnicmp(sTopic, "extbtn", 6) == 0) {
      theConfig.showButtonActions();
  	} else if (wal_strnicmp(sTopic, "rf", 2) == 0) {
      theRadio.PrintRFDetails();
      SERIAL_LN("");
    } else if (wal_strnicmp(sTopic, "ble", 3) == 0) {
#ifndef DISABLE_BLE
      SERIAL_LN("** BLE Module is %s **", theBLE.isGood() ? "good" : "error");
      SERIAL_LN("  Name:%s, PIN:%s", theBLE.getName().c_str(), theBLE.getPin().c_str());
      SERIAL_LN("  Speed: %d, State: %d, AT Mode: %d", theBLE.getSpeed(), theBLE.getState(), theBLE.isATMode());
      CloudOutput("s_ble:%d-%s-%d-%d", theBLE.isGood(), theBLE.getName().c_str(), theBLE.getState(), theBLE.getSpeed());
#else
      SERIAL_LN("** Not support BLE Module on this device **");
      CloudOutput("No BLE module");
#endif
  	} else if (wal_strnicmp(sTopic, "time", 4) == 0) {
      time_t time = Time.now();
      SERIAL_LN("Now is %s, %s\n\r", Time.format(time, TIME_FORMAT_ISO8601_FULL).c_str(), theSys.m_tzString.c_str());
      CloudOutput("s_time:%s-%s", Time.format(time, TIME_FORMAT_ISO8601_FULL).c_str(), theSys.m_tzString.c_str());
  	} else if (wal_strnicmp(sTopic, "var", 3) == 0) {
  		SERIAL_LN("mSysID = \t\t\t%s", theSys.m_SysID.c_str());
  		SERIAL_LN("m_SysStatus = \t\t\t%d", theSys.m_SysStatus);
      SERIAL_LN("useCloud = \t\t\t%d", theConfig.GetUseCloud());
  		SERIAL_LN("m_tzString = \t\t\t%s", theSys.m_tzString.c_str());
      SERIAL_LN("m_strCldCmd = \t\t%s\n\r", theSys.m_strCldCmd.c_str());
  		SERIAL_LN("m_lastMsg = \t\t\t%s", theSys.m_lastMsg.c_str());
      SERIAL_LN("");
      SERIAL_LN("sensorBitmap = \t\t\t0x%04X", theConfig.GetSensorBitmap());
      SERIAL_LN("indBrightness = \t\t%d", theConfig.GetBrightIndicator());
      SERIAL_LN("relay_keys = \t\t\t0x%02X", theConfig.GetRelayKeys());
  		//SERIAL_LN("rfPowerLevel = \t\t\t%d", theConfig.GetRFPowerLevel());
      //SERIAL_LN("m_temperature = \t\t%.2f", theSys.m_temperature);
  		//SERIAL_LN("m_humidity = \t\t\t%.2f", theSys.m_humidity);
  		//SERIAL_LN("m_brightness = \t\t\t%u", theSys.m_brightness);
      //SERIAL_LN("m_dust = \t\t\t%d", theSys.m_dust);
  		//SERIAL_LN("m_motion = \t\t\t%s", (theSys.m_motion ? "true" : "false"));
      SERIAL_LN("");
      SERIAL_LN("Main DeviceID = \t\t%d-%d", CURRENT_DEVICE, CURRENT_SUBDEVICE);
      SERIAL_LN("typeMainDevice = \t\t%d", theConfig.GetMainDeviceType());
      SERIAL_LN("numDevices = \t\t\t%d", theConfig.GetNumDevices());
      SERIAL_LN("numNodes = \t\t\t%d", theConfig.GetNumNodes());
      SERIAL_LN("maxBaseNetworkDuration = \t%d", theConfig.GetMaxBaseNetworkDur());
      SERIAL_LN("bmrt =   \t\t\t%d", theConfig.GetBcMsgRptTimes());
      SERIAL_LN("nmrt =   \t\t\t%d", theConfig.GetNdMsgRptTimes());
      SERIAL_LN("loopkc = \t\t\t%d", theSys.GetLoopKeyCode());
      SERIAL_LN("loop kcto = \t\t\t%d", theConfig.GetTimeLoopKC());
      SERIAL_LN("hwsObj = \t\t\t%d", theConfig.GetRelayKeyObj());
      SERIAL_LN("PPT Pin = %s\n\r", theConfig.GetPPTAccessCode().c_str());
    } else if (wal_strnicmp(sTopic, "flag", 4) == 0) {
      SERIAL_LN("WAN Chip: \t\t\t%s", theConfig.GetDisableWiFi() ? "disabled" : "enabled");
  		SERIAL_LN("m_isRF = \t\t\t%d", theSys.IsRFGood());
  		SERIAL_LN("m_isBLE = \t\t\t%d", theSys.IsBLEGood());
  		SERIAL_LN("m_isLAN = \t\t\t%d", theSys.IsLANGood());
  		SERIAL_LN("m_isWAN = \t\t\t%d", theSys.IsWANGood());
		SERIAL_LN("lamp Chip: \t\t\t%s", theConfig.GetDisableLamp() ? "disabled" : "enabled");
      SERIAL_LN("");
      SERIAL_LN("fixedNodeID = \t\t\t%d", theConfig.IsFixedNID());
      SERIAL_LN("enableCloudSerialCmd = \t\t%d", theConfig.IsCloudSerialEnabled());
      SERIAL_LN("enableDailyTimeSync = \t\t%d", theConfig.IsDailyTimeSyncEnabled());
      SERIAL_LN("enableSpeaker = \t\t%d", theConfig.IsSpeakerEnabled());
      SERIAL_LN("bBaseNetworkEnabled = \t\t%d", theRadio.isBaseNetworkEnabled());
      SERIAL_LN("stWiFi = \t\t\t%d", theConfig.GetWiFiStatus());
      SERIAL_LN("hwsw =   \t\t\t%d", theConfig.GetHardwareSwitch());
      SERIAL_LN("");
  		SERIAL_LN("m_isLoaded = \t\t\t%d", theConfig.IsConfigLoaded());
  		SERIAL_LN("m_isChanged = \t\t\t%d", theConfig.IsConfigChanged());
  		SERIAL_LN("m_isDSTChanged = \t\t%d", theConfig.IsDSTChanged());
  		SERIAL_LN("m_isSCTChanged = \t\t%d", theConfig.IsSCTChanged());
  		SERIAL_LN("m_isRTChanged = \t\t%d", theConfig.IsRTChanged());
  		SERIAL_LN("m_isSNTChanged = \t\t%d", theConfig.IsSNTChanged());
      SERIAL_LN("IsNIDChanged = \t\t\t%d\n\r", theConfig.IsNIDChanged());
  	} else if (wal_strnicmp(sTopic, "table", 5) == 0) {
      SERIAL_LN("CONFIG_SIZE: \t\t\t\t%u", sizeof(Config_t));
  		SERIAL_LN("DST_ROW_SIZE: \t\t\t\t%u", DST_ROW_SIZE);
  		SERIAL_LN("RT_ROW_SIZE: \t\t\t\t%u", RT_ROW_SIZE);
  		SERIAL_LN("SCT_ROW_SIZE: \t\t\t\t%u", SCT_ROW_SIZE);
  		SERIAL_LN("MAX_SCT_ROWS: \t\t\t\t%d", MAX_SCT_ROWS);
  		SERIAL_LN("SNT_ROW_SIZE: \t\t\t\t%u", SNT_ROW_SIZE);

  		SERIAL_LN("");
      SERIAL_LN("DevStatus_table %d items:", theSys.DevStatus_table.size());
      for (int i = 0; i < theSys.DevStatus_table.size(); i++)
        theSys.print_devStatus_table(i);
      SERIAL_LN("");
      SERIAL_LN("Rule_table %d items:", theSys.Rule_table.size());
      for (int i = 0; i < theSys.Rule_table.size(); i++)
        theSys.print_rule_table(i);
      SERIAL_LN("");
      SERIAL_LN("Schedule_table %d items:", theSys.Schedule_table.size());
      for (int i = 0; i < theSys.Schedule_table.size(); i++)
        theSys.print_schedule_table(i);
      SERIAL_LN("");
      SERIAL_LN("Scenario_table %d items:", theSys.Scenario_table.size());
      for (int i = 0; i < theSys.Scenario_table.size(); i++)
        theSys.print_scenario_table(i);
      SERIAL_LN("");
    } else if (wal_strnicmp(sTopic, "device", 6) == 0) {
      SERIAL_LN("DST_ROW_SIZE: \t\t\t\t%u, items: %d", DST_ROW_SIZE, theSys.DevStatus_table.size());
      SERIAL_LN("DevStatus_table:");
      CloudOutput(theSys.print_devStatus_table(0));
  		for (int i = 1; i < theSys.DevStatus_table.size(); i++) {
  			theSys.print_devStatus_table(i);
      }
      SERIAL_LN("");
    } else if (wal_strnicmp(sTopic, "remote", 6) == 0) {
      SERIAL("Main remote: %d type: %d", theConfig.m_stMainRemote.node_id, theConfig.m_stMainRemote.type);
      SERIAL_LN(" %s token: %d", (theConfig.m_stMainRemote.present ? "present" : "not present"), theConfig.m_stMainRemote.token);
      SERIAL_LN("");
    } else if (wal_strnicmp(sTopic, "version", 7) == 0) {
      SERIAL_LN("System  Version: %s", System.version().c_str());
      SERIAL_LN("Product Version: %d\n\r", theConfig.GetVersion());
      CloudOutput("s_version:%s-%d", System.version().c_str(), theConfig.GetVersion());
  	} else if (wal_strnicmp(sTopic, "debug", 5) == 0) {
      CloudOutput(theLog.PrintDestInfo());
  	} else {
      retVal = false;
    }
  } else {
    retVal = false;
  }

  return retVal;
}

// ping - ping IP address or hostname
bool SerialConsoleClass::doPing(const char *cmd)
{
  char *sIPaddress = next();
  PingAddress(sIPaddress);

  return true;
}

// do - execute action, e.g. turn on the lamp
bool SerialConsoleClass::doAction(const char *cmd)
{
  IF_SERIAL_DEBUG(SERIAL_LN("doAction(%s)\n\r", cmd));

  bool retVal = false;

  char *sTopic = next();
  if( sTopic ) {
    if (wal_strnicmp(sTopic, "on", 2) == 0) {
      SERIAL_LN("**Light is ON\n\r");
      theSys.DevSoftSwitch(DEVICE_SW_ON);
      retVal = true;
    } else if (wal_strnicmp(sTopic, "off", 3) == 0) {
      SERIAL_LN("**Light is OFF\n\r");
      theSys.DevSoftSwitch(DEVICE_SW_OFF);
      retVal = true;
    } else if (wal_strnicmp(sTopic, "color", 5) == 0) {
      // ToDo:
      SERIAL_LN("**Color changed\n\r");
      retVal = true;
    }
  }

  return retVal;
}

// test - Test commands, e.g. ping
bool SerialConsoleClass::doTest(const char *cmd)
{
  IF_SERIAL_DEBUG(SERIAL_LN("doTest(%s)\n\r", cmd));

  bool retVal = false;

  char *sTopic = next();
  char *sParam, *sParam1;
  if( sTopic ) {
    if (wal_strnicmp(sTopic, "ping", 4) == 0) {
      char *sIPaddress = next();
      PingAddress(sIPaddress);
      retVal = true;
    } else if (wal_strnicmp(sTopic, "ledring", 7) == 0) {
      UC testNo = 0;
      sParam = next();
      if( sParam) { testNo = (UC)atoi(sParam); }
      SERIAL("Checking brightness indicator LEDs...");
      SERIAL_LN("%s\n\r", thePanel.CheckLEDRing(testNo) ? "done" : "error");
      retVal = true;
    } else if (wal_strnicmp(sTopic, "ledrgb", 6) == 0) {
      // ToDo:
    } else if (wal_strnicmp(sTopic, "send", 4) == 0) {
      sParam = next();
      if( strlen(sParam) >= 3 ) {
        String strMsg = sParam;
        theRadio.ProcessSend(strMsg);
        retVal = true;
      }
#ifndef DISABLE_ASR
    } else if (wal_strnicmp(sTopic, "asr", 3) == 0) {
      sParam = next();
      if( strlen(sParam) > 0 ) {
        SERIAL("\n\r");
        theASR.sendCommand(atoi(sParam));
        retVal = true;
      }
#endif
    } else if (wal_strnicmp(sTopic, "keymap", 6) == 0) {
      sParam = next();
      if( sParam ) {
        UC keyID = atoi(sParam);
        sParam1 = next();
        if( sParam1 ) {
          UC _sw = atoi(sParam1);
          SERIAL_LN("Turn keymap %d to %d\n\r", keyID, _sw);
          theSys.DevHardSwitch(keyID, _sw);
          retVal = true;
        }
      }
#ifndef DISABLE_BLE
    } else if (wal_strnicmp(sTopic, "ble", 3) == 0) {
      sParam = next();
      if( strlen(sParam) >= 3 ) {
        String strMsg = sParam;
        if( strMsg.charAt(strMsg.length()-1) != '\n') {
          strMsg += '\n';
        }
        theBLE.sendCommand(strMsg);
        retVal = true;
      }
#endif
    }
  }

  return retVal;
}

// send - Send message, shortcut of test send
bool SerialConsoleClass::doSend(const char *cmd)
{
  IF_SERIAL_DEBUG(SERIAL_LN("doSend(%s)\n\r", cmd));

  bool retVal = false;

  char *sParam = next();
  if( strlen(sParam) >= 3 ) {
    String strMsg = sParam;
    theRadio.ProcessSend(strMsg);
    retVal = true;
  }

  return retVal;
}

// set - Config command
bool SerialConsoleClass::doSet(const char *cmd)
{
  IF_SERIAL_DEBUG(SERIAL_LN("doSet(%s)\n\r", cmd));

  bool retVal = false;

  char *sTopic = next();
  char *sParam1, *sParam2, *sParam3, *sParam4;
  if( sTopic ) {
    if (wal_strnicmp(sTopic, "tz", 2) == 0) {
      sParam1 = next();
      if( sParam1) {
        float fltTmp = atof(sParam1);
        if( theConfig.SetTimeZoneOffset((SHORT)(fltTmp * 60)) ) {
          SERIAL_LN("Set Time Zone to %f\n\r", fltTmp);
          CloudOutput("tz:%f", fltTmp);
        } else {
          SERIAL_LN("Failed to SetTimeZone:%s\n\r", sParam1);
          CloudOutput("Failed to SetTimeZone:%s", sParam1);
        }
        retVal = true;
      }
    } else if (wal_strnicmp(sTopic, "dst", 3) == 0) {
      sParam1 = next();
      if( atoi(sParam1) == 0 ) {
        theConfig.SetDaylightSaving(0);
        SERIAL_LN("Unset Daylight Saving Time\n\r");
        CloudOutput("dst:0");
      } else {
        theConfig.SetDaylightSaving(1);
        SERIAL_LN("Set Daylight Saving Time\n\r");
        CloudOutput("dst:1");
      }
      retVal = true;
    } else if (wal_strnicmp(sTopic, "time", 4) == 0) {
      sParam1 = next();
      String strTemp = sParam1;
      retVal = (theSys.CldSetCurrentTime(strTemp) == 0);
    } else if (wal_strnicmp(sTopic, "date", 4) == 0) {
      sParam1 = next();
      String strTemp = sParam1;
      retVal = (theSys.CldSetCurrentTime(strTemp) == 0);
    } else if (wal_strnicmp(sTopic, "nodeid", 6) == 0) {
      sParam1 = next();
      if( sParam1) {
        uint8_t bNodeID = (uint8_t)(atoi(sParam1) % 256);
        retVal = theRadio.ChangeNodeID(bNodeID);
      }
    } else if (wal_strnicmp(sTopic, "base", 4) == 0) {
      sParam1 = next();
      if( sParam1) {
        theRadio.enableBaseNetwork(atoi(sParam1) > 0);
        SERIAL_LN("Base RF network is %s\n\r", (theRadio.isBaseNetworkEnabled() ? "enabled" : "disabled"));
        CloudOutput("base:%s", (theRadio.isBaseNetworkEnabled() ? "enabled" : "disabled"));
        retVal = true;
      }
    } else if (wal_strnicmp(sTopic, "maxebn", 6) == 0) {
      sParam1 = next();
      US nDur = MAX_BASE_NETWORK_DUR;
      if( sParam1 ) {
        nDur = (US)atoi(sParam1);
      }
      theConfig.SetMaxBaseNetworkDur(nDur);
      SERIAL_LN("Max Base RF network duration set %d\n\r", nDur);
      CloudOutput("maxebn:%d", nDur);
      retVal = true;
    } else if (wal_strnicmp(sTopic, "flag", 4) == 0) {
      // Change flag value
      sParam1 = next();   // Get flag name
      if( sParam1) {
        sParam2 = next();   // Get flag value
        if( sParam2 ) {
          if (wal_strnicmp(sParam1, "csc", 3) == 0) {
            theConfig.SetCloudSerialEnabled(atoi(sParam2) > 0);
            SERIAL_LN("Cloud Serial Command is %s\n\r", (theConfig.IsCloudSerialEnabled() ? "enabled" : "disabled"));
            CloudOutput("f_csc:%s", (theConfig.IsCloudSerialEnabled() ? "enabled" : "disabled"));
            retVal = true;
          } else if (wal_strnicmp(sParam1, "cdts", 4) == 0) {
            theConfig.SetDailyTimeSyncEnabled(atoi(sParam2) > 0);
            SERIAL_LN("Cloud Daily TimeSync is %s\n\r", (theConfig.IsDailyTimeSyncEnabled() ? "enabled" : "disabled"));
            CloudOutput("f_cdts:%s", (theConfig.IsDailyTimeSyncEnabled() ? "enabled" : "disabled"));
            retVal = true;
          } else if (wal_strnicmp(sParam1, "fnid", 4) == 0) {
            theConfig.SetFixedNID(atoi(sParam2) > 0);
            SERIAL_LN("Fixed NodeID is %s\n\r", (theConfig.IsFixedNID() ? "enabled" : "disabled"));
            CloudOutput("f_fnid:%d", theConfig.IsFixedNID());
            retVal = true;
          } else if (wal_strnicmp(sParam1, "hwsw", 4) == 0) {
            theConfig.SetHardwareSwitch(atoi(sParam2) > 0);
            SERIAL_LN("Default use %s switch\n\r", (theConfig.GetHardwareSwitch() ? "Hardware" : "Software"));
            CloudOutput("hwsw:%d", theConfig.GetHardwareSwitch());
            retVal = true;
          } else if (wal_strnicmp(sParam1, "wifi", 4) == 0) {
            theConfig.SetDisableWiFi(atoi(sParam2) == 0);
            retVal = true;
          }
		  else if (wal_strnicmp(sParam1, "lamp", 4) == 0) {
			  theConfig.SetDisableLamp(atoi(sParam2) == 0);
			  CloudOutput("lamp:%d", theConfig.GetDisableLamp());
			  retVal = true;
          }
        } else {
          SERIAL_LN("Require flag value, use '? set flag' for detail\n\r");
          retVal = true;
        }
      } else {
        SERIAL_LN("Require flag name and value, use '? set flag' for detail\n\r");
        retVal = true;
      }
    } else if (wal_strnicmp(sTopic, "var", 3) == 0) {
      // Change variable value
      sParam1 = next();   // Get variable name
      if( sParam1) {
        sParam2 = next();   // Get variable value
        if( sParam2 ) {
          if (wal_strnicmp(sParam1, "senmap", 6) == 0) {
            theConfig.SetSensorBitmap((US)atoi(sParam2));
            SERIAL_LN("SensorBitmap is 0x%04X\n\r", theConfig.GetSensorBitmap());
            CloudOutput("v_senmap:0x%04X", theConfig.GetSensorBitmap());
            retVal = true;
          } else if (wal_strnicmp(sParam1, "devst", 5) == 0) {
            if( !theSys.SetStatus((UC)atoi(sParam2)) ) {
              SERIAL_LN("Failed to set Device Status: %s\n\r", sParam2);
              CloudOutput("Failed to set DevStatus %s", sParam2);
            }
            retVal = true;
          } else if (wal_strnicmp(sParam1, "bmrt", 4) == 0) {
            theConfig.SetBcMsgRptTimes((UC)atoi(sParam2));
            SERIAL_LN("Bcast Msg Repeat Times: %d\n\r", theConfig.GetBcMsgRptTimes());
            CloudOutput("v_bmrt:%d", theConfig.GetBcMsgRptTimes());
            retVal = true;
          } else if (wal_strnicmp(sParam1, "nmrt", 4) == 0) {
            theConfig.SetNdMsgRptTimes((UC)atoi(sParam2));
            SERIAL_LN("Node Msg Repeat Times: %d\n\r", theConfig.GetNdMsgRptTimes());
            CloudOutput("v_nmrt:%d", theConfig.GetNdMsgRptTimes());
            retVal = true;
          } else if (wal_strnicmp(sParam1, "rfch", 4) == 0) {
            theConfig.SetRFChannel((UC)atoi(sParam2));
            SERIAL_LN("RF Channel: %d    \n\r", theRadio.getChannel(false));
            CloudOutput("v_rfch:%d", theRadio.getChannel(false));
            retVal = true;
          } else if (wal_strnicmp(sParam1, "rfpl", 4) == 0) {
            theConfig.SetRFPowerLevel((UC)atoi(sParam2));
            SERIAL_LN("RF PowerLevel: %d\n\r", theRadio.getPALevel(false));
            CloudOutput("v_rfpl:%d", theRadio.getPALevel(false));
            retVal = true;
          } else if (wal_strnicmp(sParam1, "rfdr", 4) == 0) {
            theConfig.SetRFDataRate((UC)atoi(sParam2));
            SERIAL_LN("RF Speed: %d      \n\r", theRadio.getDataRate(false));
            CloudOutput("v_rfdr:%d", theRadio.getDataRate(false));
            retVal = true;
          }
        } else {
          SERIAL_LN("Require var value, use '? set var' for detail\n\r");
          retVal = true;
        }
      } else {
        SERIAL_LN("Require var name and value, use '? set var' for detail\n\r");
        retVal = true;
      }
    } else if (wal_strnicmp(sTopic, "spkr", 4) == 0) {
      // Enable or disable speaker
      sParam1 = next();   // Get speaker flag
      if( sParam1) {
        theConfig.SetSpeakerEnabled(atoi(sParam1) > 0);
        SERIAL_LN("Speaker is %s\n\r", (theConfig.IsSpeakerEnabled() ? "enabled" : "disabled"));
        CloudOutput("spkr:%s", (theConfig.IsSpeakerEnabled() ? "enabled" : "disabled"));
        retVal = true;
      } else {
        SERIAL_LN("Require spkr flag value [0|1], use '? set spkr' for detail\n\r");
        retVal = true;
      }
    } else if (wal_strnicmp(sTopic, "cloud", 5) == 0) {
      // Cloud Option
      sParam1 = next();
      if( sParam1) {
        theConfig.SetUseCloud(atoi(sParam1));
        SERIAL_LN("Cloud option set to %d\n\r", theConfig.GetUseCloud());
        CloudOutput("cloud:%d", theConfig.GetUseCloud());
        retVal = true;
      } else {
        SERIAL_LN("Require Cloud option [0|1|2], use '? set cloud' for detail\n\r");
        retVal = true;
      }
    } else if (wal_strnicmp(sTopic, "maindev", 7) == 0) {
      // Main device id
      sParam1 = next();
      if( sParam1) {
        theConfig.SetMainDeviceID(atoi(sParam1));
        SERIAL_LN("Main device changed to %d\n\r", CURRENT_DEVICE);
        CloudOutput("maindev:%d", CURRENT_DEVICE);
        retVal = true;
      } else {
        SERIAL_LN("Require a valid nodeID\n\r");
        retVal = true;
      }
    } else if (wal_strnicmp(sTopic, "subid", 5) == 0) {
      // Sub device id
      sParam1 = next();
      if( sParam1) {
        theConfig.SetSubDeviceID(atoi(sParam1));
        SERIAL_LN("SubNID changed to %d\n\r", CURRENT_SUBDEVICE);
        CloudOutput("sid:%d", CURRENT_SUBDEVICE);
        retVal = true;
      }
    } else if (wal_strnicmp(sTopic, "remote", 6) == 0) {
      // Remote controlled device
      sParam1 = next();     // Get remote node_id
      if( sParam1) {
        sParam2 = next();   // Get device node_id
        if( sParam2 ) {
          theConfig.SetRemoteNodeDevice(atoi(sParam1), atoi(sParam2));
          SERIAL_LN("Remote %s controls device %s\n\r", sParam1, sParam2);
          CloudOutput("remote:%s-%s", sParam1, sParam2);
          retVal = true;
        } else {
          SERIAL_LN("Require a valid device nodeID\n\r");
          retVal = true;
        }
      } else {
        SERIAL_LN("Require a valid remote nodeID\n\r");
        retVal = true;
      }
    } else if (wal_strnicmp(sTopic, "blename", 7) == 0) {
      // BLE Name
      sParam1 = next();
      if( sParam1) {
        theConfig.SetBLEName(sParam1);
        SERIAL_LN("Set BLE name to %s\n\r", sParam1);
        CloudOutput("blename:%s", sParam1);
        retVal = true;
      } else {
        SERIAL_LN("Require BLE name string\n\r");
        retVal = true;
      }
    } else if (wal_strnicmp(sTopic, "blepin", 6) == 0) {
      // BLE Pin
      sParam1 = next();
      if( strlen(sParam1) == 4 ) {
        theConfig.SetBLEPin(sParam1);
        SERIAL_LN("Set BLE PIN to %s\n\r", sParam1);
        CloudOutput("blepin:%s", sParam1);
        retVal = true;
      } else {
        SERIAL_LN("Require 4 digits BLE pin\n\r");
        retVal = true;
      }
    } else if (wal_strnicmp(sTopic, "hwsobj", 6) == 0) {
      // Relay key object type
      sParam1 = next();
      if( sParam1) {
        theConfig.SetRelayKeyObj(atoi(sParam1));
        SERIAL_LN("Set HW switch obj: %d\n\r", theConfig.GetRelayKeyObj());
        CloudOutput("hwsobj:%d", theConfig.GetRelayKeyObj());
        retVal = true;
      }
    } else if (wal_strnicmp(sTopic, "loopkc", 6) == 0) {
      // Relay key loop code
      sParam1 = next();
      if( sParam1) {
        if( theSys.SetLoopKeyCode(atoi(sParam1)) ) {
          SERIAL_LN("Set loop keycode: %d\n\r", theSys.GetLoopKeyCode());
          CloudOutput("loopkc:%d", theSys.GetLoopKeyCode());
          retVal = true;
        }
      }
    } else if (wal_strnicmp(sTopic, "kcto", 4) == 0) {
      // Relay key loop code
      sParam1 = next();
      if( sParam1) {
        theConfig.SetTimeLoopKC(atoi(sParam1));
        SERIAL_LN("Set loop keycode timeout: %d\n\r", theConfig.GetTimeLoopKC());
        CloudOutput("kcto:%d", theConfig.GetTimeLoopKC());
        retVal = true;
      }
    } else if (wal_strnicmp(sTopic, "pptpin", 6) == 0) {
      // PPT Access Code
      sParam1 = next();
      theConfig.SetPPTAccessCode(sParam1);
      SERIAL_LN("Set PPT PIN to %s\n\r", sParam1);
      CloudOutput("pptpin:%s", sParam1);
      retVal = true;
    } else if (wal_strnicmp(sTopic, "asrsnt", 6) == 0) {
      // ASR command scenario
      sParam1 = next();     // Get code
      if( sParam1) {
        sParam2 = next();   // Get scenario id
        if( sParam2 ) {
          theConfig.SetASR_SNT((UC)atoi(sParam1), (UC)atoi(sParam2));
          SERIAL_LN("ASR cmd %s will trigger SNT_id %s\n\r", sParam1, sParam2);
          CloudOutput("asrsnt:%s-%s", sParam1, sParam2);
          retVal = true;
        } else {
          SERIAL_LN("Require a valid ASR code (1 to 16)\n\r");
          retVal = true;
        }
      } else {
        SERIAL_LN("Require a valid scenario ID\n\r");
        retVal = true;
      }
    } else if (wal_strnicmp(sTopic, "keymap", 6) == 0) {
      // Keymap item
      sParam1 = next();     // Get key
      if( sParam1) {
        sParam2 = next();   // Get nodeID
        if( sParam2 ) {
          UC subID = 0;
          sParam3 = next();   // Get subID
          if( sParam3 ) subID = (UC)atoi(sParam3);
          theConfig.SetKeyMapItem((UC)atoi(sParam1), (UC)atoi(sParam2), subID);
          SERIAL_LN("Key%s maps to %s-%d\n\r", sParam1, sParam2, subID);
          CloudOutput("key%s:%s-%d", sParam1, sParam2, subID);
          retVal = true;
        } else {
          SERIAL_LN("Require a valid nodeID\n\r");
          retVal = true;
        }
      } else {
        SERIAL_LN("Require a valid keyID (1 to %d)\n\r", MAX_KEY_MAP_ITEMS);
        retVal = true;
      }
    } else if (wal_strnicmp(sTopic, "extbtn", 6) == 0) {
      // Change action of ext. button
      sParam1 = next();     // Get button key (0 based)
      if( sParam1) {
        sParam2 = next();   // Get operation ID
        if( sParam2 ) {
          sParam3 = next();   // Get action ID
          if( sParam3 ) {
            sParam4 = next();   // Get keymap
            if( sParam4 ) {
              theConfig.SetExtBtnAction((UC)atoi(sParam1), (UC)atoi(sParam2), (UC)atoi(sParam3), (UC)atoi(sParam4));
              SERIAL_LN("extbtn%s(%s) maps to %s-%s\n\r", sParam1, sParam2, sParam3, sParam4);
              CloudOutput("extbtn%s(%s):%s-%s", sParam1, sParam2, sParam3, sParam4);
              retVal = true;
            } else {
              SERIAL_LN("Require keymap\n\r");
              retVal = true;
            }
          } else {
            SERIAL_LN("Require a valid action id (0, 1 or 2)\n\r");
            retVal = true;
          }
        } else {
          SERIAL_LN("Require a valid operation id (0 to %d)\n\r", MAX_BTN_OP_TYPE - 1);
          retVal = true;
        }
      } else {
        SERIAL_LN("Require a valid button id (0 to %d)\n\r", MAX_NUM_BUTTONS - 1);
        retVal = true;
      }
    } else if (wal_strnicmp(sTopic, "debug", 5) == 0) {
      sParam1 = next();
      if( sParam1) {
        String strMsg = sParam1;
        retVal = theLog.ChangeLogLevel(strMsg);
        if( retVal ) {
          SERIAL_LN("Set Debug Level to %s\n\r", sParam1);
          CloudOutput("debug:%s", sParam1);
        }
      }
    }
  }

  return retVal;
}

// sys - System control
bool SerialConsoleClass::doSys(const char *cmd)
{
  IF_SERIAL_DEBUG(SERIAL_LN("doSys(%s)\n\r", cmd));

  // Interpret deeply
  return scanStateMachine();
}

bool SerialConsoleClass::doSysSub(const char *cmd)
{
  IF_SERIAL_DEBUG(SERIAL_LN("doSysSub(%s)", cmd));

  char strDisplay[64];
  const char *sTopic = CommandList[currentCommand].event;
  char *sParam1;
  if( sTopic ) {
    if (wal_strnicmp(sTopic, "reset", 5) == 0) {
      uint8_t lv_NodeID = 0;
      sParam1 = next();     // Get code
      if( sParam1) {
        lv_NodeID = (uint8_t)atoi(sParam1);
      }
      if( lv_NodeID == 0) {
        SERIAL_LN("System is about to reset...");
        CloudOutput("System is about to reset");
        theSys.Restart();
      } else {
        // Reboot node
        SERIAL_LN("Will reboot node %d", lv_NodeID);
        theSys.RebootNode(lv_NodeID);
      }
    }
    else if (wal_strnicmp(sTopic, "safe", 4) == 0) {
      SERIAL_LN("System is about to enter safe mode...");
      CloudOutput("Will enter safe mode");
      delay(1000);
      System.enterSafeMode();
    }
    else if (wal_strnicmp(sTopic, "dfu", 3) == 0) {
      SERIAL_LN("System is about to enter DFU mode...");
      CloudOutput("Will enter DFU mode");
      delay(1000);
      System.dfu();
    }
    else if (wal_strnicmp(sTopic, "listen", 6) == 0) {
      theConfig.SetDisableWiFi(false);
      SERIAL_LN("System is about to enter listening mode...");
      CloudOutput("Will enter listening mode");
      delay(1000);
      WiFi.listen();
    }
    else if (wal_strnicmp(sTopic, "update", 6) == 0) {
      // ToDo: to OTA
    }
    else if (wal_strnicmp(sTopic, "serial", 6) == 0) {
      sParam1 = next();
      if(sParam1) {
        if( wal_stricmp(sParam1, "reset") == 0 ) {
          theSys.ResetSerialPort();
        } else {
          return false;
        }
      } else {
        SERIAL_LN("Serial speed:%d\r\n", SERIALPORT_SPEED_DEFAULT);
        CloudOutput("serial:%d", SERIALPORT_SPEED_DEFAULT);
      }
    }
    else if (wal_strnicmp(sTopic, "sync", 4) == 0) {
      sParam1 = next();
      if(sParam1) {
        if( wal_stricmp(sParam1, "time") == 0 ) {
          theSys.CldSetCurrentTime();
        } else {
          // ToDo: other synchronization
        }
      } else {
        return false;
      }
    }
    else if (wal_strnicmp(sTopic, "clear", 5) == 0) {
      sParam1 = next();
      if(sParam1) {
        if( wal_stricmp(sParam1, "nodeid") == 0 ) {
          sParam1 = next();   // id
          if(sParam1) {
            if( !theConfig.lstNodes.clearNodeId((UC)atoi(sParam1)) ) {
              SERIAL_LN("Failed to clear NodeID:%s\n\r", sParam1);
              CloudOutput("Failed to clear NodeID:%s", sParam1);
            }
          }
        } else if( wal_stricmp(sParam1, "credentials") == 0 ) {
          WiFi.clearCredentials();
          SERIAL_LN("WiFi credentials cleared\n\r");
          CloudOutput("WiFi credentials cleared");
        } else {
          return false;
        }
      } else {
        return false;
      }
    }
    else if (wal_strnicmp(sTopic, "base", 4) == 0) {
      // Switch to Base Network
      theRadio.switch2BaseNetwork();
      SERIAL_LN("Switched to base network\n\r");
      CloudOutput("Switched to base network");
    }
    else if (wal_strnicmp(sTopic, "private", 7) == 0) {
      // Switch to Private Network
      theRadio.switch2MyNetwork();
      SERIAL_LN("Switched to private network: %s\n\r", PrintUint64(strDisplay, theRadio.getCurrentNetworkID()));
      CloudOutput("Switched to private network");
    }
  } else { return false; }

  return true;
}

//--------------------------------------------------
// Support Functions
//--------------------------------------------------
// Get WI-Fi crediential from serial Port
bool SerialConsoleClass::SetupWiFi(const char *cmd)
{
  String sText;
  switch( (consoleState_t)currentState ) {
  case consoleWF_YesNo:
    // Confirm menu choice
    sText = "Sure to setup Wi-Fi crediential? (y/N)";
    SERIAL_LN(sText.c_str());
    CloudOutput(sText.c_str());
    break;

  case consoleWF_GetSSID:
    sText = "Please enter SSID";
    SERIAL_LN("%s [%s]:", sText.c_str(), gstrWiFi_SSID.c_str());
    CloudOutput(sText.c_str());
    break;

  case consoleWF_GetPassword:
    // Record SSID
    if( strlen(cmd) > 0 )
      gstrWiFi_SSID = cmd;
    sText = "Please enter PASSWORD";
    SERIAL_LN("%s:", sText.c_str());
    CloudOutput(sText.c_str());
    break;

  case consoleWF_GetAUTH:
    // Record Password
    if( strlen(cmd) > 0 )
      gstrWiFi_Password = cmd;
    SERIAL_LN("Please select authentication method: [%d]", gintWiFi_Auth);
    SERIAL_LN("  0. %s", strAuthMethods[0]);
    SERIAL_LN("  1. %s", strAuthMethods[1]);
    SERIAL_LN("  2. %s", strAuthMethods[2]);
    SERIAL_LN("  3. %s", strAuthMethods[3]);
    SERIAL_LN("  (q)uit");
    CloudOutput("Select authentication method [0..3]");
    break;

  case consoleWF_Cipher:
    // Record authentication method
    if( strlen(cmd) > 0 )
      gintWiFi_Auth = atoi(cmd) % 4;
    SERIAL_LN("Please select cipher method: [%d]", gintWiFi_Cipher);
    SERIAL_LN("  0. %s", strCipherMethods[0]);
    SERIAL_LN("  1. %s", strCipherMethods[1]);
    SERIAL_LN("  2. %s", strCipherMethods[2]);
    SERIAL_LN("  3. %s", strCipherMethods[3]);
    SERIAL_LN("  (q)uit");
    CloudOutput("Select cipher method [0..3]");
    break;

  case consoleWF_Confirm:
    // Record cipher method
    if( strlen(cmd) > 0 )
      gintWiFi_Cipher = atoi(cmd) % 4;
    SERIAL_LN("Are you sure to apply the Wi-Fi credential? (y/N)");
    SERIAL_LN("  SSID: %s", gstrWiFi_SSID.c_str());
    SERIAL_LN("  Password: ******");
    SERIAL_LN("  Authentication: %s", strAuthMethods[gintWiFi_Auth]);
    SERIAL_LN("  Cipher: %s", strCipherMethods[gintWiFi_Cipher]);
    CloudOutput("Sure to apply the Wi-Fi credential? (y/N)");
    break;
  }

  return true;
}

void SerialConsoleClass::UpdateWiFiCredential()
{
  if( gintWiFi_Cipher > 0 ) {
    WiFi.setCredentials(gstrWiFi_SSID, gstrWiFi_Password, gintWiFi_Auth, gintWiFi_Cipher);
  } else if( gintWiFi_Auth > 0 ) {
    WiFi.setCredentials(gstrWiFi_SSID, gstrWiFi_Password, gintWiFi_Auth);
  } else if( gstrWiFi_Password.length() > 0 ) {
    WiFi.setCredentials(gstrWiFi_SSID, gstrWiFi_Password);
  } else {
    WiFi.setCredentials(gstrWiFi_SSID);
  }
}

bool SerialConsoleClass::SetWiFiCredential(const char *cmd)
{
  theConfig.SetDisableWiFi(false);
  UpdateWiFiCredential();
  SERIAL("Wi-Fi credential saved");
  WiFi.listen(false);
  theSys.connectWiFi();
  CloudOutput("Wi-Fi credential saved, reconnect %s", WiFi.ready() ? "OK" : "Failed");
  return true;
}

bool SerialConsoleClass::PingAddress(const char *sAddress)
{
  if( theConfig.GetDisableWiFi() ) {
    SERIAL_LN("Wi-Fi is disable!");
    return false;
  }

  if( !WiFi.ready() ) {
    SERIAL_LN("Wi-Fi is not ready!");
    return false;
  }

  // Get IP address
  IPAddress ipAddr;
  if( !String2IP(sAddress, ipAddr) )
    return false;

  // Ping 4 times
  int pingStartTime = millis();
  SERIAL("Pinging %s (", sAddress);
  TheSerial.print(ipAddr);
  SERIAL(")...");
  int myByteCount = WiFi.ping(ipAddr, 3);
  int elapsedTime = millis() - pingStartTime;
  SERIAL_LN("received %d bytes over %d ms", myByteCount, elapsedTime);
  CloudOutput("Pinging %s recieved %d", sAddress, myByteCount);

  return true;
}

bool SerialConsoleClass::String2IP(const char *sAddress, IPAddress &ipAddr)
{
  int lv_len = strlen(sAddress);
  bool bHost = false;
  ipAddr = IPAddress(0UL);      // NULL IP
  int i;

  // Blank -> google
  if( lv_len == 0 ) {
    ipAddr = IPAddress(8,8,8,8);
    return true;
  }

  // IP address or hostname
  for( i=0; i<lv_len; i++ ) {
    if( (sAddress[i] >= '0' && sAddress[i] <= '9') || sAddress[i] == '.' ) {
      continue;
    }
    bHost = true;
    break;
  }

  // Resole IP from hostname
  if( bHost ) {
    ipAddr = WiFi.resolve(sAddress);
    //IF_SERIAL_DEBUG(SERIAL("Resoled %s is ", sAddress));
    //IF_SERIAL_DEBUG(TheSerial.println(ipAddr));
  } else if( lv_len > 6 ) {
    // Split IP segments
    UC ipSeg[4];
    String sTemp = sAddress;
    int nPos = 0, nPos2;
    for( i = 0; i < 3; i++ ) {
      nPos2 = sTemp.indexOf('.', nPos);
      if( lv_len > 0 ) {
        ipSeg[i] = (UC)(sTemp.substring(nPos, nPos2).toInt());
        nPos = nPos2 + 1;
      } else {
        nPos = 0;
        break;
      }
    }
    if( nPos > 0 && nPos < lv_len ) {
      ipSeg[i] = (UC)(sTemp.substring(nPos).toInt());
      for( i = 0; i < 4; i++ ) {
        ipAddr[i] = ipSeg[i];
      }
    } else {
      return false;
    }
  } else {
    return false;
  }

  return true;
}

// Simulate to run a command string
bool SerialConsoleClass::ExecuteCloudCommand(const char *cmd)
{
  clearBuffer();
  setCommandBuffer(cmd);
  isInCloudCommand = true;
  bool rc = scanStateMachine();
  isInCloudCommand = false;
  return rc;
}

// Output concise result to the cloud
void SerialConsoleClass::CloudOutput(const char *msg, ...)
{
  if( !isInCloudCommand ) return;

  int nSize = 0;
  char buf[MAX_MESSAGE_LEN];

  // Prepare message
  va_list args;
  va_start(args, msg);
  nSize = vsnprintf(buf, MAX_MESSAGE_LEN, msg, args);
  buf[nSize] = '\0';

  // Set message
  theSys.m_lastMsg = buf;
}

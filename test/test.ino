#ifdef UNIT_TEST_ENABLE

#include "application.h"
#include "unitTest.h"

#include "SparkIntervalTimer.h"

#include "xlSmartController.h"
#include "xliCommon.h"
#include "xliMemoryMap.h"
#include "xliPinMap.h"
#include "xliConfig.h"

#include "xlxCloudObj.h"
#include "xlxConfig.h"
#include "xlxLogger.h"
#include "xlxSerialConsole.h"

test(example)
{
  //String in ="";
  /// Format_1: Single row
  theSys.CldJSONConfig("{'op':1, 'fl':0, 'run':0, 'uid':'s1','ring1':[1,8,8,8,8,8], 'ring2':[1,8,8,8,8,8], 'ring3':[1,8,8,8,8,8], 'filter':0}");
  theSys.CldJSONCommand("{'cmd':'serial', 'data':'?'}");
  theSys.CldJSONCommand("{'cmd':'serial', 'data':'? show'}");
  theSys.CldJSONCommand("{'cmd':'serial', 'data':'check rf'}");
  theSys.CldJSONCommand("{'cmd':'serial', 'data':'check wifi'}");
  theSys.CldJSONCommand("{'cmd':'serial', 'data':'show debug'}");
  theSys.CldJSONCommand("{'cmd':'serial', 'data':'show net'}");
  theSys.CldJSONCommand("{'cmd':'serial', 'data':'ping'}");
  theSys.CldJSONCommand("{'cmd':'serial', 'data':'sys reset'}");

  /// Format_2: Multiple rows
  //theSys.CldJSONConfig("{'rows':2, 'data': [{'op':1, 'fl':0, 'run':0, 'uid':'s1', 'hue':0x0101080808080800}, {'op':1, 'fl':0, 'run':0, 'uid':'s1', 'hue':0x0201080808080800}, {'op':1, 'fl':0, 'run':0, 'uid':'s1', 'hue':0x0301080808080800}]");

  /// Format_3: Concatenate strings
  //// First string
  theSys.CldJSONConfig("{'x0':'{\"op\":1, \"fl\":0, \"run\":0, \"uid\":\"s1\",\"ring1\":[1,8,8,8,8,8], '}");
  //// Strings in middle
  theSys.CldJSONConfig("{'x1': '\"ring2\":[1,8,8,8,8,8], \"ring3\":[1,8,8,8,8,8], '}");
  theSys.CldJSONConfig("\"filter\":0}");
  //// Last string: same as Format_1 or Format_2

  //// Format_3 test case
  theSys.CldJSONCommand("{'x0': '{\"cmd\":\"serial\", '}");
  theSys.CldJSONCommand("{'x1': '\"data\":\"check '}");
  theSys.CldJSONCommand("wifi\"}");
  //// Format_3 test case
  theSys.CldJSONCommand("{'x0': ' '}");
  theSys.CldJSONCommand("{'cmd':'serial', 'data':'show net'}");
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// CloudObjClass Tests
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// SmartControllerClass Tests
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// ConfigClass Tests
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// RF24InterfaceClass Tests
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// LoggerClass Tests
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>


//><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>
// Intergration Tests
//><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><>


//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
// Call Start Func to Init Tests
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
  bool flag;

  void testOrderedList(bool desc) {
    SERIAL_LN("testOrderedList()");
    NodeListClass lstTest(32, desc);
    NodeIdRow_t lv_Node;
    lv_Node.nid = 1;
    lv_Node.recentActive = Time.now();
    SERIAL_LN("Add node:%d at %d", lv_Node.nid, lstTest.add(&lv_Node));
    lv_Node.nid = 2;
    lv_Node.recentActive = Time.now() + lv_Node.nid;
    SERIAL_LN("Add node:%d at %d", lv_Node.nid, lstTest.add(&lv_Node));
    lv_Node.nid = 6;
    lv_Node.recentActive = Time.now() + lv_Node.nid;
    SERIAL_LN("Add node:%d at %d", lv_Node.nid, lstTest.add(&lv_Node));
    lv_Node.nid = 4;
    lv_Node.recentActive = Time.now() + lv_Node.nid;
    SERIAL_LN("Add node:%d at %d", lv_Node.nid, lstTest.add(&lv_Node));
    lv_Node.nid = 3;
    lv_Node.recentActive = Time.now() + lv_Node.nid;
    SERIAL_LN("Add node:%d at %d", lv_Node.nid, lstTest.add(&lv_Node));

    SERIAL_LN("Node List - count:%d, size:%d", lstTest.count(), lstTest.size());
    lv_Node.nid = 2;
    lv_Node.recentActive = 123456;
    lstTest.update(&lv_Node);
    lv_Node.recentActive = 0;
    if( lstTest.get(&lv_Node) >= 0 ) {
      SERIAL_LN("Node:%d, value:%d", lv_Node.nid, lv_Node.recentActive);
    }

    for(int i=0; i < lstTest.count(); i++) {
      SERIAL_LN("Index: %d - node:%d", i, lstTest._pItems[i].nid);
    }

    lv_Node.nid = 3;
    if( lstTest.remove(&lv_Node) ) {
      SERIAL_LN("One node:%d removed, Node List - count:%d, size:%d", lv_Node.nid, lstTest.count(), lstTest.size());
    }
  }

  int start(String input)
  {
    flag = true;
    return 1;
  }

  void setup()
  {
    Particle.function("RunUnitTests", start);
    flag = false;
    Serial.begin(SERIALPORT_SPEED_DEFAULT);

  	//Test output location
  	Test::out = &Serial;

  	//Test Selection (use ::exclude(char *pattern) and ::include(char *pattern))
  	//Test::exclude("*");
  	//Test::include("SmartControllerClass_*");

	   //Additional Setup
    for(int i = 10; i > 0; i--)
    {
      Serial.println(i);
      delay(500);
    }
    Serial.println ("starting setup functions");
  	//IntervalTimer sysTimer;
  	theSys.Init();
  	theConfig.LoadConfig();
  	theSys.InitPins();
  	theSys.InitRadio();
  	theSys.InitNetwork();
  	theSys.InitCloudObj();
  	theSys.InitSensors();
    theConsole.Init();
  	theSys.Start();
  	while (Time.now() < 2000) {
  		Particle.process();
  	}

    testOrderedList(false);
  }

  void loop()
  {
    //Serial.print (".");
    static UC tick = 0;

    IF_MAINLOOP_TIMER( theSys.ProcessCommands(), "ProcessCommands" );

    IF_MAINLOOP_TIMER( theSys.CollectData(tick++), "CollectData" );

  	IF_MAINLOOP_TIMER( theSys.ReadNewRules(), "ReadNewRules" );

    IF_MAINLOOP_TIMER( theSys.SelfCheck(RTE_DELAY_SELFCHECK), "SelfCheck" );

    if (flag)
      Test::run();
  }

#endif

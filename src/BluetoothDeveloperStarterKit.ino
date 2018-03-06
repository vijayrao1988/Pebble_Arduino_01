/*
 * Author: Vijay Rao
 *
 * Version History
 * V1.0:
 * First version
 *
 *
 */

#include <Bounce2.h>
#include <TimeLib.h>
#include <CurieBLE.h>
#include <Time.h>
#include <TimeLib.h>
#include <TimeAlarms.h>

#define solenoidP 8
#define solenoidN 7
#define button 2
#define flowSensor 4
#define ledPower 3
#define ledStop  5
#define ledStart 6


timeDayOfWeek_t alarmDay[7] = {dowSunday, dowMonday, dowTuesday, dowWednesday, dowThursday, dowFriday, dowSaturday};

String inputString = "";         // a String to hold incoming data
boolean stringComplete = false;  // whether the string is complete
Bounce interruptButtonBouncer = Bounce();

//Alarm IDs and corresponding volumes & durations
AlarmId id[32];
static uint16_t volume[28];
static uint16_t duration[28];
static volatile uint16_t flowCounter = 0,countStop=0, countStart=0;
static volatile uint8_t systemPause=0;

// BLE objects
BLEPeripheral blePeripheral;


// GAP properties
char device_name[] = "Pebble";

// Characteristic Properties

//Pebble Characteristic Properties
unsigned char CurrentTime_props = BLERead | BLEWrite | 0;
unsigned char BatteryLevel_props = BLERead | 0;
unsigned char Pots_props = BLERead | BLEWrite | 0;
unsigned char NewTimePoint_props = BLEWrite | 0;
unsigned char ValveCommand_props = BLEWrite | 0;
unsigned char LogEvent_props = BLEWrite | BLERead | 0;
//unsigned char LogService_ReadIndex_props = BLEWrite | 0;


char AttributeValue[32];

// Services and Characteristics
//Pebble Services and Characteristics
BLEService PebbleService("12345678123412341234123456789ABC");

//BLEService BatteryService("6E521ABEB56F4B058465A1CEE41BB141");
BLECharacteristic BatteryLevel("6E52C8E5B56F4B058465A1CEE41BB141", BatteryLevel_props, 1);
//Total Length = 1 byte
//1 byte = uint8 Level

//BLEService CurrentTimeService("6E52A9DAB56F4B058465A1CEE41BB141");
BLECharacteristic CurrentTime("6E52AC46B56F4B058465A1CEE41BB141", CurrentTime_props, 7);
//Total Length = 7 bytes
//1 byte = uint8 hours
//1 byte = uint8 minutes
//1 byte = uint8 seconds
//1 byte = uint8 Days
//1 byte = uint8 Months
//2 byte = uint16 Years

//BLEService PotsService("6E529F14B56F4B058465A1CEE41BB141");
BLECharacteristic Pots("6E52E386B56F4B058465A1CEE41BB141", Pots_props, 1);
//1 byte = Total Length
//1 byte = uint8 Number of Pots

//BLEService TimePointService("6E52214FB56F4B058465A1CEE41BB141");
BLECharacteristic NewTimePoint("6E529480B56F4B058465A1CEE41BB141", NewTimePoint_props, 9);
//Total Length = 9 bytes
//1 byte = uint8 Index (Time Point Number)
//1 byte = uint8 Day of the Week
//1 byte = uint8 hours
//1 byte = uint8 minutes
//1 byte = uint8 seconds
//2 bytes = uint16 Duration
//2 bytes = uint16 Volume

//BLEService ValveControllerService("6E52C714B56F4B058465A1CEE41BB141");
BLECharacteristic ValveCommand("6E52CFDBB56F4B058465A1CEE41BB141", ValveCommand_props, 1);
//Total Length = 1 byte
//1 byte = uint8 CommandCode

//BLEService LogService("6E52ABCDB56F4B058465A1CEE41BB141");
BLECharacteristic LogEvent("6E521234B56F4B058465A1CEE41BB141", LogEvent_props, 15);
//Total Length = 15 bytes
//4 bytes : long time
//1 byte : code
//10 bytes : data
//BLECharacteristic LogService_ReadIndex("6E521235B56F4B058465A1CEE41BB141", LogService_ReadIndex_props, 2);
//Total Length = 2 bytes
//2 bytes : uint16 readingIndex

struct logEvent {
  unsigned char eventCode;
  unsigned long eventTime;
  unsigned char data[10];
} logData[300];

static uint16_t logDataCursor = 0;

void setup() {
  pinMode(13, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(11, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(8, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(ledPower, OUTPUT);
  pinMode(ledStop, OUTPUT);
  pinMode(ledStart, OUTPUT);
  pinMode(button, INPUT);
  digitalWrite(button, HIGH);
  pinMode(flowSensor, INPUT);
  interruptButtonBouncer .attach(button);
  interruptButtonBouncer .interval(5);
  attachInterrupt(digitalPinToInterrupt(button), beep, FALLING);
  attachInterrupt(digitalPinToInterrupt(flowSensor), count, CHANGE);
  interrupts();

  digitalWrite(ledPower, HIGH);
  digitalWrite(8, HIGH);
  digitalWrite(7, HIGH);

  digitalWrite(10, HIGH);  // turn the Buzzer on (HIGH is the voltage level)
  digitalWrite(11, HIGH);
  digitalWrite(12, HIGH);
  digitalWrite(13, HIGH);
  delay(250);              // wait for a second
  digitalWrite(10, LOW);  // turn the Buzzer on (HIGH is the voltage level)
  digitalWrite(11, LOW);
  digitalWrite(12, LOW);
  digitalWrite(13, LOW);
  delay(250);              // wait for a second
  digitalWrite(10, HIGH);  // turn the Buzzer on (HIGH is the voltage level)
  digitalWrite(11, HIGH);
  digitalWrite(12, HIGH);
  digitalWrite(13, HIGH);
  delay(250);              // wait for a second
  digitalWrite(10, LOW);  // turn the Buzzer on (HIGH is the voltage level)
  digitalWrite(11, LOW);
  digitalWrite(12, LOW);
  digitalWrite(13, LOW);
  delay(250);              // wait for a second
  digitalWrite(10, HIGH);  // turn the Buzzer on (HIGH is the voltage level)
  digitalWrite(11, HIGH);
  digitalWrite(12, HIGH);
  digitalWrite(13, HIGH);
  delay(100);              // wait for a second
  digitalWrite(10, LOW);  // turn the Buzzer on (HIGH is the voltage level)
  digitalWrite(11, LOW);
  digitalWrite(12, LOW);
  digitalWrite(13, LOW);


  char batteryLevel = 100;
  const char * batteryLevelPtr = &batteryLevel;

  Serial.begin(115200);
  //while (! Serial); // Wait until Serial is ready
  Serial.println("setup()");

// set advertising packet content
  blePeripheral.setLocalName(device_name);


// add services and characteristics
  blePeripheral.addAttribute(PebbleService);
  //blePeripheral.addAttribute(CurrentTimeService);
  blePeripheral.addAttribute(CurrentTime);

  //blePeripheral.addAttribute(PotsService);
  blePeripheral.addAttribute(Pots);

  //blePeripheral.addAttribute(BatteryService);
  blePeripheral.addAttribute(BatteryLevel);

  //blePeripheral.addAttribute(TimePointService);
  blePeripheral.addAttribute(NewTimePoint);

  //blePeripheral.addAttribute(ValveControllerService);
  blePeripheral.addAttribute(ValveCommand);

  //blePeripheral.addAttribute(LogService);
  blePeripheral.addAttribute(LogEvent);

  Serial.println("attribute table constructed");
  // begin advertising
  blePeripheral.begin();
  Serial.println("Advertising");
  BatteryLevel.setValue(batteryLevelPtr);
  //LogService_EventCharacteristic.setValue(logEventPtr);
}


void solenoidOpen() {
   Serial.println("Opening Solenoid.");
   digitalWrite(ledStart, HIGH);
   digitalWrite(solenoidP, LOW);
   digitalWrite(solenoidN, HIGH);
   delay(100);              // wait for a second
   digitalWrite(solenoidP, HIGH);
   digitalWrite(solenoidN, HIGH);
}

void solenoidClose() {
   Serial.println("Closing Solenoid.");
   digitalWrite(ledStart, LOW);
   digitalWrite(solenoidP, HIGH);
   digitalWrite(solenoidN, LOW);
   delay(100);              // wait for a second
   digitalWrite(solenoidP, HIGH);
   digitalWrite(solenoidN, HIGH);
}

void solenoidCloseLog()
{
  logData[logDataCursor].eventCode = 0x12;
  logData[logDataCursor].eventTime = now();
  logData[logDataCursor].data[8] = (unsigned char)(flowCounter/256);
  logData[logDataCursor].data[9] = (unsigned char)(flowCounter % 256);
  logDataCursor++;
  solenoidClose();
  Serial.print("flowCounter = ");
  Serial.println(flowCounter, DEC);
  flowCounter = 0;
}

void beep() {
  //PM.wakeFromDoze();
  noInterrupts();
  digitalWrite(2, LOW);
  pinMode(2, OUTPUT);
  Serial.println("Interrupts disabled. External Button Interrupt Triggered.");
  digitalWrite(13, HIGH);
  delay(50);              // wait for a second
  digitalWrite(13, LOW);
  Alarm.timerOnce(10, debounce);
  // begin advertising
  blePeripheral.begin();
  Serial.println("advertising");
}

void count() {
  flowCounter++;
}

void debounce() {
  Serial.println("Interrupts re-enabled.");
  interrupts();
  pinMode(2, INPUT);
  blePeripheral.end();
  Serial.println("Advertising stopped");
  Alarm.delay(1000);
  //PM.doze();
}

void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    if (inChar == '\n') {
      stringComplete = true;
    }
  }
}

void waterDischarge(uint16_t volume, uint16_t duration)
{
  if(volume==0)
    {
     id[28] = Alarm.timerOnce(duration*60, solenoidCloseLog);
     flowCounter = 0;
     }
  else
  {
    if(volume>=2000)
   {
      countStop = volume/1.3;
      Serial.print("countStop= ");
      Serial.println(countStop, DEC);
      countStart=1;
      flowCounter=0;
    }
   else
    {
      countStop = volume/1.4;
      Serial.print("countStop = ");
      Serial.println(countStop, DEC);
      countStart=1;
      flowCounter=0;
    }
   }
 }

void sessionAlarm0() {
  //call a one time timer according to the value duration[1] with solenoidClose to be executed when this timer is triggered
  if(systemPause == 0)
  {
  waterDischarge(volume[0],duration[0]);
  logData[logDataCursor].eventCode = 0x11;
  logData[logDataCursor].eventTime = now();
  logData[logDataCursor].data[6] = (unsigned char) (duration[0] / 256);
  logData[logDataCursor].data[7] = (unsigned char) (duration[0] % 256);
  logData[logDataCursor].data[8] = (unsigned char) (volume[0] / 256);
  logData[logDataCursor].data[9] = (unsigned char) (volume[0] % 256);
  logDataCursor++;
  solenoidOpen();
  Serial.println("session Alarm 0");
  }
}

void sessionAlarm1() {
  //call a one time timer according to the value duration[1] with solenoidClose to be executed when this timer is triggered
  if(systemPause == 0)
  {
  waterDischarge(volume[1],duration[1]);
  logData[logDataCursor].eventCode = 0x11;
  logData[logDataCursor].eventTime = now();
  logData[logDataCursor].data[6] = (unsigned char)(duration[1]/256);
  logData[logDataCursor].data[7] = (unsigned char)(duration[1]%256);
  logData[logDataCursor].data[8] = (unsigned char)(volume[1]/256);
  logData[logDataCursor].data[9] = (unsigned char)(volume[1]%256);
  logDataCursor++;
  solenoidOpen();
  Serial.println("session Alarm 1");
  }
}

void sessionAlarm2() {
  //call a one time timer according to the value duration[1] with solenoidClose to be executed when this timer is triggered
  if(systemPause == 0)
  {
  waterDischarge(volume[2],duration[2]);
  logData[logDataCursor].eventCode = 0x11;
  logData[logDataCursor].eventTime = now();
  logData[logDataCursor].data[6] = (unsigned char)(duration[2]/256);
  logData[logDataCursor].data[7] = (unsigned char)(duration[2]%256);
  logData[logDataCursor].data[8] = (unsigned char)(volume[2]/256);
  logData[logDataCursor].data[9] = (unsigned char)(volume[2]%256);
  logDataCursor++;
  solenoidOpen();
  Serial.println("session Alarm 2");
  }
}

void sessionAlarm3() {
  //call a one time timer according to the value duration[1] with solenoidClose to be executed when this timer is triggered
  if(systemPause == 0)
 {
  waterDischarge(volume[3],duration[3]);
  logData[logDataCursor].eventCode = 0x11;
  logData[logDataCursor].eventTime = now();
  logData[logDataCursor].data[6] = (unsigned char)(duration[3]/256);
  logData[logDataCursor].data[7] = (unsigned char)(duration[3]%256);
  logData[logDataCursor].data[8] = (unsigned char)(volume[3]/256);
  logData[logDataCursor].data[9] = (unsigned char)(volume[3]%256);
  logDataCursor++;
  solenoidOpen();
  Serial.println("session Alarm 3");
  }
}

void sessionAlarm4() {
  //call a one time timer according to the value duration[1] with solenoidClose to be executed when this timer is triggered
  if(systemPause == 0)
  {
  waterDischarge(volume[4],duration[4]);
  logData[logDataCursor].eventCode = 0x11;
  logData[logDataCursor].eventTime = now();
  logData[logDataCursor].data[6] = (unsigned char)(duration[4]/256);
  logData[logDataCursor].data[7] = (unsigned char)(duration[4]%256);
  logData[logDataCursor].data[8] = (unsigned char)(volume[4]/256);
  logData[logDataCursor].data[9] = (unsigned char)(volume[4]%256);
  logDataCursor++;
  solenoidOpen();
  Serial.println("session Alarm 4");
  }
}

void sessionAlarm5() {
  //call a one time timer according to the value duration[1] with solenoidClose to be executed when this timer is triggered
 if(systemPause == 0)
{
  waterDischarge(volume[5],duration[5]);
  logData[logDataCursor].eventCode = 0x11;
  logData[logDataCursor].eventTime = now();
  logData[logDataCursor].data[6] = (unsigned char)(duration[5]/256);
  logData[logDataCursor].data[7] = (unsigned char)(duration[5]%256);
  logData[logDataCursor].data[8] = (unsigned char)(volume[5]/256);
  logData[logDataCursor].data[9] = (unsigned char)(volume[5]%256);
  logDataCursor++;
  solenoidOpen();
  Serial.println("session Alarm 5");
 }
}

void sessionAlarm6() {
  //call a one time timer according to the value duration[1] with solenoidClose to be executed when this timer is triggered
  if(systemPause == 0)
{
  waterDischarge(volume[6],duration[6]);
  logData[logDataCursor].eventCode = 0x11;
  logData[logDataCursor].eventTime = now();
  logData[logDataCursor].data[6] = (unsigned char)(duration[6]/256);
  logData[logDataCursor].data[7] = (unsigned char)(duration[6]%256);
  logData[logDataCursor].data[8] = (unsigned char)(volume[6]/256);
  logData[logDataCursor].data[9] = (unsigned char)(volume[6]%256);
  logDataCursor++;
  solenoidOpen();
  Serial.println("session Alarm 6");
 }
}

void sessionAlarm7() {
  //call a one time timer according to the value duration[1] with solenoidClose to be executed when this timer is triggered
if(systemPause == 0)
{
  waterDischarge(volume[7],duration[7]);
  logData[logDataCursor].eventCode = 0x11;
  logData[logDataCursor].eventTime = now();
  logData[logDataCursor].data[6] = (unsigned char)(duration[7]/256);
  logData[logDataCursor].data[7] = (unsigned char)(duration[7]%256);
  logData[logDataCursor].data[8] = (unsigned char)(volume[7]/256);
  logData[logDataCursor].data[9] = (unsigned char)(volume[7]%256);
  logDataCursor++;
  solenoidOpen();
  Serial.println("session Alarm 7");
 }
}

void sessionAlarm8() {
  //call a one time timer according to the value duration[1] with solenoidClose to be executed when this timer is triggered
if(systemPause == 0)
{
  waterDischarge(volume[8],duration[8]);
  logData[logDataCursor].eventCode = 0x11;
  logData[logDataCursor].eventTime = now();
  logData[logDataCursor].data[6] = (unsigned char)(duration[8]/256);
  logData[logDataCursor].data[7] = (unsigned char)(duration[8]%256);
  logData[logDataCursor].data[8] = (unsigned char)(volume[8]/256);
  logData[logDataCursor].data[9] = (unsigned char)(volume[8]%256);
  logDataCursor++;
  solenoidOpen();
  Serial.println("session Alarm 8");
}
}

void sessionAlarm9() {
  //call a one time timer according to the value duration[1] with solenoidClose to be executed when this timer is triggered
if(systemPause == 0)
{
  Serial.println("session Alarm 9");
  waterDischarge(volume[9],duration[9]);
  logData[logDataCursor].eventCode = 0x11;
  logData[logDataCursor].eventTime = now();
  logData[logDataCursor].data[6] = (unsigned char)(duration[9]/256);
  logData[logDataCursor].data[7] = (unsigned char)(duration[9]%256);
  logData[logDataCursor].data[8] = (unsigned char)(volume[9]/256);
  logData[logDataCursor].data[9] = (unsigned char)(volume[9]%256);
  logDataCursor++;
  solenoidOpen();
 }
}

void sessionAlarm10() {
  //call a one time timer according to the value duration[1] with solenoidClose to be executed when this timer is triggered
 if(systemPause == 0)
{
  Serial.println("session Alarm 10");
  waterDischarge(volume[10],duration[10]);
  logData[logDataCursor].eventCode = 0x11;
  logData[logDataCursor].eventTime = now();
  logData[logDataCursor].data[6] = (unsigned char)(duration[10]/256);
  logData[logDataCursor].data[7] = (unsigned char)(duration[10]%256);
  logData[logDataCursor].data[8] = (unsigned char)(volume[10]/256);
  logData[logDataCursor].data[9] = (unsigned char)(volume[10]%256);
  logDataCursor++;
  solenoidOpen();
 }
}

void sessionAlarm11() {
  //call a one time timer according to the value duration[1] with solenoidClose to be executed when this timer is triggered
 if(systemPause == 0)
{
  Serial.println("session Alarm 11");
  waterDischarge(volume[11],duration[11]);
  logData[logDataCursor].eventCode = 0x11;
  logData[logDataCursor].eventTime = now();
  logData[logDataCursor].data[6] = (unsigned char)(duration[11]/256);
  logData[logDataCursor].data[7] = (unsigned char)(duration[11]%256);
  logData[logDataCursor].data[8] = (unsigned char)(volume[11]/256);
  logData[logDataCursor].data[9] = (unsigned char)(volume[11]%256);
  logDataCursor++;
  solenoidOpen();
 }
}

void sessionAlarm12() {
  //call a one time timer according to the value duration[1] with solenoidClose to be executed when this timer is triggered
 if(systemPause == 0)
 {
  Serial.println("session Alarm 12");
  waterDischarge(volume[12],duration[12]);
  logData[logDataCursor].eventCode = 0x11;
  logData[logDataCursor].eventTime = now();
  logData[logDataCursor].data[6] = (unsigned char)(duration[12]/256);
  logData[logDataCursor].data[7] = (unsigned char)(duration[12]%256);
  logData[logDataCursor].data[8] = (unsigned char)(volume[12]/256);
  logData[logDataCursor].data[9] = (unsigned char)(volume[12]%256);
  logDataCursor++;
  solenoidOpen();
 }
}

void sessionAlarm13()
{
  //call a one time timer according to the value duration[1] with solenoidClose to be executed when this timer is triggered
if(systemPause == 0)
{
  Serial.println("session Alarm 13");
  waterDischarge(volume[13],duration[13]);
  logData[logDataCursor].eventCode = 0x11;
  logData[logDataCursor].eventTime = now();
  logData[logDataCursor].data[6] = (unsigned char)(duration[13]/256);
  logData[logDataCursor].data[7] = (unsigned char)(duration[13]%256);
  logData[logDataCursor].data[8] = (unsigned char)(volume[13]/256);
  logData[logDataCursor].data[9] = (unsigned char)(volume[13]%256);
  logDataCursor++;
  solenoidOpen();
 }
}

void sessionAlarm14() {
  //call a one time timer according to the value duration[1] with solenoidClose to be executed when this timer is triggered
if(systemPause == 0)
{
  Serial.println("session Alarm 14");
  waterDischarge(volume[14],duration[14]);
  logData[logDataCursor].eventCode = 0x11;
  logData[logDataCursor].eventTime = now();
  logData[logDataCursor].data[6] = (unsigned char)(duration[14]/256);
  logData[logDataCursor].data[7] = (unsigned char)(duration[14]%256);
  logData[logDataCursor].data[8] = (unsigned char)(volume[14]/256);
  logData[logDataCursor].data[9] = (unsigned char)(volume[14]%256);
  logDataCursor++;
  solenoidOpen();
 }
}

void sessionAlarm15() {
  //call a one time timer according to the value duration[1] with solenoidClose to be executed when this timer is triggered
if(systemPause == 0)
{
  Serial.println("session Alarm 15");
  waterDischarge(volume[15],duration[15]);
  logData[logDataCursor].eventCode = 0x11;
  logData[logDataCursor].eventTime = now();
  logData[logDataCursor].data[6] = (unsigned char)(duration[15]/256);
  logData[logDataCursor].data[7] = (unsigned char)(duration[15]%256);
  logData[logDataCursor].data[8] = (unsigned char)(volume[15]/256);
  logData[logDataCursor].data[9] = (unsigned char)(volume[15]%256);
  logDataCursor++;
  solenoidOpen();
 }
}

void sessionAlarm16() {
  //call a one time timer according to the value duration[1] with solenoidClose to be executed when this timer is triggered
 if(systemPause == 0)
{
  Serial.println("session Alarm 16");
  waterDischarge(volume[16],duration[16]);
  logData[logDataCursor].eventCode = 0x11;
  logData[logDataCursor].eventTime = now();
  logData[logDataCursor].data[6] = (unsigned char)(duration[16]/256);
  logData[logDataCursor].data[7] = (unsigned char)(duration[16]%256);
  logData[logDataCursor].data[8] = (unsigned char)(volume[16]/256);
  logData[logDataCursor].data[9] = (unsigned char)(volume[16]%256);
  logDataCursor++;
  solenoidOpen();
 }
}

void sessionAlarm17() {
  //call a one time timer according to the value duration[1] with solenoidClose to be executed when this timer is triggered
 if(systemPause == 0)
{
  Serial.println("session Alarm 17");
  waterDischarge(volume[17],duration[17]);
  logData[logDataCursor].eventCode = 0x11;
  logData[logDataCursor].eventTime = now();
  logData[logDataCursor].data[6] = (unsigned char)(duration[17]/256);
  logData[logDataCursor].data[7] = (unsigned char)(duration[17]%256);
  logData[logDataCursor].data[8] = (unsigned char)(volume[17]/256);
  logData[logDataCursor].data[9] = (unsigned char)(volume[17]%256);
  logDataCursor++;
  solenoidOpen();
 }
}

void sessionAlarm18() {
  //call a one time timer according to the value duration[1] with solenoidClose to be executed when this timer is triggered
if(systemPause == 0)
{
  Serial.println("session Alarm 18");
  waterDischarge(volume[18],duration[18]);
  logData[logDataCursor].eventCode = 0x11;
  logData[logDataCursor].eventTime = now();
  logData[logDataCursor].data[6] = (unsigned char)(duration[18]/256);
  logData[logDataCursor].data[7] = (unsigned char)(duration[18]%256);
  logData[logDataCursor].data[8] = (unsigned char)(volume[18]/256);
  logData[logDataCursor].data[9] = (unsigned char)(volume[18]%256);
  logDataCursor++;
  solenoidOpen();
 }
}

void sessionAlarm19() {
  //call a one time timer according to the value duration[1] with solenoidClose to be executed when this timer is triggered
 if(systemPause == 0)
{
  Serial.println("session Alarm 19");
  waterDischarge(volume[19],duration[19]);
  logData[logDataCursor].eventCode = 0x11;
  logData[logDataCursor].eventTime = now();
  logData[logDataCursor].data[6] = (unsigned char)(duration[19]/256);
  logData[logDataCursor].data[7] = (unsigned char)(duration[19]%256);
  logData[logDataCursor].data[8] = (unsigned char)(volume[19]/256);
  logData[logDataCursor].data[9] = (unsigned char)(volume[19]%256);
  logDataCursor++;
  solenoidOpen();
 }
}

void sessionAlarm20() {
  //call a one time timer according to the value duration[1] with solenoidClose to be executed when this timer is triggered
if(systemPause == 0)
{
  Serial.println("session Alarm 20");
  waterDischarge(volume[20],duration[20]);
  logData[logDataCursor].eventCode = 0x11;
  logData[logDataCursor].eventTime = now();
  logData[logDataCursor].data[6] = (unsigned char)(duration[20]/256);
  logData[logDataCursor].data[7] = (unsigned char)(duration[20]%256);
  logData[logDataCursor].data[8] = (unsigned char)(volume[20]/256);
  logData[logDataCursor].data[9] = (unsigned char)(volume[20]%256);
  logDataCursor++;
  solenoidOpen();
 }
}

void sessionAlarm21() {
  //call a one time timer according to the value duration[1] with solenoidClose to be executed when this timer is triggered
if(systemPause == 0)
{
  Serial.println("session Alarm 21");
  waterDischarge(volume[21],duration[21]);
  logData[logDataCursor].eventCode = 0x11;
  logData[logDataCursor].eventTime = now();
  logData[logDataCursor].data[6] = (unsigned char)(duration[21]/256);
  logData[logDataCursor].data[7] = (unsigned char)(duration[21]%256);
  logData[logDataCursor].data[8] = (unsigned char)(volume[21]/256);
  logData[logDataCursor].data[9] = (unsigned char)(volume[21]%256);
  logDataCursor++;
  solenoidOpen();
 }
}

void sessionAlarm22() {
  //call a one time timer according to the value duration[1] with solenoidClose to be executed when this timer is triggered
if(systemPause == 0)
{
  Serial.println("session Alarm 22");
  waterDischarge(volume[22],duration[22]);
  logData[logDataCursor].eventCode = 0x11;
  logData[logDataCursor].eventTime = now();
  logData[logDataCursor].data[6] = (unsigned char)(duration[22]/256);
  logData[logDataCursor].data[7] = (unsigned char)(duration[22]%256);
  logData[logDataCursor].data[8] = (unsigned char)(volume[22]/256);
  logData[logDataCursor].data[9] = (unsigned char)(volume[22]%256);
  logDataCursor++;
  solenoidOpen();
 }
}

void sessionAlarm23() {
  //call a one time timer according to the value duration[1] with solenoidClose to be executed when this timer is triggered
 if(systemPause == 0)
{
  Serial.println("session Alarm 23");
  waterDischarge(volume[23],duration[23]);
  logData[logDataCursor].eventCode = 0x11;
  logData[logDataCursor].eventTime = now();
  logData[logDataCursor].data[6] = (unsigned char)(duration[23]/256);
  logData[logDataCursor].data[7] = (unsigned char)(duration[23]%256);
  logData[logDataCursor].data[8] = (unsigned char)(volume[23]/256);
  logData[logDataCursor].data[9] = (unsigned char)(volume[23]%256);
  logDataCursor++;
  solenoidOpen();
 }
}

void sessionAlarm24() {
  //call a one time timer according to the value duration[1] with solenoidClose to be executed when this timer is triggered
 if(systemPause == 0)
{
  Serial.println("session Alarm 24");
  waterDischarge(volume[24],duration[24]);
  logData[logDataCursor].eventCode = 0x11;
  logData[logDataCursor].eventTime = now();
  logData[logDataCursor].data[6] = (unsigned char)(duration[24]/256);
  logData[logDataCursor].data[7] = (unsigned char)(duration[24]%256);
  logData[logDataCursor].data[8] = (unsigned char)(volume[24]/256);
  logData[logDataCursor].data[9] = (unsigned char)(volume[24]%256);
  logDataCursor++;
  solenoidOpen();
 }
}

void sessionAlarm25() {
  //call a one time timer according to the value duration[1] with solenoidClose to be executed when this timer is triggered
 if(systemPause == 0)
{
  Serial.println("session Alarm 25");
  waterDischarge(volume[25],duration[25]);
  logData[logDataCursor].eventCode = 0x11;
  logData[logDataCursor].eventTime = now();
  logData[logDataCursor].data[6] = (unsigned char)(duration[25]/256);
  logData[logDataCursor].data[7] = (unsigned char)(duration[25]%256);
  logData[logDataCursor].data[8] = (unsigned char)(volume[25]/256);
  logData[logDataCursor].data[9] = (unsigned char)(volume[25]%256);
  logDataCursor++;
  solenoidOpen();
 }
}

void sessionAlarm26() {
  //call a one time timer according to the value duration[1] with solenoidClose to be executed when this timer is triggered
 if(systemPause == 0)
{
  Serial.println("session Alarm 26");
  waterDischarge(volume[26],duration[26]);
  logData[logDataCursor].eventCode = 0x11;
  logData[logDataCursor].eventTime = now();
  logData[logDataCursor].data[6] = (unsigned char)(duration[26]/256);
  logData[logDataCursor].data[7] = (unsigned char)(duration[26]%256);
  logData[logDataCursor].data[8] = (unsigned char)(volume[26]/256);
  logData[logDataCursor].data[9] = (unsigned char)(volume[26]%256);
  logDataCursor++;
  solenoidOpen();
 }
}

void sessionAlarm27() {
  //call a one time timer according to the value duration[1] with solenoidClose to be executed when this timer is triggered
 if(systemPause == 0)
{
  Serial.println("session Alarm 27");
  waterDischarge(volume[27],duration[27]);
  logData[logDataCursor].eventCode = 0x11;
  logData[logDataCursor].eventTime = now();
  logData[logDataCursor].data[6] = (unsigned char)(duration[27]/256);
  logData[logDataCursor].data[7] = (unsigned char)(duration[27]%256);
  logData[logDataCursor].data[8] = (unsigned char)(volume[27]/256);
  logData[logDataCursor].data[9] = (unsigned char)(volume[27]%256);
  logDataCursor++;
  solenoidOpen();
 }
}


void loop() {
  // listen for BLE peripherals to connect:
  BLECentral central = blePeripheral.central();
  time_t t;
  if(!central)
  {
    digitalWrite(ledPower, HIGH);
    delay(250);
    digitalWrite(ledPower, LOW);
    delay(250);
  }
  Alarm.delay(0);
  if(flowCounter>=countStop && countStart==1)
   {
     logData[logDataCursor].eventCode = 0x12;
     logData[logDataCursor].eventTime = now();
     logData[logDataCursor].data[8] = (unsigned char)(flowCounter/256);
     logData[logDataCursor].data[9] = (unsigned char)(flowCounter % 256);
     logDataCursor++;
     solenoidClose();
     Serial.print("flowCounter = ");
     Serial.println(flowCounter, DEC);
     countStart=0;
     flowCounter=0;
   }

  // if a central is connected to peripheral:
  if (central) {
    Serial.print("Connected to central: ");
    Serial.println(central.address());
    digitalWrite(ledPower, HIGH);
    //Recording log event sample.
    logData[logDataCursor].eventCode = 0x01;
    logData[logDataCursor].eventTime = now();
    //centralAddress.getBytes(logData[logDataCursor].data, 12);
    //String centralAddress = String(central.address());
    //(central.address()).toCharArray(logData[logDataCursor].data, 10);
    //strcpy(logData[logDataCursor].data, (central.address()));

//TODO:@Vijay: The following loop must convert the address string to numbers such that the 12 ascii characters representing the address must fit into 6 unsigned characters.

    //The following 2 loops should initialize 6 unsigned char bytes to zero and then save the address in the 5 bytes
    uint16_t i;
    //this loop initializes the data to 0 to avoid stale data
    for(i=0;central.address()[i]!=NULL;i++) {
      logData[logDataCursor].data[i/3] = 0;
    }

    //this loop reads the address and converts the ascii to intergers
    for(i=0;central.address()[i]!=NULL;i++) {
      int placeValue = 1;
      if((i % 3) == 0)
      {
        placeValue = 16; //if the number is divisible by 3, it is to by multiplied by 16.
      }
      else if (((i - 1) % 3) == 0)
      {
        placeValue = 1; //if the number is 1 more than a number divisible by 3, it is to be added directly without multiplication
      }
      if(((central.address())[i]>='A'&&(central.address())[i]<='F'))
        logData[logDataCursor].data[i/3] = logData[logDataCursor].data[i/3] + (placeValue * ((unsigned char)(central.address())[i] - 55)); //A = 65, F = 70. These represent 10 to 15 in hexadecimal. Therefore, subtract 55.
      if(((central.address())[i]>='0'&&(central.address())[i]<='9'))
        logData[logDataCursor].data[i/3] = logData[logDataCursor].data[i/3] + (placeValue * ((unsigned char)(central.address())[i] - 48)); //0 = 48, 9 = 57. These represent 0 to 9 in hexadecimal. Therefore, subtract 48.
    }
     Serial.print(logData[logDataCursor].data[0], HEX);
     Serial.print(logData[logDataCursor].data[1], HEX);
     Serial.print(logData[logDataCursor].data[2], HEX);
     Serial.print(logData[logDataCursor].data[3], HEX);
     Serial.print(logData[logDataCursor].data[4], HEX);
     Serial.println(logData[logDataCursor].data[5], HEX);
    logDataCursor++;
    //

    // while the central is still connected to peripheral:
    while (central.connected()) {
        Alarm.delay(0);
        //Serial.print("Volume : ");
        //Serial.println(volume);
        if(flowCounter>=countStop && countStart==1)
         {
           logData[logDataCursor].eventCode = 0x12;
           logData[logDataCursor].eventTime = now();
           logData[logDataCursor].data[8] = (unsigned char)(flowCounter/256);
           logData[logDataCursor].data[9] = (unsigned char)(flowCounter % 256);
           logDataCursor++;
           solenoidClose();
           Serial.print("flowCounter = ");
           Serial.println(flowCounter, DEC);
           countStart=0;
           flowCounter=0;
         }

        ////////////////////
        //Time Synchronization Service
        ////////////////////
        if (CurrentTime.written()) {
        // application logic for handling WRITE or WRITE_WITHOUT_RESPONSE on characteristic Current Time Service Current Time goes here
         digitalWrite(13, HIGH);
         delay(50);              // wait for a second
         digitalWrite(13, LOW);
         delay(50);              // wait for a second

         uint16_t copyingIndex;
         for(copyingIndex = 0; copyingIndex<CurrentTime.valueLength(); copyingIndex++)
         {
           AttributeValue[copyingIndex] = CurrentTime.value()[copyingIndex];
         }
         AttributeValue[copyingIndex] = NULL;

         Serial.println("CurrentTime.written()");
         Serial.print(AttributeValue[0], DEC);
         Serial.print(",");
         Serial.print(AttributeValue[1], DEC);
         Serial.print(",");
         Serial.print(AttributeValue[2], DEC);
         Serial.print(",");
         Serial.print(AttributeValue[3], DEC);
         Serial.print(",");
         Serial.print(AttributeValue[4], DEC);
         Serial.print(",");
         Serial.print(AttributeValue[5], DEC);
         Serial.print(",");
         Serial.print(AttributeValue[6], DEC);
         Serial.println(".");

         // setting up system time
        setTime(AttributeValue[0], AttributeValue[1], AttributeValue[2], AttributeValue[3], AttributeValue[4], ((256 * (unsigned char) AttributeValue[5]) + (unsigned char) AttributeValue[6]));
        t = now();
        Serial.print(hour(t));
        Serial.print(":");
        Serial.print(minute(t));
        Serial.print(":");
        Serial.print(second(t));
        Serial.print(",");
        Serial.print(day(t));
        Serial.print("/");
        Serial.print(month(t));
        Serial.print("/");
        Serial.print(year(t));
        Serial.print(",");
        Serial.print(dayStr(weekday(t)));
        Serial.println(".");
        }

        ////////////////////
        //Pots Service
        ////////////////////
        if (Pots.written()) {
        // application logic for handling WRITE or WRITE_WITHOUT_RESPONSE on characteristic Pots goes here
         uint16_t copyingIndex;
         for(copyingIndex = 0; copyingIndex<Pots.valueLength(); copyingIndex++)
         {
           AttributeValue[copyingIndex] = Pots.value()[copyingIndex];
         }
         AttributeValue[copyingIndex] = NULL;
         Serial.println("Pots.written()");
         Serial.print(AttributeValue[0], DEC);
         logData[logDataCursor].eventCode = 0x71;
         logData[logDataCursor].eventTime = now();
        logData[logDataCursor].data[7] = AttributeValue[0];
         logDataCursor++;
         Serial.println(".");
         digitalWrite(13, HIGH);
         delay(50);              // wait for a second
         digitalWrite(13, LOW);
         delay(50);              // wait for a second

        }

        ////////////////////
        //Log Service
        ////////////////////
        if (LogEvent.written()) {
        // application logic for handling WRITE or WRITE_WITHOUT_RESPONSE on characteristic Pots goes here
         uint16_t copyingIndex;
         for(copyingIndex = 0; copyingIndex<LogEvent.valueLength(); copyingIndex++)
         {
           AttributeValue[copyingIndex] = LogEvent.value()[copyingIndex];
         }
         AttributeValue[copyingIndex] = NULL;
        // const unsigned char logEventData[15] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
         Serial.print("LogService_EventCharacteristic written. Characteristic will be set to logData of ");
         Serial.print((((unsigned char) AttributeValue[0]) * 256) + (unsigned char) AttributeValue[1], DEC);
         Serial.println(".");
         uint16_t serialNumber = (((unsigned char) AttributeValue[0]) * 256) + (unsigned char) AttributeValue[1];
         unsigned long eventTime = logData[serialNumber].eventTime - 19800; //This time has to be in GMT. IST - 5hours30minutes gives GMT.
         unsigned char time[4];
         time[0] = (unsigned char)(eventTime >> 24);
         time[1] = (unsigned char)(eventTime >> 16);
         time[2] = (unsigned char)(eventTime >> 8);
         time[3] = (unsigned char)(eventTime);
         const unsigned char logEventData[15] = {logData[serialNumber].eventCode, time[0],time[1],time[2],time[3],logData[serialNumber].data[0],logData[serialNumber].data[1],logData[serialNumber].data[2],logData[serialNumber].data[3],logData[serialNumber].data[4],logData[serialNumber].data[5], logData[serialNumber].data[6], logData[serialNumber].data[7], logData[serialNumber].data[8], logData[serialNumber].data[9]};
         LogEvent.setValue(logEventData, 15);

         Serial.println(eventTime, DEC);
         Serial.print(time[0], DEC);
         Serial.print(" ");
         Serial.print(time[1], DEC);
         Serial.print(" ");
         Serial.print(time[2], DEC);
         Serial.print(" ");
         Serial.println(time[3], DEC);
         //const unsigned char logEvent[15] = {time[0],time[1],time[2],time[3],'5','6','7','8','9','A','B','C','D','E','F'};

         //LogEvent.setValue(logEvent, 15);
         digitalWrite(13, HIGH);
         delay(50);              // wait for a second
         digitalWrite(13, LOW);
         delay(50);              // wait for a second

        }

        ////////////////////
        //Valve Controller Commands
        ////////////////////
        if (ValveCommand.written()) {
        // application logic for handling WRITE or WRITE_WITHOUT_RESPONSE on characteristic Pots goes here
         uint16_t copyingIndex;
         for(copyingIndex = 0; copyingIndex<ValveCommand.valueLength(); copyingIndex++)
         {
           AttributeValue[copyingIndex] = ValveCommand.value()[copyingIndex];
         }
         AttributeValue[copyingIndex] = NULL;

         Serial.println("ValveCommand.written()");
         switch(AttributeValue[0])
         {
          case 1:
            if(systemPause == 0)
            {
              Serial.println("Flush open written");
              flowCounter = 0;
              logData[logDataCursor].eventCode = 0x51;
              logData[logDataCursor].eventTime = now();
              logDataCursor++;
              solenoidOpen();
            }
            else
                Serial.println("Pebble is paused");
          break;

          case 2:
            Serial.println("Start written");
            digitalWrite(ledStop, LOW);
            logData[logDataCursor].eventCode = 0x61;
            logData[logDataCursor].eventTime = now();
            logDataCursor++;
            systemPause = 0;           //Pebble is unpaused
          break;

          case 3:
            Serial.println("Stop written");
            logData[logDataCursor].eventCode = 0x62;
            logData[logDataCursor].eventTime = now();
            logDataCursor++;
            int j;
            for(j = 0; j < 32; j++)
            {
              Alarm.free(id[j]);
            }
            Serial.println("All Time points erased");
            digitalWrite(ledStart, LOW);
            uint8_t m;
            for(m=0; m<5; m++)
            {
            digitalWrite(ledStop, HIGH);
            delay(200);
            digitalWrite(ledStop, LOW);
            delay(200);
           }
          break;

          case 4:
            Serial.println("Pause written");
            logData[logDataCursor].eventCode = 0x63;
            logData[logDataCursor].eventTime = now();
            logDataCursor++;
            systemPause = 1;
            Serial.println("Pebble paused ");
            //uint8_t m;
            //for(m=0; m<5; m++)
            //{
              digitalWrite(ledPower, HIGH);
              digitalWrite(ledStop, HIGH);
            //  delay(500);
            //  digitalWrite(ledStop, LOW);
            //  digitalWrite(ledPower, LOW);
            //delay(500);
          // }
            //digitalWrite(ledPower, HIGH);
          break;

          case 5:
            if(systemPause == 0)
            {
             Serial.println("Flush close written");
             logData[logDataCursor].eventCode = 0x52;
             logData[logDataCursor].eventTime = now();
             logData[logDataCursor].data[8] = (unsigned char)(flowCounter/256);
             logData[logDataCursor].data[9] = (unsigned char)(flowCounter % 256);
             logDataCursor++;
             solenoidClose();
             }
             else
                 Serial.println("Pebble is paused");
             break;

          default:
            Serial.println("Unknown command");
          break;
         }
         Serial.println(".");
         digitalWrite(13, HIGH);
         delay(50);              // wait for a second
         digitalWrite(13, LOW);
         delay(50);              // wait for a second

        }


        ////////////////////
        //Time Points Service
        ////////////////////
        if (NewTimePoint.written()) {
        // application logic for handling WRITE or WRITE_WITHOUT_RESPONSE on characteristic Current Time Service Current Time goes here
         sprintf(AttributeValue,"%c",NULL);
         uint16_t copyingIndex;
         for(copyingIndex = 0; copyingIndex<NewTimePoint.valueLength(); copyingIndex++)
         {
           AttributeValue[copyingIndex] = NewTimePoint.value()[copyingIndex];
         }
         AttributeValue[copyingIndex] = NULL;
         //strncpy(AttributeValue,(char*)TimePointService_NewPoint.value(),TimePointService_NewPoint.valueLength());
         //Serial.println(AttributeValue);

         /*Serial.println("TimePointService_NewWateringTimePoint.written()");
         Serial.print(AttributeValue[0], DEC);
         Serial.print(",");
         Serial.print(AttributeValue[1], DEC);
         Serial.print(",");
         Serial.print(AttributeValue[2], DEC);
         Serial.print(",");
         Serial.print(AttributeValue[3], DEC);
         Serial.print(",");
         Serial.print(AttributeValue[4], DEC);
         Serial.print(",");
         Serial.print(AttributeValue[5], DEC);
         Serial.print(",");
         Serial.print(AttributeValue[6], DEC);
         Serial.print(",");
         Serial.print(AttributeValue[7], DEC);
         Serial.print(",");
         Serial.print(AttributeValue[8], DEC);
         Serial.println(".");*/

         if(AttributeValue[0] == 0)
         {
          //reset all alarms for time point index
          logData[logDataCursor].eventCode = 0x14;
          logData[logDataCursor].eventTime = now();
          logDataCursor++;

          int i;
          for(i = 0; i < 32; i++)
          {
            Alarm.free(id[i]);
          }
          Serial.println("Old time points erased.");

         }
         else
         {
           logData[logDataCursor].eventCode = 0x15;
           logData[logDataCursor].eventTime = now();
           uint16_t copyValue=0;
           for(copyValue=0; copyValue<9; copyValue++)
           {
             logData[logDataCursor].data[copyValue+1] = (unsigned char)AttributeValue[copyValue];
           }
           logDataCursor++;
          //set alarm as per time point index (1 - 32)
          switch(AttributeValue[0])
          {
            case 1: //Alarm1
              id[AttributeValue[0]-1] = Alarm.alarmRepeat(alarmDay[(int) AttributeValue[1] -1], (int) AttributeValue[2], (int) AttributeValue[3], (int) AttributeValue[4], sessionAlarm0);
              Serial.print("Setting Alarm ");
              Serial.println(alarmDay[(int) AttributeValue[1] -1], DEC);
              Serial.print(AttributeValue[2], DEC);
              Serial.print(":");
              Serial.print(AttributeValue[3], DEC);
              Serial.print(":");
              Serial.println(AttributeValue[4], DEC);
              duration[AttributeValue[0]-1] = (((unsigned char) AttributeValue[5]) * 256) + (unsigned char) AttributeValue[6];
              volume[AttributeValue[0]-1] = (((unsigned char) AttributeValue[7]) * 256) + (unsigned char) AttributeValue[8];
              Serial.print("Volume : ");
              Serial.print(volume[AttributeValue[0]-1]);
              Serial.print("  Duration : ");
              Serial.println(duration[AttributeValue[0]-1]);
            break;

            case 2: //Alarm2
              id[AttributeValue[0]-1] = Alarm.alarmRepeat(alarmDay[(int) AttributeValue[1] -1], (int) AttributeValue[2], (int) AttributeValue[3], (int) AttributeValue[4], sessionAlarm1);
              Serial.print("Setting Alarm ");
              Serial.println(alarmDay[(int) AttributeValue[1] -1], DEC);
              Serial.print(AttributeValue[2], DEC);
              Serial.print(":");
              Serial.print(AttributeValue[3], DEC);
              Serial.print(":");
              Serial.println(AttributeValue[4], DEC);
              duration[AttributeValue[0]-1] = (((unsigned char) AttributeValue[5]) * 256) + (unsigned char) AttributeValue[6];
              volume[AttributeValue[0]-1] = (((unsigned char) AttributeValue[7]) * 256) + (unsigned char) AttributeValue[8];
              Serial.print("Volume : ");
              Serial.print(volume[AttributeValue[0]-1]);
              Serial.print("  Duration : ");
              Serial.println(duration[AttributeValue[0]-1]);
            break;

            case 3: //Alarm3
              id[AttributeValue[0]-1] = Alarm.alarmRepeat(alarmDay[(int) AttributeValue[1] -1], (int) AttributeValue[2], (int) AttributeValue[3], (int) AttributeValue[4], sessionAlarm2);
              Serial.print("Setting Alarm ");
              Serial.println(alarmDay[(int) AttributeValue[1] -1], DEC);
              Serial.print(AttributeValue[2], DEC);
              Serial.print(":");
              Serial.print(AttributeValue[3], DEC);
              Serial.print(":");
              Serial.println(AttributeValue[4], DEC);
              duration[AttributeValue[0]-1] = (((unsigned char) AttributeValue[5]) * 256) + (unsigned char) AttributeValue[6];
              volume[AttributeValue[0]-1] = (((unsigned char) AttributeValue[7]) * 256) + (unsigned char) AttributeValue[8];
              Serial.print("Volume : ");
              Serial.print(volume[AttributeValue[0]-1]);
              Serial.print("  Duration : ");
              Serial.println(duration[AttributeValue[0]-1]);
            break;

            case 4: //Alarm4
              id[AttributeValue[0]-1] = Alarm.alarmRepeat(alarmDay[(int) AttributeValue[1] -1], (int) AttributeValue[2], (int) AttributeValue[3], (int) AttributeValue[4], sessionAlarm3);
              Serial.print("Setting Alarm ");
              Serial.println(alarmDay[(int) AttributeValue[1] -1], DEC);
              Serial.print(AttributeValue[2], DEC);
              Serial.print(":");
              Serial.print(AttributeValue[3], DEC);
              Serial.print(":");
              Serial.println(AttributeValue[4], DEC);
              duration[AttributeValue[0]-1] = (((unsigned char) AttributeValue[5]) * 256) + (unsigned char) AttributeValue[6];
              volume[AttributeValue[0]-1] = (((unsigned char) AttributeValue[7]) * 256) + (unsigned char) AttributeValue[8];
              Serial.print("Volume : ");
              Serial.print(volume[AttributeValue[0]-1]);
              Serial.print("  Duration : ");
              Serial.println(duration[AttributeValue[0]-1]);
            break;


            case 5: //Alarm5
              id[AttributeValue[0]-1] = Alarm.alarmRepeat(alarmDay[(int) AttributeValue[1] -1], (int) AttributeValue[2], (int) AttributeValue[3], (int) AttributeValue[4], sessionAlarm4);
              Serial.print("Setting Alarm ");
              Serial.println(alarmDay[(int) AttributeValue[1] -1], DEC);
              Serial.print(AttributeValue[2], DEC);
              Serial.print(":");
              Serial.print(AttributeValue[3], DEC);
              Serial.print(":");
              Serial.println(AttributeValue[4], DEC);
              duration[AttributeValue[0]-1] = (((unsigned char) AttributeValue[5]) * 256) + (unsigned char) AttributeValue[6];
              volume[AttributeValue[0]-1] = (((unsigned char) AttributeValue[7]) * 256) + (unsigned char) AttributeValue[8];
              Serial.print("Volume : ");
              Serial.print(volume[AttributeValue[0]-1]);
              Serial.print("  Duration : ");
              Serial.println(duration[AttributeValue[0]-1]);
            break;

            case 6: //Alarm6
              id[AttributeValue[0]-1] = Alarm.alarmRepeat(alarmDay[(int) AttributeValue[1] -1], (int) AttributeValue[2], (int) AttributeValue[3], (int) AttributeValue[4], sessionAlarm5);
              Serial.print("Setting Alarm ");
              Serial.println(alarmDay[(int) AttributeValue[1] -1], DEC);
              Serial.print(AttributeValue[2], DEC);
              Serial.print(":");
              Serial.print(AttributeValue[3], DEC);
              Serial.print(":");
              Serial.println(AttributeValue[4], DEC);
              duration[AttributeValue[0]-1] = (((unsigned char) AttributeValue[5]) * 256) + (unsigned char) AttributeValue[6];
              volume[AttributeValue[0]-1] = (((unsigned char) AttributeValue[7]) * 256) + (unsigned char) AttributeValue[8];
              Serial.print("Volume : ");
              Serial.print(volume[AttributeValue[0]-1]);
              Serial.print("  Duration : ");
              Serial.println(duration[AttributeValue[0]-1]);
            break;

            case 7: //Alarm7
              id[AttributeValue[0]-1] = Alarm.alarmRepeat(alarmDay[(int) AttributeValue[1] -1], (int) AttributeValue[2], (int) AttributeValue[3], (int) AttributeValue[4], sessionAlarm6);
              Serial.print("Setting Alarm ");
              Serial.println(alarmDay[(int) AttributeValue[1] -1], DEC);
              Serial.print(AttributeValue[2], DEC);
              Serial.print(":");
              Serial.print(AttributeValue[3], DEC);
              Serial.print(":");
              Serial.println(AttributeValue[4], DEC);
              duration[AttributeValue[0]-1] = (((unsigned char) AttributeValue[5]) * 256) + (unsigned char) AttributeValue[6];
              volume[AttributeValue[0]-1] = (((unsigned char) AttributeValue[7]) * 256) + (unsigned char) AttributeValue[8];
              Serial.print("Volume : ");
              Serial.print(volume[AttributeValue[0]-1]);
              Serial.print("  Duration : ");
              Serial.println(duration[AttributeValue[0]-1]);
            break;

            case 8: //Alarm8
              id[AttributeValue[0]-1] = Alarm.alarmRepeat(alarmDay[(int) AttributeValue[1] -1], (int) AttributeValue[2], (int) AttributeValue[3], (int) AttributeValue[4], sessionAlarm7);
              Serial.print("Setting Alarm ");
              Serial.println(alarmDay[(int) AttributeValue[1] -1], DEC);
              Serial.print(AttributeValue[2], DEC);
              Serial.print(":");
              Serial.print(AttributeValue[3], DEC);
              Serial.print(":");
              Serial.println(AttributeValue[4], DEC);
              duration[AttributeValue[0]-1] = (((unsigned char) AttributeValue[5]) * 256) + (unsigned char) AttributeValue[6];
              volume[AttributeValue[0]-1] = (((unsigned char) AttributeValue[7]) * 256) + (unsigned char) AttributeValue[8];
              Serial.print("Volume : ");
              Serial.print(volume[AttributeValue[0]-1]);
              Serial.print("  Duration : ");
              Serial.println(duration[AttributeValue[0]-1]);
            break;

            case 9: //Alarm9
              id[AttributeValue[0]-1] = Alarm.alarmRepeat(alarmDay[(int) AttributeValue[1] -1], (int) AttributeValue[2], (int) AttributeValue[3], (int) AttributeValue[4], sessionAlarm8);
              Serial.print("Setting Alarm ");
              Serial.println(alarmDay[(int) AttributeValue[1] -1], DEC);
              Serial.print(AttributeValue[2], DEC);
              Serial.print(":");
              Serial.print(AttributeValue[3], DEC);
              Serial.print(":");
              Serial.println(AttributeValue[4], DEC);
              duration[AttributeValue[0]-1] = (((unsigned char) AttributeValue[5]) * 256) + (unsigned char) AttributeValue[6];
              volume[AttributeValue[0]-1] = (((unsigned char) AttributeValue[7]) * 256) + (unsigned char) AttributeValue[8];
              Serial.print("Volume : ");
              Serial.print(volume[AttributeValue[0]-1]);
              Serial.print("  Duration : ");
              Serial.println(duration[AttributeValue[0]-1]);
            break;

            case 10: //Alarm10
              id[AttributeValue[0]-1] = Alarm.alarmRepeat(alarmDay[(int) AttributeValue[1] -1], (int) AttributeValue[2], (int) AttributeValue[3], (int) AttributeValue[4], sessionAlarm9);
              Serial.print("Setting Alarm ");
              Serial.println(alarmDay[(int) AttributeValue[1] -1], DEC);
              Serial.print(AttributeValue[2], DEC);
              Serial.print(":");
              Serial.print(AttributeValue[3], DEC);
              Serial.print(":");
              Serial.println(AttributeValue[4], DEC);
              duration[AttributeValue[0]-1] = (((unsigned char) AttributeValue[5]) * 256) + (unsigned char) AttributeValue[6];
              volume[AttributeValue[0]-1] = (((unsigned char) AttributeValue[7]) * 256) + (unsigned char) AttributeValue[8];
              Serial.print("Volume : ");
              Serial.print(volume[AttributeValue[0]-1]);
              Serial.print("  Duration : ");
              Serial.println(duration[AttributeValue[0]-1]);
            break;

            case 11: //Alarm11
              id[AttributeValue[0]-1] = Alarm.alarmRepeat(alarmDay[(int) AttributeValue[1] -1], (int) AttributeValue[2], (int) AttributeValue[3], (int) AttributeValue[4], sessionAlarm10);
              Serial.print("Setting Alarm ");
              Serial.println(alarmDay[(int) AttributeValue[1] -1], DEC);
              Serial.print(AttributeValue[2], DEC);
              Serial.print(":");
              Serial.print(AttributeValue[3], DEC);
              Serial.print(":");
              Serial.println(AttributeValue[4], DEC);
              duration[AttributeValue[0]-1] = (((unsigned char) AttributeValue[5]) * 256) + (unsigned char) AttributeValue[6];
              volume[AttributeValue[0]-1] = (((unsigned char) AttributeValue[7]) * 256) + (unsigned char) AttributeValue[8];
              Serial.print("Volume : ");
              Serial.print(volume[AttributeValue[0]-1]);
              Serial.print("  Duration : ");
              Serial.println(duration[AttributeValue[0]-1]);
            break;

            case 12: //Alarm12
              id[AttributeValue[0]-1] = Alarm.alarmRepeat(alarmDay[(int) AttributeValue[1] -1], (int) AttributeValue[2], (int) AttributeValue[3], (int) AttributeValue[4], sessionAlarm11);
              Serial.print("Setting Alarm ");
              Serial.println(alarmDay[(int) AttributeValue[1] -1], DEC);
              Serial.print(AttributeValue[2], DEC);
              Serial.print(":");
              Serial.print(AttributeValue[3], DEC);
              Serial.print(":");
              Serial.println(AttributeValue[4], DEC);
              duration[AttributeValue[0]-1] = (((unsigned char) AttributeValue[5]) * 256) + (unsigned char) AttributeValue[6];
              volume[AttributeValue[0]-1] = (((unsigned char) AttributeValue[7]) * 256) + (unsigned char) AttributeValue[8];
              Serial.print("Volume : ");
              Serial.print(volume[AttributeValue[0]-1]);
              Serial.print("  Duration : ");
              Serial.println(duration[AttributeValue[0]-1]);
            break;

            case 13: //Alarm13
              id[AttributeValue[0]-1] = Alarm.alarmRepeat(alarmDay[(int) AttributeValue[1] -1], (int) AttributeValue[2], (int) AttributeValue[3], (int) AttributeValue[4], sessionAlarm12);
              Serial.print("Setting Alarm ");
              Serial.println(alarmDay[(int) AttributeValue[1] -1], DEC);
              Serial.print(AttributeValue[2], DEC);
              Serial.print(":");
              Serial.print(AttributeValue[3], DEC);
              Serial.print(":");
              Serial.println(AttributeValue[4], DEC);
              duration[AttributeValue[0]-1] = (((unsigned char) AttributeValue[5]) * 256) + (unsigned char) AttributeValue[6];
              volume[AttributeValue[0]-1] = (((unsigned char) AttributeValue[7]) * 256) + (unsigned char) AttributeValue[8];
              Serial.print("Volume : ");
              Serial.print(volume[AttributeValue[0]-1]);
              Serial.print("  Duration : ");
              Serial.println(duration[AttributeValue[0]-1]);
            break;

            case 14: //Alarm14
              id[AttributeValue[0]-1] = Alarm.alarmRepeat(alarmDay[(int) AttributeValue[1] -1], (int) AttributeValue[2], (int) AttributeValue[3], (int) AttributeValue[4], sessionAlarm13);
              Serial.print("Setting Alarm ");
              Serial.println(alarmDay[(int) AttributeValue[1] -1], DEC);
              Serial.print(AttributeValue[2], DEC);
              Serial.print(":");
              Serial.print(AttributeValue[3], DEC);
              Serial.print(":");
              Serial.println(AttributeValue[4], DEC);
              duration[AttributeValue[0]-1] = (((unsigned char) AttributeValue[5]) * 256) + (unsigned char) AttributeValue[6];
              volume[AttributeValue[0]-1] = (((unsigned char) AttributeValue[7]) * 256) + (unsigned char) AttributeValue[8];
              Serial.print("Volume : ");
              Serial.print(volume[AttributeValue[0]-1]);
              Serial.print("  Duration : ");
              Serial.println(duration[AttributeValue[0]-1]);
            break;

            case 15: //Alarm15
              id[AttributeValue[0]-1] = Alarm.alarmRepeat(alarmDay[(int) AttributeValue[1] -1], (int) AttributeValue[2], (int) AttributeValue[3], (int) AttributeValue[4], sessionAlarm14);
              Serial.print("Setting Alarm ");
              Serial.println(alarmDay[(int) AttributeValue[1] -1], DEC);
              Serial.print(AttributeValue[2], DEC);
              Serial.print(":");
              Serial.print(AttributeValue[3], DEC);
              Serial.print(":");
              Serial.println(AttributeValue[4], DEC);
              duration[AttributeValue[0]-1] = (((unsigned char) AttributeValue[5]) * 256) + (unsigned char) AttributeValue[6];
              volume[AttributeValue[0]-1] = (((unsigned char) AttributeValue[7]) * 256) + (unsigned char) AttributeValue[8];
              Serial.print("Volume : ");
              Serial.print(volume[AttributeValue[0]-1]);
              Serial.print("  Duration : ");
              Serial.println(duration[AttributeValue[0]-1]);
            break;

            case 16: //Alarm16
              id[AttributeValue[0]-1] = Alarm.alarmRepeat(alarmDay[(int) AttributeValue[1] -1], (int) AttributeValue[2], (int) AttributeValue[3], (int) AttributeValue[4], sessionAlarm15);
              Serial.print("Setting Alarm ");
              Serial.println(alarmDay[(int) AttributeValue[1] -1], DEC);
              Serial.print(AttributeValue[2], DEC);
              Serial.print(":");
              Serial.print(AttributeValue[3], DEC);
              Serial.print(":");
              Serial.println(AttributeValue[4], DEC);
              duration[AttributeValue[0]-1] = (((unsigned char) AttributeValue[5]) * 256) + (unsigned char) AttributeValue[6];
              volume[AttributeValue[0]-1] = (((unsigned char) AttributeValue[7]) * 256) + (unsigned char) AttributeValue[8];
              Serial.print("Volume : ");
              Serial.print(volume[AttributeValue[0]-1]);
              Serial.print("  Duration : ");
              Serial.println(duration[AttributeValue[0]-1]);
            break;

            case 17: //Alarm17
              id[AttributeValue[0]-1] = Alarm.alarmRepeat(alarmDay[(int) AttributeValue[1] -1], (int) AttributeValue[2], (int) AttributeValue[3], (int) AttributeValue[4], sessionAlarm16);
              Serial.print("Setting Alarm ");
              Serial.println(alarmDay[(int) AttributeValue[1] -1], DEC);
              Serial.print(AttributeValue[2], DEC);
              Serial.print(":");
              Serial.print(AttributeValue[3], DEC);
              Serial.print(":");
              Serial.println(AttributeValue[4], DEC);
              duration[AttributeValue[0]-1] = (((unsigned char) AttributeValue[5]) * 256) + (unsigned char) AttributeValue[6];
              volume[AttributeValue[0]-1] = (((unsigned char) AttributeValue[7]) * 256) + (unsigned char) AttributeValue[8];
              Serial.print("Volume : ");
              Serial.print(volume[AttributeValue[0]-1]);
              Serial.print("  Duration : ");
              Serial.println(duration[AttributeValue[0]-1]);
            break;

            case 18: //Alarm18
              id[AttributeValue[0]-1] = Alarm.alarmRepeat(alarmDay[(int) AttributeValue[1] -1], (int) AttributeValue[2], (int) AttributeValue[3], (int) AttributeValue[4], sessionAlarm17);
              Serial.print("Setting Alarm ");
              Serial.println(alarmDay[(int) AttributeValue[1] -1], DEC);
              Serial.print(AttributeValue[2], DEC);
              Serial.print(":");
              Serial.print(AttributeValue[3], DEC);
              Serial.print(":");
              Serial.println(AttributeValue[4], DEC);
              duration[AttributeValue[0]-1] = (((unsigned char) AttributeValue[5]) * 256) + (unsigned char) AttributeValue[6];
              volume[AttributeValue[0]-1] = (((unsigned char) AttributeValue[7]) * 256) + (unsigned char) AttributeValue[8];
              Serial.print("Volume : ");
              Serial.print(volume[AttributeValue[0]-1]);
              Serial.print("  Duration : ");
              Serial.println(duration[AttributeValue[0]-1]);
            break;

            case 19: //Alarm19
              id[AttributeValue[0]-1] = Alarm.alarmRepeat(alarmDay[(int) AttributeValue[1] -1], (int) AttributeValue[2], (int) AttributeValue[3], (int) AttributeValue[4], sessionAlarm18);
              Serial.print("Setting Alarm ");
              Serial.println(alarmDay[(int) AttributeValue[1] -1], DEC);
              Serial.print(AttributeValue[2], DEC);
              Serial.print(":");
              Serial.print(AttributeValue[3], DEC);
              Serial.print(":");
              Serial.println(AttributeValue[4], DEC);
              duration[AttributeValue[0]-1] = (((unsigned char) AttributeValue[5]) * 256) + (unsigned char) AttributeValue[6];
              volume[AttributeValue[0]-1] = (((unsigned char) AttributeValue[7]) * 256) + (unsigned char) AttributeValue[8];
              Serial.print("Volume : ");
              Serial.print(volume[AttributeValue[0]-1]);
              Serial.print("  Duration : ");
              Serial.println(duration[AttributeValue[0]-1]);
            break;

            case 20: //Alarm20
              id[AttributeValue[0]-1] = Alarm.alarmRepeat(alarmDay[(int) AttributeValue[1] -1], (int) AttributeValue[2], (int) AttributeValue[3], (int) AttributeValue[4], sessionAlarm19);
              Serial.print("Setting Alarm ");
              Serial.println(alarmDay[(int) AttributeValue[1] -1], DEC);
              Serial.print(AttributeValue[2], DEC);
              Serial.print(":");
              Serial.print(AttributeValue[3], DEC);
              Serial.print(":");
              Serial.println(AttributeValue[4], DEC);
              duration[AttributeValue[0]-1] = (((unsigned char) AttributeValue[5]) * 256) + (unsigned char) AttributeValue[6];
              volume[AttributeValue[0]-1] = (((unsigned char) AttributeValue[7]) * 256) + (unsigned char) AttributeValue[8];
              Serial.print("Volume : ");
              Serial.print(volume[AttributeValue[0]-1]);
              Serial.print("  Duration : ");
              Serial.println(duration[AttributeValue[0]-1]);
            break;

            case 21: //Alarm21
              id[AttributeValue[0]-1] = Alarm.alarmRepeat(alarmDay[(int) AttributeValue[1] -1], (int) AttributeValue[2], (int) AttributeValue[3], (int) AttributeValue[4], sessionAlarm20);
              Serial.print("Setting Alarm ");
              Serial.println(alarmDay[(int) AttributeValue[1] -1], DEC);
              Serial.print(AttributeValue[2], DEC);
              Serial.print(":");
              Serial.print(AttributeValue[3], DEC);
              Serial.print(":");
              Serial.println(AttributeValue[4], DEC);
              duration[AttributeValue[0]-1] = (((unsigned char) AttributeValue[5]) * 256) + (unsigned char) AttributeValue[6];
              volume[AttributeValue[0]-1] = (((unsigned char) AttributeValue[7]) * 256) + (unsigned char) AttributeValue[8];
              Serial.print("Volume : ");
              Serial.print(volume[AttributeValue[0]-1]);
              Serial.print("  Duration : ");
              Serial.println(duration[AttributeValue[0]-1]);
            break;

            case 22: //Alarm22
              id[AttributeValue[0]-1] = Alarm.alarmRepeat(alarmDay[(int) AttributeValue[1] -1], (int) AttributeValue[2], (int) AttributeValue[3], (int) AttributeValue[4], sessionAlarm21);
              Serial.print("Setting Alarm ");
              Serial.println(alarmDay[(int) AttributeValue[1] -1], DEC);
              Serial.print(AttributeValue[2], DEC);
              Serial.print(":");
              Serial.print(AttributeValue[3], DEC);
              Serial.print(":");
              Serial.println(AttributeValue[4], DEC);
              duration[AttributeValue[0]-1] = (((unsigned char) AttributeValue[5]) * 256) + (unsigned char) AttributeValue[6];
              volume[AttributeValue[0]-1] = (((unsigned char) AttributeValue[7]) * 256) + (unsigned char) AttributeValue[8];
              Serial.print("Volume : ");
              Serial.print(volume[AttributeValue[0]-1]);
              Serial.print("  Duration : ");
              Serial.println(duration[AttributeValue[0]-1]);
            break;

            case 23: //Alarm23
              id[AttributeValue[0]-1] = Alarm.alarmRepeat(alarmDay[(int) AttributeValue[1] -1], (int) AttributeValue[2], (int) AttributeValue[3], (int) AttributeValue[4], sessionAlarm22);
              Serial.print("Setting Alarm ");
              Serial.println(alarmDay[(int) AttributeValue[1] -1], DEC);
              Serial.print(AttributeValue[2], DEC);
              Serial.print(":");
              Serial.print(AttributeValue[3], DEC);
              Serial.print(":");
              Serial.println(AttributeValue[4], DEC);
              duration[AttributeValue[0]-1] = (((unsigned char) AttributeValue[5]) * 256) + (unsigned char) AttributeValue[6];
              volume[AttributeValue[0]-1] = (((unsigned char) AttributeValue[7]) * 256) + (unsigned char) AttributeValue[8];
              Serial.print("Volume : ");
              Serial.print(volume[AttributeValue[0]-1]);
              Serial.print("  Duration : ");
              Serial.println(duration[AttributeValue[0]-1]);
            break;

            case 24: //Alarm24
              id[AttributeValue[0]-1] = Alarm.alarmRepeat(alarmDay[(int) AttributeValue[1] -1], (int) AttributeValue[2], (int) AttributeValue[3], (int) AttributeValue[4], sessionAlarm23);
              Serial.print("Setting Alarm ");
              Serial.println(alarmDay[(int) AttributeValue[1] -1], DEC);
              Serial.print(AttributeValue[2], DEC);
              Serial.print(":");
              Serial.print(AttributeValue[3], DEC);
              Serial.print(":");
              Serial.println(AttributeValue[4], DEC);
              duration[AttributeValue[0]-1] = (((unsigned char) AttributeValue[5]) * 256) + (unsigned char) AttributeValue[6];
              volume[AttributeValue[0]-1] = (((unsigned char) AttributeValue[7]) * 256) + (unsigned char) AttributeValue[8];
              Serial.print("Volume : ");
              Serial.print(volume[AttributeValue[0]-1]);
              Serial.print("  Duration : ");
              Serial.println(duration[AttributeValue[0]-1]);
            break;

            case 25: //Alarm25
              id[AttributeValue[0]-1] = Alarm.alarmRepeat(alarmDay[(int) AttributeValue[1] -1], (int) AttributeValue[2], (int) AttributeValue[3], (int) AttributeValue[4], sessionAlarm24);
              Serial.print("Setting Alarm ");
              Serial.println(alarmDay[(int) AttributeValue[1] -1], DEC);
              Serial.print(AttributeValue[2], DEC);
              Serial.print(":");
              Serial.print(AttributeValue[3], DEC);
              Serial.print(":");
              Serial.println(AttributeValue[4], DEC);
              duration[AttributeValue[0]-1] = (((unsigned char) AttributeValue[5]) * 256) + (unsigned char) AttributeValue[6];
              volume[AttributeValue[0]-1] = (((unsigned char) AttributeValue[7]) * 256) + (unsigned char) AttributeValue[8];
              Serial.print("Volume : ");
              Serial.print(volume[AttributeValue[0]-1]);
              Serial.print("  Duration : ");
              Serial.println(duration[AttributeValue[0]-1]);
            break;

            case 26: //Alarm26
              id[AttributeValue[0]-1] = Alarm.alarmRepeat(alarmDay[(int) AttributeValue[1] -1], (int) AttributeValue[2], (int) AttributeValue[3], (int) AttributeValue[4], sessionAlarm25);
              Serial.print("Setting Alarm ");
              Serial.println(alarmDay[(int) AttributeValue[1] -1], DEC);
              Serial.print(AttributeValue[2], DEC);
              Serial.print(":");
              Serial.print(AttributeValue[3], DEC);
              Serial.print(":");
              Serial.println(AttributeValue[4], DEC);
              duration[AttributeValue[0]-1] = (((unsigned char) AttributeValue[5]) * 256) + (unsigned char) AttributeValue[6];
              volume[AttributeValue[0]-1] = (((unsigned char) AttributeValue[7]) * 256) + (unsigned char) AttributeValue[8];
              Serial.print("Volume : ");
              Serial.print(volume[AttributeValue[0]-1]);
              Serial.print("  Duration : ");
              Serial.println(duration[AttributeValue[0]-1]);
            break;

            case 27: //Alarm27
              id[AttributeValue[0]-1] = Alarm.alarmRepeat(alarmDay[(int) AttributeValue[1] -1], (int) AttributeValue[2], (int) AttributeValue[3], (int) AttributeValue[4], sessionAlarm26);
              Serial.print("Setting Alarm ");
              Serial.println(alarmDay[(int) AttributeValue[1] -1], DEC);
              Serial.print(AttributeValue[2], DEC);
              Serial.print(":");
              Serial.print(AttributeValue[3], DEC);
              Serial.print(":");
              Serial.println(AttributeValue[4], DEC);
              duration[AttributeValue[0]-1] = (((unsigned char) AttributeValue[5]) * 256) + (unsigned char) AttributeValue[6];
              volume[AttributeValue[0]-1] = (((unsigned char) AttributeValue[7]) * 256) + (unsigned char) AttributeValue[8];
              Serial.print("Volume : ");
              Serial.print(volume[AttributeValue[0]-1]);
              Serial.print("  Duration : ");
              Serial.println(duration[AttributeValue[0]-1]);
            break;

            case 28: //Alarm28
              id[AttributeValue[0]-1] = Alarm.alarmRepeat(alarmDay[(int) AttributeValue[1] -1], (int) AttributeValue[2], (int) AttributeValue[3], (int) AttributeValue[4], sessionAlarm27);
              Serial.print("Setting Alarm ");
              Serial.println(alarmDay[(int) AttributeValue[1] -1], DEC);
              Serial.print(AttributeValue[2], DEC);
              Serial.print(":");
              Serial.print(AttributeValue[3], DEC);
              Serial.print(":");
              Serial.println(AttributeValue[4], DEC);
              duration[AttributeValue[0]-1] = (((unsigned char) AttributeValue[5]) * 256) + (unsigned char) AttributeValue[6];
              volume[AttributeValue[0]-1] = (((unsigned char) AttributeValue[7]) * 256) + (unsigned char) AttributeValue[8];
              Serial.print("Volume : ");
              Serial.print(volume[AttributeValue[0]-1]);
              Serial.print("  Duration : ");
              Serial.println(duration[AttributeValue[0]-1]);
            break;
          }
         }


         digitalWrite(13, HIGH);
         delay(50);              // wait for a second
         digitalWrite(13, LOW);
         delay(50);              // wait for a second

        }


        ////////////////////
        //Battery Service
        ////////////////////
        if (BatteryLevel.read()) {
          Serial.println("Battery Level Read");
        }
    }
    // when the central disconnects, print it out:
    Serial.print("Disconnected from central: ");
    logData[logDataCursor].eventCode = 0x02;
    logData[logDataCursor].eventTime = now();
    //this loop initializes the data to 0 to avoid stale data
    for(i=0;central.address()[i]!=NULL;i++) {
      logData[logDataCursor].data[i/3] = 0;
    }

    //this loop reads the address and converts the ascii to intergers
    for(i=0;central.address()[i]!=NULL;i++) {
      int placeValue = 1;
      if((i % 3) == 0)
      {
        placeValue = 16; //if the number is divisible by 3, it is to by multiplied by 16.
      }
      else if (((i - 1) % 3) == 0)
      {
        placeValue = 1; //if the number is 1 more than a number divisible by 3, it is to be added directly without multiplication
      }
      if(((central.address())[i]>='A'&&(central.address())[i]<='F'))
        logData[logDataCursor].data[i/3] = logData[logDataCursor].data[i/3] + (placeValue * ((unsigned char)(central.address())[i] - 55)); //A = 65, F = 70. These represent 10 to 15 in hexadecimal. Therefore, subtract 55.
      if(((central.address())[i]>='0'&&(central.address())[i]<='9'))
        logData[logDataCursor].data[i/3] = logData[logDataCursor].data[i/3] + (placeValue * ((unsigned char)(central.address())[i] - 48)); //0 = 48, 9 = 57. These represent 0 to 9 in hexadecimal. Therefore, subtract 48.
    }
    logDataCursor++;
    Serial.println(central.address());
  }
}

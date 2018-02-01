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

timeDayOfWeek_t alarmDay[7] = {dowSunday, dowMonday, dowTuesday, dowWednesday, dowThursday, dowFriday, dowSaturday};

String inputString = "";         // a String to hold incoming data
boolean stringComplete = false;  // whether the string is complete
Bounce interruptButtonBouncer = Bounce();

//Alarm IDs and corresponding volumes & durations
AlarmId id[32];
static uint16_t volume[28];
static uint16_t duration[28];
static uint16_t flowCounter = 0;

// BLE objects
BLEPeripheral blePeripheral;


// GAP properties
char device_name[] = "Pebble";

// Characteristic Properties

//Pebble Characteristic Properties
unsigned char CurrentTimeService_CurrentTime_props = BLERead | BLEWrite | 0;
unsigned char BatteryService_BatteryLevel_props = BLERead | 0;
unsigned char PotsService_Pots_props = BLERead | BLEWrite | 0;
unsigned char TimePointService_NewPoint_props = BLEWrite | 0;
unsigned char ValveControllerService_Command_props = BLEWrite | 0;
unsigned char LogService_EventCharacteristic_props = BLEWrite | BLERead | 0;
//unsigned char LogService_ReadIndex_props = BLEWrite | 0;


char AttributeValue[32];

// Services and Characteristics
//Pebble Services and Characteristics
BLEService Pebble_Service("12345678123412341234123456789ABC");

//BLEService BatteryService("6E521ABEB56F4B058465A1CEE41BB141");
BLECharacteristic BatteryService_BatteryLevel("6E52C8E5B56F4B058465A1CEE41BB141", BatteryService_BatteryLevel_props, 1);
//Total Length = 1 byte
//1 byte = uint8 Level

//BLEService CurrentTimeService("6E52A9DAB56F4B058465A1CEE41BB141");
BLECharacteristic CurrentTimeService_CurrentTime("6E52AC46B56F4B058465A1CEE41BB141", CurrentTimeService_CurrentTime_props, 7);
//Total Length = 7 bytes
//1 byte = uint8 hours
//1 byte = uint8 minutes
//1 byte = uint8 seconds
//1 byte = uint8 Days
//1 byte = uint8 Months
//2 byte = uint16 Years

//BLEService PotsService("6E529F14B56F4B058465A1CEE41BB141");
BLECharacteristic PotsService_Pots("6E52E386B56F4B058465A1CEE41BB141", PotsService_Pots_props, 1);
//1 byte = Total Length
//1 byte = uint8 Number of Pots

//BLEService TimePointService("6E52214FB56F4B058465A1CEE41BB141");
BLECharacteristic TimePointService_NewPoint("6E529480B56F4B058465A1CEE41BB141", TimePointService_NewPoint_props, 9);
//Total Length = 9 bytes
//1 byte = uint8 Index (Time Point Number)
//1 byte = uint8 Day of the Week
//1 byte = uint8 hours
//1 byte = uint8 minutes
//1 byte = uint8 seconds
//2 bytes = uint16 Duration
//2 bytes = uint16 Volume

//BLEService ValveControllerService("6E52C714B56F4B058465A1CEE41BB141");
BLECharacteristic ValveControllerService_Command("6E52CFDBB56F4B058465A1CEE41BB141", ValveControllerService_Command_props, 1);
//Total Length = 1 byte
//1 byte = uint8 CommandCode

//BLEService LogService("6E52ABCDB56F4B058465A1CEE41BB141");
BLECharacteristic LogService_EventCharacteristic("6E521234B56F4B058465A1CEE41BB141", LogService_EventCharacteristic_props, 15);
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
  unsigned char data[9];
} logData[1000];

uint16_t logDataCursor = 0;

void setup() {
  pinMode(13, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(11, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(8, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(button, INPUT);
  digitalWrite(button, HIGH);
  pinMode(flowSensor, INPUT);
  interruptButtonBouncer .attach(button);
  interruptButtonBouncer .interval(5);
  attachInterrupt(digitalPinToInterrupt(button), beep, FALLING);
  attachInterrupt(digitalPinToInterrupt(flowSensor), count, CHANGE);
  interrupts();


  digitalWrite(8, HIGH);
  digitalWrite(7, HIGH);

  digitalWrite(10, HIGH);  // turn the Buzzer on (HIGH is the voltage level)
  digitalWrite(11, HIGH);
  digitalWrite(12, HIGH);
  digitalWrite(13, HIGH);
  delay(50);              // wait for a second
  digitalWrite(10, LOW);  // turn the Buzzer on (HIGH is the voltage level)
  digitalWrite(11, LOW);
  digitalWrite(12, LOW);
  digitalWrite(13, LOW);
  delay(50);              // wait for a second
  digitalWrite(10, HIGH);  // turn the Buzzer on (HIGH is the voltage level)
  digitalWrite(11, HIGH);
  digitalWrite(12, HIGH);
  digitalWrite(13, HIGH);
  delay(50);              // wait for a second
  digitalWrite(10, LOW);  // turn the Buzzer on (HIGH is the voltage level)
  digitalWrite(11, LOW);
  digitalWrite(12, LOW);
  digitalWrite(13, LOW);
  delay(50);              // wait for a second
  digitalWrite(10, HIGH);  // turn the Buzzer on (HIGH is the voltage level)
  digitalWrite(11, HIGH);
  digitalWrite(12, HIGH);
  digitalWrite(13, HIGH);
  delay(100);              // wait for a second
  digitalWrite(10, LOW);  // turn the Buzzer on (HIGH is the voltage level)
  digitalWrite(11, LOW);
  digitalWrite(12, LOW);
  digitalWrite(13, LOW);

  solenoidOpen();
  delay(2000);
  solenoidClose();
  delay(2000);

  solenoidOpen();
  delay(2000);
  solenoidClose();
  delay(2000);

  solenoidClose();

  char batteryLevel = 100;
  const char * batteryLevelPtr = &batteryLevel;
  const unsigned char logEvent[15] = {'1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
  //const unsigned char * logEventPtr = &logEvent;
  Serial.begin(115200);
  //while (! Serial); // Wait until Serial is ready
  Serial.println("setup()");

// set advertising packet content
  blePeripheral.setLocalName(device_name);


// add services and characteristics
  blePeripheral.addAttribute(Pebble_Service);
  //blePeripheral.addAttribute(CurrentTimeService);
  blePeripheral.addAttribute(CurrentTimeService_CurrentTime);

  //blePeripheral.addAttribute(PotsService);
  blePeripheral.addAttribute(PotsService_Pots);

  //blePeripheral.addAttribute(BatteryService);
  blePeripheral.addAttribute(BatteryService_BatteryLevel);

  //blePeripheral.addAttribute(TimePointService);
  blePeripheral.addAttribute(TimePointService_NewPoint);

  //blePeripheral.addAttribute(ValveControllerService);
  blePeripheral.addAttribute(ValveControllerService_Command);

  //blePeripheral.addAttribute(LogService);
  blePeripheral.addAttribute(LogService_EventCharacteristic);

  blePeripheral.setAdvertisedServiceUuid("180F");


  Serial.println("attribute table constructed");
  // begin advertising
  blePeripheral.begin();
  Serial.println("Advertising");
  BatteryService_BatteryLevel.setValue(batteryLevelPtr);
  //LogService_EventCharacteristic.setValue(logEventPtr);
}


void solenoidOpen() {
   Serial.println("Opening Solenoid.");
   digitalWrite(solenoidP, LOW);
   digitalWrite(solenoidN, HIGH);
   delay(100);              // wait for a second
   digitalWrite(solenoidP, HIGH);
   digitalWrite(solenoidN, HIGH);
}

void solenoidClose() {
   Serial.println("Closing Solenoid.");
   digitalWrite(solenoidP, HIGH);
   digitalWrite(solenoidN, LOW);
   delay(100);              // wait for a second
   digitalWrite(solenoidP, HIGH);
   digitalWrite(solenoidN, HIGH);
}

void beep() {
  //PM.wakeFromDoze();
  noInterrupts();
  digitalWrite(2, LOW);
  pinMode(2, OUTPUT);
  Serial.println("Interrupts disabled. External Button Interrupt Triggered.");
  digitalWrite(13, HIGH);
  delay(50);              // wait for a second\
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

void sessionAlarm0() {
  //call a one time timer according to the value duration[1] with solenoidClose to be executed when this timer is triggered
  id[28] = Alarm.timerOnce(duration[0]/1000, solenoidClose);
  solenoidOpen();
  Serial.println("session Alarm 0");
}

void sessionAlarm1() {
  //call a one time timer according to the value duration[1] with solenoidClose to be executed when this timer is triggered
  id[28] = Alarm.timerOnce(duration[1]/1000, solenoidClose);
  solenoidOpen();
  Serial.println("session Alarm 1");
}

void sessionAlarm2() {
  //call a one time timer according to the value duration[1] with solenoidClose to be executed when this timer is triggered
  id[28] = Alarm.timerOnce(duration[2]/1000, solenoidClose);
  solenoidOpen();
  Serial.println("session Alarm 2");
}

void sessionAlarm3() {
  //call a one time timer according to the value duration[1] with solenoidClose to be executed when this timer is triggered
  id[28] = Alarm.timerOnce(duration[3]/1000, solenoidClose);
  solenoidOpen();
  Serial.println("session Alarm 3");
}

void sessionAlarm4() {
  //call a one time timer according to the value duration[1] with solenoidClose to be executed when this timer is triggered
  id[28] = Alarm.timerOnce(duration[4]/1000, solenoidClose);
  solenoidOpen();
  Serial.println("session Alarm 4");
}

void sessionAlarm5() {
  //call a one time timer according to the value duration[1] with solenoidClose to be executed when this timer is triggered
  id[28] = Alarm.timerOnce(duration[5]/1000, solenoidClose);
  solenoidOpen();
  Serial.println("session Alarm 5");
}

void sessionAlarm6() {
  //call a one time timer according to the value duration[1] with solenoidClose to be executed when this timer is triggered
  id[28] = Alarm.timerOnce(duration[6]/1000, solenoidClose);
  solenoidOpen();
  Serial.println("session Alarm 6");
}

void sessionAlarm7() {
  //call a one time timer according to the value duration[1] with solenoidClose to be executed when this timer is triggered
  id[28] = Alarm.timerOnce(duration[7]/1000, solenoidClose);
  solenoidOpen();
  Serial.println("session Alarm 7");
}

void sessionAlarm8() {
  //call a one time timer according to the value duration[1] with solenoidClose to be executed when this timer is triggered
  id[28] = Alarm.timerOnce(duration[8]/1000, solenoidClose);
  solenoidOpen();
  Serial.println("session Alarm 8");
}

void sessionAlarm9() {
  //call a one time timer according to the value duration[1] with solenoidClose to be executed when this timer is triggered
  Serial.println("session Alarm 9");
  id[28] = Alarm.timerOnce(duration[9]/1000, solenoidClose);
  solenoidOpen();
}

void sessionAlarm10() {
  //call a one time timer according to the value duration[1] with solenoidClose to be executed when this timer is triggered
  Serial.println("session Alarm 10");
  id[28] = Alarm.timerOnce(duration[10]/1000, solenoidClose);
  solenoidOpen();
}

void sessionAlarm11() {
  //call a one time timer according to the value duration[1] with solenoidClose to be executed when this timer is triggered
  Serial.println("session Alarm 11");
  id[28] = Alarm.timerOnce(duration[11]/1000, solenoidClose);
  solenoidOpen();
}

void sessionAlarm12() {
  //call a one time timer according to the value duration[1] with solenoidClose to be executed when this timer is triggered
  Serial.println("session Alarm 12");
  id[28] = Alarm.timerOnce(duration[12]/1000, solenoidClose);
  solenoidOpen();
}

void sessionAlarm13() {
  //call a one time timer according to the value duration[1] with solenoidClose to be executed when this timer is triggered
  Serial.println("session Alarm 13");
  id[28] = Alarm.timerOnce(duration[13]/1000, solenoidClose);
  solenoidOpen();
}

void sessionAlarm14() {
  //call a one time timer according to the value duration[1] with solenoidClose to be executed when this timer is triggered
  Serial.println("session Alarm 14");
  id[28] = Alarm.timerOnce(duration[14]/1000, solenoidClose);
  solenoidOpen();
}

void sessionAlarm15() {
  //call a one time timer according to the value duration[1] with solenoidClose to be executed when this timer is triggered
  Serial.println("session Alarm 15");
  id[28] = Alarm.timerOnce(duration[15]/1000, solenoidClose);
  solenoidOpen();
}

void sessionAlarm16() {
  //call a one time timer according to the value duration[1] with solenoidClose to be executed when this timer is triggered
  Serial.println("session Alarm 16");
  id[28] = Alarm.timerOnce(duration[16]/1000, solenoidClose);
  solenoidOpen();
}

void sessionAlarm17() {
  //call a one time timer according to the value duration[1] with solenoidClose to be executed when this timer is triggered
  Serial.println("session Alarm 17");
  id[28] = Alarm.timerOnce(duration[17]/1000, solenoidClose);
  solenoidOpen();
}

void sessionAlarm18() {
  //call a one time timer according to the value duration[1] with solenoidClose to be executed when this timer is triggered
  Serial.println("session Alarm 18");
  id[28] = Alarm.timerOnce(duration[18]/1000, solenoidClose);
  solenoidOpen();
}

void sessionAlarm19() {
  //call a one time timer according to the value duration[1] with solenoidClose to be executed when this timer is triggered
  Serial.println("session Alarm 19");
  id[28] = Alarm.timerOnce(duration[19]/1000, solenoidClose);
  solenoidOpen();
}

void sessionAlarm20() {
  //call a one time timer according to the value duration[1] with solenoidClose to be executed when this timer is triggered
  Serial.println("session Alarm 20");
  id[28] = Alarm.timerOnce(duration[20]/1000, solenoidClose);
  solenoidOpen();
}

void sessionAlarm21() {
  //call a one time timer according to the value duration[1] with solenoidClose to be executed when this timer is triggered
  Serial.println("session Alarm 21");
  id[28] = Alarm.timerOnce(duration[21]/1000, solenoidClose);
  solenoidOpen();
}

void sessionAlarm22() {
  //call a one time timer according to the value duration[1] with solenoidClose to be executed when this timer is triggered
  Serial.println("session Alarm 22");
  id[28] = Alarm.timerOnce(duration[22]/1000, solenoidClose);
  solenoidOpen();
}

void sessionAlarm23() {
  //call a one time timer according to the value duration[1] with solenoidClose to be executed when this timer is triggered
  Serial.println("session Alarm 23");
  id[28] = Alarm.timerOnce(duration[23]/1000, solenoidClose);
  solenoidOpen();
}

void sessionAlarm24() {
  //call a one time timer according to the value duration[1] with solenoidClose to be executed when this timer is triggered
  Serial.println("session Alarm 24");
  id[28] = Alarm.timerOnce(duration[24]/1000, solenoidClose);
  solenoidOpen();
}

void sessionAlarm25() {
  //call a one time timer according to the value duration[1] with solenoidClose to be executed when this timer is triggered
  Serial.println("session Alarm 25");
  id[28] = Alarm.timerOnce(duration[25]/1000, solenoidClose);
  solenoidOpen();
}

void sessionAlarm26() {
  //call a one time timer according to the value duration[1] with solenoidClose to be executed when this timer is triggered
  Serial.println("session Alarm 26");
  id[28] = Alarm.timerOnce(duration[26]/1000, solenoidClose);
  solenoidOpen();
}

void sessionAlarm27() {
  //call a one time timer according to the value duration[1] with solenoidClose to be executed when this timer is triggered
  Serial.println("session Alarm 27");
  id[28] = Alarm.timerOnce(duration[27]/1000, solenoidClose);
  solenoidOpen();
}





void loop() {
  // listen for BLE peripherals to connect:
  BLECentral central = blePeripheral.central();
  time_t t;

  Alarm.delay(0);

  // if a central is connected to peripheral:
  if (central) {
    Serial.print("Connected to central: ");
    Serial.println(central.address());



    // while the central is still connected to peripheral:
    while (central.connected()) {
        Alarm.delay(0);
        //Serial.print("Volume : ");
        //Serial.println(volume);



        ////////////////////
        //Time Synchronization Service
        ////////////////////
        if (CurrentTimeService_CurrentTime.written()) {
        // application logic for handling WRITE or WRITE_WITHOUT_RESPONSE on characteristic Current Time Service Current Time goes here
         digitalWrite(13, HIGH);
         delay(50);              // wait for a second
         digitalWrite(13, LOW);
         delay(50);              // wait for a second

         uint16_t copyingIndex;
         for(copyingIndex = 0; copyingIndex<CurrentTimeService_CurrentTime.valueLength(); copyingIndex++)
         {
           AttributeValue[copyingIndex] = CurrentTimeService_CurrentTime.value()[copyingIndex];
         }
         AttributeValue[copyingIndex] = NULL;

         Serial.println("CurrentTimeService_CurrentTime.written()");
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
        if (PotsService_Pots.written()) {
        // application logic for handling WRITE or WRITE_WITHOUT_RESPONSE on characteristic Pots goes here
         uint16_t copyingIndex;
         for(copyingIndex = 0; copyingIndex<PotsService_Pots.valueLength(); copyingIndex++)
         {
           AttributeValue[copyingIndex] = PotsService_Pots.value()[copyingIndex];
         }
         AttributeValue[copyingIndex] = NULL;

         Serial.println("PotsService_Pots.written()");
         Serial.print(AttributeValue[0], DEC);
         Serial.println(".");
         digitalWrite(13, HIGH);
         delay(50);              // wait for a second
         digitalWrite(13, LOW);
         delay(50);              // wait for a second

        }

        ////////////////////
        //Log Service
        ////////////////////
        if (LogService_EventCharacteristic.written()) {
        // application logic for handling WRITE or WRITE_WITHOUT_RESPONSE on characteristic Pots goes here
         uint16_t copyingIndex;
         for(copyingIndex = 0; copyingIndex<LogService_EventCharacteristic.valueLength(); copyingIndex++)
         {
           AttributeValue[copyingIndex] = LogService_EventCharacteristic.value()[copyingIndex];
         }
         AttributeValue[copyingIndex] = NULL;

         Serial.print("LogService_EventCharacteristic written. Characteristic will be set to logData of ");
         Serial.print((((unsigned char) AttributeValue[0]) * 256) + (unsigned char) AttributeValue[1], DEC);
         Serial.println(".");

         unsigned long eventTime = now() - 19800; //This time has to be in GMT. IST - 5hours30minutes gives GMT.
         unsigned char time[4];
         time[0] = (unsigned char)(eventTime >> 24);
         time[1] = (unsigned char)(eventTime >> 16);
         time[2] = (unsigned char)(eventTime >> 8);
         time[3] = (unsigned char)(eventTime);
         Serial.println(eventTime, DEC);
         Serial.print(time[0], DEC);
         Serial.print(" ");
         Serial.print(time[1], DEC);
         Serial.print(" ");
         Serial.print(time[2], DEC);
         Serial.print(" ");
         Serial.println(time[3], DEC);
         const unsigned char logEvent[15] = {time[0],time[1],time[2],time[3],'5','6','7','8','9','A','B','C','D','E','F'};

         LogService_EventCharacteristic.setValue(logEvent, 15);
         digitalWrite(13, HIGH);
         delay(50);              // wait for a second
         digitalWrite(13, LOW);
         delay(50);              // wait for a second

        }

        ////////////////////
        //Valve Controller Commands
        ////////////////////
        if (ValveControllerService_Command.written()) {
        // application logic for handling WRITE or WRITE_WITHOUT_RESPONSE on characteristic Pots goes here
         uint16_t copyingIndex;
         for(copyingIndex = 0; copyingIndex<ValveControllerService_Command.valueLength(); copyingIndex++)
         {
           AttributeValue[copyingIndex] = ValveControllerService_Command.value()[copyingIndex];
         }
         AttributeValue[copyingIndex] = NULL;

         Serial.println("ValveControllerService_Command.written()");
         switch(AttributeValue[0])
         {
          case 1:
            Serial.println("Flush open written");
            solenoidOpen();
          break;

          case 2:
            Serial.println("Start written");
          break;

          case 3:
            Serial.println("Stop written");
          break;

          case 4:
            Serial.println("Pause written");
          break;

          case 5:
             Serial.println("Flush close written");
             solenoidClose();
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
        if (TimePointService_NewPoint.written()) {
        // application logic for handling WRITE or WRITE_WITHOUT_RESPONSE on characteristic Current Time Service Current Time goes here
         sprintf(AttributeValue,"%c",NULL);
         uint16_t copyingIndex;
         for(copyingIndex = 0; copyingIndex<TimePointService_NewPoint.valueLength(); copyingIndex++)
         {
           AttributeValue[copyingIndex] = TimePointService_NewPoint.value()[copyingIndex];
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
          //reset all alarms for time point index 0
          int i;
          for(i = 0; i < 32; i++)
          {
            Alarm.free(id[i]);
          }
          Serial.println("Old time points erased.");

         }
         else
         {
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
        if (BatteryService_BatteryLevel.read()) {
          Serial.println("Battery Level Read");
        }
    }
    // when the central disconnects, print it out:
    Serial.print("Disconnected from central: ");
    Serial.println(central.address());
  }
}

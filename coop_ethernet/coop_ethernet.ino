#include <WebServer.h>
#include <EEPROM.h>
#include "EEPROMAnything.h"
#include <SPI.h>
#include <Ethernet.h>
#include <math.h>
//#include <SD.h>
#include "Wire.h"
#define DS1307_ADDRESS 0x68
#define VERSION_STRING "0.1"

struct Schedule {
  long on1;
  long on2;
};

Schedule doorSchedule = {(7*60L*60L)+(30*60L), (20*60L*60L)}; //on1 7:30 on2 20:00

byte zero = 0x00; //workaround for issue #527

String request;
long fakeTime = 0;

// constants won't change. They're used here to 
// set pin numbers:
const int pushPin = 7;     // the number of the pushbutton pin
const int motorPin =  6;      // the number of the LED pin
const int OFF = 0;
const int ON = 1;

const int TRIGGER_TIME_WINDOW_SEC = 30; // how long to leave power on to the door and lock

// variables will change:
int buttonState = LOW;         // variable for reading the pushbutton status

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = { 
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(10,0,1,98);

//File root;
char configFile[ ] = "coop.conf";

#define PREFIX ""
WebServer webserver(PREFIX, 80);

P(Page_start) = "<html><head><title>Cooptroller Version " VERSION_STRING "</title></head><body>\n";
P(Page_end) = "</body></html>";
P(Get_head) = "";
P(Post_head) = "<h1>POST to ";
P(Unknown_head) = "<h1>UNKNOWN request for ";
P(Default_head) = "unidentified URL requested.</h1><br>\n";
P(Raw_head) = "raw.html requested.</h1><br>\n";
P(Parsed_head) = "parsed.html requested.</h1><br>\n";
P(Good_tail_begin) = "URL tail = '";
P(Bad_tail_begin) = "INCOMPLETE URL tail = '";
P(Tail_end) = "'<br>\n";
P(Parsed_tail_begin) = "URL parameters:<br>\n";
P(Parsed_item_separator) = " = '";
P(Params_end) = "End of parameters<br>\n";
P(Post_params_begin) = "Parameters sent by POST:<br>\n";
P(Line_break) = "<br>\n";
P(Time) = "Time: ";
P(On_one) = "On1: ";
P(On_two) = "On2: ";
P(Opening_door) = "Opening Door.... please wait<br/>\n";

// Initialize the Ethernet server library
// with the IP address and port you want to use 
// (port 80 is default for HTTP):
EthernetServer server(80);

void webPrintCurrentTimeInfo(WebServer &server)
{
  server.print("Time: ");
  long time = getDaySeconds();
  server.print(decodeTimeCode(time));
  server.printP(Line_break);
  server.printP(On_one);
  server.print(decodeTimeCode(doorSchedule.on1));
  server.printP(Line_break);
  server.printP(On_two);
  server.print(decodeTimeCode(doorSchedule.on2));
}

#define NAMELEN 32
#define VALUELEN 32

void parsedCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete)
{
  URLPARAM_RESULT rc;
  char name[NAMELEN];
  int  name_len;
  char value[VALUELEN];
  int value_len;

  /* if we're handling a GET or POST, we can output our data here.
     For a HEAD request, we just stop after outputting headers. */
  if (type == WebServer::HEAD)
  {
      server.httpSuccess();
      return;
  }
/*
  server.printP(Page_start);
  switch (type)
    {
    case WebServer::GET:
        server.printP(Get_head);
        break;
    case WebServer::POST:
        server.printP(Post_head);
        break;
    default:
        server.printP(Unknown_head);
    }
    */

/*
    server.printP(Parsed_head);
    server.printP(tail_complete ? Good_tail_begin : Bad_tail_begin);
    server.print(url_tail);
    server.printP(Tail_end);
*/

  if (strlen(url_tail))
    {
    //server.printP(Parsed_tail_begin);
    while (strlen(url_tail))
      {
      rc = server.nextURLparam(&url_tail, name, NAMELEN, value, VALUELEN);
      if (rc == URLPARAM_EOS)
      {
        //server.printP(Params_end);
      }
       else
        {
          String strName = name;
          String strVal = value;
          Serial.println(strName);
          Serial.println(strVal);
          if (strName == "door_on" && strVal == "1") {
            //open the door
            server.httpSuccess();
            server.printP(Get_head);
            server.printP(Opening_door);
            digitalWrite(motorPin, HIGH);
            delay(TRIGGER_TIME_WINDOW_SEC*1000);
          } else if (strName == "on1") {
            server.httpSuccess();
            server.printP(Get_head);
            doorSchedule.on1 = atol(value);
            writeRunningConfig();
          } else if (strName == "on2") {
            server.httpSuccess();
            server.printP(Get_head);
            doorSchedule.on2 = atol(value);
            writeRunningConfig();
          } else if (strName == "getInfo") {
            server.httpSuccess("application/json");
            server.print("{\"on1\" : ");
            server.print(doorSchedule.on1);
            server.print(", \"on2\" : ");
            server.print(doorSchedule.on2);
            server.print(", \"time\" : ");
            server.print(getDaySeconds());
            server.print("}");
            return;
          } else {
            server.httpSuccess();
            server.printP(Get_head);
          }
          
          server.printP(Page_end);
          /*
        server.print(name);
        server.printP(Parsed_item_separator);
        server.print(value);
        server.printP(Tail_end);
*/
        }
      }
    }
  if (type == WebServer::POST)
  {
    server.printP(Post_params_begin);
    while (server.readPOSTparam(name, NAMELEN, value, VALUELEN))
    {
      server.print(name);
      server.printP(Parsed_item_separator);
      server.print(value);
      server.printP(Tail_end);
    }
  }
  webPrintCurrentTimeInfo(server);
  server.printP(Page_end);

}

void my_failCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete)
{
  /* this line sends the standard "we're all OK" headers back to the
     browser */
  server.httpFail();

  /* if we're handling a GET or POST, we can output our data here.
     For a HEAD request, we just stop after outputting headers. */
  if (type == WebServer::HEAD)
    return;

  server.printP(Page_start);
  switch (type)
    {
    case WebServer::GET:
        server.printP(Get_head);
        break;
    case WebServer::POST:
        server.printP(Post_head);
        break;
    default:
        server.printP(Unknown_head);
    }

    server.printP(Default_head);
    server.printP(tail_complete ? Good_tail_begin : Bad_tail_begin);
    server.print(url_tail);
    server.printP(Tail_end);
    server.printP(Page_end);

}

void writeRunningConfig()
{
  EEPROM_writeAnything(0, doorSchedule);
}

void setup() {
  // initialize the LED pin as an output:
  pinMode(motorPin, OUTPUT);
  // initialize the pushbutton pin as an input:
  pinMode(pushPin, INPUT);
  Wire.begin();
  Serial.begin(9600);
  
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  
  Serial.println("Reading config from eeprom");
  Schedule scheduleConfig = {0, 0};
  EEPROM_readAnything(0, scheduleConfig);
  if (scheduleConfig.on1 > 0 && scheduleConfig.on2 > 0)
  {
    doorSchedule = scheduleConfig;
  }

  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);

  /* setup our default command that will be run when the user accesses
   * the root page on the server */
  webserver.setDefaultCommand(&parsedCmd);

  /* setup our default command that will be run when the user accesses
   * a page NOT on the server */
  webserver.setFailureCommand(&my_failCmd);

  /* run the same command if you try to load /index.html, a common
   * default page name */
  webserver.addCommand("index.html", &parsedCmd);

  /* start the webserver */
  webserver.begin();

  //uncomment to update the RTC module
  //should only need to if the battery in your RTC dies or you get a new one
  //setDateTime();
}

void loop(){
  
  char buff[64];
  int len = 64;
  
  delay(100);
  // read the state of the pushbutton value:
  buttonState = digitalRead(pushPin);
  Serial.println(buttonState);

  if (buttonState == LOW) {     
    // turn LED on:    
    digitalWrite(motorPin, HIGH);
    delay(TRIGGER_TIME_WINDOW_SEC*1000);
  }
  

  long time = getDaySeconds();

  if (time > 86400)
  {
    Serial.println("Time was invalid.. skipping");
    return;
  }
  Serial.print("Time: ");
  Serial.println(time);
  
  Serial.print("on1: ");
  Serial.println(decodeTimeCode(doorSchedule.on1));
  Serial.print("on2: ");
  Serial.println(decodeTimeCode(doorSchedule.on2));
  
  if ((time >= doorSchedule.on1 && time < doorSchedule.on1 + TRIGGER_TIME_WINDOW_SEC) ||
        (time >= doorSchedule.on2 && time < doorSchedule.on2 + TRIGGER_TIME_WINDOW_SEC)
     )  
  {
    //open the door
    Serial.println("POWER IS ON TO THE DOOR");
    digitalWrite(motorPin, HIGH);
  }
  else
  {
    digitalWrite(motorPin, LOW);
    Serial.println("power is off to the door");
  }
  
  // listen for incoming clients
  webserver.processConnection(buff, &len);
  
}

byte bcdToDec(byte val)  {
// Convert binary coded decimal to normal decimal numbers
  return ( (val/16*10) + (val%16) );
}

byte decToBcd(byte val){
// Convert normal decimal numbers to binary coded decimal
  return ( (val/10*16) + (val%10) );
}

long getDaySeconds()
{
  return fakeTime;
  // Reset the register pointer
  Wire.beginTransmission(DS1307_ADDRESS);

  byte zero = 0x00;
  Wire.write(zero);
  Wire.endTransmission();

  Wire.requestFrom(DS1307_ADDRESS, 7);

  int second = bcdToDec(Wire.read());
  int minute = bcdToDec(Wire.read());
  int hour = bcdToDec(Wire.read() & 0b111111); //24 hour time
  int weekDay = bcdToDec(Wire.read()); //0-6 -> sunday - Saturday
  int monthDay = bcdToDec(Wire.read());
  int month = bcdToDec(Wire.read());
  int year = bcdToDec(Wire.read());

/*
  Serial.print(hour);
  Serial.print(":");
    Serial.print(minute);
      Serial.print(":");
        Serial.println(second);
        */
  long out = hour*60;
  out = out*60;
  out = out + (minute*60);
  out = out + second;
  return out;
}
  

void printDate(){

  // Reset the register pointer
  Wire.beginTransmission(DS1307_ADDRESS);

  byte zero = 0x00;
  Wire.write(zero);
  Wire.endTransmission();

  Wire.requestFrom(DS1307_ADDRESS, 7);

  int second = bcdToDec(Wire.read());
  int minute = bcdToDec(Wire.read());
  int hour = bcdToDec(Wire.read() & 0b111111); //24 hour time
  int weekDay = bcdToDec(Wire.read()); //0-6 -> sunday - Saturday
  int monthDay = bcdToDec(Wire.read());
  int month = bcdToDec(Wire.read());
  int year = bcdToDec(Wire.read());

  //print the date EG   3/1/11 23:59:59
  Serial.print(month);
  Serial.print("/");
  Serial.print(monthDay);
  Serial.print("/");
  Serial.print(year);
  Serial.print(" ");
  Serial.print(hour);
  Serial.print(":");
  Serial.print(minute);
  Serial.print(":");
  Serial.println(second);

}

void setDateTime(){

  byte second =      0; //0-59
  byte minute =      59; //0-59
  byte hour =        21; //0-23
  byte weekDay =     7; //1-7
  byte monthDay =    25; //1-31
  byte month =       8; //1-12
  byte year  =       12; //0-99

  Wire.beginTransmission(DS1307_ADDRESS);
  Wire.write(zero); //stop Oscillator

  Wire.write(decToBcd(second));
  Wire.write(decToBcd(minute));
  Wire.write(decToBcd(hour));
  Wire.write(decToBcd(weekDay));
  Wire.write(decToBcd(monthDay));
  Wire.write(decToBcd(month));
  Wire.write(decToBcd(year));

  Wire.write(zero); //start 

  Wire.endTransmission();

}

String decodeTimeCode(long tc) {
  int hours = floor(tc/3600);
  long remain = tc-(hours*3600.0L);
  int minutes = 0;
  if (remain > 0) {
    minutes = remain/60;
  }
  String out = "";
  out = out + hours;
  out = out + ":";
  if (minutes < 10) {
    out = out + "0";
  }
  out = out + minutes;
  return out;
}

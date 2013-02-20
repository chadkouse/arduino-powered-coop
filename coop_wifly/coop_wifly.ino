
#include <SPI.h>
#include <WiFly.h>
#include "Wire.h"
#define maxLength 25
#define DS1307_ADDRESS 0x68
byte zero = 0x00; //workaround for issue #527

boolean runWifi = false;
char ssid[] = "Studio";
WiFlyServer server(80);
String request;
long lastIpCheck=0;
boolean wifiUp = false;
String serverIp = "0.0.0.0";
long fakeTime = 0;
const int IP_CHECK_TIME_INTERVAL_SECONDS = 100;
String inString = String(80);

// constants won't change. They're used here to 
// set pin numbers:
const int pushPin = 7;     // the number of the pushbutton pin
const int motorPin =  6;      // the number of the LED pin
const int OFF = 0;
const int ON = 1;

//TODO: pull these times from a web server periodically.
const long OPEN_TIME_SECONDS=(7*60L*60L)+(30*60L); // 7:00am
const long CLOSE_TIME_SECONDS=(19L*60L*60L)+(10*60L); // 19:10pm
const int TRIGGER_TIME_WINDOW_SEC = 30; // how long to leave power on to the door and lock

// variables will change:
int buttonState = LOW;         // variable for reading the pushbutton status

void setup() {
  // initialize the LED pin as an output:
  pinMode(motorPin, OUTPUT);      
  // initialize the pushbutton pin as an input:
  pinMode(pushPin, INPUT);  
  Wire.begin();
  Serial.begin(9600);
  
  if (runWifi)
  {
    WiFly.begin();
    if (!WiFly.join(ssid)) {
      Serial.println("OH NOES! COULD NOT CONNECT TO WIFI!");
    }
    else
    {
      wifiUp = true;
      serverIp = WiFly.ip();
      Serial.print("IP: ");
      Serial.println(serverIp);
      server.begin();
      SpiSerial.begin();
      delay(500);
    }
  }
  
  //setDateTime();
}

void loop(){
  delay(100);
  // read the state of the pushbutton value:
  buttonState = digitalRead(pushPin);

  if (buttonState == LOW) {     
    // turn LED on:    
    digitalWrite(motorPin, HIGH);
    delay(TRIGGER_TIME_WINDOW_SEC*1000);
  }
  
  if (runWifi)
  {
    /* webserver portion */
    WiFlyClient client = server.available();
    if (client) {
      // an http request ends with a blank line
      boolean current_line_is_blank = true;
      while (client.connected()) 
      {
        if (client.available()) 
        {
          char c = client.read();
          Serial.write(c);
          if (inString.length()<maxLength) inString +=c;
          // if we've gotten to the end of the line (received a newline
          // character) and the line is blank, the http request has ended,
          // so we can send a reply
          if (c == '\n' && current_line_is_blank) 
          {
            Serial.println(inString);
            String get = inString.substring(5);
            Serial.println(get);
            get = get.substring(0, get.indexOf(" "));
            get.trim();
            Serial.print("COMMAND:");
            Serial.println(get);
            // send a standard http response header
            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/html");
            client.println();
            
            if (get == "door_up")
            {
              client.print("opening door");
              digitalWrite(motorPin, HIGH);
              delay(TRIGGER_TIME_WINDOW_SEC*1000);
            }
            break;
            
          }
          if (c == '\n') {
            // we're starting a new line
            current_line_is_blank = true;
          } else if (c != '\r') {
            // we've gotten a character on the current line
            current_line_is_blank = false;
          }
        }
      }
      // give the web browser time to receive the data
      inString = "";
      delay(100);
      client.stop();
    }    
    
    if(SpiSerial.available() > 0)
    {
      if (SpiSerial.findUntil("Discon", "\r")) //If Wifly is disconnected from WiFi Network 
      {
        Serial.println("Disconnected!");
        wifiUp = false;
      }
    }
    
    if (!wifiUp)
    {
      SpiSerial.write("$$$");
      delay(1000);
      if (true || SpiSerial.findUntil("CMD", "\r"))
      {
        while (SpiSerial.available() > 0)
        {
          Serial.write(SpiSerial.read());
        }
        Serial.println("Entered CMD mode");
        SpiSerial.write("join Studio\r");   // 'Test' is the SSID of your network
        delay(3000);
        Flush_RX();
        SpiSerial.write("show c\r");      // checks if there has been a connection
        delay(1000);
        if (SpiSerial.findUntil("0\r", ">\r"))
        {
          Serial.println("Connected!");
          Flush_RX();
          wifiUp = true;
          server.begin();
        }
        else
        {
          Serial.println("Not Connected... Retrying");
          Flush_RX();
          wifiUp = false;
        }
      }
      SpiSerial.write("exit\r");
    }
    /* end webserver portion */  
  }
  
  long time = getDaySeconds();
  if (time > 86400)
  {
    Serial.println("Time was invalid.. skipping");
    return;
  }
  Serial.print("Time: ");
  Serial.println(time);
  
  Serial.print("on: ");
  Serial.println(OPEN_TIME_SECONDS);
  Serial.print("of: ");
  Serial.println(CLOSE_TIME_SECONDS);
  Serial.print("ip: ");
  Serial.println(serverIp);
  
  if ((time >= OPEN_TIME_SECONDS && time < OPEN_TIME_SECONDS + TRIGGER_TIME_WINDOW_SEC) ||
        (time >= CLOSE_TIME_SECONDS && time < CLOSE_TIME_SECONDS + TRIGGER_TIME_WINDOW_SEC)
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

void Flush_RX(void)
{
  int j = 0;
  while(j < 1000)
    {
      while(SpiSerial.available() > 0)
      {
          Serial.write(SpiSerial.read());
      }  

      j++;
    }
}

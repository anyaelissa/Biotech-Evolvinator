/*
Evolvinator
 
 REV 1
 
 This program controls the Evolvinator. There are 3 main components:
 Pump Control - dictates the speed of the p-1 perstaltic pump
 OD Sensor - makes OD measurements
 - also controls the valve 
 Temp Control - monitors the temperature and feeds back to the heat resistors
 
 Search ENTER to see where you need to input parameters
 Search CALIBRATION to see where you need to input calibration data
 */

/* >>>>>>>>>>>>>>>>>>>>>>>>>>> Constants and Variables <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */
// Libraries
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <TimeLib.h>
#include <SD.h>
#include <PID_v1.h>
#include <Wire.h>
#include <hd44780.h>                       // main hd44780 header https://github.com/duinoWitchery/hd44780
#include <hd44780ioClass/hd44780_I2Cexp.h> // i2c expander i/o class header

// Ethernet
byte mac[] = { 
  0x90, 0xA2, 0xDA, 0x00, 0x4F, 0x74 };   // ENTER MAC address of ethernet shield
EthernetServer server(80);                // default web request server
EthernetUDP Udp;

// Time
unsigned long currentMs;
unsigned long oldMsTempRead = 0;
unsigned long oldMsTempCheck = 0;
unsigned long oldMsODRead = 0;
unsigned long oldMsPulseFed = 0;         
unsigned long oldMsLcdWrite = 0;
unsigned long oldMsAdafruitWrite = 0;

const int localPort = 8888;               // local port to listen for UDP packets
byte timeServer[] = { 
  216, 239, 35, 0};                       // time1.google.com NTP server
const int NTP_PACKET_SIZE= 48;            // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE];      // buffer to hold incoming and outgoing packets from NTP

time_t tStart;                            // starting time
time_t t;                                 // current time
time_t tElapsed;                          // elapsed time (s)
time_t tPulse;                            // time of the last pulse
time_t tBackup;                           // reference time for if time sync with NTP server fails
unsigned long msBackup;                   // associated arduino clock time
unsigned long tUnixStart;                 // unix time at run start
unsigned long tUnix;                      // unix time
unsigned long msElapsedPrestart;          // ms elapsed run start.

// Flow
const byte pinP1FlowWrite = 0;            // which pin tells p1 (through pin 14) what speed (0-200 Hz)
unsigned long feedFrequency = 180000;     // frequency of the pulses given (default 1 ever 3 minutes)
float totalVol = 0;                       // total volume added

// OD
const byte pinODLED = 1;                  // pin that powers the OD LED
const byte pinODRead = A1;                // pin that reads the OD sensor
const byte pinValve = 3;                  // pin that controls the valve
float ODDesired = 0.5;                    // Set desired OD
float ODMin[10];                          // stores recent OD measurements (current = ODMin[0]                         
float OD3MinAvg;
float ODZero = 0;                         // photodiode blank reading 

// Temp - temp is temperature of sensor (metal) unless otherwise indicated
const byte pinTempRead = A0;              // analog input will read variable voltage from AD22100
const byte pinTempWrite = 5;              // sends PWM to resistors to control temp
float tempDesired = 37;                   // Set desired temperature
float tempPrintAvg;                       // temperature converted to water temp
double temp, tempPWM, tempDesiredPID;
double aggKp = .5, aggKi = 0.1, aggKd = 0.1;
double consKp = .2, consKi = 0.01, consKd = 0.05;
PID tempPID(&temp, &tempPWM, &tempDesiredPID, aggKp, aggKi, aggKd, DIRECT);

// UV LED
const byte pinUVLED = 2;                  // pin that powers the UV LED

// Modes
boolean debugMode = true;
boolean calibrationMode = false;

// SD
const int pinSD = 4;

// LCD

hd44780_I2Cexp lcd;                        // declare lcd object: auto locate & auto config expander chip


/* >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Setup - runs once <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */
void setup() {
  // General
  analogReference(DEFAULT);                // analog input reference is 5 V

  // Serial, Ethernet
  Serial.begin(9600);
  pinMode(53, OUTPUT);                      // SS pin on Mega
  Ethernet.begin(mac);                      // Use for DHCP
  // TODO: add error handling if DHCP fails
  if (debugMode) {
    Serial.print("Local IP address: ");
    Serial.println(Ethernet.localIP());
  }
  server.begin();
  delay(1);                                 // give it a msecond to connect


  // Pump Control
  pinMode(pinP1FlowWrite, OUTPUT);          // output to pump what speed it should go (tone between 0-200 Hz)
  flowSet();

  // OD Sensor
  pinMode(pinODLED, OUTPUT);                // pin that controls LED is output
  digitalWrite(pinODLED, LOW);              // light off by default
  pinMode(pinODRead, INPUT);                // pin that reads photodiode is input
  digitalWrite(pinODRead, HIGH);            // enable 20k pullup resistor
  pinMode(pinValve, OUTPUT);                // pin that controls valve is output
  digitalWrite(pinValve, LOW);              // valve open at start
  ODCalibrate();

  // Temp Control
  pinMode(pinTempRead, INPUT);              // pin that reads the temp sensor is input
  pinMode(pinTempWrite, OUTPUT);            // pin that controls heating resistors is output
  tempSet();
  tempPID.SetMode(AUTOMATIC);
  tempPID.SetSampleTime(10000);
  tempPID.SetOutputLimits(0, 1);            // set max to 1 until ready to use heater, then set to 70

  // Timer
  Udp.begin(localPort);
  setSyncProvider(getTime);
  setSyncInterval(60 * 5);                  // sync interval default is 5 mins
  tUnixStart = tUnix; 
  tBackup = now();                          // set back up time
  msBackup = millis();                      // set assocated backup time on Arduino's internal clock
  msElapsedPrestart = millis();             // ms passed since power on to calculate unix time.
  if (debugMode) {
    Serial.print("Back up time: ");
    Serial.println(tBackup);
  }

  // SD
  SD.begin(pinSD);

  // LCD
  lcd.begin(20,4);
  lcd.backlight();
  lcd.clear();
  lcd.print("Evolvinator start...");
  lcd.setCursor(0, 1);
  lcd.print(Ethernet.localIP());

  // Adafruit IO
  AdafruitIOInitialize();

  // Rotary Encoder
  setupRotaryEncoder();
}

/* >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Loop - is program <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< */
void loop() {

  // Maintain DHCP lease
  Ethernet.maintain();
  
  // If run has started
  //if (tStart) {
  if (true) {
    // Take OD measurement ever minute
    currentMs = millis();
    //if (currentMs - oldMsODRead > 60000) {
      ODRead();
      oldMsODRead = currentMs;
    //}

    // Feed pulse if threshold is reached and it's been long enough
    currentMs = millis();
    if (OD3MinAvg > ODDesired && currentMs - oldMsPulseFed > feedFrequency) {
      pulseFeed();
      oldMsPulseFed = currentMs;
    }
  }

  // Check temp every 5 seconds
  currentMs = millis();                    
  if (currentMs - oldMsTempRead > 5000) {  
    tempRead();
    oldMsTempRead = currentMs;
  }
  // PID adjust every 10 seconds
  tempWrite(); 

  // Check and adjust time if neccessary
  currentMs = millis();
  timeCheck();  
  
  // Check for web requests
  webLoop(); 

  // Update LCD
  if (currentMs - oldMsLcdWrite > 1000) {
    oldMsLcdWrite = currentMs;
    LcdUpdate();
  }

  // Log to Adafruit every 10 seconds
  if (currentMs - oldMsAdafruitWrite > 10000) {
    oldMsAdafruitWrite = currentMs;
    LogDataToAdafruitIO();
  }

  // Read rotary encoder
  readRotaryEncoder();
}

/* >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Functions - List function calls below <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< 
 0 Start Run
 1 Flow Control
 1a flowSet
 1b pulseFeed
 1c addMedia 
 2 OD Sensor
 2a ODRead
 2b ODCalibrate
 3 Temperature Control
 3a tempSet
 3b tempRead
 3c tempWrite
 4 Time
 5 WebLoop
 6 SD Card
 6a SDInitialize
 6b SDDataLog
 6c SDWebLoad
 */

// 0 startRun  ----------------------------------
void startRun() {
  tStart = now();
  tElapsed = 0;
  tUnixStart += (millis() - msElapsedPrestart) / 1000;    // to adjust unix time
  tUnix = tUnixStart + tElapsed;
  SDInitialize();
  digitalWrite(pinValve, LOW);          // open air valve
}


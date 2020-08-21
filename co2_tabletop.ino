
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <WiFiUdp.h>
#include <SPI.h>
#include <SD.h>
#include "SCD30.h"
#include <Wire.h>
#include "rgb_lcd.h"
#include <RTCZero.h>
#include <ChainableLED.h>


#if defined(ARDUINO_ARCH_AVR)
#pragma message("Defined architecture for ARDUINO_ARCH_AVR.")
#define SERIAL Serial
#elif defined(ARDUINO_ARCH_SAM)
#pragma message("Defined architecture for ARDUINO_ARCH_SAM.")
#define SERIAL SerialUSB
#elif defined(ARDUINO_ARCH_SAMD)
#pragma message("Defined architecture for ARDUINO_ARCH_SAMD.")
#define SERIAL SerialUSB
#elif defined(ARDUINO_ARCH_STM32F4)
#pragma message("Defined architecture for ARDUINO_ARCH_STM32F4.")
#define SERIAL SerialUSB
#else
#pragma message("Not found any architecture.")
#define SERIAL Serial
#endif

#define PIR_MOTION_SENSOR 2

#define NUM_LEDS  1

ChainableLED leds(5, 6, NUM_LEDS);

RTCZero rtc;

rgb_lcd lcd;

//chip select pin for SD 
const int chipSelect = 4;

//output of current time based on compile time and internal rtc
String timeStr = "";

//records time since beginning of program that CO2 sensor was last updated
long previousMillis = 0;
// SAMPLE RATE OF CO2 SENSOR... CHANGE THIS TO ANY NUMBER BETWEEN 2000 AND 170000
long interval = 2000;

// set the rtc time from the compile time:
void setTimeFromCompile() {
  // get the compile time string:
  String compileTime = String(__TIME__);
  // break the compile time on the colons:
  int h = compileTime.substring(0, 2).toInt();
  int m = compileTime.substring(3, 5).toInt();
  int s = compileTime.substring(6, 8).toInt();

  // set the time from the derived numbers:
  rtc.setTime(h, m, s);
}

// set the rtc time from the compile date:
void setDateFromCompile() {
  String months[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
  };
  // get the compile date:
  String compileDate = String(__DATE__);
  // get the date substring
  String monthStr = compileDate.substring(0, 3);

  int m = 0;    // variable for the date as an integer
  // see which month matches the month string:
  for (int i = 0; i < 12; i++) {
    if (monthStr == months[i]) {
      // the first month is 1, but its array position is 0, so:
      m = i + 1;
      // no need to continue the rest of the for loop:
      break;
    }
  }

  // get the day and year as substrings, and convert to numbers:
  int d = compileDate.substring(4, 6).toInt();
  int y = compileDate.substring(9, 11).toInt();
  // set the date from the derived numbers:
  rtc.setDate(d, m, y);
}

//function that outputs the current time as a string
String getTimeStr() {
  timeStr = "";
  timeStr += rtc.getHours();
  timeStr += ":";
  timeStr += rtc.getMinutes();
  timeStr += ":";
  timeStr += rtc.getSeconds();
  return timeStr;
}

void setup() {

  SERIAL.begin(9600);
  Wire.begin();

  lcd.begin(16, 2);
  rtc.begin();
  scd30.initialize();
  leds.init();
//  scd30.setAutoSelfCalibration(1); // uncomment this to begin CO2 calibration... take outside for 1hr per day for 7 days... if power disconnected calibration must be restarted

//initialize SD card
  Serial.print("Initializing SD card...");

  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    while (1);
  }
  Serial.println("card initialized.");
  // SERIAL.println("SCD30 Raw Data");


  pinMode(PIR_MOTION_SENSOR, INPUT);

  setTimeFromCompile();
  setDateFromCompile();
}

void loop() {

  //stores time since program began  
  unsigned long currentMillis = millis();

  //if motion sensor sees object, turn on LED
  if (digitalRead(PIR_MOTION_SENSOR))
  {
    leds.setColorRGB(0, 0, 0, 255);  
  }
  else
  {
    leds.setColorRGB(0, 0, 0, 0);  
  }
  
  //create array to store the three results from the C02 sensor (C02, Temp, Humidity)
  float result[3] = {0};
  
  //if interval has elapsed, refresh sensor value and print 
  if ((scd30.isAvailable()) && (currentMillis - previousMillis > interval)) {

    //update time since last sensor sample 
    previousMillis = currentMillis;

    //update sensor values
    scd30.getCarbonDioxideConcentration(result);

    //print results to the LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("CO2");
    lcd.setCursor(0, 1);
    lcd.print(round(result[0]));

    lcd.setCursor(5, 0);
    lcd.print("Temp.");
    lcd.setCursor(5, 1);
    lcd.print(result[1]);

    lcd.setCursor(11, 0);
    lcd.print("Hum.");
    lcd.setCursor(11, 1);
    lcd.print(result[2]);

    // open a file. note that only one file can be open at a time,
    // so you have to close this one before opening another.
    File dataFile = SD.open("datalog.txt", FILE_WRITE);

    // if the file is available, write to it:
    if (dataFile) {
      dataFile.print(getTimeStr());
      dataFile.print(",");
      dataFile.print(result[0]);
      dataFile.print(",");
      dataFile.print(result[1]);
      dataFile.print(",");
      dataFile.print(result[2]);
      dataFile.println(" ");
      dataFile.close();
     //`  print to the serial port too:
            SERIAL.print(getTimeStr());
            SERIAL.print(",");
            SERIAL.print(result[0]);
            SERIAL.print(",");
            SERIAL.print(result[1]);
            SERIAL.print(",");
            SERIAL.print(result[2]);
            SERIAL.println(" ");

    }
    // if the file isn't open, pop up an error:
    else {
      Serial.println("error opening datalog.txt");
    }

  }

delay(15);
}

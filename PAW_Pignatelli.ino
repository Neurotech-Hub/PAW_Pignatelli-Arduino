/*PAW
 * Modified by Matt Gaidica, PhD | Neurotech Hub | gaidica@wustl.edu
 * forked from: [PAW/PAW/v2/PAW_v2.ino at main · dterstege/PAW](https://github.com/dterstege/PAW/blob/main/PAW/v2/PAW_v2.ino)
 *
 * Printable Arduino-based Wheel logger
 * 
 * PAW was created by Dylan Terstege, a PhD candidate studying under the supervision of Dr Jonathan Epp
 * 
 * This file is part of PAW.
 * PAW is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 * PAW is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with Foobar. If not, see <https://www.gnu.org/licenses/>.

  This script is to be used to track the timing of wheel rotations in the running wheel projects
  The circuit:
    [A3213EUA-T Allegro MicroSystems | Sensors, Transducers | DigiKey](https://www.digikey.com/en/products/detail/allegro-microsystems/A3213EUA-T/1006106): Pin A5
    [Adalogger FeatherWing - RTC + SD Add-on For All Feather Boards : ID 2922 : Adafruit Industries, Unique & fun DIY electronics and kits](https://www.adafruit.com/product/2922)
    [Adafruit Feather M0 Basic Proto - ATSAMD21 Cortex M0 : ID 2772 : Adafruit Industries, Unique & fun DIY electronics and kits](https://www.adafruit.com/product/2772)

    created  Aug 2020
    by Dylan J. Terstege
    Epp Lab
 */

// Packages to Include
#include <RTClib.h>
#include <SD.h>  //load SD module packages

#define CS_PIN 10
static bool WRITE_DATA = true;

// Init RTC
RTC_PCF8523 rtc;

// Init storage variables
int const sensor = A5;          //hall effect sensor pin
int hallState;                  //numeric variable for hall field intensity
int wheelcount = 0;             //number of wheel rotations
char filename[] = "00000.CSV";  //CSV file name, char vector
File myFile;                    //variable for indexing into CSV file
char timeBuffer[9];             // Buffer for HH:mm:ss

// INITIAL CODE SETUP
void setup() {
  pinMode(sensor, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);
  delay(2000);  // dumb wait to connect

  // Setup RTC
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }

  if (!rtc.initialized() || rtc.lostPower()) {
    Serial.println("RTC is NOT initialized, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
    //
    // Note: allow 2 seconds after inserting battery or applying external power
    // without battery before calling adjust(). This gives the PCF8523's
    // crystal oscillator time to stabilize. If you call adjust() very quickly
    // after the RTC is powered, lostPower() may still return true.
  }
  rtc.start();

  if (!WRITE_DATA) {
    Serial.println("DEBUG MODE --- NOT WRITING");
  }

  //Write initial output file
  DateTime now = rtc.now();
  getFileName(now);  //assess date
  SD.begin(CS_PIN);  //microSD CS pin
  myFile = SD.open(filename, FILE_WRITE);
  if (!myFile) {
    while (1) {
      Serial.println("Error opening file! Please reset.");
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
      delay(500);
    }
  }
  myFile.println("Rotation, Time");  //column header init
  myFile.close();                    //close CSV file
  Serial.print("Init with file: ");
  Serial.println(filename);
}

// MAIN CODE LOOP
void loop() {
  // Reading Hall State
  int hallState = digitalRead(sensor);  //Read hall sensor
  if (hallState)                        //when magnetic field is not detected
  {
    digitalWrite(LED_BUILTIN, LOW);  //do nothing, field is above threshold
  } else                              //when magnetic field is detected, write to csv
  {
    wheelcount++;  //add rotation count
    Serial.println(wheelcount);
    while (!hallState)  //field falls below threshold, init write procedures
    {
      hallState = digitalRead(sensor);  //Read hall sensor
      digitalWrite(LED_BUILTIN, HIGH);   //do nothing - prevents writing while stationary
    }
    if (WRITE_DATA) {
      // CSV Writing
      DateTime now = rtc.now();
      getFileName(now);  //potentially update current date, if new day

      // Check the Date
      if (SD.exists(filename)) {
        //do nothing - file is already there
      } else {
        //New Day - Start New File                          //START A NEW CSV FILE
        myFile = SD.open(filename, FILE_WRITE);  //open CSV file
        myFile.println("Rotation, Time");        //column header init
        myFile.close();                          //close CSV file
        wheelcount = 1;                          //reset wheelcount
      }

      myFile = SD.open(filename, FILE_WRITE);  //open CSV file
      if (myFile) {
        myFile.print(wheelcount);  //number of rotations
        myFile.print(",");         //comma delineator
        snprintf(timeBuffer, sizeof(timeBuffer), "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
        myFile.println(timeBuffer);
        myFile.close();  //close CSV file
      } else {
        while (1) {
          Serial.println("Error opening file! Please reset.");
          digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
          delay(500);
        }
      }
      delay(200);  //min time between rotations in ms, users may choose to edit this
    }
  }
}


// FUNCTION FOR DATE RETRIEVAL
void getFileName(DateTime now) {
  // Get month and day as integers
  int month = now.month();
  int day = now.day();

  // Format the filename as MMDD.csv
  sprintf(filename, "%02d%02d.csv", month, day);
}

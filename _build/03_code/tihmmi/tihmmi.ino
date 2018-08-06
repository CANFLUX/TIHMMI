#include <Adafruit_GPS.h>
#include <SoftwareSerial.h>
#include <SD.h>
#include <SPI.h>
#include <avr/sleep.h>
#include <math.h>

// Initiate SoftwareSerial communication with GPS
Adafruit_GPS GPS(&Serial2);

// Library for Thermocouple
#include <PlayingWithFusion_MAX31856.h>
#include <PlayingWithFusion_MAX31856_STRUCT.h>

// Library for infrared:
#include <Adafruit_MLX90614.h>
#include <Wire.h>

// initialize the infrared sensor
Adafruit_MLX90614 mlx = Adafruit_MLX90614();

// Library for DHT sensor
#include <DHT.h>
// Define pin on Mega - 53
#define DHTPIN 2     // what digital pin we're connected to
// Specify the type of sensor
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
// Initialize DHT sensor.
DHT dht(DHTPIN, DHTTYPE);


//pwfusion_max31856_r01

// Set GPSECHO to 'false' to turn off echoing the GPS data to the Serial console
// Set to 'true' if you want to debug and listen to the raw GPS sentences.
#define GPSECHO  true

// Number of decimal places when printing results.
#define DECIMALS 5

// Set the pins used
#define chipSelect 10
#define ledPin 13
#define SD_workingLEDpin 44    // This pin is HIGH if there is an SD writing problem
#define LEDintensity 30        // intensity of LED on the SD_workingLEDpin
//#define RELAY_LI820 3          // Pin that turns the LI820 relay ON
#define RELAY_PUMP  4          // Pin that turns the pump relay ON

/* ---- TEMP SENSOR --- */
// Set the pins used for temperature - Data wire is plugged into port 2 on the Arduino
//#define ONE_WIRE_BUS 2 //51
//#define TEMPERATURE_PRECISION 10


/* --- Thermocouple --- 
*  Circuit:
*    Arduino Uno   Arduino Mega  -->  SEN-30005
*    DIO pin 10      DIO pin 10  -->  CS0
*    MOSI: pin 11  MOSI: pin 51  -->  SDI (must not be changed for hardware SPI)
*    MISO: pin 12  MISO: pin 50  -->  SDO (must not be changed for hardware SPI)
*    SCK:  pin 13  SCK:  pin 52  -->  SCLK (must not be changed for hardware SPI)
*    D03           ''            -->  FAULT (not used in example, pin broken out for dev)
*    D02           ''            -->  DRDY (not used in example, only used in single-shot mode)
*    GND           GND           -->  GND
*    5V            5V            -->  Vin (supply with same voltage as Arduino I/O, 5V)
*     NOT CONNECTED              --> 3.3V (this is 3.3V output from on-board LDO. DO NOT POWER THIS PIN!
*/
uint8_t TC0_CS  = 49;
//uint8_t TC1_CS  =  9;
uint8_t TC0_FAULT = 3;                     // not used in this example, but needed for config setup
uint8_t TC0_DRDY  = 2;                     // not used in this example, but needed for config setup

PWF_MAX31856  thermocouple0(TC0_CS, TC0_FAULT, TC0_DRDY);


// Set to true if you want two output strings to be printed on Serial using two different
// methods (troubleshooting only)
#define DEBUG_PRINT  false

const int MAX_BUFFER_LEN = 200;
boolean usingInterrupt = false;
void useInterrupt(boolean); // Func prototype keeps Arduino 0023 happy

long lCounter = 0;
int lPointer = 0;
char oldFileName[25];
char newFileName[25];
File logfile;

// --- debugging --- //
unsigned long millis_old;

// Visual Feedback LEDs
int led_gpsfix = 5;
int led_datalogging = 6;


void setup() {

  digitalWrite(RELAY_PUMP,HIGH);

  // Set both file names to null
  oldFileName[0] = 0;
  newFileName[0] = 0;

  // PC Serial port output at 115200 baud
  Serial.begin(115200);
  Serial.println("LI-820 Logging program by Zoran Nesic, Biomet/Micromet - UBC");
  Serial.println(freeRam());

  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(10, OUTPUT);
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect, 11, 12, 13)) {
    Serial.println("Card init. failed!");
    error(2);
  }
  // Open a "dummy" file as a place holder.  It will get closed in the main loop.
  logfile = SD.open("dummy", FILE_WRITE);
  if ( ! logfile ) {
    Serial.print("Couldnt create "); Serial.println(newFileName);
    error(3);
  }

  // 9600 NMEA is the default baud rate for Adafruit GPS
  GPS.begin(9600);

  // uncomment this line to turn on RMC (recommended minimum) and GGA (fix data) including altitude
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);

  // Set the update rate
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);   // 1 Hz update rate

  // Request updates on antenna status, comment out to keep quiet
  GPS.sendCommand(PGCMD_ANTENNA);

  // the nice thing about this code is you can have a timer0 interrupt go off
  // every 1 millisecond, and read data from the GPS for you. that makes the
  // loop code a heck of a lot easier!
  useInterrupt(true);
  
  // Init Thermocouple   
  // setup for the the SPI library:
  Serial.println("Init Thermocouple");
   // setup for the the SPI library:
  SPI.begin();                            // begin SPI
  SPI.setClockDivider(SPI_CLOCK_DIV16);   // SPI speed to SPI_CLOCK_DIV16 (1MHz)
  SPI.setDataMode(SPI_MODE3);             // MAX31856 is a MODE3 device
  
  // call config command... options can be seen in the PlayingWithFusion_MAX31856.h file
  thermocouple0.MAX31856_config(K_TYPE, CUTOFF_60HZ, AVG_SEL_4SAMP);
  Serial.println("Thermocouple Initialized!");
  
  // Begin logging the DHT sensor
   Serial.println("DHT Initialized!");
  dht.begin();
  
  //  Begin logging the infrared:
   Serial.println("MLX infrared Initialized!");
  mlx.begin();  
  
  delay(1000);
  // Ask for firmware version
  Serial2.println(PMTK_Q_RELEASE);
}



uint32_t timer = millis();
void loop() {

  char *p, *p1;
  char tmpStr[20];
  char newPathName[10];
  
  //  Thermocouple variables
  static struct var_max31856 TC_CH0;
  double tmp;
  struct var_max31856 *tc_ptr;
  
  // IR sensor vars
  double iramb;
  double irobj;
  
  /*  --- start loop on thermocouple --- */
    // Read CH 0
  tc_ptr = &TC_CH0;                             // set pointer
  thermocouple0.MAX31856_update(tc_ptr);        // Update MAX31856 channel 0
  
  // Thermocouple channel 0
//  Serial.print("Thermocouple 0: ");            // Print TC0 header
  if(TC_CH0.status)
  {
    // lots of faults possible at once, technically... handle all 8 of them
    // Faults detected can be masked, please refer to library file to enable faults you want represented
    Serial.println("fault(s) detected");
    Serial.print("Fault List: ");
    if(0x01 & TC_CH0.status){Serial.print("OPEN  ");}
    if(0x02 & TC_CH0.status){Serial.print("Overvolt/Undervolt  ");}
    if(0x04 & TC_CH0.status){Serial.print("TC Low  ");}
    if(0x08 & TC_CH0.status){Serial.print("TC High  ");}
    if(0x10 & TC_CH0.status){Serial.print("CJ Low  ");}
    if(0x20 & TC_CH0.status){Serial.print("CJ High  ");}
    if(0x40 & TC_CH0.status){Serial.print("TC Range  ");}
    if(0x80 & TC_CH0.status){Serial.print("CJ Range  ");}
    Serial.println(" ");
  }
  else  // no fault, print temperature data
  {
    //Serial.println("no faults detected");
    // MAX31856 Internal Temp
    //    tmp = (double)TC_CH0.ref_jcn_temp * 0.015625;  // convert fixed pt # to double
    //    Serial.print("Tint = ");                      // print internal temp heading
    //    if((-100 > tmp) || (150 < tmp)){Serial.println("unknown fault");}
    //    else{Serial.println(tmp);}
    
    // MAX31856 External (thermocouple) Temp
    tmp = (double)TC_CH0.lin_tc_temp * 0.0078125;           // convert fixed pt # to double
    //    Serial.print("TC Temp = ");                   // print TC temp heading
    //    Serial.println(tmp);
  }
    /*  --- end loop on thermocouple --- */
    
  
  /* --- DHT sensor loop  --- */
   // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  //  float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  
  // Infrared thermometer:
  iramb = mlx.readAmbientTempC();
  irobj = mlx.readObjectTempC();
  //  Serial.print("Ambient = "); Serial.print(mlx.readAmbientTempC()); 
  //  Serial.print("*C\tObject = "); Serial.print(mlx.readObjectTempC()); Serial.println("*C");

  //  add "h", "t", and "tmp" to the myBuffer
  // if a it's time to receive GPS's data then we can check the checksum, parse it...
  if (GPS.newNMEAreceived()) {

    if (!GPS.parse(GPS.lastNMEA())) {  // this also sets the newNMEAreceived() flag to false
      return;  // we can fail to parse a sentence in which case we should just wait for another
    }

    else {

      // by adding GPS.fix && I have disabled the option of logging LI-820 data without
      // when GPS is not having fix.  Maybe not a bad thing here but it may mess with
      // my thinking later on if I try to use the same setup in another setting
      if ( GPS.fix && GPS.year > 12 && GPS.year < 99 && GPS.month > 0 && GPS.month < 13
           && GPS.hour > -1 && GPS.hour < 25 && GPS.minute > -1 && GPS.minute < 60) {

        // If proper GPS data has been received and the clock data makes sense
        // open a new file for writing
        getFileName(newFileName);
        // if the new file is different than the old file name then
        // flush the buffer and close the old file
        if (strcmp(newFileName, oldFileName) != 0) {
          Serial.println("Old file flushed and closed");
          if (logfile) {
            // if logfile is open then flush and close
            logfile.flush();
            logfile.close();
          }
          // do oldFileName = newFileName
          strcpy(oldFileName, newFileName);
        }

        if (! logfile) {
          // If there is no logfile then open a new one.
          getPathName(newPathName);
          getFileName(newFileName);
          SD.mkdir(newPathName);
          Serial.print("Made new folder: "); Serial.println(newPathName);
          Serial.print("Atempt to open: "); Serial.println(newFileName);
          logfile = SD.open(newFileName, FILE_WRITE);
          if ( ! logfile ) {
            Serial.print("Couldnt create - "); Serial.println(newFileName);
            error(3);
          }
          Serial.print("Writing to "); Serial.println(newFileName);
          strcpy(oldFileName, newFileName);
        }
      }
    }
  }

  char buffGPSPos[100];
  char myBuffer[150];
  char myNumber[12];
  myBuffer[0] = 0;

  // First create myBuffer string by adding GPS date, time, fix and fix_quality
  char buff1[50]; sprintf(buff1, "%02i/%02i/%4i,%02i:%02i:%02i.%03i,%i,%i", GPS.day, GPS.month, GPS.year + 2000,
                          GPS.hour, GPS.minute, GPS.seconds, GPS.milliseconds,
                          GPS.fix,
                          GPS.fixquality
                         );
  strcat(myBuffer, buff1);

  if (GPS.fix) {
    strcat(myBuffer, ",");

    // if all of the above are true - light on
    digitalWrite(led_gpsfix, HIGH);

    // Add minus sign for the South hemisphere and store to myBuffer
    if (GPS.lat == 'N') {
      dtostrf(geoConvert(GPS.latitude), 10, DECIMALS, myNumber);
    }
    else {
      dtostrf(-geoConvert(GPS.latitude), 10, DECIMALS, myNumber);
    }

    strcat(myBuffer, myNumber);
    strcat(myBuffer, ",");

    // add minus sign for West longitudes and store to myBuffer
    if (GPS.lon == 'E') {
      dtostrf(geoConvert(GPS.longitude), 10, DECIMALS, myNumber);
    }
    else {
      dtostrf(-geoConvert(GPS.longitude), 10, DECIMALS, myNumber);
    }
    strcat(myBuffer, myNumber);

    // add GPS speed, altitude and # of satellites to the output buffer (myBuffer)
    strcat(myBuffer, ","); dtostrf(GPS.speed * 1.852, 6, DECIMALS, myNumber); strcat(myBuffer, myNumber); // to convert knots to kmh
    strcat(myBuffer, ","); dtostrf(GPS.altitude, 6, DECIMALS, myNumber); strcat(myBuffer, myNumber);
    strcat(myBuffer, ","); dtostrf(GPS.satellites, 2, 0, myNumber); strcat(myBuffer, myNumber);
    
    /*--- tmp, h, an t variables --- */
    // add the "h", "t", and "tmp" variables to the myBuffer:
    strcat(myBuffer, ","); dtostrf(tmp, 3, DECIMALS, myNumber); strcat(myBuffer, myNumber);
    strcat(myBuffer, ","); dtostrf(h, 3, DECIMALS, myNumber); ;strcat(myBuffer, myNumber);
    strcat(myBuffer, ","); dtostrf(t, 3, DECIMALS, myNumber); ;strcat(myBuffer, myNumber);
    
    /* --- add the IR sensor output to myBuffer --- */
    strcat(myBuffer, ","); dtostrf(iramb, 3, DECIMALS, myNumber); strcat(myBuffer, myNumber);
    strcat(myBuffer, ","); dtostrf(irobj, 3, DECIMALS, myNumber); strcat(myBuffer, myNumber);
    
    
    if (DEBUG_PRINT) {
      Serial.print(GPS.latitude, 5); Serial.print(GPS.lat);
      Serial.print(", ");
      Serial.print(GPS.longitude, 5); Serial.print(GPS.lon);
      Serial.print(","); Serial.print(GPS.speed);
      Serial.print(", "); Serial.print(GPS.altitude);
      Serial.print(", "); Serial.print((int)GPS.satellites);
    }
  }
  else {
    // No GPS data.  Print out empty fields.
    sprintf(tmpStr, "%s", ",,,,,");
    strcat(myBuffer, tmpStr);

    // if no gps - then blink light
    digitalWrite(led_gpsfix, HIGH);
    delay(500);
    digitalWrite(led_gpsfix, LOW);
    delay(500);

    if (DEBUG_PRINT) {
      Serial.print(",,,,");
    }
  }

  // Visual Output for LEDs logging data
  if (!myBuffer) {
    digitalWrite(led_datalogging, HIGH);
    delay(500);
    digitalWrite(led_datalogging, LOW);
    delay(500);
  }
  if (myBuffer) {
    // Visual Output for LEDs logging data
    digitalWrite(led_datalogging, HIGH);
  }

  Serial.println(myBuffer);

  // Save the buffer to SD card:
  if (logfile) {
    // if logfile is open then save the new line and flush the buffer
    int bytesWriten = logfile.println(myBuffer);
    logfile.flush();

    if (bytesWriten < getLen(myBuffer)) {
      Serial.println("Error writing on SD");
      // If there is an error writing on the SD card while logfile != false
      // then user must have removed the card.  Turn on the external LED to
      // make sure that user will notice this and reset the module once the card
      // is back and ready.

      analogWrite(SD_workingLEDpin, LEDintensity);
    }
    else {
      // Card writing worked.  Turn OFF the external LED.
      analogWrite(SD_workingLEDpin, 0);
    }
  }
  else {
    // Otherwise an incomplete line has been received from LI820.
    // Delete the incomplete line and keep waiting for the next one
    //    lPointer = 0;
    //    newLI820Line[lPointer] = '\0';

    Serial.println("-9");
  }

  delay(1000);
} // end loop

/*-----------------------------------------------
Function useInterrupt
  - Initiates or stops the interrupts
-------------------------------------------------*/
void useInterrupt(boolean v) {
  if (v) {
    // Timer0 is already used for millis() - we'll just interrupt somewhere
    // in the middle and call the "Compare A" function above
    OCR0A = 0xAF;
    TIMSK0 |= _BV(OCIE0A);
    usingInterrupt = true;
  } else {
    // do not call the interrupt function COMPA anymore
    TIMSK0 &= ~_BV(OCIE0A);
    usingInterrupt = false;
  }
}

/*-----------------------------------------------
Interrupt routine
  - Interrupt is called once a millisecond,
    looks for any new GPS data, and stores it
-------------------------------------------------*/
SIGNAL(TIMER0_COMPA_vect) {
  char c = GPS.read();
  // if you want to debug, this is a good time to do it!
  //Serial.print(c);
#ifdef UDR0
  //  if (GPSECHO)
  //    if (c) UDR0 = c;
  // writing direct to UDR0 is much much faster than Serial.print
  // but only one character can be written at a time.
#endif
}

/*-----------------------------------------------
convert geocoordinates
  - Create lat/long conversion function
  - (see: http://arduinodev.woofex.net/2013/02/06/adafruit_gps_forma/)
-------------------------------------------------*/
//
double geoConvert( float degMin ) {
  double min = 0.0;
  double decDeg = 0.0;

  // get the minutes, fmod() requires double
  min = fmod( (double)degMin, 100.0);

  // rebuild coordinates in decimal degrees
  degMin = (int)( degMin / 100 );
  decDeg = degMin + ( min / 60 );

  return decDeg;
}

/*---------------------------------------
Function freeRam:
  - Calculates available RAM memory
  - Arduino Mega 2560 starts with 8k free
-----------------------------------------*/
int freeRam () {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

/*---------------------------------------------------------------
Function parseFloat:
  - Parses a string looking for a start string (like: "<co2>"")
  - grabs the characters that follow the start string and
    converts them to a float.
-----------------------------------------------------------------*/
float parseFloat( char *cnewLine, char *startString) {
  float result;
  char *p;
  p = strstr(cnewLine, startString) + getLen(startString);
  result = atof(p);
  return result;

}

/*---------------------------------------------------------
Function getLen:
  - returns the length of an char array (before the NULL)
-----------------------------------------------------------*/
int getLen( char *buffer) {
  int i = 0;
  while (buffer[i])
    i++;
  return i;
}

/*---------------------------------------------------------
Function getFileName:
  - creates a file name for each hhour of data.
  - The time stamp refers to the end of half hour period
-----------------------------------------------------------*/
void getFileName(char fileName[]) {
  // Create file name of the format: yymmddqq.dat
  // where qq goes from 02 to 96
  // 00:00 - 00:29 -> qq = 02
  // 00:30 - 00:59 -> qq = 04
  // 01:00 - 01:29 -> qq = 06
  // 23:30 - 23:59 -> qq = 96

  // Return in the array is too short
  /*  if (sizeof(*fileName) < 15 ){
      Serial.println(sizeof(*fileName));
      Serial.println("Didnt get the file name!");
     return;
    }
  */
  int qq;
  qq = GPS.hour * 4;
  if (GPS.minute >= 0 && GPS.minute <= 29) {
    qq += 2;
  }
  else {
    qq += 4;
  }
  sprintf(fileName, "%02i%02i%02i/%02i%02i%02i%02i.dat", GPS.year, GPS.month, GPS.day, GPS.year, GPS.month, GPS.day, qq);
  //  Serial.println(fileName);
}

/*---------------------------------------------------------
Function getPathName:
  - creates a path name for the current time.
-----------------------------------------------------------*/
void getPathName(char pathName[]) {
  // Create path name of the format: yymmdd

  sprintf(pathName, "%02i%02i%02i", GPS.year, GPS.month, GPS.day);
  //  Serial.println(pathName);
}

/*------------------------------
Function error:
  - blinks out an error code
--------------------------------*/
void error(uint8_t errno) {
  /*
    if (SD.errorCode()) {
      putstring("SD error: ");
      Serial.print(card.errorCode(), HEX);
      Serial.print(',');
      Serial.println(card.errorData(), HEX);
    }
    */
  while (1) {
    uint8_t i;
    for (i = 0; i < errno; i++) {
      digitalWrite(ledPin, HIGH);
      analogWrite(SD_workingLEDpin, LEDintensity);
      delay(100);
      digitalWrite(ledPin, LOW);
      analogWrite(SD_workingLEDpin, 0);
      delay(100);
    }
    for (i = errno; i < 10; i++) {
      delay(200);
    }
  }
}

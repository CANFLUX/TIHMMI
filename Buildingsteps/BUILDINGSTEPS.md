# ASSAMBLING TIHMMI

## Step 1: Put the Arduino Mega and the Adafruit GPS logger shield together as the instruction indicates



## Step 2: Adding the Digital Thermocouple Interface (MAX318566) to the Arduino Mega
 A- Connect the Thermocouple to the Interface.
 B- Connect the Interface to the Arduino Mega according to the diagram:

![](Steps1.jpg)

    CS to PIN 49
    SCK to PIN 52
    SDO to PIN 50
    SDI to PIN 51
    GRN to GRN
    VDI to 5V



## Step 3: Adding the AM4022 to the Arduino Mega

![](Step2.jpg)

 Connect the Temperature and Humidity sensor as the diagram:
   Red Wire (PWR) to the 5V
   Black Wire (GRN) to the GRN
   Yellow Wire (Data) to the PIN 2, be aware that you need to put a 10K resistor between these wire and the red wire.
 

## Step 4: Adding the MLX90614 to the Arduino Mega

![](Step3.jpg)

 Connect the Infrared Termometer sensor as the diagram:
   PWR to 5V
   GRN to GRN
   SCL to PIN 21 (SCL), be aware that you need to put a 10K resistor between these wire and the PWR.
   SDA to PIN 20 (SDA), be aware that you need to put a 10K resistor between these wire and the PWR.

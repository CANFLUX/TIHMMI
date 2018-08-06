# ARMANDO A TIHMMI

## Paso 1: Ensamblar Arduino Mega y el Adafruit GPS logger shield como indican las instrucciones de la plaqueta

## Paso 2: Incoporar Digital Thermocouple Interface (MAX318566) al Arduino Mega
A - Soldar los pines a la Digital Thermocouple Interface.
![](images/Img1.jpg)

B - Conectar la temocupla a la interface.

C - Conectar la interface al Arduino Mega según las siguientes instrucciones:
 
| Interface   | Arduino Mega |
|-------------|--------------|
| CS          | PIN 49       |
| SCK         | PIN 52       |
| SDO         | PIN 50       |
| SDI         | PIN 51       |
| GRN         | GRN          |
| VDI         | 5V           |

 
Para mayores detalle vea el siguiente diagrama:
![](images/Step1.jpg)

## Paso 3: Incorporar el AM4022 al Arduino Mega
Conectar el sensor de Temperatura y humedad según las siguientes instrucciones:

| Sensor         | Arduino Mega |
|----------------|--------------|
| Cable rojo     | 5V           |
| Cable Balnco   | GRN          |
| Cable amarillo | PIN 2        |


Necesitaras poner resistores de 10K resistor entre el cable amarillo y el rojo.

Para mayores detalle vea el siguiente diagrama:
![](images/Steps2.jpg)


## Step 4: Incorporar el MLX90614 al Arduino Mega
Conectar el sensor de Temeratura Infrarroja siguiendo estas instrucciones:

| Sensor      | Arduino Mega |
|-------------|--------------|
| PWR         | 5V           |
| GRN         | GRN          |
| SCL         | PIN 21(SCL)  |
| SDA         | PIN 20(SDA)  |


Necesitaras un resistor de 10K entre el SDA y el PWR.
Necesitaras un resistor de 10K entre el SCL y el PWR.

Para mayores detalle vea el siguiente diagrama:
![](images/Steps3.jpg)

 

#pragma once

// Board settings

// Uncomment één van volgende defines

// #define ENV_HOVER3MGYRO_ESP8266_LOLIND1MINILITE
// #define ENV_HOVER3MGYRO_ESP32C3_SUPERMINI
// #define ENV_HOVER3MGYRO_ESP32S2_LOLIN_S2_MINI

// #define ENV_BLIMP_ESP32C3_SUPERMINI_V0

// Als de defines in platformio.ini gedefinieerd zijn:
// #define ENV_USER_DEFINED

/*
Als je een ander board wenst te definiëren, zijn volgende defines nodig:
* De wifi instellingen WIFI_SOFTAP_SSID_PREFIX, WIFI_SOFTAP_PASSWORD, WIFI_SOFTAP_CHANNEL
* - USE_CONFIG_HOVERSERVO, USE_CONFIG_HOVER3M of USE_CONFIG_BLIMP 
    volgens de motorsetup respectievelijk 
    - een hover met servo en 1 Z-motor
    - een hover met 1 Z-motor en 2 bidirectionele motoren
    - een blimp (zeppelin) met 1 Z-motor en 2 bidirectionele motoren
    
* Als een gyro gebruikt wordt, zijn volgende defines nodig:
- USE_FASTIMU
- FASTIMU_TYPE // Currently supported by FASTIMU: MPU9255 MPU9250 MPU6886 MPU6500 MPU6050 ICM20689 ICM20690 BMI055 BMX055 BMI160 LSM6DS3 LSM6DSL QMI8658
- IMU_I2C_ADDRESS // 0x68 standaard bij MPU6050, 0x6B standaard bij LSM6DS3
- GYRO_REGELING_P
- GYRO_REGELING_MAX_DRAAI
- GYRO_REGELING_BIAS
- GYRO_DIRECTION : GYRO_DIRECTION_X, GYRO_DIRECTION_Y of GYRO_DIRECTION_Z
- (optioneel) GYRO_FLIP : gebruik de negatieve waarde van de gyro: als de gyro omgekeerd hangt
- (optioneel) PIN_SDA en PIN_SCL : indien niet gedefinieerd, worden de standaard Wire library pinnen van het bord gebruikt. 
  Als één van de I2C pinnen ook als PIN_LEDCONNECTIE gebruikt wordt, definieer ook PIN_LED_DUALUSE

* Als je seriële output wenst (en de RX/TX pinnen zijn niet in gebruik voor andere doelen):
#define DEBUG_SERIAL Serial

Volgende pinnen worden gedefinieerd:
- PIN_1AMOTOR
- PIN_2AMOTOR
- PIN_1BMOTOR
- PIN_2BMOTOR
- PIN_ZMOTOR    
- (optioneel) PIN_LEDCONNECTIE   


Daarnaast zijn volgende defines verplicht (maar kunnen omgewisseld worden)
#define LED_BRIGHTNESS_ON  HIGH
#define LED_BRIGHTNESS_OFF LOW

- MOTORZ_TIME_UP

Op ESP8266-chips wordt het voltage gemeten, voeg volgende define toe. Pas de voltagefactor aan, dat is bij elke chip verschillend. 
Calibreer bv. met USB stroom die 3.3V op de chip moet geven
#define VOLTAGE_FACTOR 1060.0f 

*/

enum 
{
    GYRO_DIRECTION_X,
    GYRO_DIRECTION_Y,
    GYRO_DIRECTION_Z,
};

#ifndef ENV_USER_DEFINED

#define WIFI_SOFTAP_PASSWORD "12345678"
#define WIFI_SOFTAP_CHANNEL 1 // 1-13

#if defined(CONFIG_IDF_TARGET_ESP32C3)
#define VOLTAGE_THRESHOLD 3.1 // onder dit voltage uit, om op hol slaan te vermijden op ESP32C3. Gemeten op batterij zelf.
#else
#define VOLTAGE_THRESHOLD 2.7 // onder dit voltage uit, om de batterij te beschermen, gemeten na de spanningsregelaar bij ESP8266.
#endif

#define ACCELERATION_THRESHOLD 0.1 // boven de som van kwadraten van de acceleraties boven deze waarde wordt als botsing beschouwd. VOOR BOTSDETECTIE
#define TIMEOUT_MS_COLLISION 3000L // Aantal milliseconden kleurverandering blijven tonen bij botsing VOOR BOTSDETECTIE

#endif

#if defined(ENV_HOVER3MGYRO_ESP8266_LOLIND1MINILITE)

/*
   Wemos D1 mini:
   SCL: D1
   SDA: D2
   XDA: niet aangesloten
   XCL: niet aangesloten
   AD0: niet aangesloten  . De vice heeft 0x68 als I2C adres, waarschijnlijk wordt het 0x69 als je dit naar 3.3V verhoogt
   INT: niet aangesloten
   VCC: 3V
   GND: uiteraard
*/
#define USE_CONFIG_HOVER3M

// #define DEBUG_SERIAL Serial

#define PIN_1AMOTOR          D8 // D8 = GPIO15 op D1 mini lite
#define PIN_2AMOTOR          D7 // D8 = GPIO15 op D1 mini lite
#define PIN_1BMOTOR          D6 // D8 = GPIO15 op D1 mini lite
#define PIN_2BMOTOR          D0 // D0 = GPIO16 op D1 mini lite
#define PIN_ZMOTOR           D3 // D8 = GPIO15 op D1 mini lite

// De ingebouwde LED zit meestal op GPIO2 of GPIO16
#define PIN_LEDCONNECTIE   2 

#define PIN_SDA           4 // D2 = GPIO4 op Wemos D1 mini lite
#define PIN_SCL            5 // D1 = GPIO5 op Wemos D1 mini lite

#define MOTORZ_TIME_UP 200 // ms to go to ease to full power of a motor

// Pas de voltagefactor aan, dat is bij elke chip verschillend. Calibreer bv. met USB stroom die 3.3V op de chip moet geven
#define VOLTAGE_FACTOR 910.0f 

#define LED_BRIGHTNESS_ON  LOW
#define LED_BRIGHTNESS_OFF HIGH

#elif defined(ENV_HOVER3MGYRO_ESP32C3_SUPERMINI)

#define USE_CONFIG_HOVER3M

//#define DEBUG_SERIAL Serial

#define PIN_1AMOTOR          0
#define PIN_2AMOTOR          1
#define PIN_1BMOTOR          2
#define PIN_2BMOTOR          3
#define PIN_ZMOTOR           4

#define PIN_LEDCONNECTIE   8

#define PIN_SDA            9 // Positie SDA op XIAO reeks
#define PIN_SCL            10 // Positie SCL op XIAO reeks

#define MOTORZ_TIME_UP 2000 // ms to go to ease to full power of a motor

#define LED_BRIGHTNESS_ON  LOW
#define LED_BRIGHTNESS_OFF HIGH


#elif defined(ENV_HOVER3MGYRO_ESP32S2_LOLIN_S2_MINI)

#define USE_CONFIG_HOVER3M

//#define DEBUG_SERIAL Serial

#define PIN_1AMOTOR          12 // Positie D8 op Wemos D1 mini
#define PIN_2AMOTOR          11 // Positie D7 op Wemos D1 mini
#define PIN_1BMOTOR          9  // Positie D6 op Wemos D1 mini
#define PIN_2BMOTOR          5  // Positie D0 op Wemos D1 mini
#define PIN_ZMOTOR           18 // Positie D3 op Wemos D1 mini
#define PIN_LEDCONNECTIE     15 // De ingebouwde LED 

#define PIN_SDA              33
#define PIN_SCL              35

#define USE_WS2812FX
#define PIN_WS2812FX       16 // dual use led
#define WS2812FX_NUMLEDS    5
#define WS2812FX_RGB_ORDER  NEO_GRB
#define WS2812FX_BRIGHTNESS 35 // 0 .. 255
#define WS2812FX_SPEED 1000 // in ms
#define WS2812FX_COLOR 0x007BFF // blauw
#define WS2812FX_COLLISION 0xFF0000 // rood
#define WS2812FX_MODE FX_MODE_FADE // Volledige lijst op https://github.com/kitesurfer1404/WS2812FX/blob/master/src/modes_arduino.h

#define MOTORZ_TIME_UP 200 // ms to go to ease to full power of a motor

#define LED_BRIGHTNESS_ON  HIGH
#define LED_BRIGHTNESS_OFF LOW

#elif defined(ENV_HOVER3MGYRO_ESP8266_LOLIND1MINILITE_WS2812FX)

#define USE_CONFIG_HOVER3M

// #define DEBUG_SERIAL Serial

#define PIN_1AMOTOR          D8 // D8 = GPIO15 op D1 mini lite
#define PIN_2AMOTOR          D7 // D8 = GPIO15 op D1 mini lite
#define PIN_1BMOTOR          D6 // D8 = GPIO15 op D1 mini lite
#define PIN_2BMOTOR          D0 // D0 = GPIO16 op D1 mini lite
#define PIN_ZMOTOR           D3 // D8 = GPIO15 op D1 mini lite

// De ingebouwde LED zit meestal op GPIO2 of GPIO16
#define PIN_LEDCONNECTIE   2 
#define PIN_LED_DUALUSE

#define MOTORZ_TIME_UP 200 // ms to go to ease to full power of a motor

#define USE_WS2812FX
#define PIN_WS2812FX       D4 // =GPIO2 dual use led
#define WS2812FX_NUMLEDS    5
#define WS2812FX_RGB_ORDER  NEO_GRB
#define WS2812FX_BRIGHTNESS 35 // 0 .. 255
#define WS2812FX_SPEED 1000 // in ms
#define WS2812FX_COLOR 0x007BFF
#define WS2812FX_COLLISION 0xFF0000 // rood
#define WS2812FX_MODE FX_MODE_FADE // Volledige lijst op https://github.com/kitesurfer1404/WS2812FX/blob/master/src/modes_arduino.h

#define PIN_SDA           4 // D2 = GPIO4 op Wemos D1 mini lite
#define PIN_SCL            5 // D1 = GPIO5 op Wemos D1 mini lite

// Pas de voltagefactor aan, dat is bij elke chip verschillend. Calibreer bv. met USB stroom die 3.3V op de chip moet geven
#define VOLTAGE_FACTOR 910.0f 

#define LED_BRIGHTNESS_ON  LOW
#define LED_BRIGHTNESS_OFF HIGH

#elif defined(ENV_BLIMP_ESP32C3_SUPERMINI_V0)
#define USE_CONFIG_BLIMP2Z

// No DEBUG_SERIAL Serial : pin 20 & 21 in use

#define PIN_1AMOTOR          5
#define PIN_2AMOTOR          6
#define PIN_1BMOTOR          20
#define PIN_2BMOTOR          21
#define PIN_1ZMOTOR          7
#define PIN_2ZMOTOR          10
#define PIN_LEDCONNECTIE     8 
#define PIN_LED_DUALUSE
#define PIN_BATMONITOR     1

#define USE_FASTIMU
#define FASTIMU_TYPE MPU6050
#define IMU_I2C_ADDRESS 0x68
#define GYRO_DIRECTION GYRO_DIRECTION_Z
#define GYRO_FLIP

#define PIN_SDA            3           
#define PIN_SCL            4

#define USE_WS2812FX
#define PIN_WS2812FX       8 // dual use led
#define WS2812FX_NUMLEDS    5
#define WS2812FX_RGB_ORDER  NEO_GRB
#define WS2812FX_BRIGHTNESS 35 // 0 .. 255
#define WS2812FX_SPEED 1000 // in ms
#define WS2812FX_COLOR 0x007BFF // blauw
#define WS2812FX_COLLISION 0xFF0000 // rood
#define WS2812FX_MODE FX_MODE_FADE // Volledige lijst op https://github.com/kitesurfer1404/WS2812FX/blob/master/src/modes_arduino.h

#define MOTORZ_TIME_UP 200 // ms to go to ease to full power of a motor
#define MOTORZ_MINSPEED (PWM_RANGE/8)

// Pas de voltagefactor aan, dat is bij elke chip verschillend. Calibreer bv. met USB stroom die 3.3V op de chip moet geven
#define VOLTAGE_FACTOR 850.0f 

#define LED_BRIGHTNESS_ON  LOW
#define LED_BRIGHTNESS_OFF HIGH

#elif defined ENV_USER_DEFINED
// defines staan buiten de code

#else
// Geen ENV_XX geselecteerd
#error "Defineer één van bovenstaande defines"


#endif

#ifndef ENV_USER_DEFINED

#if defined(USE_CONFIG_HOVERSERVO)

#define WIFI_SOFTAP_SSID_PREFIX "hover-"

#elif defined (USE_CONFIG_HOVER3M)

#define WIFI_SOFTAP_SSID_PREFIX "hover3m-"

// gyro instellingen voor Hover3M
#define USE_FASTIMU
#define FASTIMU_TYPE MPU6050
#define IMU_I2C_ADDRESS 0x68
#define GYRO_DIRECTION GYRO_DIRECTION_Z
#define GYRO_REGELING_MAX_P     2.4
#define GYRO_REGELING_MAX_DRAAI 0.5
#define GYRO_REGELING_BIAS      1.0

#define XY_MOTOR_MAX    1.0 // later regelbaar maken 0.0 - 1.0

#elif defined (USE_CONFIG_BLIMP)

// Gyro instellingen voor Blimp
#define USE_FASTIMU
#define FASTIMU_TYPE MPU6050
#define IMU_I2C_ADDRESS 0x68
#define GYRO_DIRECTION GYRO_DIRECTION_Z
#define GYRO_FLIP
#define GYRO_REGELING_MAX_P     2.4
#define GYRO_REGELING_MAX_DRAAI 0.5
#define GYRO_REGELING_BIAS      1.0

#define XY_MOTOR_MAX    1.0 // later regelbaar maken 0.0 - 1.0

#define WIFI_SOFTAP_SSID_PREFIX "Blimp-"

#elif defined (USE_CONFIG_BLIMP2Z)

// Gyro instellingen voor Blimp
#define GYRO_REGELING_MAX_P     2.4
#define GYRO_REGELING_MAX_DRAAI 0.5
#define GYRO_REGELING_BIAS      1.0

#define XY_MOTOR_MAX   0.75 // later regelbaar maken 0.0 - 1.0

#define WIFI_SOFTAP_SSID_PREFIX "Blimp-"


#endif

#endif

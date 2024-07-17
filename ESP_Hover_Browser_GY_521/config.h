#pragma once

// Board settings

// Uncomment één van volgende defines

// #define ENV_HOVER3MGYRO_ESP8266_LOLIND1MINILITE
// #define ENV_HOVER3MGYRO_ESP32C3_SUPERMINI
// #define ENV_HOVER3MGYRO_ESP32S2_LOLIN_S2_MINI

// Als de defines in platformio.ini gedefinieerd zijn:
// #define ENV_USER_DEFINED

/*
Als je een ander board wenst te definiëren, zijn volgende defines nodig:
* Als een gyro gebruikt wordt, zijn volgende defines nodig:
- USE_GY521
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

// Volgende defines zijn op alle borden van toepassing
#define WIFI_SOFTAP_SSID_PREFIX_HOVER   "hover-"
#define WIFI_SOFTAP_SSID_PREFIX_HOVER3M "hover3m-"
#define WIFI_SOFTAP_SSID_PREFIX_BLIMP   "Blimp-"



#define WIFI_SOFTAP_PASSWORD "12345678"
#define WIFI_SOFTAP_CHANNEL 1 // 1-13

#define VOLTAGE_THRESHOLD 2.7 // onder dit voltage valt de ESP8266-chip uit om de batterij te beschermen

#define USE_GY521
#define GYRO_DIRECTION GYRO_DIRECTION_Z
#define GYRO_REGELING_P         4.0
#define GYRO_REGELING_MAX_DRAAI 0.5
#define GYRO_REGELING_BIAS      1.0

#endif

#if defined(ENV_HOVER3MGYRO_ESP8266_LOLIND1MINILITE)
#define WIFI_SOFTAP_SSID_PREFIX WIFI_SOFTAP_SSID_PREFIX_HOVER3M

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

#define DEBUG_SERIAL Serial

#define PIN_1AMOTOR          D8 // D8 = GPIO15 op D1 mini lite
#define PIN_2AMOTOR          D7 // D8 = GPIO15 op D1 mini lite
#define PIN_1BMOTOR          D6 // D8 = GPIO15 op D1 mini lite
#define PIN_2BMOTOR          D0 // D0 = GPIO16 op D1 mini lite
#define PIN_ZMOTOR           D3 // D8 = GPIO15 op D1 mini lite

// De ingebouwde LED zit meestal op GPIO2 of GPIO16
#define PIN_LEDCONNECTIE   2 

#define PIN_SDA           4 // D2 = GPIO4 op Wemos D1 mini lite
#define PIN_SCL            5 // D1 = GPIO5 op Wemos D1 mini lite

// Pas de voltagefactor aan, dat is bij elke chip verschillend. Calibreer bv. met USB stroom die 3.3V op de chip moet geven
#define VOLTAGE_FACTOR 910.0f 

#define LED_BRIGHTNESS_ON  LOW
#define LED_BRIGHTNESS_OFF HIGH



#elif defined(ENV_HOVER3MGYRO_ESP32C3_SUPERMINI)
#define WIFI_SOFTAP_SSID_PREFIX WIFI_SOFTAP_SSID_PREFIX_HOVER3M

#define DEBUG_SERIAL Serial

#define PIN_1AMOTOR          0
#define PIN_2AMOTOR          1
#define PIN_1BMOTOR          2
#define PIN_2BMOTOR          3
#define PIN_ZMOTOR           4

#define PIN_LEDCONNECTIE   8

#define PIN_SDA            9 // Positie SDA op XIAO reeks
#define PIN_SCL            10 // Positie SCL op XIAO reeks

#define LED_BRIGHTNESS_ON  LOW
#define LED_BRIGHTNESS_OFF HIGH



#elif defined(ENV_HOVER3MGYRO_ESP32S2_LOLIN_S2_MINI)
#define WIFI_SOFTAP_SSID_PREFIX WIFI_SOFTAP_SSID_PREFIX_HOVER3M

#define DEBUG_SERIAL Serial

#define PIN_1AMOTOR          12 // Positie D8 op Wemos D1 mini
#define PIN_2AMOTOR          11 // Positie D7 op Wemos D1 mini
#define PIN_1BMOTOR          9  // Positie D6 op Wemos D1 mini
#define PIN_2BMOTOR          5  // Positie D0 op Wemos D1 mini
#define PIN_ZMOTOR           18 // Positie D3 op Wemos D1 mini
#define PIN_LEDCONNECTIE     15 // De ingebouwde LED 

#define PIN_SDA              33
#define PIN_SCL              35

#define LED_BRIGHTNESS_ON  HIGH
#define LED_BRIGHTNESS_OFF LOW




#elif defined ENV_USER_DEFINED
// defines staan buiten de code

#else
// Geen ENV_XX geselecteerd
#error "Defineer één van bovenstaande defines"

#endif

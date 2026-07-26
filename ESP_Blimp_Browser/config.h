#pragma once

// Board settings

// Uncomment one of the following defines, depending on the hardware and application

// #define ENV_HOVER3MGYRO_ESP8266_LOLIND1MINILITE
// #define ENV_HOVER3MGYRO_ESP32C3_SUPERMINI
// #define ENV_HOVER3MGYRO_ESP32S2_LOLIN_S2_MINI

// #define ENV_BLIMP_ESP32C3_SUPERMINI_V0
// #define ENV_BLIMP_ESP32C3_WROOM_V0
// there is no ENV_BLIMP_ESP32C3_WROOM_V1
// #define ENV_BLIMP_ESP32C3_WROOM_V2
// #define ENV_BLIMP_ESP32C3_WROOM_V3
// #define ENV_BLIMP_ESP32C3_WROOM_V3_REVERSED_MOTORS

// if the defines are defined in platformio.ini :
// #define ENV_USER_DEFINED

/*
If you want to define another board, the following  defined are needed:
* The Wi-Fi settings
WIFI_SOFTAP_SSID_PREFIX, WIFI_SOFTAP_PASSWORD, WIFI_SOFTAP_CHANNEL
* - USE_CONFIG_HOVERSERVO, USE_CONFIG_HOVER3M of USE_CONFIG_BLIMP 
    Corresponding to the motorsetup respectively 
    - a hovercraft with a servo and 1 Z-motor
    - a hovercraft with 1 Z-motor and 2 bidirectional
    - a blimp (airship, zeppelin) with 1 Z-motor and 2 bidirectional motors
    
* If a gyro is used, the following defines are needed:
- USE_FASTIMU
- FASTIMU_TYPE // Currently supported by FASTIMU: MPU9255 MPU9250 MPU6886 MPU6500 MPU6050 ICM20689 ICM20690 BMI055 BMX055 BMI160 LSM6DS3 LSM6DSL QMI8658
- IMU_I2C_ADDRESS // 0x68 standard for MPU6050, 0x6B standard for LSM6DS3
- GYRO_REGELING_P
- GYRO_REGELING_MAX_DRAAI
- GYRO_REGELING_BIAS
- GYRO_LPF_TF   Tf in seconds
- GYRO_DIRECTION : GYRO_DIRECTION_X, GYRO_DIRECTION_Y of GYRO_DIRECTION_Z
- (optional) GYRO_FLIP : use the opposite of the gyro value: if the gyro hangs upside down
- (optional) PIN_SDA en PIN_SCL : if not defined, the standard Wire library pins of the board are used. 
- (optional) XY_MOTOR_LIMIT_SLIDER : to set the top slider as the max motorpower (0 .. 1). If not, the top slider sets the P-value for the gyro based control.
  If one of the I2C pins is also used as PIN_LEDCONNECTIE, then define PIN_LED_DUALUSE

* If you want serial output (and the serial communication pins are not used for something else):
#define DEBUG_SERIAL Serial

Following pins are defined:
- PIN_1AMOTOR
- PIN_2AMOTOR
- PIN_1BMOTOR
- PIN_2BMOTOR
- PIN_ZMOTOR    
- (optional) PIN_LEDCONNECTIE   

Also following defines are mandatory (but can be switched)
#define LED_BRIGHTNESS_ON  HIGH
#define LED_BRIGHTNESS_OFF LOW

- MOTORZ_TIME_UP

On ESP8266-chips the 3.3V and on ESP32 boards with the necessary resistor bridge the battery voltage can be monitored by adding the following defines
Calibrate the value, which differs for each chip.
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
#define WIFI_SOFTAP_CHANNEL 1 // 1-13, preferably use only channels 1, 6, and 11 to avoid interference.

#if defined(CONFIG_IDF_TARGET_ESP32C3)
#define VOLTAGE_THRESHOLD 3.0 // Below this voltage the board is shut down, to avoid battery damage and uncontrolled behavior (measured on the battery).
#else
#define VOLTAGE_THRESHOLD 2.7 // Below this voltage the board is shut down, to avoid battery damage and uncontrolled behavior (measured after the voltage regulator with 0.3V voltage drop).
#endif

#define ACCELERATION_THRESHOLD 0.1 // If the sum of the square of the accelerations is above this value, this is considered as a collision. For collision detection 
#define TIMEOUT_MS_COLLISION 3000L // With added WS2812:  number of milliseconds another colour is shown at collision detection

#endif

#if defined(ENV_HOVER3MGYRO_ESP8266_LOLIND1MINILITE)

/*
   Wemos D1 mini (up to V3)
   SCL: D1
   SDA: D2
   XDA: not connected 
   XCL:  not connected
   AD0:  not connected
   INT: not connected
   5V: battery
   GND: uiteraard
*/
#define USE_CONFIG_HOVER3M

// #define DEBUG_SERIAL Serial

#define PIN_1AMOTOR          D8 // D8 = GPIO15 on D1 mini lite
#define PIN_2AMOTOR          D7 // D8 = GPIO15 on D1 mini lite
#define PIN_1BMOTOR          D6 // D8 = GPIO15 on D1 mini lite
#define PIN_2BMOTOR          D0 // D0 = GPIO16 on D1 mini lite
#define PIN_ZMOTOR           D3 // D8 = GPIO15 on D1 mini lite

// built in LED is most often on GPIO2 of GPIO16
#define PIN_LEDCONNECTIE   2 

#define PIN_SDA           4 // D2 = GPIO4 on Wemos D1 mini lite
#define PIN_SCL            5 // D1 = GPIO5 on Wemos D1 mini lite

#define MOTORZ_TIME_UP 1000 // ms to go to ease to full power of a motor

// Calibrate the  voltage factor (different for each chip)
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

#define PIN_SDA            9 // Position SDA on XIAO reeks
#define PIN_SCL            10 // Position SCL on XIAO reeks

#define MOTORZ_TIME_UP 2000 // ms time to ease to full power of a motor

#define LED_BRIGHTNESS_ON  LOW
#define LED_BRIGHTNESS_OFF HIGH


#elif defined(ENV_HOVER3MGYRO_ESP32S2_LOLIN_S2_MINI)

#define USE_CONFIG_HOVER3M

//#define DEBUG_SERIAL Serial

#define PIN_1AMOTOR          12 // D8 on Wemos D1 mini
#define PIN_2AMOTOR          11 // D7 on Wemos D1 mini
#define PIN_1BMOTOR          9  // D6 on Wemos D1 mini
#define PIN_2BMOTOR          5  // D0 on Wemos D1 mini
#define PIN_ZMOTOR           18 // D3 on D1 mini
#define PIN_LEDCONNECTIE     15 // built in LED 

#define PIN_SDA              33
#define PIN_SCL              35

#define USE_WS2812FX
#define PIN_WS2812FX       16 // dual use led
#define WS2812FX_NUMLEDS    5
#define WS2812FX_RGB_ORDER  NEO_GRB
#define WS2812FX_BRIGHTNESS 35 // 0 .. 255
#define WS2812FX_SPEED 1000 // in ms
#define WS2812FX_COLOR 0x007BFF // blue
#define WS2812FX_COLLISION 0xFF0000 // red
#define WS2812FX_MODE FX_MODE_FADE // Full list on https://github.com/kitesurfer1404/WS2812FX/blob/master/src/modes_arduino.h

#define MOTORZ_TIME_UP 200 // time in ms to ease to full power of a motor

#define LED_BRIGHTNESS_ON  HIGH
#define LED_BRIGHTNESS_OFF LOW

#elif defined(ENV_HOVER3MGYRO_ESP8266_LOLIND1MINILITE_WS2812FX)

#define USE_CONFIG_HOVER3M

// #define DEBUG_SERIAL Serial

#define PIN_1AMOTOR          D8 // D8 = GPIO15 on D1 mini lite
#define PIN_2AMOTOR          D7 // D8 = GPIO15 on D1 mini lite
#define PIN_1BMOTOR          D6 // D8 = GPIO15 on D1 mini lite
#define PIN_2BMOTOR          D0 // D0 = GPIO16 on D1 mini lite
#define PIN_ZMOTOR           D3 // D8 = GPIO15 on D1 mini lite

// Built in LED is usually on GPIO2 or GPIO16
#define PIN_LEDCONNECTIE   2 
#define PIN_LED_DUALUSE

#define MOTORZ_TIME_UP 1000 // time in ms to ease to full power of a motor

#define USE_WS2812FX
#define PIN_WS2812FX       D4 // =GPIO2 dual use led
#define WS2812FX_NUMLEDS    5
#define WS2812FX_RGB_ORDER  NEO_GRB
#define WS2812FX_BRIGHTNESS 35 // 0 .. 255
#define WS2812FX_SPEED 1000 // in ms
#define WS2812FX_COLOR 0x007BFF // blue
#define WS2812FX_COLLISION 0xFF0000 // red
#define WS2812FX_MODE FX_MODE_FADE // Full list on https://github.com/kitesurfer1404/WS2812FX/blob/master/src/modes_arduino.h

#define PIN_SDA           4 // D2 = GPIO4 on Wemos D1 mini lite
#define PIN_SCL            5 // D1 = GPIO5 on Wemos D1 mini lite

// Calibrate the  voltage factor (different for each chip)
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
#define PIN_BATMONITOR     1

#define USE_FASTIMU
#define FASTIMU_TYPE MPU6050
#define IMU_I2C_ADDRESS 0x68
#define GYRO_DIRECTION GYRO_DIRECTION_Z
#define GYRO_FLIP

#define PIN_SDA            3           
#define PIN_SCL            4

#define USE_WS2812FX
#define PIN_WS2812FX       9 // dual use led
#define WS2812FX_NUMLEDS    5
#define WS2812FX_RGB_ORDER  NEO_GRB
#define WS2812FX_BRIGHTNESS 35 // 0 .. 255
#define WS2812FX_SPEED 1000 // in ms
#define WS2812FX_COLOR 0x007BFF // blue
#define WS2812FX_COLLISION 0xFF0000 // red
#define WS2812FX_MODE FX_MODE_FADE // Full list on https://github.com/kitesurfer1404/WS2812FX/blob/master/src/modes_arduino.h

#define MOTORZ_TIME_UP 200 // ms to go to ease to full power of a motor
#define MOTORZ_MINSPEED (PWM_RANGE/8)

// Calibrate the  voltage factor (different for each chip)
#define VOLTAGE_FACTOR 850.0f 

#define LED_BRIGHTNESS_ON  LOW
#define LED_BRIGHTNESS_OFF HIGH

#elif defined(ENV_BLIMP_ESP32C3_WROOM_V0) // Maker Fair Gent 2025
#define USE_CONFIG_BLIMP2Z

// No DEBUG_SERIAL Serial : pin 20 & 21 in use

#define PIN_1AMOTOR          21
#define PIN_2AMOTOR          20
#define PIN_1BMOTOR          4
#define PIN_2BMOTOR          5
#define PIN_1ZMOTOR          6
#define PIN_2ZMOTOR          7
#define PIN_LEDCONNECTIE     8 
#define PIN_LED_DUALUSE // dual use led
#define USE_WS2812FX
#define PIN_WS2812FX       9 
#define PIN_BATMONITOR     1

#define USE_FASTIMU
#define FASTIMU_TYPE LSM6DS3
#define IMU_I2C_ADDRESS 0x6B
#define GYRO_DIRECTION GYRO_DIRECTION_Z
#define GYRO_FLIP

#define PIN_SDA            19          
#define PIN_SCL            10

#define USE_WS2812FX
#define PIN_WS2812FX       9 // dual use led
#define WS2812FX_NUMLEDS    1
#define WS2812FX_RGB_ORDER  NEO_GRB
#define WS2812FX_BRIGHTNESS 35 // 0 .. 255
#define WS2812FX_SPEED 1000 // in ms
#define WS2812FX_COLOR 0x007BFF // blue
#define WS2812FX_COLLISION 0xFF0000 // red
#define WS2812FX_MODE FX_MODE_FADE // Full list on https://github.com/kitesurfer1404/WS2812FX/blob/master/src/modes_arduino.h

#define MOTORZ_TIME_UP 500 // time in ms to ease to full power of a motor
#define MOTORZ_MINSPEED (PWM_RANGE/8)

// Calibrate the  voltage factor (different for each chip)
#define VOLTAGE_FACTOR 820.0f 

#define LED_BRIGHTNESS_ON  LOW
#define LED_BRIGHTNESS_OFF HIGH

#elif defined(ENV_BLIMP_ESP32C3_WROOM_V2) // Maker Fair Gent 2025
#define USE_CONFIG_BLIMP2Z

// #define DEBUG_SERIAL Serial

#define PIN_1AMOTOR          18
#define PIN_2AMOTOR          3
#define PIN_1BMOTOR          4
#define PIN_2BMOTOR          5
#define PIN_1ZMOTOR          6
#define PIN_2ZMOTOR          7
#define PIN_LEDCONNECTIE     8 
#define PIN_LED_DUALUSE // dual use led
#define USE_WS2812FX
#define PIN_WS2812FX       9 
#define PIN_BATMONITOR     1

#define USE_FASTIMU
#define FASTIMU_TYPE LSM6DS3
#define IMU_I2C_ADDRESS 0x6B
#define GYRO_DIRECTION GYRO_DIRECTION_Z
#define GYRO_FLIP

#define PIN_SDA            19          
#define PIN_SCL            10

#define USE_WS2812FX
#define PIN_WS2812FX       9 // dual use led
#define WS2812FX_NUMLEDS    1
#define WS2812FX_RGB_ORDER  NEO_GRB
#define WS2812FX_BRIGHTNESS 35 // 0 .. 255
#define WS2812FX_SPEED 1000 // in ms
#define WS2812FX_COLOR 0x007BFF // blue
#define WS2812FX_COLLISION 0xFF0000 // red
#define WS2812FX_MODE FX_MODE_FADE // Full list on https://github.com/kitesurfer1404/WS2812FX/blob/master/src/modes_arduino.h

#define MOTORZ_TIME_UP 500 // ms to go to ease to full power of a motor
#define MOTORZ_MINSPEED (PWM_RANGE/8)

// Calibrate the  voltage factor (different for each chip)
#define VOLTAGE_FACTOR 820.0f 

#define LED_BRIGHTNESS_ON  LOW
#define LED_BRIGHTNESS_OFF HIGH

#elif defined(ENV_BLIMP_ESP32C3_WROOM_V3) || defined(ENV_BLIMP_ESP32C3_WROOM_V3_REVERSED_MOTORS)
// PCB V3, V3.1, V3.2 & V3.3,  Makerday Hasselt 2025, Maker Days Eindhoven 2025, Maker Faire Gent 2026, Fri3d Camp 2026, Maker Days Eindhoven 2026, FTI NEXT dagen oktoberfest 2026

#define USE_CONFIG_BLIMP2Z

// #define DEBUG_SERIAL Serial

#if defined(ENV_BLIMP_ESP32C3_WROOM_V3)
#define PIN_1AMOTOR          10
#define PIN_2AMOTOR          7
#define PIN_1BMOTOR          0
#define PIN_2BMOTOR          3
#define PIN_1ZMOTOR          4 
#define PIN_2ZMOTOR          5
#endif

#if defined(ENV_BLIMP_ESP32C3_WROOM_V3_REVERSED_MOTORS)
#define PIN_1AMOTOR          7
#define PIN_2AMOTOR          10
#define PIN_1BMOTOR          3
#define PIN_2BMOTOR          0
#define PIN_1ZMOTOR          5 
#define PIN_2ZMOTOR          4
#endif

#define PIN_LEDCONNECTIE     8 
#define PIN_LED_DUALUSE // dual use led
#define USE_WS2812FX
#define PIN_WS2812FX       9 
#define PIN_BATMONITOR     1

#define USE_FASTIMU
#define FASTIMU_TYPE LSM6DS3
#define IMU_I2C_ADDRESS 0x6B
#define GYRO_DIRECTION GYRO_DIRECTION_Z
#define GYRO_FLIP

#define PIN_SDA            2
#define PIN_SCL            6

#define USE_WS2812FX
#define PIN_WS2812FX       9 // dual use led/boot
#define WS2812FX_NUMLEDS    1
#define WS2812FX_RGB_ORDER  NEO_GRB
#define WS2812FX_BRIGHTNESS 26 // 0 .. 255
#define WS2812FX_SPEED 1000 // in ms
#define WS2812FX_COLOR 0x007BFF // blue
#define WS2812FX_COLLISION 0xFF0000 // red
#define WS2812FX_MODE FX_MODE_FADE // Full list on https://github.com/kitesurfer1404/WS2812FX/blob/master/src/modes_arduino.h


#define MOTORZ_TIME_UP 500 // time in ms to ease to full power of a motor
#define MOTORZ_MINSPEED (PWM_RANGE/8)

// Calibrate the  voltage factor (different for each chip)
#define VOLTAGE_FACTOR 820.0f 

#define LED_BRIGHTNESS_ON  LOW
#define LED_BRIGHTNESS_OFF HIGH

#elif defined ENV_USER_DEFINED
// defines are outside the code

#else
// No ENV_XX selected
#error "Define one of the defines above"


#endif

#ifndef ENV_USER_DEFINED

#if defined(USE_CONFIG_HOVERSERVO)

#define WIFI_SOFTAP_SSID_PREFIX "hover-"

#elif defined (USE_CONFIG_HOVER3M)

#define WIFI_SOFTAP_SSID_PREFIX "hover3m-"

// gyro instellingen for Hover3M
#define USE_FASTIMU
#define FASTIMU_TYPE MPU6050
#define IMU_I2C_ADDRESS 0x68
#define GYRO_DIRECTION GYRO_DIRECTION_Z
#define GYRO_REGELING_MAX_P     2.4
#define GYRO_REGELING_MAX_DRAAI 0.5
#define GYRO_REGELING_BIAS      1.0
#define GYRO_LPF_TF             0.080 // Tf in seconds

#define XY_MOTOR_MAX    1.0
#define XY_MOTOR_LIMIT_SLIDER

#elif defined (USE_CONFIG_BLIMP)
// z-motor unidirectional 

// Gyro settings for blimp
#define USE_FASTIMU
#define FASTIMU_TYPE MPU6050
#define IMU_I2C_ADDRESS 0x68
#define GYRO_DIRECTION GYRO_DIRECTION_Z
#define GYRO_FLIP
#define GYRO_REGELING_MAX_P     2.4
#define GYRO_REGELING_MAX_DRAAI 0.5
#define GYRO_REGELING_BIAS      1.0
#define GYRO_LPF_TF             0.080 // Tf in seconds

#define XY_MOTOR_MAX    1.0
#define XY_MOTOR_LIMIT_SLIDER

#define WIFI_SOFTAP_SSID_PREFIX "Blimp-"

#elif defined (USE_CONFIG_BLIMP2Z)
// z-motor bidirectional 

// Gyro settings for blimp
#define GYRO_REGELING_MAX_P     2.4
#define GYRO_REGELING_MAX_DRAAI 0.5
#define GYRO_REGELING_BIAS      1.0
#define GYRO_LPF_TF             0.080 // Tf in seconds

#define XY_MOTOR_MAX   1.0
#define XY_MOTOR_LIMIT_SLIDER

#define WIFI_SOFTAP_SSID_PREFIX "Blimp-"


#endif

#endif





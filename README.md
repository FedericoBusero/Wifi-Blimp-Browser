# Wifi-Blimp-Browser
Wi-Fi controlled (from browser) blimp (airship, dirigeable, zeppelin) on ESP8266 or ESP32-C3, with optional gyro (e.g. GY-521 or LSM6DS3TR-C).

![blimp.png](blimp.png "Blimp example pictures")

Wifi-Blimp-Browser contains the software for remote controlled vehicles (such as blimps) developed by masynmachien, based on the ESP8266 or ESP32. Control is done via an interactive web interface in the browser, meaning there is no need to install a separate app.

The project uses an IMU (gyroscope) for stabilising straight-line movement and supports various hardware configurations.

This project focuses on controlling vehicles (blimps, hovercrafts) with bidirectional left and right DC motors and a third DC motor for lift (uni- or bidirectional) or hovering. If you’d prefer to control a hovercraft using a servo, please see the other project, [Wifi-Hovercraft-Browser](https://github.com/FedericoBusero/Wifi-Hovercraft-Browser)

## Features
* **Web-based interface:** Control your vehicle using a joystick and sliders in your browser (Chrome, Safari, Firefox).
* Communication takes place via **Wi-Fi** using a **SoftAP** (WifiPoint). The vehicle therefore has its own on-board access point, so no actual internet connection is required.
* It was developed for airships (blimps, airships), but there are also configuration examples for hovercrafts (the type with left and right motors, rather than servos).
* **Gyro stabilisation:** Uses the FastIMU library and a low-pass filter for stable flight or navigation.
* Although the configuration is based on the ESP32-C3, the code is compatible with other ESP32 or ESP8266 chips.

## Hardware Requirements


* **Web-based Interface:** Bedien je voertuig via een joystick en sliders in je browser (Chrome, Safari, Firefox).
* De communicatie verloopt via **Wifi** m.b.v. een **SoftAP** (WifiPoint). Het voertuig heeft dus een eigen access point aan boord, er is dus geen echte internetverbinding.
* Het is ontwikkeld voor zeppelins(blimp) maar er zijn ook configuratievoorbeelden voor hovercrafts (die met linker- en rechtermotor werken, dus niet met servo).
* **Gyro-stabilisatie:** Maakt gebruik van de `FastIMU` library en een Low Pass Filter voor stabiele vlucht of vaart.
* Hoewel de configuratie gebaseerd is op ESP32-C3, is code compatibel met andere ESP32 of ESP8266-chips.

## Hardware Vereisten

Afhankelijk van de gekozen configuratie in `config.h`:
* **Microcontroller:** ESP32-C3
* **IMU:** bv. LSM6DS3TR-C of MPU6050
* **3 DC-Motoren:** Ondersteuning voor DC-motoren en bidirectionele motoren (afhankelijk van het type: Blimp of Hover3M). Dat zijn 2 motoren voor het links/rechts sturen en 1 motor voor het op/neer sturen.
* 3-voudige **H-brug**
* **LiPo-batterij**
* **Ballon** met helium gevuld

## Installatie & Gebruik

### 1. Bibliotheken installeren
Zorg dat de volgende libraries in je Arduino IDE zijn geïnstalleerd (ESP32C3-versie):
* [ArduinoWebsockets](https://github.com/gilmaimon/ArduinoWebsockets) door Gil Maimon.
* [FastIMU](https://github.com/LiquidCGS/FastIMU) (minimaal versie 1.2.8).
* [AsyncTCP](https://github.com/me-no-dev/AsyncTCP)
* [ESPAsyncWebSrv](https://github.com/dvarrel/ESPAsyncWebSrv), versie 1.2.9

### 2. Configuraties
Open `config.h` en kies je hardwareprofiel door de relevante `#define` te uncommenten:
```cpp
// Voorbeeld: Kies voor een Blimp met 3 motoren op een ESP32C3
#define ENV_BLIMP_ESP32C3_WROOM_V3
```
Dit is de blimp die gebruikt is op Hasselt 2025, Makerfair Eindhoven 2025 en Makerfait Gent 2026.

### 3. Uploaden
Upload de code naar je ESP-board via de Arduino IDE of PlatformIO.

### 4. Verbinding maken
1.  Zoek op je smartphone of computer naar het wifi-netwerk met de naam `hover-xxxx` of `blimp-xxxx`.
2.  Gebruik het standaard wachtwoord: `12345678`.
3.  Open je browser (Chrome, Firefox, Safari, ..) en ga naar: `http://192.168.4.1` of `http://h.be`.

## Bediening
![screenshot.png](screenshot.png "Blimp control screenshot")

De webinterface bevat de volgende elementen:
* **Bovenste Statusbalk:** Toont de verbindingsstatus (en optioneel de gyro draaisnelheid) en (indien ondersteund) de batterijspanning.
* **P-Factor Slider:** Regelt de gevoeligheid van de gyroscoop-regeling.
* **Zweefmotor Slider:** Stelt de kracht van de motor in die het voertuig laat zweven.
* **Joystick:** Bestuurt de richting en de stuwkracht van de motoren.

## Tips
* **Wifi-behoud:** Omdat het netwerk geen internet heeft, kan je telefoon vragen of je verbonden wilt blijven. Kies "Ja".
* **Calibratie:** De batterijspanning kan gecalibreerd worden in de code met de `VOLTAGE_FACTOR`.
* **Noodstop:** Bij verlies van de verbinding (Disconnect) zullen de motoren uit veiligheidsoverwegingen stoppen.

## Hoe maak je een blimp/zeppelin?
Voor workshops kan je terecht bij [masynmachien](https://www.masynmachien.be/)
* De Nederlandstalige bouwbeschrijving van masynmachiens Wifi bestuurde "zeppelin" vind je [hier](https://drive.google.com/file/d/1kgbARLVWbW1ju_md69NMXsFJ5Pm3_wp5/view?pli=1)
* English build instructions of MasynMachien's Wi-Fi Controlled "Zeppelin" can be found [here](https://drive.google.com/file/d/1wyDzlwFDYCTluwN-NWTXZ2bfNyR8x3mo/view)
---
*Ontwikkeld voor hobbyisten en educatieve doeleinden.*

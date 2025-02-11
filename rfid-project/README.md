# Cat Door Manager 3000
Mit dieser Applikation kann eine Katzenklappe gesteuert werden, sowie die Eintritte mit RFID Tags gesteuert werden.
Beim Start des ESP wird ein Wlan aufgebaut. Um das WebGUI zu nutzen muss sich mit diesem verbunden werden.
SSID: ESP32-AP
Passwort: 12345678

## Components
RC522 RFID Reader; abobija_rc522
Microservo SG90; espressif_servo
Wifi; esp_wifi
Webserver; esp_http_server

## Code
### Main.c
Konfiguration von Servo und RFID Reader.
Initialisierung der Komponenten und Starten des Webservers.
Logik für die Zugangssteuerung.

### Webserver.c
Konfiguration des Wlan AP und Webserver.
Erstellen der Pages und registrieren der URIs.
Kommunikation zwischen Webserver und Main funktioniert über API Endpoints.

### Tags.h
Enthält ein Struct um einen Tag mit UUID und Namen zu speichern, sowie ein Array für die Valid Tags.
Enthält zusätzlich die nötigen Funktionsdeklarationen und Variablen für die Kommunikation zwischen Webserver.c und Main.c.

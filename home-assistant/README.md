# Home Assistant Projekt
Diese Applikation liest verschiedene Sensoren aus und sendet die Daten mit MQTT an den Home Assistant.  

## Components
ESP32 Development Board  
ICM42688P Beschleunigungssensor  
SHTC3 Temperatur- und Luftfeuchtigkeitssensor  


## Code
### main.c
Initialisierung aller Components und Erstellen der Tasks zum auslesen und senden der Daten.

### wifi.c
Implementation der Wifi Connection. Hier muss noch SSID und Passwort gesetzt werden.

### mqtt.c
Implementation des MQTT.

### sensors/*
Implementation der einzelnen Sensoren.

### Homeassistant
![Homeassistant UI](homeassistant/homeassistant.png)

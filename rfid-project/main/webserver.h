#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "esp_err.h"

// Starte den Webserver.
// Gibt ESP_OK zurück, wenn der Server erfolgreich gestartet wurde.
esp_err_t start_webserver(void);

// Stoppt den Webserver.
void stop_webserver(void);

#endif // WEBSERVER_H

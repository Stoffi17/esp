#ifndef MQTT_H
#define MQTT_H

#include "esp_err.h"

esp_err_t mqtt_app_start(void);
esp_err_t mqtt_publish(const char *topic, const char *payload);

#endif // MQTT_H
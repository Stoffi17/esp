file(GLOB SENSOR_SOURCES "sensors/*.c")
file(GLOB MQTT_SOURCES "mqtt/*.c")

idf_component_register(
    SRCS 
        "main.c"
        ${SENSOR_SOURCES}
        ${MQTT_SOURCES}
    INCLUDE_DIRS 
        "."
        "sensors"
        "mqtt"
)

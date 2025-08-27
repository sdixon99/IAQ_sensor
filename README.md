Code and enclosure for Home Assistant Indoor Air Quality sensor. It uses Renesas HS4001 to measure temperature and humidity and passes those values to a Renesas RR46410 sensor to compute IAQ. The resulting values are sent to Home Assistant via MQTT. Device discovery messages are sent upon each boot of the device and upon any reconnections so that HA can automatically discover the sensor and the configuration.

The target microprocessor for this project is a Seeed Studios Xiao ESP32C3

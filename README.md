# ESP8266PowerMeter

An implementation of ESP8266 nodemcu module for three phase/single phase application.
The project uses a non invasive current sensor (SCT 013 -075) and 220:12 v voltage transformer.

This reads the instataneous current and voltage reading from the sensors. These readings are numerically rendered to
get the corrensponding elecrical load amount in watts and then in kWh for power and energy respectively.

By mqtt, the values of power and energy are published to the raspberry pi.

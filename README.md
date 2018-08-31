# ESP8266 Pool Manager
Provides a complete pool management (T°C, pH, pump...) with an LCD display. Based on ESP8266 chip. More to come
## Project initialization on desktop machine
Execute `bash init_project.bash` to retrieve needed library. It's a simple system to avoid git modules so far. I Will implement Google Repo ASAIC.
## Chip deployement
Use firmware.bin to ensure maximum compatibility (it's a firmware compiled with excellent [NodeMCU build tool](http://nodemcu-build.com)). It embeds following modules:
- bit
- crypto
- dht
- ds18b20
- file
- gpio
- i2c
- net
- node
- tmr 
- uart
- wifi
- tls

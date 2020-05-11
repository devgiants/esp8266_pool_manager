# Arduino Library base folder and example structure
PROJECT_BASE = /home/idaia.group/nbonniot/esp/pool_manager

# Arduino CLI executable name and directory location
ARDUINO_CLI = arduino-cli
ARDUINO_CLI_DIR = .

# Arduino CLI Board type
#BOARD_TYPE = esp8266:esp8266:nodemcuv2
BOARD_TYPE = esp32:esp32:esp32

# Default port to upload to
SERIAL_PORT = /dev/ttyUSB0

#Optional verbose compile/upload trigger
V ?= 0
VERBOSE=

# Build path -- used to store built binary and object files
BUILD_DIR=build
BUILD_PATH=$(PROJECT_BASE)/$(BUILD_DIR)

ifneq ($(V), 0)
    VERBOSE=-v
endif
.PHONY: build deploy clean

install-dependancies:
	$(ARDUINO_CLI) lib install OneWire@2.3.5
	$(ARDUINO_CLI) lib install DallasTemperature@3.8.0
	$(ARDUINO_CLI) lib install ArduinoLog
	#$(ARDUINO_CLI) lib install LiquidCrystal_I2C

build:
	$(ARDUINO_CLI) compile --build-path=$(BUILD_PATH) --build-cache-path=$(BUILD_PATH) -b $(BOARD_TYPE) $(PROJECT_BASE)

deploy: build
	$(ARDUINO_CLI) upload $(VERBOSE) -p $(SERIAL_PORT) --fqbn $(BOARD_TYPE) $(PROJECT_BASE) --verify

debug:
	$(ARDUINO_CLI) debug -b $(BOARD_TYPE) -p $(SERIAL_PORT) $(PROJECT_BASE)

clean:
	@rm -rf $(BUILD_PATH)
	@rm $(PROJECT_BASE)/*.bin
	@rm $(PROJECT_BASE)/*.hex

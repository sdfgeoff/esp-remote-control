BASEDIR=$(CURDIR)

# Flashing parameters
ESPTOOL=esptool
PORT=/dev/ttyUSB0
FLASH_BAUD=460800
FLASH_MODE ?= dio
FLASH_FREQ ?= 40m
BAUD=115200

# Filepaths
IDF_PATH = $(BASEDIR)/lib/esp-idf
MICROPYTHON_PATH = $(BASEDIR)/lib/micropython
PYTHON_C_MODULES_PATH = $(BASEDIR)/lib/python_c_modules

TRANSMITTER_PYTHON_DIR = $(BASEDIR)/esp32_transmitter
RECEIVER_PYTHON_DIR = $(BASEDIR)/esp32_receiver

# Build flags
COMMON_MAKE_FLAGS = IDF_PATH=$(IDF_PATH) USER_C_MODULES=$(PYTHON_C_MODULES_PATH)
TRANSMITTER_MAKE_FLAGS= $(COMMON_MAKE_FLAGS) FROZEN_MPY_DIR=$(TRANSMITTER_PYTHON_DIR)
RECEIVER_MAKE_FLAGS= $(COMMON_MAKE_FLAGS) FROZEN_MPY_DIR=$(RECEIVER_PYTHON_DIR)

# Output files
TRANSMITTER_OUTPUT_FILE = $(BASEDIR)/esp32-transmitter.bin
RECEIVER_OUTPUT_FILE = $(BASEDIR)/esp32-receiver.bin

all: build-transmitter build-receiver

build-transmitter:
	cd $(MICROPYTHON_PATH)/ports/esp32; \
	make $(TRANSMITTER_MAKE_FLAGS);
	mv $(MICROPYTHON_PATH)/ports/esp32/build/firmware.bin $(TRANSMITTER_OUTPUT_FILE)

build-receiver:
	cd $(MICROPYTHON_PATH)/ports/esp32; \
	make $(RECEIVER_MAKE_FLAGS);
	mv $(MICROPYTHON_PATH)/ports/esp32/build/firmware.bin $(RECEIVER_OUTPUT_FILE)

flash-transmitter:
	echo "Uploading transmitter"
	$(ESPTOOL) --chip esp32 --port $(PORT) --baud $(FLASH_BAUD) write_flash -z --flash_mode $(FLASH_MODE) --flash_freq $(FLASH_FREQ) 0x1000 $(TRANSMITTER_OUTPUT_FILE)

flash-receiver:
	echo "Uploading receiver"
	$(ESPTOOL) --chip esp32 --port $(PORT) --baud $(FLASH_BAUD) write_flash -z --flash_mode $(FLASH_MODE) --flash_freq $(FLASH_FREQ) 0x1000 $(RECEIVER_OUTPUT_FILE)


clean:
	cd $(MICROPYTHON_PATH)/ports/esp32;\
	make clean $(COMMON_MAKE_FLAGS)


erase:
	echo "Erasing flash"
	$(ESPTOOL) --chip esp32 --port $(PORT) --baud $(BAUD) erase_flash

monitor:
	miniterm.py $(PORT) $(BAUD)


transmitter: build-transmitter erase flash-transmitter

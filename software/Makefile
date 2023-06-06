CFLAGS += -W -Wall --std=gnu11 -O3
CFLAGS += -fno-diagnostics-show-caret
CFLAGS += -D_GNU_SOURCE
CFLAGS += `pkg-config --cflags libusb-1.0`

SRCS = \
  usb.c \
  fpga.c \
  fx2lp.c \
  capture.c \
  os_common.c \
  usb_sniffer.c \

HDRS = \
  usb.h \
  fpga.h \
  fx2lp.h \
  capture.h \
  os_common.h \
  usb_sniffer.h \

UNAME := $(shell uname)

ifeq ($(UNAME), Linux)
  BIN = usb_sniffer
  CFLAGS += -DOS_LINUX
  LDFLAGS += -lm -lrt
  LDFLAGS += `pkg-config --libs libusb-1.0`
  EXTCAP_PATH = ~/.config/wireshark/extcap
else
  BIN = usb_sniffer.exe
  CFLAGS += -DOS_WINDOWS
  LDFLAGS += -Wl,--subsystem,console
  LIB_PATH = `pkg-config --variable=libdir libusb-1.0`
  LDFLAGS += $(LIB_PATH)/libusb-1.0.a
  LDFLAGS += $(LIB_PATH)/libwinpthread.a
  EXTCAP_PATH = $(APPDATA)/Wireshark/extcap/
endif

all: $(BIN)

$(BIN): $(SRCS) $(HDRS)
	gcc $(CFLAGS) $(SRCS) -o $(BIN) $(LDFLAGS)

clean:
	rm -fr $(BIN)

install: $(BIN)
	cp $(BIN) $(EXTCAP_PATH)

prog_eeprom:
	./usb_sniffer --mcu-eeprom ../firmware/usb_sniffer.bin

prog_sram:
	./usb_sniffer --fpga-sram ../fpga/impl/usb_sniffer_impl.bit

prog_flash:
	./usb_sniffer --fpga-flash ../fpga/impl/usb_sniffer_impl.jed



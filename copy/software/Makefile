CFLAGS += -W -Wall --std=gnu11 -O3
CFLAGS += -D_GNU_SOURCE

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
  CFLAGS += `pkg-config --cflags libusb-1.0`
  CFLAGS += -fno-diagnostics-show-caret
  LDFLAGS += -lm -lrt
  LDFLAGS += `pkg-config --libs libusb-1.0`
	MKDIR_CMD = mkdir -p
  EXTCAP_PATH = ~/.local/lib/wireshark/extcap
else ifeq ($(UNAME), Darwin)
  BREW_PKGCONFIG := $(shell brew --cellar pkg-config)/$(shell brew list --versions pkg-config | tr ' ' '\n' | tail -1)/bin/pkg-config
  BIN = usb_sniffer
  CFLAGS += -DOS_LINUX
  CFLAGS += `$(BREW_PKGCONFIG) --cflags libusb-1.0`
  LDFLAGS += `$(BREW_PKGCONFIG) --libs libusb-1.0`
	MKDIR_CMD = mkdir -p
  EXTCAP_PATH = ~/.local/lib/wireshark/extcap
else
  BIN = usb_sniffer.exe
  CFLAGS += -DOS_WINDOWS
  CFLAGS += `pkg-config --cflags libusb-1.0`
  LDFLAGS += -Wl,--subsystem,console
  LIB_PATH = `pkg-config --variable=libdir libusb-1.0`
  LDFLAGS += $(LIB_PATH)/libusb-1.0.a
  LDFLAGS += $(LIB_PATH)/libwinpthread.a
	MKDIR_CMD = md
  EXTCAP_PATH = $(APPDATA)/Wireshark/extcap/
endif

all: $(BIN)

$(BIN): $(SRCS) $(HDRS)
	gcc $(CFLAGS) $(SRCS) -o $(BIN) $(LDFLAGS)

clean:
	rm -fr $(BIN)

install: $(BIN)
	$(MKDIR_CMD) $(EXTCAP_PATH)
	cp $(BIN) $(EXTCAP_PATH)

prog_eeprom:
	./usb_sniffer --mcu-eeprom ../firmware/usb_sniffer.bin

prog_sram:
	./usb_sniffer --fpga-sram ../fpga/impl/usb_sniffer_impl.bit

prog_flash:
	./usb_sniffer --fpga-flash ../fpga/impl/usb_sniffer_impl.jed

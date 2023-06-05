CFLAGS  += -mmcs51 --std-c99 --opt-code-speed --no-xinit-opt --model-large
LDFLAGS += --code-loc 0x0000 --code-size 0x1800
LDFLAGS += --xram-loc 0x1800 --xram-size 0x0800

SRCS = \
  usb_sniffer.c \
  usb_sniffer.h \
  usb.h \
  usb.c \
  usb_descriptors.h \
  fx2_regs.h \

all: usb_sniffer.ihx usb_sniffer.bin

usb_sniffer.ihx: $(SRCS)
	sdcc $(CFLAGS) $(LDFLAGS) usb_sniffer.c

usb_sniffer.bin: usb_sniffer.ihx
	sdobjcopy -O binary -I ihex usb_sniffer.ihx usb_sniffer.bin

clean:
	rm -f usb_sniffer.asm  usb_sniffer.bin  usb_sniffer.ihx  usb_sniffer.lk  usb_sniffer.lst  usb_sniffer.map  usb_sniffer.mem  usb_sniffer.rel  usb_sniffer.rst  usb_sniffer.sym



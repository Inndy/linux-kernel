#
# Makefile for the common code of Broadcom 97xxx boards
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
# Note 2! The CFLAGS definitions are now in the main makefile...
#

.S.s:
	$(CPP) $(CFLAGS) $< -o $*.s
.S.o:
	$(CC) $(CFLAGS) -c $< -o $*.o

O_TARGET	:= brcmstb.a

export-objs	:= brcmserial.o

obj-y = brcmserial.o

obj-$(CONFIG_USB_OHCI_BRCM)	+= pciutil.o

include $(TOPDIR)/Rules.make

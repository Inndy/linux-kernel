#
# Makefile for BCM93730 board
#
# Copyright (C) 2001-2005 Broadcom Corporation
#
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#
# Makefile for BCM973560 board
#
# when     who what
# -------- --- ------------------------------------------------------
# 04/20/05 THT Original codings
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
# Note 2! The CFLAGS definitions are now in the main makefile...
#

.S.s:
	$(CPP) $(AFLAGS) $< -o $@
.S.o:
	$(CC) $(AFLAGS) -c $< -o $@

O_TARGET	:= bcm93730.a

export-objs	:= led.o board.o

obj-y = setup.o prom.o int-handler.o irq.o led.o brcmpci.o uart_put.o board.o

obj-$(CONFIG_PC_KEYB)		+= kbd.o
obj-$(CONFIG_REMOTE_DEBUG)	+= dbg_io.o

include $(TOPDIR)/Rules.make

BASE = ../../../
FCMAKE = C:/Program Files/FITkit/bin/fcmake.exe
FKFLASH = C:/Program Files/FITkit/bin/fkflash.exe
FKTERM = C:/Program Files/FITkit/bin/fkterm.exe
FKTEST = C:/Program Files/FITkit/bin/fktest.exe
PROJECT = project.xml
OUTPUTPREFIX = morseo
DEPENDENCIES = ../../demo/keyboard_lcd

all: dependencycheck build/morseo_f1xx.hex build/morseo_f2xx.hex

#MCU part
#=====================================================================
HEXFILE_F1XX = build/morseo_f1xx.hex
HEXFILE_F2XX = build/morseo_f2xx.hex

build/mcu/display_f1xx.o: ../../../mcu/libs/lcd/display.c
	$(comp_tpl_f1xx)

build/mcu/display_f2xx.o: ../../../mcu/libs/lcd/display.c
	$(comp_tpl_f2xx)

build/mcu/keyboard_f1xx.o: ../../../mcu/libs/keyboard/keyboard.c
	$(comp_tpl_f1xx)

build/mcu/keyboard_f2xx.o: ../../../mcu/libs/keyboard/keyboard.c
	$(comp_tpl_f2xx)

build/mcu/main_f1xx.o: mcu/main.c
	$(comp_tpl_f1xx)

build/mcu/main_f2xx.o: mcu/main.c
	$(comp_tpl_f2xx)

OBJFILES_F1XX = build/mcu/display_f1xx.o build/mcu/keyboard_f1xx.o build/mcu/main_f1xx.o
OBJFILES_F2XX = build/mcu/display_f2xx.o build/mcu/keyboard_f2xx.o build/mcu/main_f2xx.o

#FPGA part
#=====================================================================
BINFILE = ../../demo/keyboard_lcd/build/kblcd.bin

PKGS_LIST = ../../../mcu/libs/lcd/package.xml
PKGS_LIST += ../../../mcu/libs/keyboard/package.xml

include $(BASE)/base/Makefile.inc

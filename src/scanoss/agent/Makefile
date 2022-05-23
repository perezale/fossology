######################################################################
# Copyright (C) 2018-2022 SCANOSS.COM
# 
#  This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
######################################################################

LANG = C
TOP = ../../..
VARS = $(TOP)/Makefile.conf
DEPS = $(TOP)/Makefile.deps
include $(VARS)

CFLAGS_LOCAL = $(FO_CFLAGS) 
LDFLAGS_LOCAL = $(FO_LDFLAGS) -lpq 
LDFLAGS+=-lpthread  -lcurl -lm -lgcrypt
CCFLAGS+= -fPIC -g -Wall  -std=gnu99 -I./inc -I./external/inc

SOURCES_SCANNER=$(wildcard src/*.c) $(wildcard src/**/*.c)  $(wildcard external/*.c) $(wildcard external/**/*.c)
OBJECTS_SCANNER=$(SOURCES_SCANNER:.c=.o)

TARGET_SCANNER=scanner

EXE = scanoss
HDRS = snippet_scan.h
BIN_NAME = scanner

all: $(EXE) $(TARGET_SCANNER)

$(EXE): main.c $(FOLIB) snippet_scan.o $(VARS) $(HDRS)
	$(CC) main.c snippet_scan.o $(LDFLAGS_LOCAL) $(CFLAGS_LOCAL) $(DEFS) -o $@


$(TARGET_SCANNER): $(OBJECTS_SCANNER)
	$(CC) -g -o $(BIN_NAME) $^ $(LDFLAGS)


snippet_scan.o: snippet_scan.c snippet_scan.h
	$(CC) -c $< $(CFLAGS_LOCAL)

snippet_scan_cov.o: snippet_scan.c snippet_scan.h
	$(CC) -c $< $(FLAG_COV) $(CFLAGS_LOCAL) -o $@

%.o: %.c
	$(CC) $(CCFLAGS) -o $@ -c $<

install: all
	$(INSTALL_PROGRAM) $(EXE) $(DESTDIR)$(MODDIR)/$(EXE)/agent/$(EXE)
	cp $(BIN_NAME) /usr/bin
uninstall:
	rm -rf $(DESTDIR)$(MODDIR)/$(EXE)/agent

clean:
	rm -f $(EXE) *.o core *.gc*

.PHONY: all install uninstall clean

include $(DEPS)

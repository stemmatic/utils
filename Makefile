
APPS=uncol stats chomp pvar

LDLIBS += -lm

BIN=$(HOME)/bin

APPEXES=$(APPS:%=$(BIN)/%)

all:	$(APPEXES)

$(APPEXES): $(BIN)/%: %
	cp $< $(BIN)/


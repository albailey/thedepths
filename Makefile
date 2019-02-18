GAME=the_depths
CC=cc65
CFLAGS=-Oi

# Add an environment variable with the path to your NES emulator called: 
# NES_EMU
# Add an environment variable with the path to CC65
# CC65_HOME
# You may need to set CC65_HOME/bin to your PATH

.PHONY: all clean run

objfiles= crt0.o utils.o $(GAME).o
SOURCES = $(GAME).c

all: $(GAME).nes

clean:
	@rm -fv $(objfiles)
	rm -f $(GAME).s
	rm -f $(GAME).nes

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

$(GAME).nes: $(GAME).c
	$(CC) $(CFLAGS) $(GAME).c --add-source
	ca65 crt0.s
	ca65 utils.s
	ca65 $(GAME).s
	ld65 -o $(GAME).nes -C nes.cfg $(objfiles) nes.lib

run: $(GAME).nes
	$(NES_EMU) $(GAME).nes

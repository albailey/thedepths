set GAME_TITLE=the_depths


del %GAME_TITLE%.nes
del *.o
del %GAME_TITLE%.s

# nes.lib is located in cc65/lib
# findable through env variables:
# LD65_LIB
# or: CC65_HOME/lib

cc65 -Oi %GAME_TITLE%.c --add-source
ca65 crt0.s
ca65 utils.s
ca65 %GAME_TITLE%.s
ld65 -C nes.cfg -o %GAME_TITLE%.nes crt0.o utils.o %GAME_TITLE%.o nes.lib
pause

%GAME_TITLE%.nes

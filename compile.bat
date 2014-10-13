set GAME_TITLE=the_depths


del %GAME_TITLE%.nes
del *.o
del %GAME_TITLE%.s

cc65 -Oi %GAME_TITLE%.c --add-source
ca65 crt0.s
ca65 utils.s
ca65 %GAME_TITLE%.s
ld65 -C nes.cfg -o %GAME_TITLE%.nes crt0.o utils.o %GAME_TITLE%.o runtime.lib
pause

%GAME_TITLE%.nes

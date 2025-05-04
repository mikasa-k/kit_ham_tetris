@echo off
windres -i resource.rc -o resource.o
gcc -o "kit_tetris" main.c resource.o -mwindows -lgdi32 -lmsimg32 -lwinmm -finput-charset=UTF-8 -Wall -static -static-libgcc -static-libstdc++ -lstdc++ -lgcc -lwinpthread -ffunction-sections -fdata-sections -Wl,--gc-sections

CC = g++
CFLAGS = -c
LFLAGS = -static-libgcc -static-libstdc++ -Wl,--subsystem,console

clean:
	del *.o NGChat.exe

all: NGChat.exe

NGChat.exe: main.o icon.o
	$(CC) main.o icon.o -o NGChat.exe $(LFLAGS)

main.o: main.cpp
	$(CC) $(CFLAGS) main.cpp main.o

icon.o: icon.rc
	windres icon.rc -O coff -o icon.o
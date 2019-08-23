CC = g++ -Wno-deprecated -o lanft

main:
	$(CC) main.cpp misc.cpp misc.h TX.cpp TX.h RX.cpp RX.h

main.o: main.cpp TX.h misc.h RX.h
misc.o: misc.cpp misc.h
misc.o: misc.h
TX.o: TX.cpp TX.h misc.h
TX.o: TX.h misc.h
RX.o: RX.cpp RX.h misc.h
RX.o: RX.h misc.h

clean:
	rm *.h.gch

install:
	sudo cp -v lanft /usr/local/bin/

uninstall:
	sudo rm -i -v /usr/local/bin/lanft



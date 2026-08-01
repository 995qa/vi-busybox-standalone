vi:
	gcc -O2 -Wall -I. -o vi vi.c main.c
clean:
	rm vi
install:
	cp vi /usr/local/bin
uninstall:
	rm /usr/local/bin/vi

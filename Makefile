vi:
	gcc -O2 -Wall -I. -o vi vi.c main.c
clean:
	rm -f vi
install:
	cp vi /usr/local/bin
uninstall:
	rm -f /usr/local/bin/vi

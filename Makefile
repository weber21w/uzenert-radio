CC=gcc

uzenet-radio: uzenet-radio.o -pthread
	$(CC) -o uzenet-radio uzenet-radio.o

uzenet-radio.o : uzenet-radio.c
	$(CC) -c uzenet-radio.c

clean :
	rm -f uzenet-radio uzenet-radio.o

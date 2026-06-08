outbound-filter: main.o parser.o
	gcc -o outbound-filter main.o parser.o
main.o: main.c
	gcc -c main.c -o main.o
parser.o: src/parser/parser.c src/parser/parser.h
	gcc -c src/parser/parser.c -o parser.o
clean:
	rm -f *.o outbound-filter

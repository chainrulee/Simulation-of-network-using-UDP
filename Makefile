main: main.o dijkstra.o
	gcc -o main main.o dijkstra.o
main.o: main.c dijkstra.h
	gcc -c main.c
mytool1.o: dijkstra.c dijkstra.h
	gcc -c dijkstra.c

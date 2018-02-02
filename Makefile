main: main.o control switch
	gcc -o main main.c
control: control.o dijkstra.o
	gcc -o control control.o dijkstra.o
control.o: control.c dijkstra.h
	gcc -c control.c
dijkstra.o: dijkstra.c dijkstra.h
	gcc -c dijkstra.c
switch: switch.o
	gcc -l pthread -o switch switch.c
.PHONY: clean
clean:
	rm *.o main control switch

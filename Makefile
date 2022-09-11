default:
	gcc -O3 x11.c -lX11 -lm
	./a.out

debug:
	gcc -pg x11.c -lX11 -lm
	./a.out
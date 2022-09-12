default:
	gcc -O3 src/x11.c src/lin_alg.c src/camera.c -lX11 -lm
	./a.out

debug:
	gcc -pg x11.c -lX11 -lm
	./a.out
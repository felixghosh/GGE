default:
	gcc -O3 src/x11.c src/lin_alg.c src/camera.c -lX11 -lm -lpthread
	./a.out

debug:
	gcc -g src/x11.c src/lin_alg.c src/camera.c -lX11 -lm -lpthread
	./a.out
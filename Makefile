default:
	gcc -O3 src/x11.c src/lin_alg.c src/camera.c -lX11 -lm -lpthread -lXfixes
	./a.out

debug:
	gcc -g src/x11.c src/lin_alg.c src/camera.c -lX11 -lm -lpthread -lXfixes
	./a.out

sdl:
	gcc -g src/sdl2.c src/lin_alg.c src/camera.c -lm -lpthread -lSDL2
	./a.out
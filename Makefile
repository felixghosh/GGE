default:
	gcc -O3 src/sdl2.c src/lin_alg.c src/camera.c -lm -lpthread -lSDL2 -lSDL2_ttf
	./a.out

debug:
	gcc -g src/sdl2.c src/lin_alg.c src/camera.c -lm -lpthread -lSDL2 -lSDL2_ttf
	./a.out

prof:
	gcc -pg src/sdl2.c src/lin_alg.c src/camera.c -lm -lpthread -lSDL2 -lSDL2_ttf
	./a.out

x11:
	gcc -O3 src/x11.c src/lin_alg.c src/camera.c -lX11 -lm -lpthread -lXfixes
	./a.out

x11_debug:
	gcc -g src/x11.c src/lin_alg.c src/camera.c -lX11 -lm -lpthread -lXfixes
	./a.out

default:
	gcc -O3 src/main.c src/media.c src/engine.c src/lin_alg.c src/camera.c -lm -lpthread -lSDL2 -lSDL2_ttf -lSDL2_mixer
	./a.out

fullscreen:
	gcc -O3 src/main.c src/media.c src/engine.c src/lin_alg.c src/camera.c -lm -lpthread -lSDL2 -lSDL2_ttf -lSDL2_mixer
	./a.out -f

debug:
	gcc -g src/main.c src/media.c src/engine.c src/lin_alg.c src/camera.c -lm -lpthread -lSDL2 -lSDL2_ttf -lSDL2_mixer
	./a.out

prof:
	gcc -pg src/main.c src/media.c src/engine.c src/lin_alg.c src/camera.c -lm -lpthread -lSDL2 -lSDL2_ttf -lSDL2_mixer
	./a.out

x11:
	gcc -O3 src/x11.c src/lin_alg.c src/camera.c -lX11 -lm -lpthread -lXfixes
	./a.out

x11_debug:
	gcc -g src/x11.c src/lin_alg.c src/camera.c -lX11 -lm -lpthread -lXfixes
	./a.out

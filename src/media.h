#include <SDL2/SDL_mixer.h>
#include <stdbool.h>

extern Mix_Chunk* gun_sound;
extern Mix_Chunk* hit_sound;
extern Mix_Music* music;

bool load_media();

void free_media();
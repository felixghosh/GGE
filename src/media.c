#include "media.h"
#include <stdbool.h>

Mix_Chunk* gun_sound = NULL;
Mix_Chunk* hit_sound = NULL;
Mix_Music* music = NULL;

bool load_media(){
    bool success = true;


    //Load sound effects
    gun_sound = Mix_LoadWAV( "sounds/gun2.mp3" );
    if( gun_sound == NULL )
    {
        printf( "Failed to load gun sound effect! SDL_mixer Error: %s\n", Mix_GetError() );
        success = false;
    }

    hit_sound = Mix_LoadWAV( "sounds/hit.mp3" );
    if( hit_sound == NULL )
    {
        printf( "Failed to load hut sound effect! SDL_mixer Error: %s\n", Mix_GetError() );
        success = false;
    }

    //Load music
    music = Mix_LoadMUS("sounds/music.mp3");
    if(music == NULL){
        printf( "Failed to load music! SDL_mixer Error: %s\n", Mix_GetError() );
        success = false;
    }


    return success;
}

void free_media()
{
    //Free the sound effects
    Mix_FreeChunk(gun_sound);
    Mix_FreeChunk(hit_sound);
    
    //Free the music
    Mix_FreeMusic(music);

    //Quit SDL_mixer
    Mix_Quit();
}
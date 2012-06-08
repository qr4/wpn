#ifndef LAYERRENDERERS_H
#define LAYERRENDERERS_H

#include <SDL/SDL.h>
#include "../ClientLib/types.h"
#include "sdl.h"

void draw_influence(SDL_Surface *buffer);
void draw_asteroids(SDL_Surface *buffer);
void draw_planets(SDL_Surface *buffer);
void draw_bases(SDL_Surface *buffer);
void draw_ships(SDL_Surface *buffer);
void draw_shots(SDL_Surface *buffer);
void draw_explosions(SDL_Surface *buffer);
void zoom_textures();

#endif  /*LAYERRENDERERS_H*/

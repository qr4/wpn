#ifndef _MY_SDL_H
#define _MY_SDL_H

#include <stdio.h>
#include <SDL/SDL.h>
#include <SDL_image.h>
#include <SDL/SDL_gfxPrimitives.h>
#include <SDL/SDL_rotozoom.h>


#include "types.h"

void SDLinit();
void SDLplot();
void checkSDLevent();
void drawAsteroid(asteroid_t* a);
void drawBase(base_t * b);
void drawExplosion(explosion_t* e);
void drawPlanet(planet_t* p);
void drawShip(ship_t * s);
void drawShot(shot_t * s);
void drawSlot(float x, float y, char type);

#endif

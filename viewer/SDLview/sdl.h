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
double player_to_h(int playerid);
Uint8 red_from_H(double h);
Uint8 green_from_H(double h);
Uint8 blue_from_H(double h);
void checkSDLevent();
void drawAsteroid(asteroid_t* a);
void drawBase(base_t * b);
void drawExplosion(explosion_t* e);
void drawPlanet(planet_t* p);
void drawShip(ship_t * s);
void drawShot(shot_t * s);
void drawSlot(float x, float y, char type);
void drawText(int x,int y, char* text);
void find_object_at(int click_x, int click_y);


#endif

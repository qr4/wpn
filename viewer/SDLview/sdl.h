#ifndef _MY_SDL_H
#define _MY_SDL_H

#include <stdio.h>
#include <SDL/SDL.h>
#include <SDL_image.h>
#include <SDL/SDL_gfxPrimitives.h>
#include <SDL/SDL_rotozoom.h>


#include "types.h"
extern SDL_Surface* asteroid_image;
extern SDL_Surface* base_small_image;
extern SDL_Surface* base_medium_image;
extern SDL_Surface* base_large_image;
extern SDL_Surface* base_huge_image;
extern SDL_Surface* explosion_image;
extern SDL_Surface* planet_image;
extern SDL_Surface* ship_small_image;
extern SDL_Surface* ship_medium_image;
extern SDL_Surface* ship_large_image;
extern SDL_Surface* ship_huge_image;
extern SDL_Surface* shot_image;
extern SDL_Surface* slot_empty_image;
extern SDL_Surface* slot_L_image;
extern SDL_Surface* slot_R_image;
extern SDL_Surface* slot_T_image;

void SDLinit();
void SDLplot();
double player_to_h(int playerid);
Uint8 red_from_H(double h);
Uint8 green_from_H(double h);
Uint8 blue_from_H(double h);
void checkSDLevent();
void drawText(SDL_Surface *buffer, int x,int y, char* text);
void find_object_at(int click_x, int click_y);


#endif

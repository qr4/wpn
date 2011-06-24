
// This code is written by Jonas Thiem (www.homeofjones.de) and shall be
// freely used in all projects that need it (public domain).
// It would be nice if this notice remains but it is not required.

#include <string.h>
#include <stdlib.h>

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

#include "fontrender.h"

static SDL_Surface* font = NULL;
void fontrender(int x, int y, const char* str, SDL_Surface* target) {
	//renders a string. use \n for line breaks (not \r\n)
	float charwidth = 7;
	float charheight = 14;
	unsigned int charrowlength = 32;
	//check for font surface
	if (!font) {
		//adjust font path here!
		font = IMG_Load("font.png");
		if (!font) {
			return;
		}
	}
	int r = 0;
	unsigned int len = strlen(str);
	float outputx = 0;
	//loop through all chars and render them
	while (r < len) {
		if (str[r] == '\n') {
			//line break
			outputx = 0;
			y += charheight;
			r++;
			continue;
		}
		int charvalue;
		charvalue = str[r]-' ';
		//limit to the ascii range in our font
		if (charvalue > charrowlength*3) {charvalue = '?'-' ';}
		//output a char if we got one
		if (charvalue >= 0) {
			float charx = charvalue; //char id we want
			float chary = 0;
			//shift down in rows as required
			while (charx >= charrowlength) {charx -= charrowlength; chary++;}
			//blit rectangles
			SDL_Rect sourcerect;
			memset(&sourcerect,0,sizeof(sourcerect));
			sourcerect.x = (int)(charx * charwidth);
			sourcerect.y = (int)(chary * charheight);
			sourcerect.w = (int)(charwidth);
			sourcerect.h = (int)(charheight);
			SDL_Rect destrect;
			destrect.x = x + outputx;
			destrect.y = y;
			//now blit!
			SDL_BlitSurface(font, &sourcerect, target, &destrect);
		}
		outputx += charwidth;
		r++;
	}
}
int fontdrawwidth(unsigned int font, const char* str) {
	//just a cheap hack that doesn't care about \n in the string :)
	return (int)(7.0 * strlen(str));
}


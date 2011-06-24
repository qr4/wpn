
// This code is written by Jonas Thiem (www.homeofjones.de) and shall be
// freely used in all projects that need it (public domain).
// It would be nice if this notice remains but it is not required.

void fontrender(int x, int y, const char* str, SDL_Surface* target); //draw a string to a target surface
int fontdrawwidth(unsigned int font, const char* str); //pixel width required to draw a string, does NOT honor \n line breaks correctly!


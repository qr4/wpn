#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "globals.h"
#include "screen.h"

void json_output(pixel_t* planets, int n_planets) {
	int i;

	printf("{ \"world\":\n");
	printf("  { \"bounding-box\": { \"xmin\": 0, \"xmax\": %d, \"ymin\": 0, \"ymax\": %d},\n", GLOBALS.WIDTH, GLOBALS.HEIGHT);
	printf("    \"planets\": [\n");
	for(i = 0; i < n_planets; i++) {
		printf("      {\"id\":%d, \"x\": %f, \"y\": %f, \"owner\": null},\n", i, planets[i].x, planets[i].y);
	}
	printf("    ],\n");
	printf("    \"ships\": [ ],\n");
	printf("    \"asteroids\": [ ],\n");
	printf("    \"players\": [\n");
	printf("      {\"id\": 1, \"name\": \"SDL\"},\n");
	printf("      {\"id\": 2, \"name\": \"Test\"},\n");
	printf("    ],\n");
	printf("    \"shots\": [ ],\n");
	printf("    \"hits\": [ ],\n");
	printf("    \"explosions\": [ ],\n");
	printf("  }\n");
	printf("}\n");
}

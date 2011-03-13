#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "globals.h"
#include "json_output.h"

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

void json_update(waypoint_t* route1, waypoint_t* route2, int n_planets) {
	waypoint_t* t1 = route1;
	waypoint_t* t2 = route2;

	while(t1 || t2) {
		printf("{ \"update\":\n");
		printf("  { \"bounding-box\": { \"xmin\": 0, \"xmax\": %d, \"ymin\": 0, \"ymax\": %d},\n", GLOBALS.WIDTH, GLOBALS.HEIGHT);
		printf("    \"ships\": [ \n");
		if(t1) {
			printf("      {\"id\": %d, \"x\": %f, \"y\": %f, \"owner\": 1, \"size\": 3, \"contents\": \"T  \", \"docked_to\": null},\n", n_planets+1, t1->point.x, t1->point.y);
			t1 = t1->next;
		}
		if(t2) {
			printf("      {\"id\": %d, \"x\": %f, \"y\": %f, \"owner\": 2, \"size\": 6, \"contents\": \"TTLRR \", \"docked_to\": null},\n", n_planets+2, t2->point.x, t2->point.y);
			t2 = t2->next;
		}
		printf("    \"], \n");
		printf("  }\n");
		printf("}\n");
	}
}

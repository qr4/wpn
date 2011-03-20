#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "globals.h"
#include "json_output.h"

void json_output(cluster_t* planets, int n_planets) {
	int i, j;

	float asteroidradius = 0;
	float planetradius = 0;

	for(i = 0; i < n_planets-1; i++) {
		if(planets[i].safety_radius > 0) {
			if(planets[i].safety_radius > asteroidradius) {
				if(planets[i].safety_radius >= planetradius) {
					planetradius = planets[i].safety_radius;
				} else {
					asteroidradius = planets[i].safety_radius;
				}
			} else {
				asteroidradius = planets[i].safety_radius;
			}
		}
	}
	fprintf(stderr, "asteroids are %f and planets are %f\n", asteroidradius, planetradius);

	int asteroidseen = 0;
	int asteroidmax = 0;
	for(i = 0; i < n_planets-1; i++) {
		if(planets[i].safety_radius == asteroidradius) {
			asteroidmax++;
		}
	}

	int planetseen = 0;
	int planetmax = 0;
	for(i = 0; i < n_planets-1; i++) {
		if(planets[i].safety_radius == planetradius) {
			planetmax++;
		}
	}

	printf("{ \"world\":\n");
	printf("  { \"bounding-box\": { \"xmin\": 0, \"xmax\": %d, \"ymin\": 0, \"ymax\": %d},\n", GLOBALS.WIDTH, GLOBALS.HEIGHT);
	printf("    \"planets\": [\n");
	for(i = 0; i < n_planets-1; i++) {
		if(planets[i].safety_radius >= planetradius) {
			printf("      {\"id\":%d, \"x\": %f, \"y\": %f, \"owner\": null}%s\n", i+1, planets[i].center.x, planets[i].center.y, planetseen < planetmax-1 ? "," : "");
			planetseen++;
		}
	}
	printf("    ],\n");
	printf("    \"asteroids\": [\n");
	for(i = 0; i < n_planets-1; i++) {
		if(planets[i].safety_radius >= asteroidradius && planets[i].safety_radius < planetradius) {
			char contents[8] = "       ";
			for(j = 0; j < i%7; j++) {
				contents[j] = 'R';
			}
			printf("      {\"id\":%d, \"x\": %f, \"y\": %f, \"contents\": %s}%s\n", i+1, planets[i].center.x, planets[i].center.y, contents, asteroidseen < asteroidmax-1 ? "," : "");
			asteroidseen++;
		}
	}
	printf("    ],\n");
	printf("    \"ships\": [ ],\n");
	printf("    \"players\": [\n");
	printf("      {\"id\": 1, \"name\": \"SDL\"},\n");
	printf("      {\"id\": 2, \"name\": \"Test\"}\n");
	printf("    ]\n");
	printf("  }\n");
	printf("}\n\n");
}

void json_update(waypoint_t* route1, waypoint_t* route2, int n_planets) {
	waypoint_t* t1 = route1;
	waypoint_t* t2 = route2;

	while(t1 || t2) {
		printf("{ \"update\":\n");
		printf("  { \"bounding-box\": { \"xmin\": 0, \"xmax\": %d, \"ymin\": 0, \"ymax\": %d},\n", GLOBALS.WIDTH, GLOBALS.HEIGHT);
		printf("    \"ships\": [ \n");
		if(t1) {
			printf("      {\"id\": %d, \"x\": %f, \"y\": %f, \"owner\": 1, \"size\": 3, \"contents\": \"T  \", \"docked_to\": null}%s\n", n_planets+1, t1->point.x, t1->point.y, t2 ? ",": "");
		}
		if(t2) {
			printf("      {\"id\": %d, \"x\": %f, \"y\": %f, \"owner\": 2, \"size\": 6, \"contents\": \"TTLRR \", \"docked_to\": null}\n", n_planets+2, t2->point.x, t2->point.y);
		}
		if((t1 && ! t1->next) || (t2 && ! t2->next)) {
			printf("    ], \n");
			printf("    \"explosions\": [\n");
			if(t1 && ! t1->next) {
				printf("      {\"id\": %d, \"x\": %f, \"y\": %f}%s\n", n_planets+1, t1->point.x, t1->point.y, (t2 && !t2->next) ? ",": "");
			}
			if(t2 && ! t2->next) {
				printf("      {\"id\": %d, \"x\": %f, \"y\": %f}\n", n_planets+2, t2->point.x, t2->point.y);
			}
		}
		printf("    ],\n");
		if(t1) {
			t1 = t1->next;
		}
		if(t2) {
			t2 = t2->next;
		}
		printf("    \"shots\": [ ],\n");
		printf("    \"hits\": [ ]\n");
		printf("  }\n");
		printf("}\n\n");
	}
}

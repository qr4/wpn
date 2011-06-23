#include "json.h"

extern bbox_t boundingbox;

extern asteroid_t* asteroids;
extern int n_asteroids;
extern int n_asteroids_max;

extern explosion_t* explosions;
extern int n_explosions;
extern int n_explosions_max;

extern planet_t* planets;
extern int n_planets;
extern int n_planets_max;

extern ship_t* ships;
extern int n_ships;
extern int n_ships_max;

extern shot_t* shots;
extern int n_shots;
extern int n_shots_max;

extern float zoom;
extern float offset_x;
extern float offset_y;

int parseJson(buffer_t* b) {
	json_error_t error;
	json_t* root = json_loads(b->data, 0, &error);
	if(!root) {
		fprintf(stderr, "Error %s in line %d, column %d\n", error.text, error.line, error.column);
		exit(1);
	} else {
		//fprintf(stderr, "Found valid json\n");
		json_t* world = json_object_get(root, "world");
		if(world) {
			//fprintf(stderr, "Turns out it is a world object\n");
			jsonWorld(world);
		}
		json_t* update = json_object_get(root, "update");
		if(update) {
			//fprintf(stderr, "Turns out it is an update object\n");
			jsonUpdate(update);
		}
	}
	return 0;
}

void jsonWorld(json_t* world) {
	json_t* j_asteroids = json_object_get(world, "asteroids");
	if(j_asteroids) {
		jsonAsteroids(j_asteroids, OVERWRITE);
	}
	json_t* j_bbox = json_object_get(world, "bounding-box");
	if(j_bbox) {
		jsonBbox(j_bbox);
	}
	json_t* j_planets = json_object_get(world, "planets");
	if(j_planets) {
		jsonPlanets(j_planets, OVERWRITE);
	}
	json_t* j_ships = json_object_get(world, "ships");
	if(j_ships) {
		jsonShips(j_ships, OVERWRITE);
	}
}

void jsonUpdate(json_t* update) {
	json_t* j_asteroids = json_object_get(update, "asteroids");
	if(j_asteroids) {
		jsonAsteroids(j_asteroids, UPDATE);
	}
	json_t* j_bbox = json_object_get(update, "bounding-box");
	if(j_bbox) {
		jsonBbox(j_bbox);
	}
	json_t* j_planets = json_object_get(update, "planets");
	if(j_planets) {
		jsonPlanets(j_planets, UPDATE);
	}
	json_t* j_ships = json_object_get(update, "ships");
	if(j_ships) {
		jsonShips(j_ships, UPDATE);
	}
	n_shots = 0;
	json_t* j_shots = json_object_get(update, "shots");
	if(j_shots) {
		jsonShots(j_shots);
	}
	// Explosions nach ships, damit diese korrekt zerstört werden
	json_t* j_explosions = json_object_get(update, "explosions");
	if(j_explosions) {
		jsonExplosions(j_explosions);
	}
}

void jsonExplosions(json_t* e) {
	int i;

	if(e && json_is_array(e)) {
		json_t* j_explosion;
		for(i = 0; i < json_array_size(e); i++) {
			j_explosion = json_array_get(e, i);
			if(json_is_object(j_explosion)) {
				jsonExplosion(j_explosion);
			}
		}
	}
}

void jsonShips(json_t* s, int updatemode) {
	int i;

	if(updatemode == OVERWRITE) {
		for(i = 0; i < n_ships; i++) {
			ships[i].active = 0;
		}
	}

	if(s && json_is_array(s)) {
		json_t* j_ship;
		for(i = 0; i < json_array_size(s); i++) {
			j_ship = json_array_get(s, i);
			if(json_is_object(j_ship)) {
				jsonShip(j_ship);
			}
		}
	}
}

void jsonShots(json_t* s) {
	int i;

	if(s && json_is_array(s)) {
		json_t* j_shot;
		for(i = 0; i < json_array_size(s); i++) {
			j_shot = json_array_get(s, i);
			if(json_is_object(j_shot)) {
				jsonShot(j_shot);
			}
		}
	}
}

void jsonPlanets(json_t* p, int updatemode) {
	int i;

	if(updatemode == OVERWRITE) {
		for(i = 0; i < n_planets; i++) {
			planets[i].active = 0;
		}
	}

	if(p && json_is_array(p)) {
		json_t* j_planet;
		for(i = 0; i < json_array_size(p); i++) {
			j_planet = json_array_get(p, i);
			if(json_is_object(j_planet)) {
				jsonPlanet(j_planet);
			}
		}
	}

	// Repack would be nice
}

void jsonExplosion(json_t* explosion) {
	int i;
	int id = 0;
	float x,y;

	json_t* j_id = json_object_get(explosion, "exploding_entity");
	if(!j_id || !json_is_integer(j_id)) return;
	id = json_integer_value(j_id);
	// Invalidate the ship with this id
	for(i = 0; i < n_ships; i++) {
		if(ships[i].id == id) {
			//fprintf(stderr, "Invalidated ship %d\n", i);
			ships[i].active = 0;
		}
	}

	json_t* j_x = json_object_get(explosion, "x");
	if(!j_x || !json_is_real(j_x)) return;
	x = json_real_value(j_x);

	json_t* j_y = json_object_get(explosion, "y");
	if(!j_y || !json_is_real(j_y)) return;
	y = json_real_value(j_y);

	//fprintf(stderr, "Bäm! x=%f, y=%f\n",x,y);
	createExplosion(x, y);
}

void jsonShip(json_t* ship) {
	int id = 0;
	float x,y;
	int owner = 0;
	int size = 0;
	const char* contents;
	int docked_to = 0;

	json_t* j_id = json_object_get(ship, "id");
	if(!j_id || !json_is_integer(j_id)) return;
	id = json_integer_value(j_id);

	json_t* j_x = json_object_get(ship, "x");
	if(!j_x || !json_is_real(j_x)) return;
	x = json_real_value(j_x);

	json_t* j_y = json_object_get(ship, "y");
	if(!j_y || !json_is_real(j_y)) return;
	y = json_real_value(j_y);

	json_t* j_owner = json_object_get(ship, "owner");
	if(j_owner) {
		if(json_is_null(j_owner)) {
			// Unowned
		} else if(json_is_integer(j_owner)) {
			owner = json_integer_value(j_owner);
		} else {
			return;
		}
	} else {
		return;
	}

	json_t* j_size = json_object_get(ship, "size");
	if(!j_size || !json_is_integer(j_size)) return;
	size = json_integer_value(j_size);

	json_t* j_contents = json_object_get(ship, "contents");
	if(!j_contents || !json_is_string(j_contents)) return;
	contents = json_string_value(j_contents);

	json_t* j_docked = json_object_get(ship, "docked_to");
	if(j_docked) {
		if(json_is_null(j_docked)) {
			// Undocked
		} else if(json_is_integer(j_docked)) {
			docked_to = json_integer_value(j_docked);
		} else {
			return;
		}
	} else {
		return;
	}

	updateShip(id, x, y, owner, size, contents, docked_to);
}

void jsonShot(json_t* shot) {
	int id;
	int owner;
	int source = 0;
	int target = 0;
	float r_src_x, r_src_y;
	float r_trg_x, r_trg_y;
	float v_src_x, v_src_y;
	float v_trg_x, v_trg_y;
	float length = 0;
	int i;

	json_t* j_id = json_object_get(shot, "id");
	if(!j_id || !json_is_integer(j_id)) return;
	id = json_integer_value(j_id);

	json_t* j_owner = json_object_get(shot, "owner");
	if(!j_owner || !json_is_integer(j_owner)) return;
	owner = json_integer_value(j_owner);

	json_t* j_source = json_object_get(shot, "source");
	if(!j_source || !json_is_integer(j_source)) return;
	source = json_integer_value(j_source);

	json_t* j_target = json_object_get(shot, "target");
	if(!j_target || !json_is_integer(j_target)) return;
	target = json_integer_value(j_target);

	char found_src = 0;
	char found_trg = 0;
	for(i = 0; i < n_ships; i++) {
		if(!found_src && ships[i].id == source) {
			r_src_x = ships[i].x;
			r_src_y = ships[i].y;
			found_src = 1;
		}
		if(!found_trg && ships[i].id == target) {
			r_trg_x = ships[i].x;
			r_trg_y = ships[i].y;
			found_trg = 1;
		}
	}
	for(i = 0; i < n_asteroids; i++) {
		if(!found_src && asteroids[i].id == source) {
			r_src_x = asteroids[i].x;
			r_src_y = asteroids[i].y;
			found_src = 1;
		}
		if(!found_trg && asteroids[i].id == target) {
			r_trg_x = asteroids[i].x;
			r_trg_y = asteroids[i].y;
			found_trg = 1;
		}
	}
	for(i = 0; i < n_planets; i++) {
		if(!found_src && planets[i].id == source) {
			r_src_x = planets[i].x;
			r_src_y = planets[i].y;
			found_src = 1;
		}
		if(!found_trg && planets[i].id == target) {
			r_trg_x = planets[i].x;
			r_trg_y = planets[i].y;
			found_trg = 1;
		}
	}
	if(found_src && found_trg) {
		length = hypot(r_trg_x - r_src_x, r_trg_y - r_src_y);
		if(length > 5 && length < 100) {
			v_trg_x = r_trg_x - 4 * (r_trg_x - r_src_x) / length;
			v_trg_y = r_trg_y - 4 * (r_trg_y - r_src_y) / length;
			v_src_x = r_trg_x - (length-1) * (r_trg_x - r_src_x) / length;
			v_src_y = r_trg_y - (length-1) * (r_trg_y - r_src_y) / length;
			createShot(id, owner, v_src_x, v_src_y, v_trg_x, v_trg_y);
		}
	}
}

void createShot(int id, int owner, float src_x, float src_y, float trg_x, float trg_y) {
	// Not important enought to realloc
	if(n_shots < n_shots_max) {
		shots[n_shots].src_x = src_x;
		shots[n_shots].src_y = src_y;
		shots[n_shots].trg_x = trg_x;
		shots[n_shots].trg_y = trg_y;
		n_shots++;
	}
}

void jsonPlanet(json_t* planet) {
	int id = 0;
	float x;
	float y;
	int owner = 0;

	json_t* j_id = json_object_get(planet, "id");
	if(!j_id || !json_is_integer(j_id)) return;
	id = json_integer_value(j_id);

	json_t* j_x = json_object_get(planet, "x");
	if(!j_x || !json_is_real(j_x)) return;
	x = json_real_value(j_x);

	json_t* j_y = json_object_get(planet, "y");
	if(!j_y || !json_is_real(j_y)) return;
	y = json_real_value(j_y);

	json_t* j_owner = json_object_get(planet, "owner");
	if(j_owner) {
		if(json_is_null(j_owner)) {
			// Unowned
		} else if(json_is_integer(j_owner)) {
			owner = json_integer_value(j_owner);
		} else {
			return;
		}
	} else {
		return;
	}

	updatePlanet(id, x, y, owner);
	//fprintf(stdout, "Found planet %d at (%f, %f), owned by %d\n", id, x, y, owner);
}

void updatePlanet(int id, float x, float y, int owner) {
	int i;
	char found = 0;
	for(i = 0; i < n_planets; i++) {
		if(planets[i].id == id) {
			planets[i].x = x;
			planets[i].y = y;
			planets[i].owner = owner;
			planets[i].active = 1;
			//fprintf(stderr, "Updated planet in slot %d\n", i);
			found = 1;
			break;
		}
	}
	if(!found) {
		if(n_planets >= n_planets_max -1) {
			planets = realloc(planets, (n_planets_max + 100) * sizeof(planet_t));
			if(!planets) {
				fprintf(stderr, "No more planets :-(\n");
				exit(1);
			}
			n_planets_max += 100;
		}
		planets[n_planets].id = id;
		planets[n_planets].x = x;
		planets[n_planets].y = y;
		planets[n_planets].owner = owner;
		planets[n_planets].active = 1;
		//fprintf(stderr, "Added planet to slot %d\n", n_planets);
		n_planets++;
	}
}

void updateShip(int id, float x, float y, int owner, int size, const char* contents, int docked_to) {
	int i;
	char found = 0;
	for(i = 0; i < n_ships; i++) {
		if(ships[i].id == id) {
			ships[i].x = x;
			ships[i].y = y;
			ships[i].owner = owner;
			ships[i].size = size;
			strncpy(ships[i].contents, contents, 10);
			ships[i].docked_to = docked_to;
			ships[i].active = 1;
			found = 1;
			break;
		}
	}
	if(!found) {
		if(n_ships >= n_ships_max -1) {
			ships = realloc(ships, (n_ships_max + 100) * sizeof(ship_t));
			if(!ships) {
				fprintf(stderr, "No more ships :-(\n");
				exit(1);
			}
			n_ships_max += 100;
		}
		ships[n_ships].id = id;
		ships[n_ships].x = x;
		ships[n_ships].y = y;
		ships[n_ships].owner = owner;
		ships[n_ships].size = size;
		strncpy(ships[n_ships].contents, contents, 10);
		ships[n_ships].docked_to = docked_to;
		ships[n_ships].active = 1;
		n_ships++;
	}

}

void createExplosion(float x, float y) {
	int i;
	char foundEmpty = 0;
	for(i = 0; i < n_explosions; i++) {
		if(explosions[i].strength <= 0) {
			explosions[i].x = x;
			explosions[i].y = y;
			explosions[i].strength = 256;
			foundEmpty = 1;
			break;
		}
	}
	if(!foundEmpty) {
		if(n_explosions >= n_explosions_max -1) {
			explosions = realloc(explosions, (n_explosions_max + 100) * sizeof(explosion_t));
			if(!explosions) {
				fprintf(stderr, "No more explosions :-(\n");
				exit(1);
			}
			n_explosions_max += 100;
		}
		explosions[n_explosions].x = x;
		explosions[n_explosions].y = y;
		explosions[n_explosions].strength = 256;
		n_explosions++;
	}
}

void jsonAsteroids(json_t* a, int updatemode) {
	int i;

	if(updatemode == OVERWRITE) {
		for(i = 0; i < n_asteroids; i++) {
			asteroids[i].active = 0;
		}
	}

	if(a && json_is_array(a)) {
		json_t* j_asteroid;
		for(i = 0; i < json_array_size(a); i++) {
			j_asteroid = json_array_get(a, i);
			if(json_is_object(j_asteroid)) {
				jsonAsteroid(j_asteroid);
			}
		}
	}
}

void jsonAsteroid(json_t* asteroid) {
	int id = 0;
	float x;
	float y;
	const char* contents;

	json_t* j_id = json_object_get(asteroid, "id");
	if(!j_id || !json_is_integer(j_id)) return;
	id = json_integer_value(j_id);

	json_t* j_x = json_object_get(asteroid, "x");
	if(!j_x || !json_is_real(j_x)) return;
	x = json_real_value(j_x);

	json_t* j_y = json_object_get(asteroid, "y");
	if(!j_y || !json_is_real(j_y)) return;
	y = json_real_value(j_y);

	json_t* j_contents = json_object_get(asteroid, "contents");
	if(!j_contents || !json_is_string(j_contents)) return;
	contents = json_string_value(j_contents);

	updateAsteroid(id, x, y, contents);
}

void updateAsteroid(int id, float x, float y, const char* contents) {
	int i;
	char found = 0;
	for(i = 0; i < n_asteroids; i++) {
		if(asteroids[i].id == id) {
			asteroids[i].x = x;
			asteroids[i].y = y;
			strncpy(asteroids[i].contents, contents, 10);
			asteroids[i].active = 1;
			found = 1;
			break;
		}
	}
	if(!found) {
		if(n_asteroids >= n_asteroids_max -1) {
			asteroids = realloc(asteroids, (n_asteroids_max + 100) * sizeof(asteroid_t));
			if(!asteroids) {
				fprintf(stderr, "No more asteroids :-(\n");
				exit(1);
			}
			n_asteroids_max += 100;
		}
		asteroids[n_asteroids].id = id;
		asteroids[n_asteroids].x = x;
		asteroids[n_asteroids].y = y;
		strncpy(asteroids[n_asteroids].contents, contents, 10);
		asteroids[n_asteroids].active = 1;
		n_asteroids++;
	}
}

void jsonBbox(json_t* bbox) {
	static char seenBbox = 0;
	if(bbox) {
		json_t* xmin = json_object_get(bbox, "xmin");
		if(json_is_number(xmin)) {
			boundingbox.xmin = json_number_value(xmin);
		}
		json_t* xmax = json_object_get(bbox, "xmax");
		if(json_is_number(xmax)) {
			boundingbox.xmax = json_number_value(xmax);
		}
		json_t* ymin = json_object_get(bbox, "ymin");
		if(json_is_number(ymin)) {
			boundingbox.ymin = json_number_value(ymin);
		}
		json_t* ymax = json_object_get(bbox, "ymax");
		if(json_is_number(ymax)) {
			boundingbox.ymax = json_number_value(ymax);
		}

		if(!seenBbox) {
			float zoomx = 640/(boundingbox.xmax - boundingbox.xmin);
			float zoomy = 480/(boundingbox.ymax - boundingbox.ymin);
			if(zoomx < zoomy) {
				zoom = zoomx;
			} else {
				zoom = zoomy;
			}
			offset_x = 320 - zoom*(boundingbox.xmin + boundingbox.xmax)/2;
			offset_y = 240 - zoom*(boundingbox.ymin + boundingbox.ymax)/2;
			seenBbox = 1;
		}
	}
	fprintf(stdout, "Updated bbox to (%f, %f) x (%f, %f)\n", boundingbox.xmin, boundingbox.xmax, boundingbox.ymin, boundingbox.ymax);
}

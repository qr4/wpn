#include "json.h"

extern bbox_t boundingbox;

extern state_t state;
extern options_t options;

extern float zoom;
extern float offset_x;
extern float offset_y;
extern json_int_t follow_ship;
extern int display_x;
extern int display_y;

extern void init_storage(storage_t *storage, size_t nmemb, size_t size);

int parseJson(buffer_t* b) {
	json_error_t error;
	json_t* root = json_loads(b->data, 0, &error);
	if(!root) {
		fprintf(stderr, "Error %s in line %d, column %d\n", error.text, error.line, error.column);
		//exit(1);
	} else {
		json_t* world = json_object_get(root, "world");
		if(world) {
			jsonWorld(world);
		}
	}
	json_decref(root);
	return 0;
}

void jsonWorld(json_t* world) {
	json_t* j_players = json_object_get(world, "players");
	if(j_players) {
		jsonPlayers(j_players);
	}
	json_t* j_asteroids = json_object_get(world, "asteroids");
	if(j_asteroids) {
		jsonAsteroids(j_asteroids);
	}
	json_t* j_bbox = json_object_get(world, "bounding-box");
	if(j_bbox) {
		jsonBbox(j_bbox);
	}
	json_t* j_planets = json_object_get(world, "planets");
	if(j_planets) {
		jsonPlanets(j_planets);
	}
	json_t* j_ships = json_object_get(world, "ships");
	if(j_ships) {
		jsonShips(j_ships);
	}
	json_t* j_bases = json_object_get(world, "bases");
	if(j_bases) {
		jsonBases(j_bases);
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

void jsonShips(json_t* s) {
	int i;

	for(i = 0; i < state.ships.n; i++) {
		state.ships.ships[i].active = 0;
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

void jsonBases(json_t* b) {
	int i;

	for(i = 0; i < state.bases.n; i++) {
		state.bases.bases[i].active = 0;
	}

	if(b && json_is_array(b)) {
		json_t* j_base;
		for(i = 0; i < json_array_size(b); i++) {
			j_base = json_array_get(b, i);
			if(json_is_object(j_base)) {
				jsonBase(j_base);
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

void jsonPlanets(json_t* p) {
	int i;

	for(i = 0; i < state.planets.n; i++) {
		state.planets.planets[i].active = 0;
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
	json_int_t id = 0;
	float x,y;

	json_t* j_id = json_object_get(explosion, "exploding_entity");
	if(!j_id || !json_is_integer(j_id)) return;
	id = json_integer_value(j_id);
	// Invalidate the ship with this id
	for(i = 0; i < state.ships.n; i++) {
		if(state.ships.ships[i].id == id) {
			//fprintf(stderr, "Invalidated ship %d\n", i);
			state.ships.ships[i].active = 0;
		}
	}
	// Invalidate the base with this id
	for(i = 0; i < state.bases.n; i++) {
		if(state.bases.bases[i].id == id) {
			//fprintf(stderr, "Invalidated base %d\n", i);
			state.bases.bases[i].active = 0;
		}
	}

	json_t* j_x = json_object_get(explosion, "x");
	if(!j_x || !json_is_real(j_x)) return;
	x = json_real_value(j_x);

	json_t* j_y = json_object_get(explosion, "y");
	if(!j_y || !json_is_real(j_y)) return;
	y = json_real_value(j_y);

	//fprintf(stderr, "Bäm! x=%f, y=%f\n",x,y);
	createExplosion(x, y, id);
}

void jsonShip(json_t* ship) {
	json_int_t id = 0;
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

	if(follow_ship != 0 && follow_ship == id) {
		options.offset_x = options.display_x/2 - x * options.zoom;
		options.offset_y = options.display_y/2 - y * options.zoom;
	}

	updateShip(id, x, y, owner, size, contents, docked_to);
}

void jsonBase(json_t* base) {
	json_int_t id = 0;
	float x,y;
	int owner = 0;
	int size = 0;
	const char* contents;
	int docked_to = 0;

	json_t* j_id = json_object_get(base, "id");
	if(!j_id || !json_is_integer(j_id)) return;
	id = json_integer_value(j_id);

	json_t* j_x = json_object_get(base, "x");
	if(!j_x || !json_is_real(j_x)) return;
	x = json_real_value(j_x);

	json_t* j_y = json_object_get(base, "y");
	if(!j_y || !json_is_real(j_y)) return;
	y = json_real_value(j_y);

	json_t* j_owner = json_object_get(base, "owner");
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

	json_t* j_size = json_object_get(base, "size");
	if(!j_size || !json_is_integer(j_size)) return;
	size = json_integer_value(j_size);

	json_t* j_contents = json_object_get(base, "contents");
	if(!j_contents || !json_is_string(j_contents)) return;
	contents = json_string_value(j_contents);

	json_t* j_docked = json_object_get(base, "docked_to");
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

	updateBase(id, x, y, owner, size, contents, docked_to);
}

void jsonShot(json_t* shot) {
	json_int_t id;
	json_int_t owner;
	float r_src_x = 0;
	float r_src_y = 0;
	float r_trg_x = 0;
	float r_trg_y = 0;

	json_t* j_id = json_object_get(shot, "id");
	if(!j_id || !json_is_integer(j_id)) return;
	id = json_integer_value(j_id);

	json_t* j_owner = json_object_get(shot, "owner");
	if(!j_owner || !json_is_integer(j_owner)) return;
	owner = json_integer_value(j_owner);

	json_t* j_src_x = json_object_get(shot, "srcx");
	if(!j_src_x || !json_is_real(j_src_x)) return;
	r_src_x = json_real_value(j_src_x);

	json_t* j_src_y = json_object_get(shot, "srcy");
	if(!j_src_y || !json_is_real(j_src_y)) return;
	r_src_y = json_real_value(j_src_y);

	json_t* j_trg_x = json_object_get(shot, "tgtx");
	if(!j_trg_x || !json_is_real(j_trg_x)) return;
	r_trg_x = json_real_value(j_trg_x);

	json_t* j_trg_y = json_object_get(shot, "tgty");
	if(!j_trg_y || !json_is_real(j_trg_y)) return;
	r_trg_y = json_real_value(j_trg_y);

	createShot(id, owner, r_src_x, r_src_y, r_trg_x, r_trg_y);
}

void createShot(json_int_t id, int owner, float src_x, float src_y, float trg_x, float trg_y) {
	int i;
	char foundEmpty = 0;
	shot_t *shots;
	shots = state.shots.shots;
	for(i = 0; i < state.shots.n; i++) {
		if(shots[i].strength <= 0) {
			shots[i].src_x = src_x;
			shots[i].src_y = src_y;
			shots[i].trg_x = trg_x;
			shots[i].trg_y = trg_y;
			shots[i].strength = 20;
			foundEmpty = 1;
			break;
		}
	}
	if(!foundEmpty) {
		if(state.shots.n >= state.shots.n_max - 1) {
			init_storage(&state.shots, state.shots.n_max + 10, sizeof(shot_t));
			shots = state.shots.shots;
		}
		shots[state.shots.n].src_x = src_x;
		shots[state.shots.n].src_y = src_y;
		shots[state.shots.n].trg_x = trg_x;
		shots[state.shots.n].trg_y = trg_y;
		shots[state.shots.n].strength = 20;
		state.shots.n++;
	}
}

void jsonPlanet(json_t* planet) {
	json_int_t id = 0;
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

void updatePlanet(json_int_t id, float x, float y, int owner) {
	int i;
	char found = 0;
	planet_t *planets;
	planets = state.planets.planets;
	size_t n = state.planets.n;
	size_t n_max = state.planets.n_max;
	for(i = 0; i < n; i++) {
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
		if(n >= n_max -1) {
			init_storage(&state.planets, n_max + 100, sizeof(planet_t));
			planets = state.planets.planets;
		}
		planets[n].id = id;
		planets[n].x = x;
		planets[n].y = y;
		planets[n].owner = owner;
		planets[n].active = 1;
		//fprintf(stderr, "Added planet to slot %d\n", n_planets);
		state.planets.n++;
	}
}

void updateShip(json_int_t id, float x, float y, int owner, int size, const char* contents, int docked_to) {
	int i;
	char found = 0;
	ship_t *ships;
	ships = state.ships.ships;
	size_t n = state.ships.n;
	size_t n_max = state.ships.n_max;
	for(i = 0; i < n; i++) {
		if(ships[i].id == id) {
			ships[i].x = x;
			ships[i].y = y;
			ships[i].owner = owner;
			ships[i].size = size;
			strncpy(ships[i].contents, contents, 25);
			ships[i].docked_to = docked_to;
			ships[i].active = 1;
			found = 1;
			break;
		}
	}
	if(!found) {
		if(n >= n_max -1) {
			init_storage(&state.ships, n_max + 100, sizeof(ship_t));
			ships = state.ships.ships;
		}
		ships[n].id = id;
		ships[n].x = x;
		ships[n].y = y;
		ships[n].owner = owner;
		ships[n].size = size;
		strncpy(ships[n].contents, contents, 25);
		ships[n].docked_to = docked_to;
		ships[n].active = 1;
		state.ships.n++;
	}

}

void updateBase(json_int_t id, float x, float y, int owner, int size, const char* contents, int docked_to) {
	int i;
	char found = 0;
	base_t *bases;
	bases = state.bases.bases;
	size_t n = state.bases.n;
	size_t n_max = state.bases.n_max;
	for(i = 0; i < n; i++) {
		if(bases[i].id == id) {
			bases[i].x = x;
			bases[i].y = y;
			bases[i].owner = owner;
			bases[i].size = size;
			strncpy(bases[i].contents, contents, 25);
			bases[i].docked_to = docked_to;
			bases[i].active = 1;
			found = 1;
			break;
		}
	}
	if(!found) {
		if(n >= n_max -1) {
			init_storage(&state.bases, n_max + 100, sizeof(base_t));
			bases = state.bases.bases;
		}
		bases[n].id = id;
		bases[n].x = x;
		bases[n].y = y;
		bases[n].owner = owner;
		bases[n].size = size;
		strncpy(bases[n].contents, contents, 25);
		bases[n].docked_to = docked_to;
		bases[n].active = 1;
		state.bases.n++;
	}

}

void createExplosion(float x, float y, json_int_t source) {
	int i;
	char foundEmpty = 0;
	explosion_t *explosions;
	explosions = state.explosions.explosions;
	size_t n = state.explosions.n;
	size_t n_max = state.explosions.n_max;
	for(i = 0; i < n; i++) {
		if(explosions[i].strength <= 0) {
			explosions[i].source = source;
			explosions[i].x = x;
			explosions[i].y = y;
			explosions[i].strength = 256;
			foundEmpty = 1;
			break;
		}
	}
	if(!foundEmpty) {
		if(n >= n_max -1) {
			init_storage(&state.explosions, n_max + 100, sizeof(explosion_t));
			explosions = state.explosions.explosions;
		}
		explosions[n].x = x;
		explosions[n].y = y;
		explosions[n].strength = 256;
		state.explosions.n++;
	}
}

void jsonPlayers(json_t* p) {
	int i;

	if(p && json_is_array(p)) {
		json_t* j_player;
		for(i = 0; i < json_array_size(p); i++) {
			j_player = json_array_get(p, i);
			if(json_is_object(j_player)) {
				jsonPlayer(j_player);
			}
		}
	}
}

void jsonPlayer(json_t* player) {
	json_int_t id = 0;
	const char* name;

	json_t* j_id = json_object_get(player, "id");
	if(!j_id || !json_is_integer(j_id)) return;
	id = json_integer_value(j_id);

	json_t* j_name = json_object_get(player, "name");
	if(!j_name || !json_is_string(j_name)) return;
	name = json_string_value(j_name);

	int i;
	char found = 0;
	player_t *players;
	players = state.players.players;
	size_t n = state.players.n;
	size_t n_max = state.players.n_max;
	for(i = 0; i < n; i++) {
		if(players[i].id == id) {
			found = 1;
			break;
		}
	}
	if(!found) {
		if(n >= n_max -1) {
			init_storage(&state.players, n_max + 100, sizeof(player_t));
			players = state.players.players;
		}
		players[n].id = id;
		players[n].name = malloc(strlen(name)+1);
		if(!players[n].name) {
			fprintf(stderr, "No more player names :-(\n");
			exit(1);
		}
		strncpy(players[n].name, name, strlen(name));
		players[n].name[strlen(name)] = '\0';
		state.players.n++;
	}
}

void jsonAsteroids(json_t* a) {
	int i;

	asteroid_t *asteroids;
	asteroids = state.asteroids.asteroids;
	size_t n = state.asteroids.n;
	for(i = 0; i < n; i++) {
		asteroids[i].active = 0;
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
	json_int_t id = 0;
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

void updateAsteroid(json_int_t id, float x, float y, const char* contents) {
	int i;
	char found = 0;

	asteroid_t *asteroids;
	asteroids = state.asteroids.asteroids;
	size_t n = state.asteroids.n;
	size_t n_max = state.asteroids.n_max;
	for(i = 0; i < n; i++) {
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
		if(n >= n_max -1) {
			init_storage(&state.asteroids, n_max + 100, sizeof(asteroid_t));
			asteroids = state.asteroids.asteroids;
		}
		asteroids[n].id = id;
		asteroids[n].x = x;
		asteroids[n].y = y;
		strncpy(asteroids[n].contents, contents, 10);
		asteroids[n].active = 1;
		state.asteroids.n++;
	}
}

void jsonBbox(json_t* bbox) {
	static char seenBbox = 0;
	float xmin = 0;
	float xmax = 170000;
	float ymin = 0;
	float ymax = 100000;
	if(bbox) {
		json_t* j_xmin = json_object_get(bbox, "xmin");
		if(json_is_number(j_xmin)) {
			xmin = json_number_value(j_xmin);
		}
		json_t* j_xmax = json_object_get(bbox, "xmax");
		if(json_is_number(j_xmax)) {
			xmax = json_number_value(j_xmax);
		}
		json_t* j_ymin = json_object_get(bbox, "ymin");
		if(json_is_number(j_ymin)) {
			ymin = json_number_value(j_ymin);
		}
		json_t* j_ymax = json_object_get(bbox, "ymax");
		if(json_is_number(j_ymax)) {
			ymax = json_number_value(j_ymax);
		}

		if((xmin != state.boundingbox.xmin) || (xmax != state.boundingbox.xmax) || (ymin != state.boundingbox.ymin) || (ymax != state.boundingbox.ymax)) {
			state.boundingbox.xmin = xmin;
			state.boundingbox.xmax = xmax;
			state.boundingbox.ymin = ymin;
			state.boundingbox.ymax = ymax;
			fprintf(stdout, "Updated bbox to (%f, %f) x (%f, %f)\n", xmin, xmax, ymin, ymax);
		} else {
			return;
		}

		if(!seenBbox) {
			float zoomx = 640/(state.boundingbox.xmax - state.boundingbox.xmin);
			float zoomy = 480/(state.boundingbox.ymax - state.boundingbox.ymin);
			if(zoomx < zoomy) {
				options.zoom = zoomx;
			} else {
				options.zoom = zoomy;
			}
			options.offset_x = 320 - options.zoom*(state.boundingbox.xmin + state.boundingbox.xmax)/2;
			options.offset_y = 240 - options.zoom*(state.boundingbox.ymin + state.boundingbox.ymax)/2;
			seenBbox = 1;
		}
	}
}

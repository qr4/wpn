#ifndef _JSON_H
#define _JSON_H

#include <math.h>
#include <string.h>
#include <jansson.h>
#include "types.h"
#include "buffer.h"

#define UPDATE 0
#define OVERWRITE 1

int parseJson(buffer_t* b);

void createExplosion(float x, float y);
void createShot(json_int_t id, int owner, float x, float y, float length, float angle);
void jsonAsteroid(json_t* asteroid);
void jsonAsteroids(json_t* a, int updatemode);
void jsonBbox(json_t* bbox);
void jsonBase(json_t* base);
void jsonBases(json_t* b, int updatemode);
void jsonExplosion(json_t* e);
void jsonExplosions(json_t* e);
void jsonPlanet(json_t* planet);
void jsonPlanets(json_t* p, int updatemode);
void jsonPlayer(json_t* p);
void jsonPlayers(json_t* p);
void jsonShip(json_t* ship);
void jsonShips(json_t* s, int updatemode);
void jsonShot(json_t* shot);
void jsonShots(json_t* s);
void jsonUpdate(json_t* update);
void jsonWorld(json_t* world);
void updateAsteroid(json_int_t id, float x, float y, const char* contents);
void updateBase(json_int_t id, float x, float y, int owner, int size, const char* contents, int docked_to);
void updatePlanet(json_int_t id, float x, float y, int owner);
void updateShip(json_int_t id, float x, float y, int owner, int size, const char* contents, int docked_to);

#endif

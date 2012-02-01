#include <pthread.h>

#include "types.h"
#include "snapshot.h"

snapshot_t *current_snapshot = NULL;
size_t timestamp = 0;
pthread_mutex_t current_mutex = PTHREAD_MUTEX_INITIALIZER;

static inline void copy_storage(storage_t *dst, storage_t *src, size_t size) {
	dst->n = src->n;
	dst->n_max = src->n_max;
	dst->data = malloc(dst->n_max * size);
	memcpy(dst->data, src->data, dst->n_max * size);
}

static void copy_state(state_t *dst, state_t *src) {
	copy_storage(&dst->asteroids,  &src->asteroids,  sizeof(asteroid_t));
	copy_storage(&dst->bases,      &src->bases,      sizeof(base_t));
	copy_storage(&dst->explosions, &src->explosions, sizeof(explosion_t));
	copy_storage(&dst->planets,    &src->planets,    sizeof(planet_t));
	copy_storage(&dst->ships,      &src->ships,      sizeof(ship_t));
	copy_storage(&dst->shots,      &src->shots,      sizeof(shot_t));
	copy_storage(&dst->players,    &src->players,    sizeof(player_t));

	dst->boundingbox = src->boundingbox;
}

static void copy_snapshot(snapshot_t *dst, snapshot_t *src) {
	dst->options = src->options;
	copy_state(&dst->state, &src->state);
	dst->timestamp = src->timestamp;
	dst->_refcount = 1;
	pthread_mutex_init(&dst->_mutex, NULL);
}

static void incref(snapshot_t *snapshot) {
	pthread_mutex_lock(&snapshot->_mutex);
	snapshot->_refcount++;
	pthread_mutex_unlock(&snapshot->_mutex);
}

void snapshot_update(snapshot_t *snapshot) {
	pthread_mutex_lock(&current_mutex);
	snapshot_release(current_snapshot);
	snapshot_t *t = malloc(sizeof(snapshot_t));
	copy_snapshot(t, snapshot);
	t->timestamp = timestamp++;
	current_snapshot = t;
	//fprintf(stderr, "register new snapshot\n");
	pthread_mutex_unlock(&current_mutex);
}

void snapshot_update_from_members(state_t *state, options_t *options) {
	snapshot_t t;
	copy_state(&t.state, state);
	t.options = *options;
	snapshot_update(&t);
}

snapshot_t *snapshot_getcurrent() {
	incref(current_snapshot);
	//fprintf(stderr, "coping\n");
	return current_snapshot;
}

void snapshot_getcurrentcopy(snapshot_t *snapshot) {
	pthread_mutex_lock(&current_mutex);
	copy_snapshot(snapshot, current_snapshot);
	pthread_mutex_unlock(&current_mutex);
}

void snapshot_release(snapshot_t *snapshot) {
	if (snapshot == NULL) return;
	pthread_mutex_lock(&snapshot->_mutex);

	if (snapshot->_refcount <= 1) {
		free(snapshot->state.asteroids.data);
		free(snapshot->state.bases.data);
		free(snapshot->state.explosions.data);
		free(snapshot->state.planets.data);
		free(snapshot->state.ships.data);
		free(snapshot->state.shots.data);
		free(snapshot->state.players.data);

		pthread_mutex_unlock(&snapshot->_mutex);

		free(snapshot);
		//fprintf(stderr, "actually getting rid of a snapshot\n");
	} else {
		snapshot->_refcount--;
		pthread_mutex_unlock(&snapshot->_mutex);
	}
}

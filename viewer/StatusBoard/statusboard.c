#include "statusboard.h"

state_t state;

void ConsumerInit() {
	const size_t size = 1000;
	init_storage(&state.asteroids,     size,   sizeof(asteroid_t));
	init_storage(&state.bases,         size,   sizeof(base_t));
	init_storage(&state.explosions,    size,   sizeof(explosion_t));
	init_storage(&state.planets,       size,   sizeof(planet_t));
	init_storage(&state.ships,         size,   sizeof(ship_t));
	init_storage(&state.shots,         size,   sizeof(shot_t));
	init_storage(&state.players,       size,   sizeof(player_t));
}

int score_cmp(const void* a, const void* b) {
	const score_t * sa = (const score_t *) a;
	const score_t * sb = (const score_t *) b;

	return  (sa->score < sb->score) - (sa->score > sb->score);
}

int ship_cmp(const void* a, const void* b) {
	const score_t * sa = (const score_t *) a;
	const score_t * sb = (const score_t *) b;

	return  (sa->ships < sb->ships) - (sa->ships > sb->ships);
}

int planet_cmp(const void* a, const void* b) {
	const score_t * sa = (const score_t *) a;
	const score_t * sb = (const score_t *) b;

	return  (sa->planets < sb->planets) - (sa->planets > sb->planets);
}

int base_cmp(const void* a, const void* b) {
	const score_t * sa = (const score_t *) a;
	const score_t * sb = (const score_t *) b;

	return  (sa->bases < sb->bases) - (sa->bases > sb->bases);
}

void print_html(const int n, score_t* scorecard) {
	FILE* file = fopen("/srv/www/htdocs/wpn/.score", "w");
	fprintf(file, "<HTML>\n<HEAD>\n");
	fprintf(file, "<LINK rel=\"stylesheet\" href=\"style.css\">\n");
	fprintf(file, "<META HTTP-EQUIV=\"refresh\" CONTENT=\"5\">\n");
	fprintf(file, "<TITLE>WPN Scoreboard</TITLE>\n");
	fprintf(file, "</HEAD>\n<BODY>\n");
	fprintf(file, "<H1>WPN Scoreboard</H1>\n");
	fprintf(file, "<DIV style=\"float: left; width: 50%%;\">\n");

	fprintf(file, "<H3>Overall</H3>\n");
	fprintf(file, "<TABLE>\n");
	fprintf(file, "<TR><TH>Name</TH><TH>Score</TH><TH>Bases</TH><TH>Planets</TH><TH>Ships</TH></TR>\n");
	int i = 0;
	qsort(scorecard, n, sizeof(score_t), score_cmp);
	while(i < n) {
		fprintf(file, "<TR><TD>%s</TD><TD>%d</TD><TD>%d</TD><TD>%d</TD><TD>%d</TD></TR>\n", scorecard[i].player.name, scorecard[i].score, scorecard[i].bases, scorecard[i].planets, scorecard[i].ships);
		i++;
	}
	fprintf(file, "</TABLE>\n");

	fprintf(file, "</DIV>\n");
	fprintf(file, "<DIV style=\"float: right; width: 50%%;\">\n");

	fprintf(file, "<H3>Ships (by slots)</H3>\n");
	fprintf(file, "<TABLE>\n");
	fprintf(file, "<TR><TH>Name</TH><TH>Ships</TH></TR>\n");
	i = 0;
	qsort(scorecard, n, sizeof(score_t), ship_cmp);
	while((i < 3) && (i < n)) {
		fprintf(file, "<TR><TD>%s</TD><TD>%d</TD></TR>\n", scorecard[i].player.name, scorecard[i].ships);
		i++;
	}
	fprintf(file, "</TABLE>\n");
	fprintf(file, "<BR>\n");

	fprintf(file, "<H3>Planets (by size)</H3>\n");
	fprintf(file, "<TABLE>\n");
	fprintf(file, "<TR><TH>Name</TH><TH>Planets</TH></TR>\n");
	i = 0;
	qsort(scorecard, n, sizeof(score_t), planet_cmp);
	while((i < 3) && (i < n)) {
		fprintf(file, "<TR><TD>%s</TD><TD>%d</TD></TR>\n", scorecard[i].player.name, scorecard[i].planets);
		i++;
	}
	fprintf(file, "</TABLE>\n");
	fprintf(file, "<BR>\n");

	fprintf(file, "<H3>Bases (by slots)</H3>\n");
	fprintf(file, "<TABLE>\n");
	fprintf(file, "<TR><TH>Name</TH><TH>Bases</TH></TR>\n");
	i = 0;
	qsort(scorecard, n, sizeof(score_t), base_cmp);
	while((i < 3) && (i < n)) {
		fprintf(file, "<TR><TD>%s</TD><TD>%d</TD></TR>\n", scorecard[i].player.name, scorecard[i].bases);
		i++;
	}
	fprintf(file, "</TABLE>\n");
	fprintf(file, "<BR>\n");

	fprintf(file, "</DIV>\n");
	fprintf(file, "</BODY>\n</HTML>\n");
	fclose(file);
	rename("/srv/www/htdocs/wpn/.score", "/srv/www/htdocs/wpn/score.htm");
}

void ConsumerFrame() {
	static int n_players = 0;
	static score_t * players = NULL;

	if(state.players.n > n_players) {
		players = realloc(players, state.players.n*sizeof(score_t));
		if(players == NULL) {
			fprintf(stderr, "Failed to allocate score card for %d players\n", state.players.n);
			exit(1);
		}
		n_players = state.players.n;
	}
	memset(players, 0, n_players*sizeof(score_t));

	for(int i = 0; i < state.players.n; i++) {
		players[i].player = state.players.players[i];

		for(int j = 0; j < state.bases.n; j++) {
			if((state.bases.bases[j].active > 0) && (state.bases.bases[j].owner ==  state.players.players[i].id)) {
				players[i].bases++;
				players[i].score += state.bases.bases[j].size;
			}
		}
		for(int j = 0; j < state.planets.n; j++) {
			if((state.planets.planets[j].active > 0) && (state.planets.planets[j].owner ==  state.players.players[i].id)) {
				players[i].planets++;
				players[i].score += 100;
			}
		}
		for(int j = 0; j < state.ships.n; j++) {
			if((state.ships.ships[j].active > 0) && (state.ships.ships[j].owner ==  state.players.players[i].id)) {
				players[i].ships++;
				players[i].score += state.ships.ships[j].size;
			}
		}
	}

	for(int j = 0; j < state.explosions.n; j++) {
		if((state.explosions.explosions[j].strength > 0)) {
			printf("%d exploded\n", state.explosions.explosions[j].source);
			state.explosions.explosions[j].strength = 0;
		}
	}

	print_html(state.players.n, players);
}

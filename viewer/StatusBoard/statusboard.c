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

void print_html(int n, score_t* scorecard) {
	int max_score = -1;
	int n_printed = 0;

	FILE* file = fopen("/srv/www/htdocs/wpn/.score", "w");
	fprintf(file, "<HTML>\n<HEAD>\n");
	fprintf(file, "<LINK rel=\"stylesheet\" href=\"style.css\">\n");
	fprintf(file, "<META HTTP-EQUIV=\"refresh\" CONTENT=\"5\">\n");
	fprintf(file, "<TITLE>WPN Scoreboard</TITLE>\n");
	fprintf(file, "</HEAD>\n<BODY>\n");
	fprintf(file, "<H1>WPN Scoreboard</H1>\n");
	fprintf(file, "<TABLE>\n");
	fprintf(file, "<TR><TH>Name</TH><TH>Score</TH><TH>Bases</TH><TH>Planets</TH><TH>Ships</TH></TR>\n");
	while(n_printed < n) {
		max_score = -1;
		for(int i = 0; i < n; i++) {
			if(scorecard[i].score > max_score) {
				max_score = scorecard[i].score;
			}
		}
		for(int i = 0; i < n; i++) {
			if(scorecard[i].score == max_score) {
				fprintf(file, "<TR><TD>%s</TD><TD>%d</TD><TD>%d</TD><TD>%d</TD><TD>%d</TD></TR>\n", scorecard[i].player.name, scorecard[i].score, scorecard[i].bases, scorecard[i].planets, scorecard[i].ships);
				scorecard[i].score = -1;
				n_printed++;
			}
		}
	}
	fprintf(file, "</TABLE>\n");
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

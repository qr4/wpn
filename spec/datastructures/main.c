#include <stdio.h>
#include "entities.h"

int main(int argc, char *argv[]) {
	entity_t e, d;
	cluster_data_t data;

	e.x = 1;
	e.y = 2;
	e.type = CLUSTER;
	e.data = &data;

	d.coords = e.coords;

	printf("%f %f\n", e.coords.x, e.coords.y);
	printf("%f %f\n", d.coords.x, d.coords.y);


	switch (e.type) {
		case CLUSTER :
			{
				printf("cluster\n");
				cluster_data_t cluster_data;
				cluster_data = *(cluster_data_t *) e.data;
				// do someting
			}
			break;
		case PLANET :
			{
				printf("planet\n");
				planet_data_t planet_data;
				planet_data = *(planet_data_t *) e.data;
				// do something else
			}
			break;
		default :
			// don't care at all
			break;
	}
	return 0;
}

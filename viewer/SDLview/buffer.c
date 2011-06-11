#include "buffer.h"

buffer_t* getBuffer() {
	buffer_t* ret = malloc(sizeof(buffer_t));
	if(!ret) {
		fprintf(stderr, "Kein Malloc, kein Buffer\n");
		exit(1);
	}

	ret->size = 8192;
	ret->fill = 0;
	ret->data = malloc(ret->size);
	if(ret->data) {
		return ret;
	} else {
		fprintf(stderr, "Kein Malloc, kein Buffer\n");
		exit(1);
	}
}

void growBuffer(buffer_t* b) {
	b->data = realloc(b->data, b->size+4096);
	if(! b->data) {
		fprintf(stderr, "Kein realloc, kein Buffer\n");
		exit(1);
	}
	memset(&(b->data[b->size]), 0, 4096);
	b->size += 4096;
}

void freeBuffer(buffer_t* b) {
	if(b && b->size > 0) {
		free(b->data);
		b->data = NULL;
		b->size = 0;
		b->fill = 0;
	}
}

void clearBuffer(buffer_t* b) {
	if(b) {
		memset(b->data, 0, b->size);
		b->fill = 0;
	}
}

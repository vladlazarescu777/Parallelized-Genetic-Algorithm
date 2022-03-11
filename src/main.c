#include <stdlib.h>
#include "genetic_algorithm.h"

int main(int argc, char *argv[]) {
	// array with all the objects that can be placed in the sack
	sack_object *objects = NULL;

	// number of objects
	int object_count = 0;

	// maximum weight that can be carried in the sack
	int sack_capacity = 0;

	// number of generations
	int generations_count = 0;

	int num_threads = 0;
	pthread_barrier_t *barrier = (pthread_barrier_t*)malloc(sizeof(pthread_barrier_t));

	if (!read_input(&objects, &object_count, &sack_capacity, &generations_count, argc, argv, &num_threads)) {
		return 0;
	} 

	individual *current_generation = (individual*) calloc(object_count, sizeof(individual));
	individual *next_generation = (individual*) calloc(object_count, sizeof(individual));
	individual *tmp = NULL;

	struct arg argmnts[num_threads];
	
	pthread_t threads[num_threads];
	
	pthread_barrier_init(barrier, NULL, num_threads);
	
	int r;
	for (int i = 0; i < num_threads; i++) {
		argmnts[i].num_threads = num_threads;
		argmnts[i].thread_id = i;
		argmnts[i].objects = (sack_object*) malloc(object_count * sizeof (sack_object));
		argmnts[i].objects = objects;
		argmnts[i].object_count = object_count;
		argmnts[i].sack_capacity = sack_capacity;
		argmnts[i].generations_count = generations_count;
		argmnts[i].current_generation = current_generation;
		argmnts[i].next_generation = next_generation;
		argmnts[i].tmp = tmp;
		argmnts[i].barrier = barrier;

		r = pthread_create(&threads[i], NULL, run_genetic_algorithm, &argmnts[i]);

		if (r) {
			printf("Eroare la crearea thread-ului %d\n", i);
			exit(-1);
		}
	}

	for (int i = 0; i < num_threads; i++) {
		r = pthread_join(threads[i], NULL);

		if (r) {
			printf("Eroare la asteptarea thread-ului %d\n", i);
			exit(-1);
		}
	}

	pthread_barrier_destroy(barrier);
	free(objects);
	return 0;
}

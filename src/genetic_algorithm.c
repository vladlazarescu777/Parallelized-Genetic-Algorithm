#include <string.h>
#include <stdlib.h>
#include "genetic_algorithm.h"

int read_input(sack_object **objects, int *object_count, int *sack_capacity, int *generations_count, int argc, char *argv[], int *num_threads)
{
	FILE *fp;

	if (argc < 3) {
		fprintf(stderr, "Usage:\n\t./tema1 in_file generations_count\n");
		return 0;
	}

	fp = fopen(argv[1], "r");
	if (fp == NULL) {
		return 0;
	}

	if (fscanf(fp, "%d %d", object_count, sack_capacity) < 2) {
		fclose(fp);
		return 0;
	}

	if (*object_count % 10) {
		fclose(fp);
		return 0;
	}

	sack_object *tmp_objects = (sack_object *) calloc(*object_count, sizeof(sack_object));

	for (int i = 0; i < *object_count; ++i) {
		if (fscanf(fp, "%d %d", &tmp_objects[i].profit, &tmp_objects[i].weight) < 2) {
			free(objects);
			fclose(fp);
			return 0;
		}
	}

	fclose(fp);

	*generations_count = (int) strtol(argv[2], NULL, 10);
	*num_threads = (int) strtol(argv[3], NULL, 10);
	
	if (*generations_count == 0) {
		free(tmp_objects);

		return 0;
	}

	*objects = tmp_objects;

	return 1;
}

void print_objects(const sack_object *objects, int object_count)
{
	for (int i = 0; i < object_count; ++i) {
		printf("%d %d\n", objects[i].weight, objects[i].profit);
	}
}

void print_generation(const individual *generation, int limit)
{
	for (int i = 0; i < limit; ++i) {
		for (int j = 0; j < generation[i].chromosome_length; ++j) {
			printf("%d ", generation[i].chromosomes[j]);
		}

		printf("\n%d - %d\n", i, generation[i].fitness);
	}
}

void print_best_fitness(const individual *generation)
{
	printf("%d\n", generation[0].fitness);
}

void compute_fitness_function(const sack_object *objects, individual *generation, int object_count, int sack_capacity, pthread_barrier_t *barrier, int num_threads, int thread_id)
{
	int weight;
	int profit;

	int start = thread_id * (double)object_count / num_threads;
    int end = min((thread_id + 1) * (double)object_count / num_threads, object_count);

	for (int i = start; i < end; ++i) {
		weight = 0;
		profit = 0;

		for (int j = 0; j < generation[i].chromosome_length; ++j) {
			if (generation[i].chromosomes[j]) {
				weight += objects[j].weight;
				profit += objects[j].profit;
			}
		}
		generation[i].fitness = (weight <= sack_capacity) ? profit : 0;
	}
	pthread_barrier_wait(barrier);
}

int cmpfunc(const void *a, const void *b)
{
	int i;
	individual *first = (individual *) a;
	individual *second = (individual *) b;

	int res = second->fitness - first->fitness; // decreasing by fitness
	if (res == 0) {
		int first_count = 0, second_count = 0;

		for (i = 0; i < first->chromosome_length && i < second->chromosome_length; ++i) {
			first_count += first->chromosomes[i];
			second_count += second->chromosomes[i];
		}

		res = first_count - second_count; // increasing by number of objects in the sack
		if (res == 0) {
			return second->index - first->index;
		}
	}

	return res;
}

int cmpfunc_par(individual first, individual second)
{
	int i;

	int res = second.fitness - first.fitness; // decreasing by fitness
	if (res == 0) {
		int first_count = 0, second_count = 0;

		for (i = 0; i < first.chromosome_length && i < second.chromosome_length; ++i) {
			first_count += first.chromosomes[i];
			second_count += second.chromosomes[i];
		}

		res = first_count - second_count; // increasing by number of objects in the sack
		if (res == 0) {
			return second.index - first.index;
		}
	}

	return res;
}

void merge(individual *arr, int l, int m, int r)
{
    int i, j, k;
    int n1 = m - l + 1;
    int n2 = r - m;
  
    individual *L = (individual *) calloc(n1, sizeof(individual));
    individual *R = (individual *) calloc(n2, sizeof(individual));
  
    for (i = 0; i < n1; i++)
        L[i] = arr[l + i];
    for (j = 0; j < n2; j++)
        R[j] = arr[m + 1 + j];
  
    i = 0;
    j = 0;
    k = l;
    while (i < n1 && j < n2) {
        if (cmpfunc_par(L[i], R[j]) < 0) {
            arr[k] = L[i];
            i++;
        }
        else {
            arr[k] = R[j];
            j++;
        }
        k++;
    }
  
    while (i < n1) {
        arr[k] = L[i];
        i++;
        k++;
    }

    while (j < n2) {
        arr[k] = R[j];
        j++;
        k++;
    }
}

void mergeSort(individual *arr, int l, int r)
{
    if (l < r) {
        int m = l + (r - l) / 2;
  
        mergeSort(arr, l, m);
        mergeSort(arr, m + 1, r);
  
        merge(arr, l, m, r);
    }
}

void mutate_bit_string_1(const individual *ind, int generation_index)
{
	int i, mutation_size;
	int step = 1 + generation_index % (ind->chromosome_length - 2);

	if (ind->index % 2 == 0) {
		// for even-indexed individuals, mutate the first 40% chromosomes by a given step
		mutation_size = ind->chromosome_length * 4 / 10;
		for (i = 0; i < mutation_size; i += step) {
			ind->chromosomes[i] = 1 - ind->chromosomes[i];
		}
	} else {
		// for even-indexed individuals, mutate the last 80% chromosomes by a given step
		mutation_size = ind->chromosome_length * 8 / 10;
		for (i = ind->chromosome_length - mutation_size; i < ind->chromosome_length; i += step) {
			ind->chromosomes[i] = 1 - ind->chromosomes[i];
		}
	}
}

void mutate_bit_string_2(const individual *ind, int generation_index)
{
	int step = 1 + generation_index % (ind->chromosome_length - 2);

	// mutate all chromosomes by a given step
	for (int i = 0; i < ind->chromosome_length; i += step) {
		ind->chromosomes[i] = 1 - ind->chromosomes[i];
	}
}

void crossover(individual *parent1, individual *child1, int generation_index)
{
	individual *parent2 = parent1 + 1;
	individual *child2 = child1 + 1;
	int count = 1 + generation_index % parent1->chromosome_length;

	memcpy(child1->chromosomes, parent1->chromosomes, count * sizeof(int));
	memcpy(child1->chromosomes + count, parent2->chromosomes + count, (parent1->chromosome_length - count) * sizeof(int));

	memcpy(child2->chromosomes, parent2->chromosomes, count * sizeof(int));
	memcpy(child2->chromosomes + count, parent1->chromosomes + count, (parent1->chromosome_length - count) * sizeof(int));
}

void copy_individual(const individual *from, const individual *to)
{
	memcpy(to->chromosomes, from->chromosomes, from->chromosome_length * sizeof(int));
}

void free_generation(individual *generation, pthread_barrier_t *barrier, int num_threads, int thread_id)
{

	int start = thread_id * (double)generation->chromosome_length / num_threads;
    int end = min((thread_id + 1) * (double)generation->chromosome_length / num_threads, generation->chromosome_length);

	for (int i = start; i < end; ++i) {
		free(generation[i].chromosomes);
		generation[i].chromosomes = NULL;
		generation[i].fitness = 0;
	}
	pthread_barrier_wait(barrier);

}

void *run_genetic_algorithm(void *arg1)
{
	int count, cursor, start = 0, end = 0;
	struct arg *aux = arg1;

	start = aux->thread_id * (double) aux->object_count/ aux->num_threads;
	end = min((aux->thread_id + 1) * (double)aux->object_count / aux->num_threads, aux->object_count);

	for (int i = start; i < end; ++i) {
		aux->current_generation[i].fitness = 0;
		aux->current_generation[i].chromosomes = (int*) calloc(aux->object_count, sizeof(int));
		aux->current_generation[i].chromosomes[i] = 1;
		aux->current_generation[i].index = i;
		aux->current_generation[i].chromosome_length = aux->object_count;

		aux->next_generation[i].fitness = 0;
		aux->next_generation[i].chromosomes = (int*) calloc(aux->object_count, sizeof(int));
		aux->next_generation[i].index = i;
		aux->next_generation[i].chromosome_length = aux->object_count;
	}
	pthread_barrier_wait(aux->barrier);

	for (int k = 0; k < aux->generations_count; ++k) {
		pthread_barrier_wait(aux->barrier);
		cursor = 0;

		compute_fitness_function(aux->objects, aux->current_generation, aux->object_count, aux->sack_capacity, aux->barrier, aux->num_threads, aux->thread_id);
		
		start = aux->thread_id * (double) aux->object_count / aux->num_threads;
		end = min((aux->thread_id + 1) * (double)aux->object_count / aux->num_threads, aux->object_count);
		
		mergeSort(aux->current_generation, start, end-1);
		pthread_barrier_wait(aux->barrier);
		int ind = 0, i = 0;

		if(aux->num_threads > 1 && aux->thread_id == 0)
		{
			if(aux->num_threads == 2)
			{
				merge(aux->current_generation, 0, (aux->object_count - 1) / 2, aux -> object_count - 1);
			}
			else if(aux->num_threads == 3)
			{
				merge(aux->current_generation, 0, (aux->object_count - 1) / 4, (aux->object_count - 1) / 2);
				merge(aux->current_generation, 0, (aux->object_count - 1) / 2 + 1, aux->object_count -1);
			}
			else
			{	
				for(i = 1; i < aux->num_threads; i++)
				{
					ind = min((i + 1) * (double)(aux->object_count) / aux -> num_threads, aux->object_count - 1);
					merge(aux->current_generation, 0, ind / 2 , ind);
				}
			}
		}
		pthread_barrier_wait(aux->barrier);
		

		count = aux->object_count * 3 / 10;

		start = aux->thread_id * (double) count/ aux->num_threads;
		end = min((aux->thread_id + 1) * (double)count / aux->num_threads, count);

		for (int i = start; i < end; ++i) {
			copy_individual(aux->current_generation + i, aux->next_generation + i);
		}
		pthread_barrier_wait(aux->barrier);
		cursor = count;

		// mutate first 20% children with the first version of bit string mutation
		count = aux->object_count * 2 / 10;

		start = aux->thread_id * (double) count/ aux->num_threads;
		end = min((aux->thread_id + 1) * (double)count / aux->num_threads, count);

		for (int i = start; i < end; ++i) {
			copy_individual(aux->current_generation + i, aux->next_generation + cursor + i);
			mutate_bit_string_1(aux->next_generation + cursor + i, k);
		}
		pthread_barrier_wait(aux->barrier);
		cursor += count;

		// mutate next 20% children with the second version of bit string mutation
		count = aux->object_count * 2 / 10;

		start = aux->thread_id * (double) count/ aux->num_threads;
		end = min((aux->thread_id + 1) * (double)count / aux->num_threads, count);

		for (int i = start; i < end; ++i) {
			copy_individual(aux->current_generation + i + count, aux->next_generation + cursor + i);
			mutate_bit_string_2(aux->next_generation + cursor + i, k);
		}
		pthread_barrier_wait(aux->barrier);
		cursor += count;

		// crossover first 30% parents with one-point crossover
		// (if there is an odd number of parents, the last one is kept as such)
		count = aux->object_count * 3 / 10;

		if (count % 2 == 1) {
			copy_individual(aux->current_generation + aux->object_count - 1, aux->next_generation + cursor + count - 1);
			count--;
		}
		pthread_barrier_wait(aux->barrier);

		start = aux->thread_id * (double) count/ aux->num_threads;
		end = min((aux->thread_id + 1) * (double)count / aux->num_threads, count);
		int start_par = (start % 2) ? start + 1 : start;

		for (int i = start_par; i < count - 1 && i < end; i += 2) {
			crossover(aux->current_generation + i, aux->next_generation + cursor + i, k);
		}
		pthread_barrier_wait(aux->barrier);

		// switch to new generation
		aux->tmp = aux->current_generation;
		aux->current_generation = aux->next_generation;
		aux->next_generation = aux->tmp;
		pthread_barrier_wait(aux->barrier);

		start = aux->thread_id * (double) aux->object_count/ aux->num_threads;
		end = min((aux->thread_id + 1) * (double)aux->object_count / aux->num_threads, aux->object_count);

		for (int i = start; i < end; ++i) {
			aux->current_generation[i].index = i;
		}
		pthread_barrier_wait(aux->barrier);

		if (k % 5 == 0 && aux->thread_id == 0) {
			print_best_fitness(aux->current_generation);
		}
	}
	pthread_barrier_wait(aux->barrier);
	compute_fitness_function(aux->objects, aux->current_generation, aux->object_count, aux->sack_capacity, aux->barrier, aux->num_threads, aux->thread_id);
	start = aux->thread_id * (double) aux->object_count / aux->num_threads;
	end = min((aux->thread_id + 1) * (double) aux->object_count / aux->num_threads, aux->object_count);
	mergeSort(aux->current_generation, start, end-1);
	pthread_barrier_wait(aux->barrier);
		
	int ind = 0, i = 0;
	if(aux->num_threads > 1 && aux->thread_id == 0)
	{
		if(aux->num_threads == 2)
		{
			merge(aux->current_generation, 0, (aux -> object_count - 1)/ 2, aux -> object_count - 1);
		}
		else if(aux->num_threads == 3)
			{
				merge(aux->current_generation, 0, (aux->object_count - 1) / 4, (aux->object_count - 1) / 2);
				merge(aux->current_generation, 0, (aux->object_count - 1) / 2 + 1, aux->object_count -1);
			}
		else
		{	
			for(i = 1; i < aux->num_threads; i++)
			{
				ind = min((i + 1) * (double)(aux->object_count) / aux->num_threads, aux->object_count - 1);
				merge(aux->current_generation, 0, ind / 2 , ind);
			}
		}
	}
	pthread_barrier_wait(aux->barrier);
	if(aux->thread_id == 0) print_best_fitness(aux->current_generation);

	// free resources for old generation
	free_generation(aux->current_generation, aux->barrier, aux->num_threads, aux->thread_id);
	//pthread_barrier_wait(aux->barrier);
	free_generation(aux->next_generation, aux->barrier, aux->num_threads, aux->thread_id);
	//pthread_barrier_wait(aux->barrier);

	// free resources
	if(aux->thread_id == 0)
	{	
	  	free(aux->current_generation);
	  	free(aux->next_generation);
	}
	pthread_exit(NULL);
	return NULL;
}
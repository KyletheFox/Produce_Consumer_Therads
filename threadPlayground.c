#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void *someFunc(void*);

// Global Variables
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
int counter = 0;

int main(int argc, char const *argv[])
{
	
	// Local Variables
	pthread_t thread[20];
	int i, j;

	// Create Threads
	for (i = 0; i < 20; ++i)	{
		pthread_create(&thread[i], NULL, someFunc, NULL);
	}

	// Join threads to synchronize threads
	for (j = 0; j < 20; j++) {	
		pthread_join(thread[0], NULL);
	}

	printf("Done with threads, back in main\n");

	return 0;
}

void *someFunc(void *nothing) {
	pthread_mutex_lock(&mutex1);
	counter++;
	pthread_mutex_unlock(&mutex1);
	printf("Counter is %d and thread number is %ld\n", counter, pthread_self());
}
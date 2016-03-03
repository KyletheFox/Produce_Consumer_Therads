#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

#define NUM_PROD_THREADS 85
#define NUM_CONS_THREADS 62
#define BUF_SIZE 25

typedef struct {
	int start;
	int end;
	char buffer[BUF_SIZE];
	sem_t bufMutex;
	sem_t slots;
	sem_t items;
} memBuf;

// Global Variables
memBuf theBuf;

void initBuf(memBuf*);
void produce(memBuf*);
void consume(memBuf*);

int main(int argc, char const *argv[])
{
	pthread_t proThread[NUM_PROD_THREADS];
	pthread_t conThread[NUM_CONS_THREADS];
	int i,j;

	// Initalize memBuf
	initBuf(&theBuf);

	// Create producer threads
	for (i = 0; i < NUM_CONS_THREADS || i < NUM_PROD_THREADS; ++i)	{
		if (i < NUM_PROD_THREADS)	{
			pthread_create(&proThread[i], NULL, produce, (void*)&theBuf);
		}
		if (i < NUM_CONS_THREADS)	{
			pthread_create(&conThread[i], NULL, consume, (void*)&theBuf);
		}
	}

	// Join Threads
	for (j = 0; j < NUM_CONS_THREADS || j < NUM_PROD_THREADS; ++j)	{
		if (j < NUM_PROD_THREADS)	{
			pthread_join(proThread[j], NULL);
		}
		if (j < NUM_CONS_THREADS)	{
			pthread_join(conThread[j], NULL);
		}
	}

	printf("Done\n");

	return 0;
}

void initBuf(memBuf *buf) {
	buf -> start = 0;
	buf -> end = 0;
	sem_init(&buf->bufMutex, 0, 1);
	sem_init(&buf->slots, 0, BUF_SIZE);
	sem_init(&buf->items, 0, 0);
}

void consume(memBuf *buf) {
	int removeItem = 0;
	
	sem_wait(&buf -> items);
	sem_wait(&buf -> bufMutex);

	buf -> buffer[(buf -> start) % BUF_SIZE] = removeItem;
	buf -> start += 1;
	
	sem_post(&buf -> bufMutex);
	sem_post(&buf -> slots);

	printf("consume thread %ld consumed item number %d\n", pthread_self(), (buf -> start - 1) % BUF_SIZE);
}

void produce(memBuf *buf) {
	int produceItem = 1;
	
	sem_wait(&buf -> slots);
	sem_wait(&buf -> bufMutex);
	
	buf -> buffer[(buf -> end) % BUF_SIZE] = produceItem;
	buf -> end += 1;
	
	sem_post(&buf -> bufMutex);
	sem_post(&buf -> items);

	printf("consume thread %ld produceItem item number %d\n", pthread_self(), (buf -> end - 1) % BUF_SIZE);
}
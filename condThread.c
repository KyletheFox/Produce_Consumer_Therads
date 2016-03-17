#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>

#define NUM_PROD_THREADS 385
#define NUM_CONS_THREADS 362
#define BUF_SIZE 25

typedef struct {
	int start;
	int end;
	char buffer[BUF_SIZE];
	pthread_mutex_t bufMutex;
	pthread_cond_t empty;
	pthread_cond_t full;
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
	pthread_mutex_init(&buf->bufMutex, NULL);
	pthread_cond_init(&buf->full, NULL);
	pthread_cond_init(&buf->empty, NULL);
}

void consume(memBuf *buf) {
	int removeItem = 0;
	
	pthread_mutex_trylock(&buf -> bufMutex);
	while((&buf->end - &buf->start) == 0) {
		pthread_cond_wait(&buf->empty, &buf->bufMutex);
	}

	buf -> buffer[(buf -> start) % BUF_SIZE] = removeItem;
	buf -> start += 1;
	
	printf("consume thread %ld consumed item number %d\n", pthread_self(), (buf -> start - 1) % BUF_SIZE);
	
	pthread_cond_signal(&buf->full);
	pthread_mutex_unlock(&buf->bufMutex);

	
}

void produce(memBuf *buf) {
	int produceItem = 1;
	
	pthread_mutex_trylock(&buf->bufMutex);
	while((&buf->end - &buf->start) == BUF_SIZE - 1) {
		pthread_cond_wait(&buf->full, &buf->bufMutex);
	}

	buf -> buffer[(buf -> end) % BUF_SIZE] = produceItem;
	buf -> end += 1;

	printf("producer thread %ld produceItem item number %d\n", pthread_self(), (buf -> end - 1) % BUF_SIZE);

	pthread_cond_signal(&buf->empty);
	pthread_mutex_unlock(&buf->bufMutex);

}
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>

#define NUM_PROD_THREADS 385
#define NUM_CONS_THREADS 362
#define BUF_SIZE 25

/*
	Struct to hold all variables need for the
	buffer to work to spec.

		Start - Holds index of the first item in
			the queue(buffer) to be consumed

		End - Holds index of the next spot for an item 
			to be produced at the end of the queue

		Buffer - Where the items are stored and consumed
			from

		BufMutex - The mutex lock which only allows one
			producer or consumer thread to enter the 
			buffer. 

		Empty - The condition variable for telling the
			monitor that the buffer is empty so don't
			allow any consumers inside the buffer. 
			Producers can still fill up the buffer.

		Full - The condition variable responsilbe for
			alerting the monitor that the buffer is full.
			the montior should not allow any more 
			producers into the thread.
*/

typedef struct {
	int start;
	int end;
	char buffer[BUF_SIZE];
	pthread_mutex_t bufMutex;
	pthread_cond_t empty;
	pthread_cond_t full;
} memBuf;

// Global Variables
memBuf theBuf;		// The buffer struct
FILE *fp;			// Log File

// Functiion Headers
void initBuf(memBuf*);
void initLog();
void produce(memBuf*);
void consume(memBuf*);

int main(int argc, char const *argv[])
{
	pthread_t proThread[NUM_PROD_THREADS];		// Array of producer threads
	pthread_t conThread[NUM_CONS_THREADS];		// Array of consumer threads
	int i,j;									// Loop counters

	// Initalize memBuf and log file
	initBuf(&theBuf);
	initLog();

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

	// Close file
	fclose(fp);

	// Alert user operation has finished
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

void initLog() {

	/*
		This function opens the file and checks for any errors. If
		there is an error opening file, it prints to console
	*/

	if ((fp = fopen("log.txt", "w")) == NULL) {
		printf("Cannot open file.\n");
	}
}

void consume(memBuf *buf) {
	int removeItem = 0;
	
	pthread_mutex_trylock(&buf -> bufMutex);
	while((&buf->end - &buf->start) == 0) {
		pthread_cond_wait(&buf->empty, &buf->bufMutex);
	}

	buf -> buffer[(buf -> start) % BUF_SIZE] = removeItem;
	buf -> start += 1;
	
	fprintf(fp, "Consumed in slot: %d\n", (buf -> start - 1) % BUF_SIZE);
	
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

	fprintf(fp, "Produced in slot: %d\n", (buf -> end - 1) % BUF_SIZE);

	pthread_cond_signal(&buf->empty);
	pthread_mutex_unlock(&buf->bufMutex);

}
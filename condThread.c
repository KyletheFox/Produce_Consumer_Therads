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
	int i,j;								// Loop counters

	// Initalize memBuf and log file
	initBuf(&theBuf);
	initLog();

	// Create producer threads
	for (i = 0; i < NUM_CONS_THREADS || i < NUM_PROD_THREADS; ++i)	{
		if (i < NUM_PROD_THREADS)	{

			// Creating all the producer threads
			pthread_create(&proThread[i], NULL, produce, (void*)&theBuf);
		}
		if (i < NUM_CONS_THREADS)	{

			// Creating all the consumer threads
			pthread_create(&conThread[i], NULL, consume, (void*)&theBuf);
		}
	}

	// Join Threads
	for (j = 0; j < NUM_CONS_THREADS || j < NUM_PROD_THREADS; ++j)	{
		if (j < NUM_PROD_THREADS)	{

			// Wait for the producer threads too finish before continuing.
			pthread_join(proThread[j], NULL);
		}
		if (j < NUM_CONS_THREADS)	{

			// Waiting for all the consumer threads too finish before continuing
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
	/*
		initBuf - This function is designed to initalize the global
			buffer struct. (memBuf) It creates the two conditions 
			and a mutex lock inside the struct. The description of 
			these locks is located above the struct typedef.

			This function also fills in the default start and
			end points of the buffer
	*/

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
	/*
		This function is what each of the consumer threads run once
		they are created. To represent a slot in the buffer that has
		been consumer, I put the value of 0 into the index of the 
		array that has been consumed. 

		Each thread will consume 20 items from the array. This is
		accomplished by looping the entire function 20 times.

		The way the function works is by using a montior. The thread
		first attempts to get the mutex lock. Once the thread aquires
		the lock, it uses a while loop to continously check if the 
		buffer is not empty. If it is empty it calls the conditional
		variable function wait. This function contiuniously checks to 
		see if the buffer is empty.

		Once the buffer become not ready, the thread is placed into the
		wait queue holding the mutex lock. The consumer thread then 
		consumes the start index of the buffer and prints a message to
		the log file.

		To finish, the thread signals the buffer full condition variable
		to allow a producer to access the buffer and unlocks buffer for
		another thread to access the buffer.
	*/

	int removeItem = 0;		// Represention value that index has been consumed
	int k;					// Loop Counter

	for (k = 0; k < 20; ++k)	{	// Loop to consume 20 elelemnts
	
		pthread_mutex_trylock(&buf -> bufMutex);	// Tries to get access to buffer

		while((&buf->end - &buf->start) == 0) {	
			/*
				Loop to check for when buffer is not empty. If the buffer
				is empty, it signals the monitor to wait for the condition
				variable of empty to any producer threads and gives them
				the mutex to access the buffer.

				If the buffer is not empty, it exits the loop with the lock
				in hand
			*/
			pthread_cond_wait(&buf->empty, &buf->bufMutex);
		}

		buf -> buffer[(buf -> start) % BUF_SIZE] = removeItem; 	// Place zero into buffer
		buf -> start += 1;										// Increments the start index
		
		fprintf(fp, "Consumed in slot: %d\n", (buf -> start - 1) % BUF_SIZE);	// Output to lof file
		
		pthread_cond_signal(&buf->full);		// Signal any producer threads.
		pthread_mutex_unlock(&buf->bufMutex);	// Unlocks buffer for another thread

	}

}

void produce(memBuf *buf) {
	/*
		This function is the function that will ran by the producer threads.
	*/
	
	int produceItem = 1;		// Value to be placed in buffer to represent a filled slot
	int l;						// Loop Counter
	
	for (l = 0; l < 20; ++l)	{

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

}
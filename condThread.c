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
		This function is the function that will run by the producer threads. The 
		producer thread places a 1 in an empty index. This 1 represents a slot
		that has been filled b a producer.

		Each producer thread will produce 20 different items inside the buffer 
		by using a for loop.

		The producer thread works by first attempting to get the mutex lock for
		the buffer. Once it has the lock it enters into a while loop which 
		spins until the buffer is not full. If the buffer is full, it waits
		for a change in the full conditon to change and once it does it 
		wakes back up with the buffer lock. 

		The producer then places the item into the index represented by the 
		end value inside the memBuf struct and increments the end value. A
		message is then printed out to the log file. Thh thread then signals
		any threads wait for a change on the empty condition and releases the
		mutex lock.

	*/
	
	int produceItem = 1;		// Value to be placed in buffer to represent a filled slot
	int l;						// Loop Counter
	
	for (l = 0; l < 20; ++l)	{			// Loop to produce 20 items

		pthread_mutex_trylock(&buf->bufMutex);					// Attempts to get the mutex locks
		while((&buf->end - &buf->start) == BUF_SIZE - 1) {
			/*
				This loops continues to check if the buffer is full. Inside
				the loop, the thread waits for the full condition to change
				and when it does it wakes up with the mutex lock.
			*/		
			pthread_cond_wait(&buf->full, &buf->bufMutex);
		}

		buf -> buffer[(buf -> end) % BUF_SIZE] = produceItem;		// Place item in buffer
		buf -> end += 1;											// Increment end index

		fprintf(fp, "Produced in slot: %d\n", (buf -> end - 1) % BUF_SIZE);		// Print msg to log file

		pthread_cond_signal(&buf->empty);			// Signal empty conditon
		pthread_mutex_unlock(&buf->bufMutex);		// Release buffer mutex lock

	}

}
#include <Windows.h>
#include <stdio.h>

#define NUM_PROD_THREADS 5
#define NUM_CONS_THREADS 4
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

		LogBuff - Temporary buffer to hold log input.
			This is changed inside each thread and must be
			locked.

		BufMutex - The mutex lock which only allows one
			producer or consumer thread to enter the 
			buffer. 

		Slots - The semaphore that locks the empty slots.
			If there are no slot locks left, producer
			threads will not be able to access the buffer.
			This means that the buffer is full and there 
			aren't any open slots to produce in.

		Items - The semaphore lock that lock the filled
			slots. If there are no Items in the buffer,
			conusmer threads will not be able to access 
			the buffer. This means there are no items to 
			consume.
*/
typedef struct {
	int start;
	int end;
	char buffer[BUF_SIZE];
	char *logBuff;
	HANDLE bufMutex;
	HANDLE slots;
	HANDLE items;
} memBuf;

// Global Variables
memBuf theBuf;		// The buffer object
FILE *fp;			// Log File tho print to

/*
	Two different types of threads
		1. proThread for producers
		2. conThread for consumers
*/
HANDLE proThread;
HANDLE conThread;

// Function Headers
void initBuf(memBuf*);
void initLog();
DWORD WINAPI produce(LPVOID *lpParam);
DWORD WINAPI consume(LPVOID *lpParam);

int main(int argc, char const *argv[])
{	
	
	HANDLE proThread[NUM_PROD_THREADS]; 	// Array of producer threads 
	HANDLE conThread[NUM_CONS_THREADS];		// Array of consumer threads
	int i,j,k,l;							// Loop Counters

	// Initalize memBuf and log file
	initBuf(&theBuf);
	initLog();

	// Create producer and consumer threads
	for (i = 0; i < NUM_CONS_THREADS || i < NUM_PROD_THREADS; ++i)	{
		
		if (i < NUM_PROD_THREADS)	{
			proThread[i] = CreateThread(
					NULL,                   // default security attributes
			        0,                      // use default stack size  
			        produce,       			// thread function name
			        &theBuf,          		// argument to thread function 
			        0,                      // use default creation flags 
			        NULL  					// returns the thread identifier 
				);
		}

		if (i < NUM_CONS_THREADS)	{
			conThread[i] = CreateThread(
					NULL,                   // default security attributes
			        0,                      // use default stack size  
			        consume,       			// thread function name
			        &theBuf,          		// argument to thread function 
			        0,                      // use default creation flags 
			        NULL  					// returns the thread identifier 
				);
		}
	}

	// Wait for all the threads to finish executing 
	WaitForMultipleObjects(NUM_PROD_THREADS, proThread, TRUE, INFINITE);
	WaitForMultipleObjects(NUM_CONS_THREADS, conThread, TRUE, INFINITE);

	// Close thread handles
	CloseHandle(theBuf.bufMutex);
	CloseHandle(theBuf.slots);
	CloseHandle(theBuf.items);

	/* 
		Need for loops to close all threads in the producer and 
		consumer thread arrays to avoid memeory leaks
	*/

	// Closing all onsumer thread handles
	for (k = 0; k < NUM_CONS_THREADS; ++k)	{
		CloseHandle(conThread[k]);
	}

	// Closing all producer thread handles
	for (l = 0; l < NUM_PROD_THREADS; ++l)	{
		CloseHandle(proThread[k]);
	}

	// Close Log File
	fclose(fp);

	printf("Done\n");

	return 0;
}

void initBuf(memBuf *buf) {
	/*
		initBuf - This function is designed to initalize the global
			buffer struct. (memBuf) It creates the two semaphore 
			and mutex locks inside the struct. The description of 
			these locks is located above the struct typedef.

			This function also fills in the default start and
			end points of the buffer
	*/
	
	buf -> bufMutex = CreateMutex(
		NULL,              // default security attributes
		FALSE,             // initial
		NULL);
	
	/*
		The slots semaphore's inital value is size of the buffer
		array - 1. This is because all the slots are open so
		all the locks for the slots are available. Producers
		are able to get locks for all of the available slots. 
	*/
	buf -> slots = CreateSemaphore(
		NULL,           // default security attributes
		BUF_SIZE - 1,   // initial count
		BUF_SIZE - 1,   // maximum count
		NULL);          // unnamed semaphore

	/*
		The item's semaphore's inital value starts out at 0. This 
		is because all the slots are empty so the there are no
		locks available for consumers to consume.
	*/
	buf -> items = CreateSemaphore(
		NULL,           // default security attributes
		0,  			// initial count
		BUF_SIZE - 1,  	// maximum count
		NULL);          // unnamed semaphore

	buf -> start = 0;
	buf -> end = 0;

}

void initLog() {
	/*
		This function opens the file and checks for any errors. If
		there is an error, it prints to console
	*/

	if ((fp = fopen(".\\log.txt", "w")) == NULL) {
		printf("Cannot open file.\n");
	}
}

DWORD WINAPI consume(LPVOID *lpParam) {
	/*
		This is the function that in run by all the consumer threads. The function
		gets a lock from the items semaphore and then waits until it gets access to
		the buffer (mutex lock). One it gets access to the buffer, it replaces the
		value (1) with 0. The 0 represents an open slot. While it still has control
		of the buffer, it increments the start by one. This is the position what the 
		next slot with a item in it.

		Once it finished with a the buffer it releases the mutex lock and also opens
		a lock in the slots semaphore. This allows the producer threads to write to
		the slot that the consumer just consumed.
	*/

	int m;
	int removeItem = 0;					// Default value left over after consuming item
	memBuf *buf = (memBuf*)lpParam;		// Casts the arg inputed to function and creates a new memBuf
										// 	struct.

		//Sleep(100);					// Puts the thread to sleep for specified amount of time

		for(m = 0; m < 20; m++) {

		WaitForSingleObject(buf -> items, INFINITE);			// Gets lock for slot with an item in it
		WaitForSingleObject(buf -> bufMutex, INFINITE);			// Gets buffer mutex lock

		/*
			This prints an error message if the consumer thread consumes an
			empty slot. Consumers are not allow to consume items that are
			not filled.	
		*/
		if (buf -> buffer[(buf -> start) % BUF_SIZE] == 0)			
			fprintf(fp, "%s\n", "This is bad: removeItem\n");

		/*
			Prints message to log file. Shows a sucessful consumption and
			the slot which it consumed from.
		*/
		else 
			fprintf(fp, "%s %d\n", "Consumed from:", (buf -> start) % BUF_SIZE);

		buf -> buffer[(buf -> start) % BUF_SIZE] = removeItem;	// Consumes the slot
		buf -> start += 1;										// Increments where the next item to consume
		
		ReleaseMutex(buf -> bufMutex);					// Releases buffer mutex lock
		ReleaseSemaphore(buf -> slots, 1, NULL);		// Releases a slot lock for producers

		}

	return 0;	// Returns nothing. Function requires return value.
}

DWORD WINAPI produce(LPVOID *lpParam) {
	/*
		This is the function that the producer thread runs. This function gets
		a lock from the slots semaphore and waits for the buffer mutex lock to
		be avaiable. Once it has access to the buffer, it produces an item (1)
		an places it into the empty slot. The function checks to make sure that
		it is not producing in a slot that is already filled. The function then 
		produces the item into the slot and increments where the next open slot 
		is.

		After the producer thread is finished with the buffer, it releases the 
		buffer mutex lock and releases a lock for the item semaphore. This allows
		the consumer threads to gain access to that slot and consume the item
		inside the slot.
	*/

	int n;
	int produceItem = 1;				// The item that the prodecer thread creates
	memBuf *buf = (memBuf*)lpParam;		// Cast the function input arg to a memBuf struct
										//		--- This is the global memBuf struct ---

		for(n = 0; n < 20; n++) {

		WaitForSingleObject(buf -> slots, INFINITE);		// Gets lock from slots semaphore
		WaitForSingleObject(buf -> bufMutex, INFINITE);		// Get buffer mutex lock

		/*
			This prints an error message if the producerthread produces in an
			filled slot. Procuders are not allow to produce an item inside a slot
			that is already filled.
		*/
		if (buf -> buffer[(buf -> end) % BUF_SIZE] == 1) 		
			fprintf(fp, "This is bad: produceItem\n");

		/*
			If slot is empty, prints message to the log file that
			says it produced an item and the slot that it produced
			in.
		*/
		else
			fprintf(fp, "Produced in: %d\n", (buf -> end) % BUF_SIZE);
		
		buf -> buffer[(buf -> end) % BUF_SIZE] = produceItem;	// Produces the item in the correct slot
		buf -> end += 1;										// Increments where the next empty slot is.

		ReleaseMutex(buf -> bufMutex);				// Releases the buffer mutex lock
		ReleaseSemaphore(buf -> items, 1, NULL);	// Releases a lock in the items semaphore for consumer threads.		
	
		}

	return 0;		// Returns nothing. Function requires return value.
}
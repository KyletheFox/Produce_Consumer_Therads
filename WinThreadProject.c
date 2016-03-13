#include <Windows.h>
#include <stdio.h>

#define NUM_PROD_THREADS 60
#define NUM_CONS_THREADS 50
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
	HANDLE bufMutex;
	HANDLE slots;
	HANDLE items;
} memBuf;

// Global Variables
memBuf theBuf;		// The buffer object
HANDLE file;		// Log File to print to

/*
	Two different types of threads
		1. proThread for producers
		2. conThread for consumers
*/
HANDLE proThread;
HANDLE conThread;

// Function Headers
void initBuf(memBuf*);
void initLog(HANDLE*);
DWORD WINAPI produce(LPVOID *lpParam);
DWORD WINAPI consume(LPVOID *lpParam);

int main(int argc, char const *argv[])
{	
	
	HANDLE proThread[NUM_PROD_THREADS]; 	// Array of producer threads 
	HANDLE conThread[NUM_CONS_THREADS];		// Array of consumer threads
	int i,j,k,l;							// Loop Counters

	// Initalize memBuf and log file
	initBuf(&theBuf);
	initLog(&file);

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

	// Close File Handle
	CloseHandle(file);

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

void initLog(HANDLE *fp) {
	/*
		This function opens the file and checks for any errors. If
		an error is found it returns.
	*/

	// Open Log File for writing too
	*fp = CreateFile(
			"log.txt", 				// File Name
			GENERIC_WRITE,			// Write Permissions only
			0,						// No sharing 
			NULL,					// Default security
			CREATE_ALWAYS,			// Always make new file, Overwrite any existing
			FILE_ATTRIBUTE_NORMAL,	// Normal File
			NULL					// No File attr
		);
 
 	// Check for creating file error
	if (file = INVALID_HANDLE_VALUE) 	{
        printf(TEXT("Terminal failure: Unable to open /log.txt for write.\n"));
        return;
	}
}

DWORD WINAPI consume(LPVOID *lpParam) {
	int removeItem = 0;

	memBuf *buf = (memBuf*)lpParam;

		Sleep(100);

		WaitForSingleObject(buf -> items, INFINITE);
		WaitForSingleObject(buf -> bufMutex, INFINITE);

		if (buf -> buffer[(buf -> start) % BUF_SIZE] == 0)	{
			printf("This is bad: removeItem\n");
		}

		buf -> buffer[(buf -> start) % BUF_SIZE] = removeItem;
		buf -> start += 1;
		
		ReleaseMutex(buf -> bufMutex);
		ReleaseSemaphore(buf -> slots, 1, NULL);
		
		printf("Consumed from: %d\n", (buf -> start - 1) % BUF_SIZE);

	return 0;
}

DWORD WINAPI produce(LPVOID *lpParam) {
	int produceItem = 1;
	//printf("I printed in a thread %d\n", produceItem);

	memBuf *buf = (memBuf*)lpParam;

		//sem_wait(&buf -> slots);
		WaitForSingleObject(buf -> slots, INFINITE);
		WaitForSingleObject(buf -> bufMutex, INFINITE);

		if (buf -> buffer[(buf -> end) % BUF_SIZE] == 1)
		{
			printf("This is bad: produceItem\n");
		}
		
		buf -> buffer[(buf -> end) % BUF_SIZE] = produceItem;
		buf -> end += 1;

		ReleaseMutex(buf -> bufMutex);
		ReleaseSemaphore(buf -> items, 1, NULL);
		//sem_post(&buf -> items);
		printf("Produced in: %d\n", (buf -> end - 1) % BUF_SIZE);	
	
	return 0;
}
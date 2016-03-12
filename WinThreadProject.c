#include <Windows.h>
#include <stdio.h>

#define NUM_PROD_THREADS 60
#define NUM_CONS_THREADS 50
#define BUF_SIZE 25

HANDLE bufMutex;
HANDLE slots;
HANDLE items;
HANDLE proThread;
HANDLE conThread;

typedef struct {
	int start;
	int end;
	char buffer[BUF_SIZE];
	HANDLE bufMutex;
	HANDLE slots;
	HANDLE items;
} memBuf;

// Global Variables
memBuf theBuf;

void initBuf(memBuf*);
DWORD WINAPI produce(LPVOID *lpParam);
DWORD WINAPI consume(LPVOID *lpParam);

int main(int argc, char const *argv[])
{	
	
	HANDLE proThread[NUM_PROD_THREADS];
	HANDLE conThread[NUM_CONS_THREADS];
	int i,j;

	// Initalize memBuf
	initBuf(&theBuf);

	//printf("%d\n", theBuf.start);

	// Create producer threads
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

			//pthread_create(&proThread[i], NULL, produce, (void*)&theBuf);
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
			//pthread_create(&conThread[i], NULL, consume, (void*)&theBuf);
		}
	}

	WaitForMultipleObjects(NUM_PROD_THREADS, proThread, TRUE, INFINITE);
	WaitForMultipleObjects(NUM_CONS_THREADS, conThread, TRUE, INFINITE);

	CloseHandle(buf.bufMutex);
	CloseHandle(buf.slots);
	CloseHandle(buf.items);

	printf("Done\n");

	return 0;
}

void initBuf(memBuf *buf) {
	
	buf -> bufMutex = CreateMutex(
		NULL,              // default security attributes
		FALSE,             // initially not owned
		NULL);
	
	buf -> slots = CreateSemaphore(
		NULL,           // default security attributes
		BUF_SIZE - 1,   // initial count
		BUF_SIZE - 1,   // maximum count
		NULL);          // unnamed semaphore

	buf -> items = CreateSemaphore(
		NULL,           // default security attributes
		0,  			// initial count
		BUF_SIZE - 1,  		// maximum count
		NULL);          // unnamed semaphore

	buf -> start = 0;
	buf -> end = 0;

	/* Linux sem initalizers
	sem_init(&buf->bufMutex, 0, 1);
	sem_init(&buf->slots, 0, BUF_SIZE);
	sem_init(&buf->items, 0, 0);
	*/
}



DWORD WINAPI consume(LPVOID *lpParam) {
	int removeItem = 0;
	//printf("I printed in another thread %d\n", removeItem);

	memBuf *buf = (memBuf*)lpParam;

		Sleep(100);

		//sem_wait(&buf -> items);
		WaitForSingleObject(buf -> items, INFINITE);
		WaitForSingleObject(buf -> bufMutex, INFINITE);

		if (buf -> buffer[(buf -> start) % BUF_SIZE] == 0)
		{
			printf("This is bad: removeItem\n");
		}
		buf -> buffer[(buf -> start) % BUF_SIZE] = removeItem;
		buf -> start += 1;
		
		ReleaseMutex(buf -> bufMutex);
		ReleaseSemaphore(buf -> slots, 1, NULL);
		//sem_post(&buf -> slots);
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

/* 	Linux Version of creating and waiting for threads
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

	*/

	/*	Linux Version

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

	*/
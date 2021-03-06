#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <math.h>
#include <sys/types.h>
#include <sys/ipc.h> 
#include <sys/shm.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#define QUEUE_SIZE 10

//Type of the circular queue elements 
typedef unsigned long QueueElem;

//semaphore for sinalizing the prime calculation end
sem_t primeCalculation;

//number chosen by the user, primes calculated up to this value
int maxNumber = 0;

//number of active threads
int threadsRunning = 0;

//pointer to the first available position in the
//final primes array
int arrayPointer = 0;

//mutexes for managing the writing in the shared memory
//and the number of threads counter
pthread_mutex_t sharedMemoryMutex;
pthread_mutex_t threadsRunningMutex;

//prototypes
void *initial_thread(void *arg);
void *filter_thread(void *arg);
int compareFunction(const void * a, const void * b);
 
//------------------------------------------------------------------------------------------ 
// Struct for representing a "circular queue" 
// Space for the queue elements will be allocated dinamically by queue_init() 
typedef struct 
{ 
	QueueElem *v; // pointer to the queue buffer unsigned int capacity; // queue capacity 
	unsigned int capacity;
	unsigned int first; // head of the queue 
	unsigned int last; // tail of the queue 
	sem_t empty; // semaphores and mutex for implementing the 
	sem_t full; // producer-consumer paradigm
	pthread_mutex_t mutex; 
} CircularQueue; 
 
//------------------------------------------------------------------------------------------ 
// Allocates space for circular queue 'q' having 'capacity' number of elements 
// Initializes semaphores & mutex needed to implement the producer-consumer paradigm 
// Initializes indexes of the head and tail of the queue 
// Returns 0 for success, 1 for failed queue initialization
int queue_init(CircularQueue **q, unsigned int capacity) // TO DO: change return value 
{ 
	*q = (CircularQueue *) malloc(sizeof(CircularQueue));
	if(q == NULL) {
		return 1;
	}

	sem_init(&((*q)->empty), 0, capacity); 
	sem_init(&((*q)->full), 0, 0); 
	pthread_mutex_init(&((*q)->mutex), NULL);

	(*q)->v = (QueueElem *) malloc(capacity * sizeof(QueueElem));
	if((*q)->v == NULL) {
		return 1;
	}

	(*q)->capacity = capacity; 
	(*q)->first = 0; 
	(*q)->last = 0;

	return 0;
} 
 
//------------------------------------------------------------------------------------------ 
// Inserts 'value' at the tail of queue 'q' 
void queue_put(CircularQueue *q, QueueElem value) 
{
	sem_wait((sem_t *) &q->empty);
	pthread_mutex_lock(&q->mutex);

    q->v[q->last] = value;
	q->last = (q->last+1)%q->capacity;

	pthread_mutex_unlock(&q->mutex);
	sem_post((sem_t *) &q->full);
} 
 
//------------------------------------------------------------------------------------------ 
// Removes element at the head of queue 'q' and returns its 'value' 
QueueElem queue_get(CircularQueue *q) 
{	
	sem_wait((sem_t *) &q->full);
	pthread_mutex_lock(&q->mutex);

	QueueElem value;
	value = q->v[q->first];
	q->first = (q->first+1)%q->capacity;

	pthread_mutex_unlock(&q->mutex);
	sem_post((sem_t *) &q->empty);
	return value;
} 
 
//------------------------------------------------------------------------------------------ 
// Frees space allocated for the queue elements and auxiliary management data 
// Must be called when the queue is no longer needed
void queue_destroy(CircularQueue *q) 
{ 
	free(q->v);
} 

int main(int argc, char *argv[]) {

	if(argc != 2) {
		printf("Usage: ./primes <number>\n");
		exit(-1);
	}

	maxNumber = strtol(argv[1], NULL, 0);

	double neededSpace = 1.2 * (maxNumber / log(maxNumber));
	long neededSpaceAproximation = ceil(neededSpace) + 1;

	sem_init(&primeCalculation, 0, 0); 

	key_t key; 
	int shmid;
	char *data;

	//allocates the required space on a shared memory segment
	key = ftok("primesMemory", 0);
	if ((shmid = shmget(key, sizeof(QueueElem)*neededSpaceAproximation, IPC_CREAT | IPC_EXCL | SHM_R | SHM_W)) == -1) {
    	perror("shmget");
    	exit(1);
    }

    // attach to the segment to get a pointer to it //
  	data = (char *) shmat(shmid, 0, 0);
    if (data == (char *) -1) {
    	perror("shmat");
    	exit(1);
    }

	//closes the pointer
    if (shmdt((char*)data) == -1) {
        perror("shmdt1");
		exit(1);	
	}

	pthread_mutex_init(&sharedMemoryMutex, NULL);
	pthread_mutex_init(&threadsRunningMutex, NULL);

	//creates the initial thread
	pthread_t initialThread;
	pthread_create(&initialThread, NULL, initial_thread, NULL);
	////pthread_join(initialThread, NULL); 

	sem_wait(&primeCalculation);

	int *pt;
	key = ftok("primesMemory", 0); /* uses the same key */ 
	shmid = shmget(key, 0, 0); 
	pt = (int *) shmat(shmid, 0, 0);
	
	//sorts the final primes array
	qsort(pt,arrayPointer,sizeof(int),compareFunction);	

	int i;
	printf("%d calculated primes up to %d: \n\n", arrayPointer, maxNumber);
	for(i=0; i<arrayPointer; i++) {
		printf("%d\n", pt[i]);
	}

	//clears the shared memory segment
	if (shmctl(shmid, IPC_RMID, 0)) {
        perror("shmctl");
		exit(1);	
	}

	//detaches the pointer from the shared memory segment
	if (shmdt((char*)pt) == -1) {
        perror("shmdt1");
		exit(1);	
	}

	return 0;
}

void *initial_thread(void *arg) 
{
	key_t key;
	int shmid;
	int *pt;

	key = ftok("primesMemory", 0); /* uses the same key */ 
	shmid = shmget(key, 0, 0); 
	pt = (int *) shmat(shmid, 0, 0);

	//sets the next prime pointer to the first available position
	arrayPointer=0;

	//sets the first prime as 2
	pt[ arrayPointer ] = 2;

	//increments the pointer
	arrayPointer++;

	//close the shared memory
	if (shmdt((char*)pt) == -1) {
        perror("shmdt1");
		exit(1);	
	}

	if(maxNumber>2) {
		CircularQueue *q;
		queue_init(&q,QUEUE_SIZE);

		pthread_t filterThread;
		pthread_create(&filterThread, NULL, filter_thread, q);

		//fills in the first filtering queue with odd numbers
		QueueElem i;
		for(i=3; i<=maxNumber; i++) {
			if(i%2 != 0) {
				queue_put(q, i);
			}
		}
		queue_put(q, 0);

		//pthread_join(filterThread, NULL);
	} else {
		sem_post(&primeCalculation);
	}

	return NULL; 
}

void *filter_thread(void *arg) 
{
	key_t key;
	int shmid;
	int *pt;

	//increments the threads counter
	pthread_mutex_lock(&threadsRunningMutex);
	threadsRunning++;
	pthread_mutex_unlock(&threadsRunningMutex);

	key = ftok("primesMemory", 0); /* uses the same key */ 
	shmid = shmget(key, 0, 0); 
	pt = (int *) shmat(shmid, 0, 0);

	CircularQueue *q = arg;

	//gets the first element of the Circular Queue that was
	//passed as an argument
	QueueElem first_number = queue_get(q);

	//if the first element of the queue is
	//bigger than the square root of the
	//chosen number, the sieving is completed
	//and all remaining numbers are primes
	if(first_number > sqrt(maxNumber)) {
		
		//pushes the first element into the
		//final primes list
		pthread_mutex_lock(&sharedMemoryMutex);
		pt[ arrayPointer ] = first_number;
		arrayPointer++;
		pthread_mutex_unlock(&sharedMemoryMutex);

		//pushes all other elements into the
		//final primes list
		QueueElem i = queue_get(q);
		
		while(i != 0) {
			pthread_mutex_lock(&sharedMemoryMutex);
			pt[ arrayPointer ] = i;
			arrayPointer++;
			pthread_mutex_unlock(&sharedMemoryMutex);

			i = queue_get(q);
		}

		queue_destroy(q);
		while(threadsRunning>1) {
			printf("Waiting...\n");
			printf("threadsRunning = %d\n", threadsRunning);
		}
		sem_post(&primeCalculation);

	} else {
		//adds the first number to the prime list
		//increases the next prime pointer
		pthread_mutex_lock(&sharedMemoryMutex);
		pt[ arrayPointer ] = first_number;
		arrayPointer++;
		pthread_mutex_unlock(&sharedMemoryMutex);

		CircularQueue *processQueue;
		queue_init(&processQueue,QUEUE_SIZE);

		pthread_t filterThread;
		pthread_create(&filterThread, NULL, filter_thread, processQueue);

		QueueElem i = queue_get(q);

		//the next filtering queue is filled with
		//all numbers except the multiples of the
		//prime calculated in this filtering thread
		while(i != 0) {
			if(i%first_number != 0) {
				queue_put(processQueue, i);
			}
			i = queue_get(q);
		}

		queue_put(processQueue, 0);

		//pthread_join(filterThread, NULL);
		queue_destroy(q);
	}

	if (shmdt((char*)pt) == -1) {
        perror("shmdt1");
		exit(1);	
	}

	pthread_mutex_lock(&threadsRunningMutex);
	threadsRunning--;
	pthread_mutex_unlock(&threadsRunningMutex);
	return NULL; 
}

int compareFunction(const void * a, const void * b)
{
   return ( *(int*)a - *(int*)b );
}
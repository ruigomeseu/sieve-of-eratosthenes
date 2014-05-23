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
#define SHARED 0 /* sem. is shared between threads */ 
#define QUEUE_SIZE 10

//------------------------------------------------------------------------------------------ 
// Type of the circular queue elements 
typedef unsigned long QueueElem;

sem_t primeCalculation;
int maxNumber = 0;
 
void *initial_thread(void *arg);
void *filter_thread(void *arg);

 
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
// TO DO BY STUDENTS: ADD ERROR TESTS TO THE CALLS & RETURN a value INDICATING (UN)SUCESS 
 
void queue_init(CircularQueue **q, unsigned int capacity) // TO DO: change return value 
{ 
	*q = (CircularQueue *) malloc(sizeof(CircularQueue)); 
	sem_init(&((*q)->empty), 0, capacity); 
	sem_init(&((*q)->full), 0, 0); 
	pthread_mutex_init(&((*q)->mutex), NULL); 
	(*q)->v = (QueueElem *) malloc(capacity * sizeof(QueueElem)); 
	(*q)->capacity = capacity; 
	(*q)->first = 0; 
	(*q)->last = 0; 
} 
 
//------------------------------------------------------------------------------------------ 
// Inserts 'value' at the tail of queue 'q' 
void queue_put(CircularQueue *q, QueueElem value) 
{
	sem_wait((sem_t *) &q->empty);

    q->v[q->last] = value;
	q->last = (q->last+1)%q->capacity;

	sem_post((sem_t *) &q->full);
} 
 
//------------------------------------------------------------------------------------------ 
// Removes element at the head of queue 'q' and returns its 'value'
QueueElem queue_get(CircularQueue *q) 
{	
	sem_wait((sem_t *) &q->full);
	
	QueueElem value;

	value = q->v[q->first];
	q->first = (q->first+1)%q->capacity;

	sem_post((sem_t *) &q->empty);
	return value;
} 
 
//------------------------------------------------------------------------------------------ 
// Frees space allocated for the queue elements and auxiliary management data 
// Must be called when the queue is no more needed 
void queue_destroy(CircularQueue *q) 
{ 
	free(q->v);
} 

int main(int argc, char *argv[]) {

	if(argc != 2) {
		printf("Incorrect number of arguments\n");
		exit(-1);
	}

	maxNumber = strtol(argv[1], NULL, 0);

	double neededSpace = 1.2 * (maxNumber / log(maxNumber)) + 1;
	int neededSpaceAproximation = ceil(neededSpace);

	int allNumbers[maxNumber-1];

	sem_init(&primeCalculation, 0, 0); 

	//fill in the numbers array with all numbers from 2...n
	int i;
	for(i=0; i<=sizeof(allNumbers)/sizeof(allNumbers[0]); i++) {
		allNumbers[i] = i+2;
	}

	key_t key; 
	int shmid;
	char *data;

	key = ftok("proc1", 0);
	if ((shmid = shmget(key, neededSpaceAproximation, S_IRUSR | S_IWUSR)) == -1) {
    	perror("shmget");
    	exit(1);
    }

    /* attach to the segment to get a pointer to it: */
  	data = (char *) shmat(shmid, NULL, 0);
    if (data == (char *) -1) {
    	perror("shmat");
    	exit(1);
    }
    for(i=0; i<=data[0]; i++) {
		data[i] = 0;
	}

    if (shmdt((char*)data) == -1) {
        perror("shmdt1");
		exit(1);
	}

	pthread_t initialThread;
	pthread_create(&initialThread, NULL, initial_thread, NULL);
	pthread_join(initialThread, NULL); 

	sem_wait(&primeCalculation);

	int *pt;
	key = ftok("proc1", 0); /* uses the same key proc1 */ 
	shmid = shmget(key, 0, 0); /* only uses, it doesn't create */ 
	pt = (int *) shmat(shmid, 0, 0);
	
	printf("All primes: \n\n");
	for(i=0; i<=pt[0]; i++) {
		printf("%d: %d\n", i, pt[i]);
	}

	shmdt(pt);

	return 0;
}

void *initial_thread(void *arg) 
{
	key_t key;
	int shmid;
	int *pt;

	key = ftok("proc1", 0);
	shmid = shmget(key, 0, 0);
	pt = (int *) shmat(shmid, 0, 0);

	//sets pt[0] as the next prime pointer to the first available position
	pt[0]=1;

	//sets the first prime as 2
	pt[ pt[0] ] = 2;

	//increments the pointer
	pt[0] = pt[0] + 1;

	//close the shared memory
	shmdt(pt);

	if(maxNumber>2) {
		CircularQueue *q;
		queue_init(&q,QUEUE_SIZE);

		pthread_t filterThread;
		pthread_create(&filterThread, NULL, filter_thread, q);

		int i;
		for(i=3; i<=maxNumber; i++) {
			if(i%2 != 0) {
				queue_put(q, i);
			}
		}
		queue_put(q, 0);

		pthread_join(filterThread, NULL);
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

	key = ftok("proc1", 0); /* usa a mesma key */ 
	shmid = shmget(key, 0, 0); /* não cria, apenas utiliza */ 
	pt = (int *) shmat(shmid, 0, 0);

	CircularQueue *q = arg;

	QueueElem first_number = queue_get(q);

	if(first_number > sqrt(maxNumber)) {
		//pushes the first element into the
		//final primes list
		pt[ pt[0] ] = first_number;;
		pt[0] = pt[0] + 1;

		//pushes all other elements into the
		//final primes list
		QueueElem i = queue_get(q);
		while(i != 0) {
			pt[ pt[0] ] = i;
			pt[0] = pt[0] + 1;

			i = queue_get(q);
		}

		queue_destroy(q);

		sem_post(&primeCalculation);
	} else {
		//adds the first number to the prime list
		//increases the next prime pointer

		pt[ pt[0] ] = first_number;
		pt[0] = pt[0] + 1;
		CircularQueue *processQueue;
		queue_init(&processQueue,QUEUE_SIZE);

		pthread_t filterThread;
		pthread_create(&filterThread, NULL, filter_thread, processQueue);

		QueueElem i = queue_get(q);

		while(i != 0) {
			if(i%first_number != 0) {
				queue_put(processQueue, i);
			}

			i = queue_get(q);
		}

		queue_put(processQueue, 0);

		pthread_join(filterThread, NULL);

		

		queue_destroy(q);
	}

	shmdt(pt);
	return NULL; 
}
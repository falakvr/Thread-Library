#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ucontext.h>
#include "mythread.h"

#define debug 0
#define mem 8*1024
#define handle_error(msg) do { perror(msg); exit(EXIT_FAILURE); } while (0)

typedef struct thread {
	int id;
	ucontext_t * ucntx;
	struct thread *next_thread;
	struct thread *next_child;
	struct thread *next_sem;
	struct thread *parent;
	int joined;
	int joined_all;
	struct queue *children;
} Thread;

typedef struct queue {
	Thread *front;
	Thread *rear;
	Thread *temp;
	Thread *front1;
	int count;
} Queue;

typedef struct semaphore {
	int initVal;
	int id;
	Queue * queue;
} Semaphore;

Queue *readyQueue;
Queue *blockedQueue;
Thread *runningThread;

ucontext_t Maincntx;

int number_of_threads = 0;
int number_of_semaphores = 0;

/*
 * C Program to Implement Queue Data Structure using Linked List
 */

int frontelement(Queue *q);
void enq(Queue *q, Thread *t, int ptr_to_update);
void deq(Queue *q, int ptr_to_update);
int isEmpty(Queue *q);
void display(Queue *q, int ptr_to_update);
Queue* create();
void removeFromQueue(Queue *q, Thread *t, int ptr_to_update);

void DeleteThread(Thread *t);

/* Create an empty queue */

Queue* create() {
	Queue *q = (Queue *) malloc(sizeof(Queue));
	q->front = q->rear = NULL;
	return q;
}

/* Returns queue size */

int queuesize(Queue *q) {
	return q->count;
}

/* Enqueing the queue */

void enq(Queue *q, Thread *t, int ptr_to_update) {
	if (q->rear == NULL && q->front == NULL) {
		//Queue empty
		q->front = q->rear = t;
	} else {
		//Queue not empty
		q->temp = t;
		if (ptr_to_update == 0) {
			// Next Thread in Ready/Block queue
			q->rear->next_thread = q->temp;
		} else if (ptr_to_update == 1) {
			// Next thread in child queue
			q->rear->next_child = q->temp;
		} else if (ptr_to_update == 2) {
			// Next thread in semaphore queue
			q->rear->next_sem = t;
		}
		q->rear = t;
	}
	// Increment count of elements of queue
	q->count++;
}

/* Displaying the queue elements */

void display(Queue *q, int ptr_to_update) {
	q->front1 = q->front;
	if ((q->front1 == NULL) && (q->rear == NULL)) {
		printf("Queue is empty");
		return;
	}
	while (q->front1 != q->rear) {
		// Check until last element is reached
		printf("%d ", q->front1->id);
		if (ptr_to_update == 0) {
			// Next thread in ready/blocked queue
			q->front1 = q->front1->next_thread;
		} else if (ptr_to_update == 1) {
			// Next thread in child queue
			q->front1 = q->front1->next_child;
		} else if (ptr_to_update == 2) {
			// Next thread in semaphore queue
			q->front1 = q->front1->next_sem;
		}
	}
	if (q->front1 == q->rear)
		printf("%d", q->front1->id);
}

/* Dequeing the queue */

void deq(Queue *q, int ptr_to_update) {
	Thread * t;
	t = q->front;

	//Dequeue thread

	if (q->front == NULL && q->rear == NULL) {
		// Empty queue
		return;
	} else if (q->front == q->rear) {
		// Only one element in queue
		q->front = NULL;
		q->rear = NULL;
	} else {
		// Otherwise
		if (ptr_to_update == 0) {
			// Next thread in ready/blocked queue
			q->front = q->front->next_thread;
		} else if (ptr_to_update == 1) {
			// Next thread in child queue
			q->front = q->front->next_child;
		} else if (ptr_to_update == 2) {
			// Next element in semaphore queue
			q->front = q->front->next_sem;
		}
	}

	//Update pointers of dequeued thread

	if (ptr_to_update == 0) {
		// Ready/Blocked queue
		t->next_thread = NULL;
	} else if (ptr_to_update == 1) {
		// Child queue
		t->next_child = NULL;
	} else if (ptr_to_update == 2) {
		// Semaphore queue
		t->next_sem = NULL;
	}

	q->count--;
}

/* Returns the front element of queue */

int frontelement(Queue *q) {
	if ((q->front != NULL) && (q->rear != NULL))
		return (q->front->id);
	else
		return 0;
}

/* Display if queue is empty or not */

int isEmpty(Queue *q) {

	if ((q->front == NULL) && (q->rear == NULL)) {
		return 1;
	} else {
		return 0;
	}
}

/* Remove from the queue */

void removeFromQueue(Queue *q, Thread *t, int ptr_to_update) {

	if (isEmpty(q) != 1) {

		Thread * current;
		Thread * prev;

		current = prev = q->front;

		//Find the thread

		while (current != NULL && current != t) {
			// Search till you reach end of queue or you find the element
			prev = current;
			if (ptr_to_update == 0) {
				// Searching in ready/blocked queue
				current = current->next_thread;
			} else if (ptr_to_update == 1) {
				// Searching in child queue
				current = current->next_child;
			} else if (ptr_to_update == 2) {
				// Searching in semaphore queue
				current = current->next_sem;
			}
		}

		//Remove the thread

		if (current == q->front && current == q->rear) {
			// Thread we are searching is the Only element in queue
			q->front = NULL;
			q->rear = NULL;
		} else if (current == q->front) {
			// Thread searching is the first element of queue
			if (ptr_to_update == 0) {
				q->front = current->next_thread;
			} else if (ptr_to_update == 1) {
				q->front = current->next_child;
			} else if (ptr_to_update == 2) {
				q->front = current->next_sem;
			}
		} else if (current == q->rear) {
			// Thread searched is the last element of the queue
			q->rear = prev;
		} else {
			if (ptr_to_update == 0) {
				prev->next_thread = current->next_thread;
			} else if (ptr_to_update == 1) {
				prev->next_child = current->next_child;
			} else if (ptr_to_update == 2) {
				prev->next_sem = current->next_sem;
			}
		}

		//Update its pointers in the respective queue

		if (ptr_to_update == 0) {
			// Ready/Blocked queue
			current->next_thread = NULL;
		} else if (ptr_to_update == 1) {
			// Child queue
			current->next_child = NULL;
		} else if (ptr_to_update == 2) {
			// Semaphore queue
			current->next_sem = NULL;
		}
	}
}

void MyThreadInit(void (*start_funct)(void *), void *args) {

	Thread *Main_Thread = (Thread *) malloc(sizeof(Thread));

	readyQueue = create();
	blockedQueue = create();

	ucontext_t * ucntx = malloc(sizeof(ucontext_t));

	if (getcontext(ucntx) == -1) {
		handle_error("getcontext error");
	}

	ucntx->uc_link = 0;
	ucntx->uc_stack.ss_sp = malloc(mem);
	ucntx->uc_stack.ss_size = mem;
	ucntx->uc_stack.ss_flags = 0;

	makecontext(ucntx, (void*) start_funct, 1, args);

	Main_Thread->children = create();
	Main_Thread->id = number_of_threads++;
	Main_Thread->joined = 0;
	Main_Thread->joined_all = 0;
	Main_Thread->next_thread = NULL;
	Main_Thread->next_child = NULL;
	Main_Thread->parent = NULL;

	Main_Thread->ucntx = ucntx;

	runningThread = Main_Thread;

	if (swapcontext(&Maincntx, Main_Thread->ucntx) == -1) {
		handle_error("swapcontext error");
	}
}

MyThread MyThreadCreate(void (*start_funct)(void *), void *args) {

	ucontext_t * ucntx = malloc(sizeof(ucontext_t));
	Thread *t1 = (Thread *) malloc(sizeof(Thread));

	if (getcontext(ucntx) == -1) {
		handle_error("getcontext error");
	}

	ucntx->uc_link = 0;
	ucntx->uc_stack.ss_sp = malloc(mem);
	ucntx->uc_stack.ss_size = mem;
	ucntx->uc_stack.ss_flags = 0;

	makecontext(ucntx, (void*) start_funct, 1, args);

	t1->children = create();
	t1->id = number_of_threads++;
	t1->joined = 0;
	t1->joined_all = 0;
	t1->next_child = NULL;
	t1->next_thread = NULL;
	t1->parent = runningThread;
	t1->ucntx = ucntx;

	// Enqueue in ready/blocked queue as well as children queue of parent
	enq(runningThread->children, t1, 1);
	enq(readyQueue, t1, 0);

	return (MyThread) t1;
}

void MyThreadYield(void) {

	Thread * t;
	Thread * current;

	current = runningThread;

	if (isEmpty(readyQueue) != 1) {

		// Invoking thread goes from running to ready queue
		enq(readyQueue, runningThread, 0);

		t = readyQueue->front;
		deq(readyQueue, 0);

		runningThread = t;

		if (swapcontext(current->ucntx, t->ucntx) == -1) {
			handle_error("swapcontext error");
		}
	}
}

int MyThreadJoin(MyThread thread) {

	Thread *child;
	Thread *t;
	Thread *current;

	child = (Thread *) thread;
	current = runningThread;

	if (child->parent == runningThread) {
		// Update join flag of child thread
		child->joined = 1;
		// Block the parent thread
		enq(blockedQueue, runningThread, 0);

		// Front of ready queue becomes running
		t = readyQueue->front;
		deq(readyQueue, 0);
		runningThread = t;

		if (debug)
			display(readyQueue, 0);
		if (debug)
			display(blockedQueue, 0);

		if (swapcontext(current->ucntx, t->ucntx) == -1) {
			handle_error("swapcontext error");
		}
		return 0;
	} else {
		return -1;
	}
}

void MyThreadJoinAll(void) {

	Thread *t;
	Thread *current;
	current = runningThread;

	if (isEmpty(runningThread->children) != 1) {
		// Update joined_all flag of parent
		runningThread->joined_all = 1;

		// Block the running thread
		enq(blockedQueue, runningThread, 0);

		// Front of ready queue becomes running
		t = readyQueue->front;
		deq(readyQueue, 0);
		runningThread = t;

		if (swapcontext(current->ucntx, t->ucntx) == -1) {
			handle_error("swapcontext error");
		}
	}
}

void MyThreadExit(void) {

	Thread * current;
	Thread * t;
	Thread * new;

	current = runningThread;

	if (current->parent != NULL && current->joined == 1) {
		// Checking join condition
		t = current->parent;
		removeFromQueue(blockedQueue, t, 0);
		enq(readyQueue, t, 0);
	} else if (current->parent != NULL && current->parent->joined_all == 1) {
		// Checking joinAll
		if ((current->parent->children->front == current)
				&& (current->parent->children->rear == current)) {
			// Thread is the last child of the parent
			t = current->parent;
			removeFromQueue(blockedQueue, t, 0);
			enq(readyQueue, t, 0);
		}
	}

	// Now delete thread and free memory
	DeleteThread(current);

	if (isEmpty(readyQueue) != 1) {
		new = readyQueue->front;
		deq(readyQueue, 0);
		runningThread = new;
		if (setcontext(new->ucntx) == -1) {
			handle_error("setcontext error");
		}
	} else {
		if (setcontext(&Maincntx) == -1) {
			handle_error("setcontext error");
		}
	}
}

void DeleteThread(Thread *t) {

	Thread * current_child;

	if (t->parent != NULL) {
		// Check if parent exists
		removeFromQueue(t->parent->children, t, 1);
		t->parent = NULL;
	}

	current_child = t->children->front;
	while (current_child != NULL) {
		// Make parent pointers of all the children null
		current_child->parent = NULL;
		current_child = current_child->next_child;
	}
	t->children = NULL;
	free(t->ucntx);
	free(t);
}

MySemaphore MySemaphoreInit(int initialValue) {
	Semaphore * sem = malloc(sizeof(Semaphore));
	if (initialValue < 0) {
		return NULL;
	} else {
		sem->initVal = initialValue;
		sem->id = number_of_semaphores++;
		sem->queue = create();
		return (MySemaphore) sem;
	}
}

void MySemaphoreSignal(MySemaphore sem) {
	Semaphore * s = (Semaphore *) sem;
	if (isEmpty(s->queue) == 1) {
		s->initVal++;
	} else if (s->initVal == 0) {
		Thread * t = s->queue->front;
		deq(s->queue, 2);
		enq(readyQueue, t, 0);
	}
}

void MySemaphoreWait(MySemaphore sem) {
	Semaphore * s = (Semaphore *) sem;
	Thread * t = runningThread;
	if (s->initVal > 0) {
		s->initVal--;
	} else if (s->initVal == 0) {
		enq(s->queue, runningThread, 2);
		runningThread = readyQueue->front;
		deq(readyQueue, 0);
		if (swapcontext(t->ucntx, runningThread->ucntx) == -1) {
			handle_error("swapcontext error");
		}
	}
}

int MySemaphoreDestroy(MySemaphore sem) {
	Semaphore * s = (Semaphore *) sem;
	if (isEmpty(s->queue) == 1) {
		s->queue = NULL;
		free(s);
		return 0;
	} else {
		return -1;
	}
}
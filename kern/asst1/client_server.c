/* This file will contain your solution. Modify it as you wish. */
#include <types.h>
#include <lib.h>
#include <synch.h>
#include <kern/errno.h>
#include "client_server.h"

/*
 * Declare any variables you need here to implement and
 *  synchronise your queues and/or requests.
 */


// linked list node queue structure to store requests
struct ReqNode {
        request_t *req;
        struct ReqNode *next;
};

struct ReqNode *head; // points to head of queue linkedlist
struct ReqNode *tail; // points to tail of queue linkedlist

static struct lock *lock_queue;
static struct cv *empty_queue;
// no specification of queue limit

struct ReqNode* new_req(request_t *req);


/* 
 * HELPER FUNCTION allocates memory to a new node, initialises req to given request,
 * initialises next to NULL, returns address of new node
 */

struct ReqNode* new_req(request_t *req) 
{
        struct ReqNode *node = kmalloc(sizeof(struct ReqNode));
        if (node == NULL)
                return NULL;
        node->req = req;
        node->next = NULL;
        return node;
}

/* work_queue_enqueue():
 *
 * req: A pointer to a request to be processed. You can assume it is
 * a valid pointer or NULL. You can't assume anything about what it
 * points to, i.e. the internals of the request type.
 *
 * This function is expected to add requests to a single queue for
 * processing. The queue is a queue (FIFO). The function then returns
 * to the caller. It can be called concurrently by multiple threads.
 *
 * Note: The above is a high-level description of behaviour, not
 * detailed psuedo code. Depending on your implementation, more or
 * less code may be required. 
 */

void work_queue_enqueue(request_t *req)
{
        lock_acquire(lock_queue);
        if (head == NULL) {
                // inserting into empty queue
                head = tail = new_req(req);
                if (head == NULL)
                        panic("failed to allocate new head request");
        } else {
                // inserting into existing queue
                tail->next = new_req(req);
                tail = tail->next;
                if (tail == NULL)
                        panic("failed to allocate new request");
        }
        // signal queue is no longer empty and release lock
        cv_broadcast(empty_queue, lock_queue);  
        lock_release(lock_queue);
}


/* 
 * work_queue_get_next():
 *
 * This function is expected to block on a synchronisation primitive
 * until there are one or more requests in the queue for processing.
 *
 * A pointer to the request is removed from the queue and returned to
 * the server.
 * 
 * Note: The above is a high-level description of behaviour, not
 * detailed psuedo code. Depending on your implementation, more or
 * less code may be required.
 */

request_t *work_queue_get_next(void)
{
		lock_acquire(lock_queue);
        // if queue is empty, cv_wait
        while (head == NULL) {
                cv_wait(empty_queue, lock_queue);
        }
        // extract request
        request_t *item;
        item = head->req;
        // move head of linked list queue
        struct ReqNode *delete = head;
        head = head->next;
        kfree(delete);

        lock_release(lock_queue);

        return item;

}

/*
 * work_queue_setup():
 * 
 * This function is called before the client and server threads are started. It is
 * intended for you to initialise any globals or synchronisation
 * primitives that are needed by your solution.
 *
 * In returns zero on success, or non-zero on failure.
 *
 * You can assume it is not called concurrently.
 */

int work_queue_setup(void)
{
        head = tail = NULL;

        lock_queue = lock_create("queue lock");
        if (lock_queue == NULL)
                return ENOMEM;

        empty_queue = cv_create("empty queue");
        if (empty_queue == NULL)
                return ENOMEM;

        return 0;
}


/* 
 * work_queue_shutdown():
 * 
 * This function is called after the participating threads have
 * exited. Use it to de-allocate or "destroy" anything allocated or created
 * on setup.
 *
 * You can assume it is not called concurrently.
 */

void work_queue_shutdown(void)
{
        // destroy linkedlist
        struct ReqNode *delete;
        while (head != NULL) {
                delete = head;
                head = head->next;
                kfree(delete);
        }
        // destroy lock & cv
        lock_destroy(lock_queue);
        cv_destroy(empty_queue);
}

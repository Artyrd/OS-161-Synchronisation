/* This file will contain your solution. Modify it as you wish. */
#include <types.h>
#include <lib.h>
#include <synch.h>
#include "producerconsumer.h"

/* Declare any variables you need here to keep track of and
   synchronise your bounded buffer. A sample declaration of a buffer is shown
   below. It is an array of pointers to items.
   
   You can change this if you choose another implementation. 
   However, your implementation should accept at least BUFFER_SIZE 
   prior to blocking
*/

#define BUFFLEN (BUFFER_SIZE + 1)

// for data struct:
data_item_t * item_buffer[BUFFER_SIZE+1];
volatile int head, tail;

// for lock & cv functions
static struct lock *lock_buff;
static struct cv *empty_buff, *full_buff;


/* consumer_receive() is called by a consumer to request more data. It
   should block on a sync primitive if no data is available in your
   buffer. It should not busy wait! */

data_item_t * consumer_receive(void)
{
        data_item_t * item;
        lock_acquire(lock_buff);
        // if buffer empty, cv_wait
        while(head == tail) {
                cv_wait(empty_buff, lock_buff);
        }
        // extract item and move tail up
        item = item_buffer[tail];
        tail = (tail + 1) % BUFFLEN; 
        // signal buffer is not full and unlock
        cv_broadcast(full_buff, lock_buff);
        lock_release(lock_buff);

        return item;
}

/* procucer_send() is called by a producer to store data in your
   bounded buffer.  It should block on a sync primitive if no space is
   available in your buffer. It should not busy wait!*/

void producer_send(data_item_t *item)
{
        lock_acquire(lock_buff);
        // if buffer full, cv_wait
        while((head + 1) % BUFFLEN == tail) {
                cv_wait(full_buff, lock_buff);
        }
        // insert item and move head
        item_buffer[head] = item;
        head = (head + 1) % BUFFLEN;
        // signal buffer is not empty and unlock
        cv_broadcast(empty_buff, lock_buff);
        lock_release(lock_buff);
}

/* Perform any initialisation (e.g. of global data) you need
   here. Note: You can panic if any allocation fails during setup */

void producerconsumer_startup(void)
{
        head = tail = 0;

        lock_buff = lock_create("buffer lock");
        if (lock_buff == NULL)
                panic("failed to create lock");

        empty_buff = cv_create("empty buffer");
        if (empty_buff == NULL)
                panic("failed to create cv for empty buffer");
 
        full_buff = cv_create("full buffer");
        if (full_buff == NULL) 
                panic("failed to create cv for full buffer");
}

/* Perform any clean-up you need here */
void producerconsumer_shutdown(void)
{
        lock_destroy(lock_buff);
        cv_destroy(empty_buff);
        cv_destroy(full_buff);
}


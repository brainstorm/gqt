#ifndef __PQ_H__
#define __PQ_H__
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sysexits.h>
#include <err.h>
 
typedef struct
{
    uint32_t sum, len;
} priority;

typedef struct
{
    int * data;
    priority pri;
} q_elem_t;

typedef struct
{
    q_elem_t *buf;
    int n, alloc;
} pri_queue_t, *pri_queue;
 
#define priq_purge(q) (q)->n = 1
#define priq_size(q) ((q)->n - 1)
pri_queue priq_new(int size);
void priq_push(pri_queue q, int *data, priority pri);
int * priq_pop(pri_queue q, priority *pri);
int* priq_top(pri_queue q, priority *pri);
void priq_combine(pri_queue q, pri_queue q2);
void priq_free(pri_queue q);
int pricmp(priority a, priority b);
#endif

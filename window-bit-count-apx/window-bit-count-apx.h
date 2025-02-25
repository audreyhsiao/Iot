#ifndef _WINDOW_BIT_COUNT_APX_
#define _WINDOW_BIT_COUNT_APX_

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>

uint64_t N_MERGES = 0; // keep track of how many bucket merges occur

#ifndef APX_POOL_SIZE
#define APX_POOL_SIZE (1<<20)  // 1 MB
#endif
/*
    TODO: You can add code here.
*/
/* global variables */
// make sure we only call malloc once 
static uint8_t*   g_base_ptr  = NULL;    // the base pointer of the memory block
static size_t  g_total_size   = 0;       // the total size of the memory block
static size_t  g_used_offset  = 0; 

static inline void init_memory_block(void){
   g_base_ptr = (unit8_t*) malloc(APX_POOL_SIZE);
   if(!g_base_ptr){
    fprintf(stderr, "Failed to allocate memory for the memory pool.\n");
   }
   g_total_size = APX_POOL_SIZE;
   g_used_offset = 0;
}

static inline void* apx_pool_alloc(size_t size){
    if(g_used_offset + size > g_total_size){
        printf("Memory pool is full. Cannot allocate more memory.\n");
        exit(1);
    }
    void* ptr = g_base_ptr + g_used_offset;
    g_used_offset += size;
    return ptr;
}
      // the offset of the used memory

typedef struct {
    uint32_t wnd_size;
    uint32_t k;
    
    uint32_t* bucket_counts; //number of 1’s that have been seen in the bucket
    uint32_t* bucket_sizes; //how many items are in the bucket
    uint32_t* bucket_indices; //the index of the bucket

    uint32_t num_buckets; //bucket that we have 
    uint32_t current_time; //current time

} StateApx;

// k = 1/eps
// if eps = 0.01 (relative error 1%) then k = 100
// if eps = 0.001 (relative error 0.1%) the k = 1000
uint64_t wnd_bit_count_apx_new(StateApx* self, uint32_t wnd_size, uint32_t k) {
    assert(wnd_size >= 1);
    assert(k >= 1);
    init_memory_block();   // initialize the memory block
    self->wnd_size = wnd_size;
    self->k = k;
    self->num_buckets = 0;
    self->current_time = 0;

    // Allocate a single block of memory for all arrays
    uint32_t total_size = 3 * wnd_size; // 3 * wnd_size is the total size of the memory block for bucket_counts, bucket_sizes, bucket_indices
    uint32_t* memory_block = apx_pool_alloc(total_size * sizeof(uint32_t)); // allocate memory for this memory block

    // Assign pointers to the appropriate sections of the memory block
    self->bucket_counts = memory_block;
    self->bucket_sizes = memory_block + wnd_size;
    self->bucket_indices = memory_block + 2 * wnd_size;

    for (uint32_t i = 0; i < wnd_size; i++) {
        self->bucket_counts[i]  = 0;
        self->bucket_sizes[i]   = 0;
        self->bucket_indices[i] = 0;
    }



    // Return the total number of bytes allocated on the heap
    return sizeof(StateApx) + total_size * sizeof(uint32_t);
}

void wnd_bit_count_apx_destruct(StateApx* self) {
   // TODO not sure
   (void)self;
}

void wnd_bit_count_apx_print(StateApx* self) {
    printf("window size: %u, k: %u\n", self->wnd_size, self->k);
    printf("Bucket state:\n");
    for(uint32_t i = 0; i < self->num_buckets; i++){
        printf("Bucket %u: counts = %u, sizes = %u, indices = %u\n", i, self->bucket_counts[i], self->bucket_sizes[i], self->bucket_indices[i]);
    }
    printf("Current time: %u\n", self->current_time);   
    // This is useful for debugging.
}

uint32_t wnd_bit_count_apx_next(StateApx* self, bool item) {
    // TODO: Fill me.
    return 0;
}

#endif // _WINDOW_BIT_COUNT_APX_

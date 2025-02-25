#ifndef _WINDOW_BIT_COUNT_APX_
#define _WINDOW_BIT_COUNT_APX_

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

uint64_t N_MERGES = 0;  // keep track of how many bucket merges occur

#ifndef APX_POOL_SIZE
#define APX_POOL_SIZE (1 << 20)  // 1 MB
#endif
/*
    TODO: You can add code here.
*/
/* global variables */
// make sure we only call malloc once
static uint8_t* g_base_ptr = NULL;  // the base pointer of the memory block
static size_t g_total_size = 0;     // the total size of the memory block
static size_t g_used_offset = 0;

static inline void init_memory_block(void) {
  g_base_ptr = (uint8_t*)malloc(APX_POOL_SIZE);
  if (!g_base_ptr) {
    fprintf(stderr, "Failed to allocate memory for the memory pool.\n");
  }
  g_total_size = APX_POOL_SIZE;
  g_used_offset = 0;
}

static inline void* apx_pool_alloc(size_t size) {
  if (g_used_offset + size > g_total_size) {
    printf("Memory pool is full. Cannot allocate more memory.\n");
    exit(1);
  }
  void* ptr = g_base_ptr + g_used_offset;
  g_used_offset += size;
  //printf("Allocated memory at %p, size = %zu\n", ptr, size);
  return ptr;
}
// the offset of the used memory

typedef struct {
  uint32_t wnd_size;
  uint32_t k;

  uint32_t* bucket_counts;   // number of 1's that have been seen in the bucket
  uint32_t* bucket_sizes;    // how many items are in the bucket
  uint32_t* bucket_indices;  // the index of the bucket

  uint32_t num_buckets;   // bucket that we have
  uint32_t current_time;  // current time

} StateApx;

// k = 1/eps
// if eps = 0.01 (relative error 1%) then k = 100
// if eps = 0.001 (relative error 0.1%) the k = 1000
uint64_t wnd_bit_count_apx_new(StateApx* self, uint32_t wnd_size, uint32_t k) {
  assert(wnd_size >= 1);
  assert(k >= 1);
  init_memory_block();  // initialize the memory block

  self->wnd_size = wnd_size;
  self->k = k;
  self->num_buckets = 0;
  self->current_time = 0;

  // Allocate a single block of memory for all arrays
  uint32_t total_size =
      3 * wnd_size;  // 3 * wnd_size is the total size of the memory block for
                     // bucket_counts, bucket_sizes, bucket_indices
  uint32_t* memory_block = (uint32_t*)apx_pool_alloc(
    total_size * sizeof(uint32_t));  // allocate memory for this memory block

  // Assign pointers to the appropriate sections of the memory block
  self->bucket_counts = memory_block;
  self->bucket_sizes = memory_block + wnd_size;
  self->bucket_indices = memory_block + 2 * wnd_size;

  for (uint32_t i = 0; i < wnd_size; i++) {
    self->bucket_counts[i] = 0;
    self->bucket_sizes[i] = 0;
    self->bucket_indices[i] = 0;
  }

  // Return the total number of bytes allocated on the heap
  printf("Total memory allocated: %zu bytes\n", sizeof(StateApx) + total_size * sizeof(uint32_t));
  return sizeof(StateApx) + total_size * sizeof(uint32_t);
}

void wnd_bit_count_apx_destruct(StateApx* self) {
  // TODO not sure
  free(g_base_ptr);

}

void wnd_bit_count_apx_print(StateApx* self) {
  printf("window size: %u, k: %u\n", self->wnd_size, self->k);
  printf("Bucket state:\n");
  for (uint32_t i = 0; i < self->num_buckets; i++) {
    printf(" ");
    printf("Bucket %u: counts = %u, sizes = %u, indices = %u\n", i,
           self->bucket_counts[i], self->bucket_sizes[i],
           self->bucket_indices[i]);
  }
  printf("Current time: %u\n", self->current_time);
  // This is useful for debugging.
}

uint32_t wnd_bit_count_apx_next(StateApx* self, bool item) {
  // TODO: Fill me.
  self->current_time++;

  while (self->num_buckets > 0) {
    uint32_t oldest_timeStamp = self->bucket_indices[0];

    // remove expired buckets
    if (self->current_time - oldest_timeStamp >= self->wnd_size) {
      self->num_buckets--;
      // move index = 1 to index = 0, etc. etc.
      for (uint32_t i = 0; i < self->num_buckets; i++) {
        self->bucket_indices[i] = self->bucket_indices[i + 1];
      }
    } else {
      break;
    }
  }

  // if new item is 1, then we need to update the bucket counts
  if (item) {
    // build a size 1 bucket
    uint32_t new_bucket_index = self->num_buckets;
    self->bucket_counts[new_bucket_index] = 1;
    self->bucket_sizes[new_bucket_index] = 1;
    self->bucket_indices[new_bucket_index] = self->current_time;
    self->num_buckets++;
  }

// check if there are k+2 buckets that have the same size
  bool merge_occurred = true;
  while (merge_occurred && self->num_buckets >= 0) {
    merge_occurred = false;
    int tail_index = self->num_buckets - 1;
    if(tail_index <= 0) break;
    uint32_t tail_size = self->bucket_sizes[tail_index];

    // see through the buckets and check if there are k+2 buckets that have the same size
    int count = 0;
    while(tail_index >= 0 && self->bucket_sizes[tail_index] == tail_size) {
        count++;
        tail_index--;
    }

    if(count >= self->k + 2) {
        // start_index ~ tail_index need to be merged
        int start_index = tail_index + 1;

        uint32_t old_size = self->bucket_sizes[start_index];
        uint32_t old_ts = self->bucket_indices[start_index];
        uint32_t old_ts2 = self->bucket_indices[start_index + 1];

        uint32_t new_size = 2 * tail_size;
        uint32_t new_ts = (old_ts > old_ts2) ? old_ts : old_ts2;
        // remove the old buckets
        for(int i = start_index; i < (int)(self->num_buckets - 2); i++) {
            self->bucket_sizes[i] = self->bucket_sizes[i + 2];
            self->bucket_indices[i] = self->bucket_indices[i + 2];
            self->bucket_counts[i] = self->bucket_counts[i + 2];
        }
        self->num_buckets -= 2;

        // put the new bucket at the start_index
        self->bucket_sizes[start_index] = new_size;
        self->bucket_indices[start_index] = new_ts;
        self->bucket_counts[start_index] = count;

        N_MERGES++;

        merge_occurred = true;
    }
  }
 uint32_t approx_count = 0;
if (self->num_buckets == 1) {
    approx_count = self->bucket_sizes[0];
} else if (self->num_buckets > 1) {
    for (uint32_t i = 1; i < self->num_buckets; i++) {
        approx_count += self->bucket_sizes[i];
    }
    approx_count += self->bucket_sizes[0] / 2;
}

  return approx_count;
}

#endif  // _WINDOW_BIT_COUNT_APX_

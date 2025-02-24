#ifndef _WINDOW_BIT_COUNT_
#define _WINDOW_BIT_COUNT_

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct {
    uint32_t wnd_size;
    uint32_t index_oldest; // index pointing to the oldest element    
    bool* wnd_buffer;   // buffer to store the window of bits
    uint32_t count; // number of 1s in the window
} State;

uint64_t wnd_bit_count_new(State* self, uint32_t wnd_size) {
    assert(wnd_size >= 1);

    self->wnd_size = wnd_size;
    self->index_oldest = 0;
    uint64_t memory = ((uint64_t) wnd_size) * sizeof(bool);
    self->wnd_buffer = (bool*) malloc(memory);
    for (uint32_t i=0; i<wnd_size; i++) {
        self->wnd_buffer[i] = false;
    }
    self->count = 0;

    return memory;
}

void wnd_bit_count_destruct(State* self) {
    free(self->wnd_buffer);
}

void wnd_bit_count_print(State* self) {
    // TODO: useful for debugging
    printf("Window size: %d\n", self->wnd_size);
    printf("Index oldest: %d\n", self->index_oldest);
    printf("Count: %d\n", self->count);
    printf("Window buffer: ");
    for (uint32_t i=0; i<self->wnd_size; i++) {
        printf("%d ", self->wnd_buffer[i]);
    }
    printf("\n");
}

uint32_t wnd_bit_count_next(State* self, bool item) {
    bool old = self->wnd_buffer[self->index_oldest];
    self->count -= old;
    self->wnd_buffer[self->index_oldest] = item;
    self->count += item;

    self->index_oldest += 1;
    if (self->index_oldest == self->wnd_size) { 
        // if index oldest equal to the wind size it is out of the buffer
        // so we wrap aroung
        self->index_oldest = 0;
    }

    // return the number of 1s in the window
    return self->count;
}

#endif // _WINDOW_BIT_COUNT_

#include <iostream>
#include <unistd.h>
#include <fstream>
#include <list>
#include <algorithm>
#include <cstdint>
#include <ctime>
#include <cstring>
#include <mutex>
#include <pthread.h>
#include "Struct.h"

//Used to determine what type of allocation is used.
#define FIRST_FIT 0
#define BEST_FIT 1
#define WORST_FIT 2

typedef int AllocType;

//Function declarations
void get_average_fragmentation();
void *alloc(size_t chunk_size);
void dealloc(void *chunk);
void create_new_chunk(size_t chunk_size, intptr_t memory_address, bool isFree);
//void print_chunks();
void read_lock();
void read_unlock();
void write_lock();
void write_unlock();
bool free_chunk(Chunk chunk);
bool allocate_chunk(Chunk chunk);
//Chunk contains(std::list<Chunk> chunks, void* chunk);

double allocationTime = 0;
double deallocationTime = 0;
double reallocationTime = 0;
AllocType algorithm = 0;

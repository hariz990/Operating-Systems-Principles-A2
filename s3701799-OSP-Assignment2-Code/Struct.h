#include <pthread.h>

struct Chunk
{
    size_t chunk_size;
    intptr_t memory_address;
    pthread_mutex_t instance_lock;

    void print() {
        std::cout << "Size of Chunk: " << chunk_size << std::endl;
        std::cout << "Memory Address: " << (void*)memory_address << std::endl;
    }

    const bool operator==(const struct Chunk& a) const
    {
        bool isEqual = false;
        if (a.chunk_size == chunk_size && a.memory_address == memory_address) {
            isEqual = true;
        }

        return isEqual;
    }

};

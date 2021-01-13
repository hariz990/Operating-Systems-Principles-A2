#include "Allocation.h"

//VARIABLES
bool delayWriting = false;
bool isInitalized = false;
int delayReaders = 0;
int randomIncrement = 0;

//LIST
std::list<Chunk> chunksFreed;     //list of freed chunks
std::list<Chunk> chunksAllocated; //list of allocated chunks

//THREADS
pthread_mutex_t lock;
pthread_cond_t cond;
pthread_cond_t write_cond;
pthread_cond_t read_cond;


void *alloc(size_t chunk_size)
{
    if (!isInitalized)
    {
        pthread_mutex_init(&lock, NULL); //initialize mutex
        pthread_cond_init(&read_cond, NULL); //initialize read condition 
        pthread_cond_init(&write_cond, NULL); //initialize write condition
        isInitalized = true;
    }
    void *old_address = sbrk(0);    //gets the current address of the edge
    void *return_address = nullptr; //initializes an address to return
    int sizeBalance = 0;                //variable that holds the size remainder in the chunk
    Chunk chunkSelected = {0, 0};   //creates a placeholder chunk that is assigned to when a valid chunk is found

    //iterates though the list of freed chunks to see if it finds a valid chunk if the list has some elements
    while (chunkSelected.chunk_size == 0 && chunksFreed.size() > 0)
    {
        read_lock(); //locking mutex, limits access since multiple threads are running

        bool foundChunkMatch = false;
        int sizeDiff = -1; //the difference between the requested size and the size of the chunk
        std::list<Chunk>::iterator iterate;

        for (iterate = chunksFreed.begin(); iterate != chunksFreed.end(); iterate++)
        {
            //if first fit, take the first available chunk
            if (algorithm == FIRST_FIT)
            {
                if (iterate->chunk_size >= (size_t)chunk_size)
                {
					foundChunkMatch = true;
					sizeBalance = iterate->chunk_size - chunk_size; //add to size remainder of the chunk
                    return_address = (void *)iterate->memory_address;
					chunkSelected = *iterate;
                    break;
                }
            }
            //if worst fit, find the chunk with the most size difference
            else if (algorithm == WORST_FIT)
            {
                int chunkDiff = iterate->chunk_size - chunk_size;
                if (chunkDiff >= 0 && (chunkDiff >= sizeDiff || sizeDiff == -1))
                {
					foundChunkMatch = true;
					sizeDiff = chunkDiff;
                    return_address = (void *)iterate->memory_address;
					sizeBalance = sizeDiff;
					chunkSelected = *iterate;
                }
            }
            //if best fit, find the chunk with the least chunk difference
            else if (algorithm == BEST_FIT)
            {
                int chunkDiff = iterate->chunk_size - chunk_size;

                if (chunkDiff >= 0 && (chunkDiff <= sizeDiff || sizeDiff == -1))
                {
					foundChunkMatch = true;
					sizeDiff = chunkDiff;
                    return_address = (void *)iterate->memory_address;
					sizeBalance = sizeDiff;
					chunkSelected = *iterate;
                }
            }
        }

        read_unlock(); //unlock mutex

        //if found a chunk
        if (chunkSelected.chunk_size != 0)
        {
            if (pthread_mutex_trylock(&chunkSelected.instance_lock) == 0) //locks mutex object
            {
                //move the chunk from the freed list to the allocated list and store the memory address
                write_lock();
                allocate_chunk(chunkSelected);
                write_unlock();
                return_address = (void *)chunkSelected.memory_address;
                break;
            }
            else
            {
				chunkSelected = {0, 0}; //intialize value of chunk placeholder
            }
        }
        else
        {
            if (!foundChunkMatch) //if chunk does not match
            {
                break;
            }
        }
    }

    //if the remaining size of the chunk is usable, split it into its own chunk
    if (sizeBalance > 0)
    {
        write_lock();
        create_new_chunk(sizeBalance, (intptr_t)((intptr_t)chunkSelected.memory_address + (intptr_t)sizeBalance), true); //creates a new chunk
        write_unlock();
    }

    //if it didnt find a usable chunk, create a new one
    if (return_address == nullptr)
    {
        read_unlock();
        sbrk(chunk_size + 1); //increase the program edge by the requested amount
        write_lock();
        create_new_chunk((size_t)chunk_size, (intptr_t)old_address, false); //create and add a new chunk
        write_unlock();
        return_address = (void *)((intptr_t)old_address);
    }

    //return the address
    return return_address;
}

//Deallocates a chunk
void dealloc(void* chunk)
{
	read_lock();
	std::list<Chunk>::iterator iterate;
	Chunk chunkFound = { 0, 0 };

	for (iterate = chunksAllocated.begin(); iterate != chunksAllocated.end(); iterate++)
	{
		if (iterate->memory_address == (intptr_t)chunk)
		{
			chunkFound = *iterate;
		}
	}
	read_unlock();

	//if it found the address, add it to the freed chunks and remove it from allocated chunks
	if (chunkFound.chunk_size != (size_t)0 && chunkFound.memory_address != (intptr_t)0)
	{
		write_lock();
		free_chunk(chunkFound);
		write_unlock();
	}
	//if it tries to deallocate a memory address that isnt found, abort the program
	else
	{
		abort();
	}
}

//Lock datastructure when reading
void read_lock()
{
    //locks the mutex
    pthread_mutex_lock(&lock);

	//waits until signalling threads to finish
    while (delayWriting)
    {
        pthread_cond_wait(&cond, &lock); //block condition signal, release locked mutex atomically 
    }

    ++delayReaders;
}

//Unlock datastructure when reading
void read_unlock()
{
    --delayReaders;

	//waits until signalling threads to finish
    while (delayReaders > 0)
    {
        pthread_cond_wait(&cond, &lock); //block condition signal, release locked mutex atomically 
    }

    pthread_cond_signal(&cond); //wake up thread
    pthread_mutex_unlock(&lock); //unlock mutex
}

//Lock datastructure when writing
void write_lock()
{
	//locks the mutex
    pthread_mutex_lock(&lock);

    while (delayWriting)
    {
        pthread_cond_wait(&cond, &lock); //calling thread block on condition signal, release locked mutex atomically 
    }
}

//Unlock datastructure when writing
void write_unlock()
{
    delayWriting = true;

    while (delayReaders > 0)
    {
        pthread_cond_wait(&cond, &lock); //block condition signal, release locked mutex atomically 
    }

    delayWriting = false;

    pthread_cond_broadcast(&cond); //unblock all threads
    pthread_mutex_unlock(&lock); //re-lock mutex
}

//Returns true if chunk is in freed chunks list
bool allocate_chunk(Chunk chunk)
{
    bool chunkAllocated = false;

	chunksFreed.remove(chunk);
	chunksAllocated.push_front(chunk);

    return chunkAllocated;
}

//Freed the chunks from allocation
bool free_chunk(Chunk chunk)
{
    bool chunkAllocated = false;
	chunksFreed.push_front(chunk);
	chunksAllocated.remove(chunk);

    pthread_mutex_unlock(&chunk.instance_lock);

    return chunkAllocated;
}

//Creates a new chunk
void create_new_chunk(size_t chunk_size, intptr_t memory_address, bool isFree)
{
	Chunk chunk = { (size_t)chunk_size, (intptr_t)memory_address };

	if (isFree)
	{
		chunksFreed.push_front(chunk);
	}
	else
	{
		chunksAllocated.push_front(chunk);
	}
}


//Used for analysis, detail information is being displayed when the program ends
void get_average_fragmentation()
{
    float fragmentationPerCent = 0; //percentage of fragmentation
	size_t sizeAllocated = 0;
	size_t sizeFreed = 0;
    void *program_edge = sbrk(0);

    std::list<Chunk>::iterator iterate;

	//Iterate through freed chunks if chunksFreed list size is greater than 0
    if (chunksFreed.size() > 0)
    {
        for (iterate = chunksFreed.begin(); iterate != chunksFreed.end(); iterate++)
        {
			sizeFreed += iterate->chunk_size;
        }
    }

	//Iterate through allocated chunks if chunksAllocated list size is greater than 0
    if (chunksAllocated.size() > 0)
    {
        for (iterate = chunksAllocated.begin(); iterate != chunksAllocated.end(); iterate++)
        {
			sizeAllocated += iterate->chunk_size;
        }
    }

	fragmentationPerCent = 1 - ((float)sizeAllocated / ((float)sizeAllocated + (float)sizeFreed));	//fragmentation percentage calculation

    std::cout << "Chunks Allocated: " << chunksAllocated.size() << " Chunks Freed: " << chunksFreed.size() << std::endl;
    std::cout << "Bytes Allocated: " << sizeAllocated << " Bytes Freed: " << sizeFreed << std::endl;
	std::cout << "\nCHUNK STATISTIC" << std::endl;
	std::cout << "Average chunk size Allocated: " << sizeAllocated / chunksAllocated.size() << " bytes" << std::endl;
	std::cout << "Average chunk size Freed: " << sizeFreed / chunksFreed.size() << " bytes" << std::endl;
    std::cout << "Percentage of Fragmentation: " << fragmentationPerCent * 100 << " %" << std::endl;
	std::cout << "\nTIME" << std::endl;
	std::cout << "Allocation time: " << allocationTime << " ms" << std::endl;
	std::cout << "Deallocation time: " << deallocationTime << " ms" << std::endl;
	std::cout << "Reallocation time: " << reallocationTime << " ms" << std::endl;
	std::cout << "\nPROGRAM EDGE" << std::endl;
	std::cout << "New program edge: " << (int*)program_edge << std::endl;
}
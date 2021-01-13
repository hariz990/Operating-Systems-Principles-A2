
#include "Allocation.cpp"
#include <pthread.h>


/* PROGRAM SETTINGS */
const int THREADS_VALUE = 8;				//number of threads 
int dataLength = 200;						//amount of data each thread allocates
std::string file_name = "names.txt";		//name of text file


//LIST & THREADS 
std::list<char *> dataList;
pthread_t threads[THREADS_VALUE];
pthread_mutex_t dataMutex;

//VARIABLES
int numberOfAlloc = 0;
int numberOfDealloc = 0;
int data = 0;
int* oldProgramEdge = 0;
int totalWords = 0;
long int fileSize = 0;
std::string get_random_string();

//FUNCTIONS
void *allocate_random_data(void *arg);
void *deallocate_random_data(void *arg);
void join_threads();
void get_total_words();
void get_file_size();

int main(int argc, char **argv)
{
	get_total_words();	//getting total words in the file
	get_file_size();	//getting the size of the file 

    //A timer to time the algorithms
    std::clock_t clockStart;

    bool programError = false;

    //Checking the arguments, setting the allocation type bestfit, worstfit or firstfit
    if (argc == 2)
    {
        std::string arg = argv[1];
        if (arg == "bestfit")
        {
			algorithm = BEST_FIT;
        }
        else if (arg == "worstfit")
        {
			algorithm = WORST_FIT;
        }
        else if (arg == "firstfit")
        {
			algorithm = FIRST_FIT;
        }
        else
        {
			programError = true;
        }
    }

    //If arguments are invalid, show error message
    if (argc != 2 || programError)
    {
        std::cout << "INVALID! Choose firstfit/bestfit/worstfit EXAMPLE: ./allocate firstfit" << std::endl;
        return EXIT_FAILURE;
    }


	std::cout << "*****PROGRAM BEGIN*****" << std::endl;
    std::cout << "Algorithm choice: " << argv[1] << std::endl;

	void* b = sbrk(0);	//get current program edge
    int *p = (int *)b;	//cast edge as int to see address
	oldProgramEdge = p;		//storing old program edge for printing

    std::cout << "Old program edge: " << p << std::endl; //print out old program edge

	std::cout << "\n*****PROGRAM STATUS*****" << std::endl;
	
	/* Allocation Process */

	//Start time
	clockStart = std::clock();	

    std::cout << "Allocating data..." << std::endl;

	//Allocates random data from a text file
    for (int i = 0; i < THREADS_VALUE; ++i)
    {
        int result = pthread_create(&threads[i], NULL, allocate_random_data, NULL); //creates a new thread 

        if (result) //if result is empty 
        {
            std::cout << "Thread " << i << " Error: " << std::strerror(result) << std::endl;
        }
    }

    //Wait for threads to finish
    join_threads();

    //Calculate final allocation time
    allocationTime = (std::clock() - clockStart) / (double)(CLOCKS_PER_SEC / 1000);

	
	/* Deallocation Process */

    std::cout << "Deallocating data..." << std::endl;

	//Start time
	clockStart = std::clock();

    //Deallocate data
    for (int i = 0; i < THREADS_VALUE; ++i)
    {
        int result = pthread_create(&threads[i], NULL, deallocate_random_data, NULL); //creates new thread 

        if (result) //if result is empty
        {
            std::cout << "Thread " << i << " Error: " << std::strerror(result) << std::endl;
        }
    }

    //Wait for threads to finish
    join_threads();

	//Calculate final deallocation time
    deallocationTime = (std::clock() - clockStart) / (double)(CLOCKS_PER_SEC / 1000);


	/* Reallocation Process */

    std::cout << "Reallocating random data from text file..." << std::endl;

	//Start timeS
	clockStart = std::clock(); 

	//Reallocating data 
    for (int i = 0; i < THREADS_VALUE; ++i)
    {
        int result = pthread_create(&threads[i], NULL, allocate_random_data, NULL);

        if (result)
        {
            std::cout << "Thread " << i << " Error: " << std::strerror(result) << std::endl;
        }
    }

	//Wait for threads to finish
    join_threads(); 

	//Calculate final reallocation time
	reallocationTime = (std::clock() - clockStart) / (double)(CLOCKS_PER_SEC / 1000);

    b = sbrk(0);  //get current program edge
    p = (int *)b; //cast edge as int to see address

	std::cout << "\n*****PROGRAM SUMMARY*****" << std::endl;
	std::cout << "FILE PROPERTIES" << std::endl;
	std::cout << "Text file: " << file_name << std::endl;
	std::cout << "Total words: " << totalWords << " words" << std::endl;
	std::cout << "File size: " << fileSize << " bytes" << std::endl;
	std::cout << "\nPROCESS" << std::endl;
	std::cout << "Allocations: " << numberOfAlloc << " Deallocations: " << numberOfDealloc << std::endl; //how many allocations and deallocations the program did
    get_average_fragmentation(); //displaying detailed analysis 
	std::cout << "Old program edge: " << oldProgramEdge << "\n" << std::endl;

    return 0;
}

void * allocate_random_data(void *arg)
{
    for (int i = 0; i < dataLength; i++)
    {
        pthread_mutex_lock(&dataMutex);										//creates a program object so multiple threads can take turns sharing data file
        std::string random_data = get_random_string();                     //gets a random data
        char *data_from_file = &random_data[0];                            //cast random data into a char*
        char *data_ = (char *)alloc(random_data.size() * sizeof(char) + 1); //allocate memory for the string
        strcpy(data_, data_from_file);                                      //set the value of the char variable data to the data from the string
        ++numberOfAlloc;
		dataList.push_front(data_); //add the data to the data list
		data++;
        pthread_mutex_unlock(&dataMutex); //unlock data mutex
    }
    
    return NULL;
}

void *deallocate_random_data(void *arg)
{
    std::list<char *>::iterator it;
    while(data > 0) {
        if (pthread_mutex_trylock(&dataMutex) == 0)
        {
            char* data_ = dataList.front();
			dataList.remove(data_);
			data--;
            dealloc((void *)data_);
            ++numberOfDealloc;
            pthread_mutex_unlock(&dataMutex);
        }
    }

    return NULL;
}

//Waits for the thread specified by thread to terminate
void join_threads()
{
    for (int i = 0; i < THREADS_VALUE; ++i)
    {
        int result = pthread_join(threads[i], NULL);
        if (result != 0)
        {
            fprintf(stderr, "%s\n", strerror(result));
        }
    }
}

//Gets a random string from a text file
std::string get_random_string()
{
    srand(time(0) + randomIncrement);
	int dataNum = 0 + (std::rand() % (totalWords)); 

    std::ifstream in(file_name);
    std::string dataString; //the string used to hold each data in the text file

    //read 20 lines from the text file
    while (std::getline(in, dataString) && dataNum >= 0)
    {
		dataNum--;
    }

	randomIncrement++;
    return dataString;
}

//Calculate total words from a text file
void get_total_words()
{
	std::ifstream in(file_name);
	std::string word; 

	while (in >> word)
	{
		totalWords++;
	}

}

//Calculate the size of files in bytes
void get_file_size()
{
	int n = file_name.length();
	char* fileName = new char[n+1];

	// copying the contents of the string to char array 
	strcpy(fileName, file_name.c_str());

	// opening the file in read mode 
	FILE* fp = fopen(fileName, "r");
	fseek(fp, 0L, SEEK_END);

	// calculating the size of the file 
	fileSize = ftell(fp);

	// closing the file 
	fclose(fp);
}
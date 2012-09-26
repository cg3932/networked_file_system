//--------------------------------------------------//
// Christian Gallai - 260218797
// disk_emu.c
// ECSE 427 - Fall 2009 - Programming Assignment 1
// October 11, 2009
//--------------------------------------------------//


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DIRTY_BLOCK 1
#define CLEAN_BLOCK 0
#define INVALID 2

//Global Variables
FILE *file_disk = NULL;
int n_reads, n_writes;
double latency, probf, rand_gen;
int size_block, size_cache, use_cache;

//Cache Struct
struct cache_struct
{char* buffer;
 int flag;
 int address;
 struct cache_struct* next;
 struct cache_struct* prev;
 };
 
struct cache_struct* block_cache;
struct cache_struct *head, *tail;

//functions
//int init_disk(char *filename, int block_size, int num_blocks, double l, double p);
//int search_cache(int search_address);
//int read_blocks(int start_address, int nblocks, void *buffer);
//int write_blocks(int start_address, int nblocks, void *buffer);
//int init_cache(int cache_size);
//void append_node(struct cache_struct *linknode);
//void remove_node(struct cache_struct *linknode);
//int cache_miss(int address, void *buffer);
//int flush_cache(void);


//BEGIN


int init_disk(char *filename, int block_size, int num_blocks, double l, double p){
	//initialize the values
	size_block= block_size;
	latency = l*1000;
	probf = p;
	
    file_disk = fopen(filename, "r+b");
    
    	if (file_disk == NULL) {
        	file_disk = fopen(filename, "w+b");
	}
    
    fseek(file_disk, block_size*num_blocks, SEEK_SET);
    fwrite(" ", 1, 1, file_disk);

    return 0;
}

int search_cache(int search_address){
	int found = 0, t = 0;
	
	for (t=0; t<size_cache; t++)
		{   	if (block_cache[t].address == search_address){
				found = 1;
				break;
			}
		}
	
	//if found return index of block, if not then return -1
	if (found == 0) {
		return -1;
	}
	else {
		return t;
	}
}

int read_blocks(int start_address, int nblocks, void *buffer){
	// simulate latency
	usleep(latency);

	//Reset failure count for function
	int num_failures_r = 0, num_successful_blocks_r = 0;
	
    if(file_disk==NULL) {
    printf("Error: can't access file.c.\n");
    return -1;
  }
  else {
	
    if (use_cache == 0){
        //non-cached mode, read from file

        fseek(file_disk, start_address*size_block, SEEK_SET);
        fread(buffer, size_block, nblocks, file_disk);
		
		//simulate failure for each read done
		 int sim_f_r=0;
		for (sim_f_r=0;sim_f_r<nblocks;sim_f_r++){
			rand_gen = ((double)random() * 1.0 / RAND_MAX);
			if (rand_gen < probf) {
				num_failures_r++;
			}
		}
		
    }
    else{
        //cached mode, read from cache
        //check cache to see if it contains block we want to read
        int n = 0, block_search_r = 0;
	int address_index_r = start_address;
	
		
        for (n=0; n<nblocks; n++)
        {	
			//Simulate failure rate of function
			rand_gen = ((double)random() * 1.0 / RAND_MAX);
			if (rand_gen > probf) {
			//the read function does not fail, continue as normal
			
				// search for the block in the cache
				block_search_r = search_cache(address_index_r);
			
				if (block_search_r >= 0){
					//block was found in the cache at location (block_search_r), copy block from the cache
					memcpy(buffer, block_cache[block_search_r].buffer, size_block);
					block_cache[block_search_r].flag = DIRTY_BLOCK;
				
					//remove block from current and position in cache and set block to tail of list, i.e. MRU (Most Recently Used)
					remove_node(&block_cache[block_search_r]);
					append_node(&block_cache[block_search_r]);
				}
				else {
					//block not found in cache, cache miss has occured
					//read the requested block from the file and call cache_miss to store block into cache and remove LRU
					fseek(file_disk, address_index_r*size_block, SEEK_SET);
					fread(buffer, size_block, 1, file_disk);
					cache_miss(address_index_r, buffer);
				}
			}
			else {
			// count the number of failures
			num_failures_r++;
			}
		
		//increment address for next block
		address_index_r++;

        }
           
}
num_successful_blocks_r = nblocks - num_failures_r;
return num_successful_blocks_r;
}
}


int write_blocks(int start_address, int nblocks, void *buffer){
	// simulate latency
	usleep(latency);

	//Reset failure count for function
	int num_failures_w = 0, num_successful_blocks_w = 0;
	
    if(file_disk==NULL) {
    printf("Error: can't create file for writing.\n");
    return -1;
  }
  else {
  
    if (use_cache == 0){
    //non-cached mode, write to file
        fseek(file_disk, start_address*size_block, SEEK_SET);
        fwrite(buffer, size_block, nblocks, file_disk); /* grab all the text */
		
		//simulate failure for each read done
		int sim_f_w=0;
		for (sim_f_w=0;sim_f_w<nblocks;sim_f_w++){
			rand_gen = ((double)random() * 1.0 / RAND_MAX);
			if (rand_gen < probf) {
				num_failures_w++;
			}
		}
    }
    else{
    //cached mode, write to cash
	//search if block is already in the cache
	int m = 0, block_search_w = 0;
	int address_index_w = start_address;
	
	for (m=0; m<nblocks;m++)
	{
		//Simulate failure rate of function
		rand_gen = ((double)random() * 1.0 / RAND_MAX);
		if (rand_gen > probf) {
		//the read function does not fail, continue as normal
			
			// search for the block in the cache
			block_search_w = search_cache(address_index_w);
			
			if (block_search_w >= 0){
					//block was found in the cache at location block_search_w, set block flag to "DIRTY"
					memcpy(block_cache[block_search_w].buffer, buffer, size_block);
					block_cache[block_search_w].flag = DIRTY_BLOCK;
				
					//remove block from current position in cache and set block to tail of list, i.e. MRU (Most Recently Used)
					remove_node(&block_cache[block_search_w]);
					append_node(&block_cache[block_search_w]);					
					
				}
			else {
					//block not found in cache, cache miss has occured
					//call cache_miss to store block into cache and remove LRU
					cache_miss(address_index_w, buffer);
				}
		}
		else{
			// count the number of failures
			num_failures_w++;
		}
		
		//increment address for next block
		address_index_w++;
    }
	}
num_successful_blocks_w = nblocks - num_failures_w;
return num_successful_blocks_w;
}
}

int init_cache(int cache_size){
	//initialize cache values
	size_cache = cache_size;
	if (cache_size > 0){
		use_cache = 1;
	} else {
		use_cache = 0;
	}
	
    if (use_cache==0){
       //non-cached mode, do not create cache
	return 0;
    }
    else{
         //cached mode, create cache
    	int i = 0;
    	block_cache = calloc(cache_size, sizeof(struct cache_struct));
		
		//initialize head and tail of cache
		head=&block_cache[0];
		tail=&block_cache[cache_size-1];
	
    	for (i=0; i<cache_size; i++){
        	block_cache[i].flag = INVALID;
        	block_cache[i].buffer = malloc(size_block);
			
			//link all the blocks together
			if (i==0){
			//first entry, i.e. head
				block_cache[i].next = &block_cache[i+1];
				block_cache[i].prev = NULL;
			}	
			else if (i==(cache_size-1)){
			//last entry i.e. tail
				block_cache[i].next = NULL;
				block_cache[i].prev = &block_cache[i-1];
			}
			else{
			//middle entries
				block_cache[i].next = &block_cache[i+1];
				block_cache[i].prev = &block_cache[i-1];
			}
	}
    return 1;
    }
}

void append_node(struct cache_struct *linknode){
	if(head == NULL) {
		head = linknode;
		linknode->prev = NULL;
	}
	else {
		tail->next = linknode;
		linknode->prev = tail;
 }

 tail = linknode;
 linknode->next = NULL;
}

void remove_node(struct cache_struct *linknode){
	if(linknode->prev == NULL){
		head = linknode->next;
	}
	else {
		linknode->prev->next = linknode->next;
	}

	if(linknode->next == NULL) {
		tail = linknode->prev;
	}
	else {
		linknode->next->prev = linknode->prev;
	}
}


int cache_miss(int address, void *buffer){
		// simulate latency
		usleep(latency);

		//block was not found in the cache, append the most recently used block to the tail of the list
		//if the head node is a dirty block then write it to the disk
		if (head->flag ==DIRTY_BLOCK ){
			fseek(file_disk, (head->address)*size_block, SEEK_SET);
			fwrite(head->buffer, size_block, 1, file_disk);
		}

		//save head node, remove block and add to tail
		struct cache_struct* saveptr = head;

		remove_node(saveptr);
		append_node(saveptr);

		//copy address and buffer into cache at saveptr
		head->flag = CLEAN_BLOCK;
		head->address = address;
		memcpy(head->buffer, buffer, size_block);

	return 0;
}

int flush_cache(void){
	// simulate latency
	usleep(latency);

	//write all the DIRTY blocks to the file
	int flush = 0;

	for(flush=0; flush<size_cache; flush++){
		if (block_cache[flush].flag == DIRTY_BLOCK){
			fseek(file_disk, (block_cache[flush].address)*size_block, SEEK_SET);
       		fwrite(block_cache[flush].buffer, size_block, 1, file_disk);
			block_cache[flush].flag == CLEAN_BLOCK;
		}
	}

	return 0;
}


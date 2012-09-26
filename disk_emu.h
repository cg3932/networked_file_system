int init_disk(char *filename, int block_size, int num_blocks, double L, double p); 
int read_blocks(int start_address, int nblocks, void *buffer);
int write_blocks(int start_address, int nblocks, void *buffer);
int init_cache(int cache_size);

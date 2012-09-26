/*--------------------------------------------------
sfs_api.c

Christian Gallai - 260218797
ECSE 427 - PA2
November 1, 2009
--------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define N 10240
#define M 1024
#define Num_Root_Blocks 1
#define num_max_files 100

// Root Directory
struct Root_D
{	char filename[12];
	char attribute;
	int date;
	int valid_file;
	int filesize;
 	int FAT_index;
 };

// File Allocation Table
struct File_A
{	int DB_index;
	int next;
	int valid_node;
 };

// Free Sector List
struct Free_S
{	int is_free_block;
 };

// File Control Block
struct File_C
{	int RD_index;
	int r_pointer;
	int w_pointer;
	int valid_FCB;
	int last_read_block_FAT;
	int last_written_block_FAT;
 };

// Structures
struct Root_D Root_Directory[Num_Root_Blocks*M/sizeof(struct Root_D)];
struct File_A FAT[N/sizeof(struct File_A)];
struct Free_S FSL[N/sizeof(struct Free_S)];
struct File_C FCB[num_max_files];

// read and write temp arrays
char temp_copy_block_w[M];
char temp_copy_block_r[M];

// Global Variables
int FAT_size = 0, FSL_size = 0;
char *filename = NULL;

int mksfs(int fresh){
	// initialize structure sizes	FAT_size = ceil(sizeof(FAT)/(float)M);
	FSL_size = ceil(sizeof(FSL)/(float)M);

	//initiliaze disk and cache
	if(filename == NULL){
		init_disk("mydiskfile.dsk", M, N, 0, 0);
	} else {
		init_disk(filename, M, N, 0, 0);
	}
	init_cache(0);
	
	if (fresh!=0){
	// intialize new root directory and store onto the disk
	memset(&Root_Directory, 0, sizeof(Root_Directory));
	write_blocks(0, Num_Root_Blocks, &Root_Directory);

	// intialize new File Allocation Table (FAT) and store onto the disk
	memset(&FAT, 0, sizeof(FAT));
	write_blocks(Num_Root_Blocks, FAT_size, &FAT);

	// initialize Free Sector List, all blocks marked as free
	memset(&FSL, 0, sizeof(FSL));
	write_blocks(N - FSL_size, FSL_size, &FSL);
	
	}

	//read file system stored on the disk
	read_blocks(0, Num_Root_Blocks, &Root_Directory);

	//initialize File Control Block
	memset(&FCB, 0, sizeof(FCB));
}

void sfs_ls(void){
	// Traverse the Root Directory and list file names
	int j = 0;
	for(j=0; j<sizeof(Root_Directory)/sizeof(struct Root_D); j++){
		if (Root_Directory[j].valid_file==1){
			// If Root Directory index contains a valid file, print the name
			printf("%s\n", Root_Directory[j].filename);
		}
	}	
}

void nfs_ls(int socket){
	char instr_ls[80];

	// send the filenames to the client. traverse the Root Directory and send the filenames.
	int j = 0;
	for(j=0; j<sizeof(Root_Directory)/sizeof(struct Root_D); j++){
		if (Root_Directory[j].valid_file==1){
			memcpy(&instr_ls[0], "1%", 2);
			strcpy(&instr_ls[2], Root_Directory[j].filename);
			send(socket, instr_ls, 80, 0);
		}
	}	
}

int find_file(char *searchname){
	// Search Root Directory for file and return index
	int f = 0, found_index = -1;
	for(f=0; f<sizeof(Root_Directory)/sizeof(struct Root_D);f++){
		if ((strncmp(Root_Directory[f].filename,searchname, 12) == 0)&&(Root_Directory[f].valid_file==1)){
			found_index = f;
			break;		
		}
	}
	
	return found_index;
}

int find_file_FCB(int RD_index_search){
	// search FCB for RD_index
	int ff = 0, found_RD_index = 0;

	for(ff=0;ff<(sizeof(FCB)/sizeof(struct File_C));ff++){
		if((FCB[ff].RD_index==RD_index_search) && (FCB[ff].valid_FCB == 1)){
			found_RD_index = 1;
			break;
		}
	}

	return found_RD_index;
}

int find_RD_emptyspace(){
	// find an invalid block in the Root Directory and return indice so that file can be placed there
	int t = 0, emptyspace_RD = -1;
	for (t=0;t<(sizeof(Root_Directory)/sizeof(struct Root_D));t++){
		if(Root_Directory[t].valid_file==0){
			// Invalid block, thus we can write to it
			emptyspace_RD = t;
			break;
		}
	}
	
	return emptyspace_RD;
}

int find_FAT_emptyspace(){
	// find an invalid block in the FAT and return indice so that indice can be used
	int g = 0, emptyspace_FAT = -1;
	for(g=0;g<(sizeof(FAT)/sizeof(struct File_A));g++){
		if(FAT[g].valid_node==0){
			emptyspace_FAT = g;
			break;
		}
	}
	
	return emptyspace_FAT;
}

int find_FCB_emptyspace(){
	// find an invalid block in the FCB and return indice so that indice can be used
	int p = 0, emptyspace_FCB = -1;

	for(p=0;p<(sizeof(FCB)/sizeof(struct File_C));p++){
		if(FCB[p].valid_FCB==0){
			emptyspace_FCB = p;
			break;
		}
	}
	return emptyspace_FCB;
}

int find_free_block(){
	// find the first free block in the free sector list and return the indice
	int y = 0, freespace_FSL = 0;
	for(y=0;y<(sizeof(FSL)/sizeof(struct Free_S));y++){
		if(FSL[y].is_free_block==0){
			freespace_FSL = y;
			break;
		}
	}
	
	return freespace_FSL;
}

void update_sfs(){
	// update the file system components from memory onto the disk
	write_blocks(0, Num_Root_Blocks, &Root_Directory);
	write_blocks(Num_Root_Blocks, FAT_size, &FAT);
	write_blocks(N - FSL_size, FSL_size, &FSL);
}

int sfs_open(char *name){
	int found_RD_index = -1, found_FCB = 0, RD_space = 0, FCB_index, FAT_index = 0, initial_FSL_location = 0;

	// Storing/updating file information into File Control Block (FCB)
	found_RD_index = find_file(name);
	
	// Find empty spaces in the Root Directory, FAT, FCB, and FSL	
	RD_space = find_RD_emptyspace();	
	FCB_index = find_FCB_emptyspace();

	if (FCB_index == -1){
		return -1;
	}

	FAT_index = find_FAT_emptyspace();
	initial_FSL_location = find_free_block();

	// check if the file is already in the FCB
	found_FCB = find_file_FCB(found_RD_index);
	
	if (found_RD_index == -1){

		if (RD_space == -1){
			return -1;
		}
		if (FAT_index == -1){
			return -1;
		}

		// File has not been found, create a new file and set the initial size to zero
		// Add to Root Directory
		strcpy(Root_Directory[RD_space].filename, name);
		Root_Directory[RD_space].FAT_index = FAT_index;
		Root_Directory[RD_space].valid_file = 1;
		Root_Directory[RD_space].filesize = 0;
		
		// setup preliminary FAT node
		FAT[FAT_index].DB_index = initial_FSL_location;
		FAT[FAT_index].next = -1;
		FAT[FAT_index].valid_node = 1;
		FSL[initial_FSL_location].is_free_block = 1;
		
		// Add to FCB
		FCB[FCB_index].valid_FCB = 1;
		FCB[FCB_index].RD_index = RD_space;
		FCB[FCB_index].r_pointer = 0;
		FCB[FCB_index].last_read_block_FAT = FAT_index;
		FCB[FCB_index].last_written_block_FAT = FAT_index;
	
		// set the write pointer to the end of the file
		FCB[FCB_index].w_pointer = Root_Directory[RD_space].filesize % M;

		// write back changes
		update_sfs();
		
		return FCB_index;
	} else if ((found_RD_index != -1) && (found_FCB == 0)) {
		// Open existing
		// Add to FCB
		FCB[FCB_index].valid_FCB = 1;
		FCB[FCB_index].RD_index = found_RD_index;
		FCB[FCB_index].r_pointer = 0;
		FCB[FCB_index].last_read_block_FAT = Root_Directory[found_RD_index].FAT_index;
		
		// traverse FAT to find last written block
		int FAT_index_search = Root_Directory[found_RD_index].FAT_index;
		while(FAT[FAT_index_search].next != -1){
			FAT_index_search = FAT[FAT_index_search].next;
		}
		FCB[FCB_index].last_written_block_FAT = FAT_index_search;

		// set the write pointer to the end of the file
		FCB[FCB_index].w_pointer = Root_Directory[found_RD_index].filesize % M;

		// write back changes
		update_sfs();

		return FCB_index;
	} else {
		return -1;
	}

	
}

int sfs_close(int fileID){
	
	if (FCB[fileID].valid_FCB == 1){
		// remove file from the FCB
		FCB[fileID].valid_FCB = 0;
	
		// write back changes
		update_sfs();
	
		return 0;
	} else {
		return -1;
	}
}

int allocate_mem_block_write(int last_filled_block){
	int free_FAT_loc_w = 0, free_FSL_location_w = 0; 
	
	// allocate FAT and FSL space
	free_FAT_loc_w = find_FAT_emptyspace();
	free_FSL_location_w = find_free_block();

	if(free_FAT_loc_w < 0){
		return -1;
	}

	if(free_FSL_location_w < 0){
		return -1;
	}

	FAT[last_filled_block].next = free_FAT_loc_w;
	FAT[free_FAT_loc_w].DB_index = free_FSL_location_w;
	FAT[free_FAT_loc_w].next = -1;
	
	// mark used blocks as used
	FAT[free_FAT_loc_w].valid_node = 1;
	FSL[free_FSL_location_w].is_free_block = 1;
	
	return free_FAT_loc_w;
}

int sfs_write(int fileID, char *buf, int length){
	int w = 0, block_to_write = 0, begin_pointer_w = 0, end_pointer_w = 0, w_length = 0, bytes_written = 0;
	int indiv_byte_written = 0, num_blocks_w = 0, bytes_left_to_write = 0, read_init_block = 0;
	
	// check that the file is valid
	if(FCB[fileID].valid_FCB == 0){
		return -1;
	}

	// prepare initial parameters for write loop
	block_to_write = FCB[fileID].last_written_block_FAT;
	begin_pointer_w = FCB[fileID].w_pointer;

	// set the end pointer for the write
	if ((begin_pointer_w + length) > M){
		end_pointer_w = M;
	} else {
		end_pointer_w = begin_pointer_w + length;
	}
	w_length = (end_pointer_w - begin_pointer_w);
	bytes_left_to_write = length;

	// for loop to write all blocks
	while(bytes_left_to_write > 0){
		// read the current block information into array
		read_init_block = read_blocks(FAT[block_to_write].DB_index + (Num_Root_Blocks + FAT_size), 1, temp_copy_block_w);
	
		// copy information from buffer and write block to the file
		memcpy(&temp_copy_block_w[begin_pointer_w], buf, w_length);
		indiv_byte_written = write_blocks(FAT[block_to_write].DB_index + (Num_Root_Blocks + FAT_size), 1, temp_copy_block_w);
		bytes_written += w_length;
		buf += w_length;
		bytes_left_to_write -= w_length;
	
		// modify changed parameters
		if (end_pointer_w == M){
			FCB[fileID].w_pointer = 0;
		} else {
			FCB[fileID].w_pointer = end_pointer_w;
		}
		Root_Directory[FCB[fileID].RD_index].filesize += w_length;
		FCB[fileID].last_written_block_FAT = block_to_write;
	
		// if there is another loop iteration, allocate new memory, and modify new parameters 
		if (bytes_left_to_write > 0){
	
			// allocate memory for the next block to be written
			block_to_write = allocate_mem_block_write(block_to_write);

			if(block_to_write < 0){
				return -1;
			}

			// set begin pointer
			if (end_pointer_w == M){
				begin_pointer_w = 0;
			} else {
				begin_pointer_w = end_pointer_w;
			}
		
			// set the end pointer for the write
			if ((begin_pointer_w + bytes_left_to_write) > M){
				end_pointer_w = M;
			} else {
				end_pointer_w = begin_pointer_w + bytes_left_to_write;
			}
			w_length = (end_pointer_w - begin_pointer_w);
		}
	}

	return bytes_written;
}

int sfs_read(int fileID, char *buf, int length){
	int r = 0, block_to_read = 0, begin_pointer_r = 0, end_pointer_r = 0, r_length = 0, bytes_read = 0;
	int indiv_byte_read = 0, num_blocks_r = 0, bytes_left_to_read = 0, Read_EOF = 0;

	// check that the file is valid
	if(FCB[fileID].valid_FCB == 0){
		return -1;
	}

	// prepare initial parameters for read loop
	block_to_read = FCB[fileID].last_read_block_FAT;
	begin_pointer_r = FCB[fileID].r_pointer;
	
	// set the end pointer for the read
	if ((begin_pointer_r + length) > M){
		end_pointer_r = M;
	} else {
		end_pointer_r = begin_pointer_r + length;
	}
	r_length = (end_pointer_r - begin_pointer_r);
	bytes_left_to_read = length;
	
	// for loop to write all blocks
	while((bytes_left_to_read > 0) && (Read_EOF == 0)){

		// Monitor read at the end of the file
		if (FAT[block_to_read].next == -1){
			// have reached the end of the file in this block
			// copy last information from buffer and read block to the file
			if (end_pointer_r > FCB[fileID].w_pointer){
				end_pointer_r = FCB[fileID].w_pointer;
			}
			r_length = (end_pointer_r - begin_pointer_r);


			indiv_byte_read = read_blocks(FAT[block_to_read].DB_index + (Num_Root_Blocks + FAT_size), 1, temp_copy_block_r);
			memcpy(buf, temp_copy_block_r + begin_pointer_r, r_length);
			bytes_read += r_length;
			buf += r_length;
			bytes_left_to_read -= r_length;

			Read_EOF = 1;
		} else {
			// have not reached the end of the file in this block, continue as normal
			// copy information from buffer and read block to the file
			indiv_byte_read = read_blocks(FAT[block_to_read].DB_index + (Num_Root_Blocks + FAT_size), 1, temp_copy_block_r);
			memcpy(buf, temp_copy_block_r + begin_pointer_r, r_length);
			bytes_read += r_length;
			buf += r_length;
			bytes_left_to_read -= r_length;
			
		}


		
		// modify changed parameters
		if (end_pointer_r == M){
			FCB[fileID].r_pointer = 0;
		} else {
			FCB[fileID].r_pointer = end_pointer_r;
		}
		FCB[fileID].last_read_block_FAT = block_to_read;
		
		// if there is another loop iteration, modify new parameters 
		if (bytes_left_to_read > 0){
			// get the next block in the file
			block_to_read = FAT[block_to_read].next;
		
			// set begin pointer
			if (end_pointer_r == M){
				begin_pointer_r = 0;
			} else {
				begin_pointer_r = end_pointer_r;
			}
			
			// set the end pointer for the read
			if ((begin_pointer_r + bytes_left_to_read) > M){
				end_pointer_r = M;
			} else {
				end_pointer_r = begin_pointer_r + bytes_left_to_read;
			}
			r_length = (end_pointer_r - begin_pointer_r);
		}
	}
	
	return bytes_read;
}

int sfs_remove(char *file){
	int find_file_RD = 0, FAT_remove_index = 0, finish_remove = 0;
	find_file_RD = find_file(file);

	// Check if the file exists, if not return error
	if (find_file_RD == -1){
		return -1;
	}	
	
	// remove the file from the root directory
	Root_Directory[find_file_RD].valid_file = 0;
		
	// Remove file from FAT and FSL
	FAT_remove_index = Root_Directory[find_file_RD].FAT_index;
	do{
		FSL[FAT[FAT_remove_index].DB_index].is_free_block = 0;
		FAT[FAT_remove_index].valid_node = 0;
		
		// check for EOF
		if (FAT[FAT_remove_index].next == -1){
			finish_remove = 1;
		}
		
		// increment FAT index to remove next
		FAT_remove_index = FAT[FAT_remove_index].next;
		
	} while(finish_remove == 0);

	
	// write back changes
	update_sfs();
	
	return 0;
}

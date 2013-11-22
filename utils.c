#define _LARGEFILE64_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/md5.h>

unsigned char *mem_hashes = NULL;
unsigned long int nmem_hashes = 0;

/*Calc MD5 sum of the memory block*/
unsigned char *get_mem_buf_hash(unsigned char *mem_buf, unsigned long int mem_size){
	unsigned char *hash, *tmp;

	hash = malloc(MD5_DIGEST_LENGTH);
	tmp = MD5(mem_buf, mem_size, NULL);

	memcpy(hash, tmp, MD5_DIGEST_LENGTH);

	return hash;
}

unsigned char *hash_to_str(unsigned char *mem_hash){
	int i;
	unsigned char *hash;

	hash = malloc(MD5_DIGEST_LENGTH * 2 + 1);

	for(i = 0; i < MD5_DIGEST_LENGTH; i++)
		sprintf(&hash[i*2], "%02x", mem_hash[i]);
	return hash;
}

/*Check if a block of memory has been dumped already (by MD5-sum)*/
int is_mem_buf_dumped(unsigned char *mem_hash){
	unsigned long int i;

	if(nmem_hashes == 0){
		return 0;
	}

	for(i = 0; i < nmem_hashes; i++){
		if(memcmp(mem_hash, mem_hashes + MD5_DIGEST_LENGTH * i, MD5_DIGEST_LENGTH) == 0){
			return 1;
		}
	}

	return 0;
}

/*Dump a block of memory to a file descriptor*/
void dump_mem_buf(unsigned char *mem_buf, unsigned long int mem_size, char *fname){
	unsigned char *mem_hash;

	mem_hash = get_mem_buf_hash(mem_buf, mem_size);

	if(!is_mem_buf_dumped(mem_hash)){
		if(nmem_hashes == 0){
			mem_hashes = malloc(MD5_DIGEST_LENGTH);
			memcpy(mem_hashes, mem_hash, MD5_DIGEST_LENGTH);
		}else{
			mem_hashes = realloc(mem_hashes, MD5_DIGEST_LENGTH * (nmem_hashes + 1));
			memcpy(mem_hashes + MD5_DIGEST_LENGTH * nmem_hashes, mem_hash, MD5_DIGEST_LENGTH);
		}
		nmem_hashes++;
	}

	free(mem_hash);
	
	FILE *f;
	f = fopen(fname, "wb");
	fwrite(mem_buf, 1, mem_size, f);
	fclose(f);

	return;
}

void clean_mem_buf_hashs(void){
	free(mem_hashes);
}
#ifndef IFACE_H_INCLUDED
#define IFACE_H_INCLUDED

#include <fcntl.h>

typedef struct funcs_t {
	unsigned char *(*get_mem_buf_hash)(unsigned char *mem_buf, unsigned long int mem_size);
	unsigned char *(*hash_to_str)(unsigned char *mem_hash);
	int (*is_mem_buf_dumped)(unsigned char *mem_hash);
	void (*dump_mem_buf)(unsigned char *mem_buf, unsigned long int mem_size, char *fname);
} funcs_t;

typedef struct plugin_t {
	void *hndl;
	char *name;
	char *description;
	struct plugin_t *next;
} plugin_t;

#endif
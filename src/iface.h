#ifndef IFACE_H_INCLUDED
#define IFACE_H_INCLUDED

typedef struct plugin_t {
	void *hndl;
	char *name;
	char *description;
} plugin_t;

#ifdef __cplusplus
#include "libs/cpp-bitstring/Bits.h"
extern "C" plugin_t *init();
extern "C" void process(unsigned char *mem_buf, unsigned long int mem_size);
#endif

#endif

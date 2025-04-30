#ifndef __MEM_DEBUG_H__
#define __MEM_DEBUG_H__

typedef unsigned int size_t;
void *jj01_malloc(unsigned int num_bytes,const char*fun ,int line);
void jj01_free(void * ptr,const char*fun ,int line);
void *jj01_realloc(void *mem_address, unsigned int newsize,const char*fun ,int line);
void *jj01_calloc(size_t n, size_t size,const char*fun ,int line);

#include "stdlib.h"
#define malloc(x)  jj01_malloc((x),__FUNCTION__,__LINE__)
#define realloc(a,b)  jj01_realloc((a),(b),__FUNCTION__,__LINE__)
#define calloc(a,b)  jj01_calloc((a),(b),__FUNCTION__,__LINE__)

#define free(x)  jj01_free((x),__FUNCTION__,__LINE__)

#endif

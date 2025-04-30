#include <stdint.h>
#include <time.h>
#undef malloc 
#undef realloc
#undef calloc
#undef free
#include "stdlib.h"//防止malloc/free被替换
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>
#define gettid syscall(SYS_gettid)
extern FILE* mem_debug_fp;
static int total_mem[65536] = {0};
static uint64_t __get_monotonic_time_ms()
{
    struct timespec ts = {0};
    clock_gettime(CLOCK_MONOTONIC, &ts);

    return ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}

void *jj01_malloc(unsigned int num_bytes,const char*fun ,int line)
{
	void * ret = malloc(num_bytes+8);
	char buf[256];
	int tid = gettid;
	total_mem[tid] += num_bytes;
	int bytes = snprintf(buf, sizeof(buf), "%llu %d [%s,%d]malloc %u %p total %d\n", __get_monotonic_time_ms(), gettid, fun, line, num_bytes, ret, total_mem[tid]);
	fwrite(buf, bytes, 1, mem_debug_fp);
	fflush(mem_debug_fp);
	*(int*)ret = num_bytes;
	return (char*)ret+8;
}
void *jj01_realloc(void *mem_address, unsigned int newsize,const char*fun ,int line)
{
	char* orig_addr = (char*)mem_address - 8;
	int orig_size = *(int*)orig_addr;
	void * ret = realloc(orig_addr,newsize+8);
	char buf[256];
	int tid = gettid;
	total_mem[tid] += newsize - orig_size;
	int bytes = snprintf(buf, sizeof(buf), "%llu %d [%s,%d]realloc %u %p total %d\n", __get_monotonic_time_ms(), gettid, fun, line, newsize - orig_size, ret, total_mem[tid]);
	fwrite(buf, bytes, 1, mem_debug_fp);
	fflush(mem_debug_fp);
	*(int*)ret = newsize;
	return (char*)ret+8;
}
void *jj01_calloc(size_t n, size_t size,const char*fun ,int line)
{
	void * ret = calloc(n,size+8);
	char buf[256];
	int tid = gettid;
	total_mem[tid] += n*size;
	int bytes = snprintf(buf, sizeof(buf), "%llu %d [%s,%d]calloc %u %p total %d\n", __get_monotonic_time_ms(), gettid, fun, line, n*size, ret, total_mem[tid]);
	fwrite(buf, bytes, 1, mem_debug_fp);
	fflush(mem_debug_fp);
	*(int*)ret = n*size;
	return (char*)ret+8;
}
void jj01_free(void * ptr,const char*fun ,int line)
{
	char buf[256];
	char* orig_ptr = (char*)ptr - 8;
	int size = *(int*)orig_ptr;
	int tid = gettid;
	total_mem[tid] -= size;
	int bytes = snprintf(buf, sizeof(buf), "%llu %d [%s,%d]free %u %p total %d\n", __get_monotonic_time_ms(), gettid, fun, line, size, ptr, total_mem[tid]);
	fwrite(buf, bytes, 1, mem_debug_fp);
	fflush(mem_debug_fp);
	free(orig_ptr);
}


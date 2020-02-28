typedef struct mem_t_
{
    struct mem_t_* next;
    void* ptr;
    size_t size;
} mem_t;

__thread mem_t* mem_head;
__thread mem_t* free_list;
__thread mem_t mem_free[4096];
__thread int free_num = 4096;

void add_to_list(mem_t* p, mem_t** list_head)
{
    p->next = *list_head;
    *list_head = p;
}

void remove_from_list(mem_t* p, mem_t** list_head)
{
    mem_t* q = *list_head;
    if (p == q){
        *list_head = p->next;
        return;
    }
    while(q && q->next){
        if (q->next == p){
            q->next = p->next;
            return;
        }
        q = q->next;
    }
}

mem_t* find_mem(void* ptr, mem_t** list_head)
{
    mem_t* p = *list_head;
    while(p){
        if (p->ptr == ptr){
            break;
        }
        p = p->next;
    }

    return p;
}

#define __USE_GNU
#include <dlfcn.h>
#include <string.h>

static void* (*libc_malloc)(size_t) = 0;
static void  (*libc_free)(void *) = 0;

int mem_size_total= 0;

void itoa(unsigned int i, char* buf)
{
    if (i==0){
        buf[0]='0';
        buf[1]='\0';
        return;
    }
    char buf2[1024];
    char* pos = buf2;
    while(i > 0){
       *pos = (i % 10) + '0';
       pos++;
       i = i / 10;
    }
    int len = pos - buf2 - 1;
    for(i = 0; i < pos - buf2; i++){
        buf[i] = buf2[len - i];
    }
    buf[i] = '\0';
    return;
}

void * malloc(size_t size)
{
    if(!libc_malloc)
    {
        libc_malloc = (void *(*)(size_t))dlsym(RTLD_NEXT, "malloc"); dlerror();
    }
    void *ptr = NULL;
    char buf[1024];
    write(STDOUT_FILENO, "&&&&& malloc ", strlen("&&&&& malloc "));
    itoa(size, buf);
    write(STDOUT_FILENO, buf, strlen(buf));
    write(STDOUT_FILENO, " bytes at \n", strlen(" bytes at \n"));
    ptr = libc_malloc(size);
    itoa((int)ptr, buf);
    write(STDOUT_FILENO, buf, strlen(buf));
    write(STDOUT_FILENO, ", called from ", strlen(", called from "));
    itoa((int)__builtin_return_address(0), buf);
    write(STDOUT_FILENO, buf, strlen(buf));
    write(STDOUT_FILENO, ", total: ", strlen(", total: "));
    itoa(mem_size_total, buf);
    write(STDOUT_FILENO, buf, strlen(buf));
    write(STDOUT_FILENO, " &&&&&\n", strlen(" &&&&&\n"));
    //printf("&&&&& malloc %d bytes at %p, called from %p, total(%d) &&&&&\n", size, ptr, __builtin_return_address(0), mem_size_total);
    if (!ptr){
        printf("malloc failed\n");
        return NULL;
    }
    mem_t* pmem = NULL;
    if (free_num > 0){
        pmem = &mem_free[free_num - 1];
        free_num--;
    }
    if (!pmem){
        if (!free_list){
            free(ptr);
            printf("*************mem used out***********\n");
            return NULL;
        }
        pmem = free_list;
        free_list = free_list->next;
    }
    __sync_fetch_and_add(&mem_size_total, size);
    pmem->ptr = ptr;
    pmem->size = size;
    add_to_list(pmem, &mem_head);
    return ptr;
}

void free(void *ptr)
{
    mem_t* pmem = find_mem(ptr, &mem_head);
    if (pmem){
        __sync_fetch_and_sub(&mem_size_total, pmem->size);
        printf("&&&&& free %d bytes at %u, called from %u ,total(%d)&&&&&\n", pmem->size, (unsigned int)pmem->ptr, (unsigned int)__builtin_return_address(0), mem_size_total);
        remove_from_list(pmem, &mem_head);
        add_to_list(pmem, &free_list);
        if(!libc_free)
        {
            libc_free = (void (*)(void *))dlsym(RTLD_NEXT, "free"); dlerror();
        }
        libc_free(ptr);
    }
    else{
        printf("**********mem not found*************\n");
    }
}
#include <strings.h>

void* calloc(size_t nmemb, size_t size)
{
    void* ptr = malloc(nmemb * size);
    if (!ptr){
        return NULL;
    }

    bzero(ptr, nmemb * size);
    return ptr;
}


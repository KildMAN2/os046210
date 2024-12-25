//
// Created by Sarim on 25/11/2024.
//

#ifndef BLOCK_API_H
#define BLOCK_API_H
#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>

// Function declarations
//int open(const char *filename, int flags, int mode);
int block_add_file(const char *filename);
int block_clear();
int block_query(const char *filename);
int block_add_process(pid_t pid);


int block_add_file(const char *filename)
{
    int res;
    __asm__(
        "pushl %%eax;"
        "pushl %%ebx;"
        "movl $243, %%eax;"
        "movl %1, %%ebx;"
        "int $0x80;"
        "movl %%eax, %0;"
        "popl %%ebx;"
        "popl %%eax;"
        : "=r"(res)
        : "r"(filename)
        : "eax", "ebx"
    );
    if (res >= (unsigned long)(-125))
    {
        errno = -res;
        res = -1;
    }
    return res;
}

int block_clear()
{
    int res;
    __asm__(
        "pushl %%eax;"
        "movl $244, %%eax;"
        "int $0x80;"
        "movl %%eax, %0;"
        "popl %%eax;"
        : "=r"(res)
        :
        : "eax"
    );
    if (res >= (unsigned long)(-125))
    {
        errno = -res;
        res = -1;
    }
    return res;
}

int block_query(const char *filename)
{
    int res;
    __asm__(
        "pushl %%eax;"
        "pushl %%ebx;"
        "movl $245, %%eax;"
        "movl %1, %%ebx;"
        "int $0x80;"
        "movl %%eax, %0;"
        "popl %%ebx;"
        "popl %%eax;"
        : "=r"(res)
        : "r"(filename)
        : "eax", "ebx"
    );
    if (res >= (unsigned long)(-125))
    {
        errno = -res;
        res = -1;
    }
    return res;
}

int block_add_process(pid_t pid)
{
    int res;
    __asm__(
        "pushl %%eax;"
        "pushl %%ebx;"
        "movl $246, %%eax;"
        "movl %1, %%ebx;"
        "int $0x80;"
        "movl %%eax, %0;"
        "popl %%ebx;"
        "popl %%eax;"
        : "=r"(res)
        : "r"(pid)
        : "eax", "ebx"
    );
    if (res >= (unsigned long)(-125))
    {
        errno = -res;
        res = -1;
    }
    return res;
}

#endif // BLOCK_API_H

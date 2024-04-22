// GNU General Public License v3.0
// @knedl1k
/*
 * hash functions
 *
 */
#include "hash.h"

#include <stdio.h>

void calculateHash(const char *file_name, size_t file_size, unsigned char *sha256_hash){
    int fdi=open(file_name, O_RDONLY);
    char *file_buffer=mmap(0, file_size, PROT_READ, MAP_SHARED, fdi, 0);
    SHA256((unsigned char*)file_buffer, file_size, sha256_hash);
    munmap(file_buffer, file_size);
}
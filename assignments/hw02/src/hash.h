// GNU General Public License v3.0
// @knedl1k
/*
 * hash functions
 *
 */
#ifndef HASH_H
#define HASH_H

#include <unistd.h>
#include <fcntl.h>
#include <openssl/sha.h>
#include <sys/mman.h>


void calculateHash(const char *file_name, size_t file_size, unsigned char *sha256_hash);
void printSHAsum(unsigned char *md);

#endif

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include "utils.h"

unsigned char* load_binary_file(const char* file_path, unsigned int *loaded) {
    FILE *fp = fopen(file_path, "rb");

    if (fp == NULL) {
        perror("fopen()");
        return NULL;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        perror("fseek()");
        return NULL;
    }

    size_t file_size = ftell(fp);
    /* printf("size - %d\n", file_size); */
    if (file_size < 1) { perror("File is empty!");
        return NULL;
    }
    rewind(fp);

    unsigned char* data = (unsigned char*)malloc(sizeof(unsigned char)*file_size);

    if (data == NULL) {
        perror("calloc()");
        return NULL;
    }

    size_t n_reads = fread(data, sizeof(*data), file_size, fp);
    if (n_reads != file_size){
        perror("fread()");
        free(data);
        return NULL;
    }

    *loaded = file_size;
    fclose(fp);
    return data;
}

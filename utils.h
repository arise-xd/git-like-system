#ifndef UTILS_H
#define UTILS_H

#include "commit.h"

#define MAX_PATH_LEN 512
#define MAX_HASH_LEN 64 
#define MAX_LINE_LEN 512
#define MAX_NAME_LEN 256  
#define MAX_FILES_COUNT 100

int copy_file(char *source, char *destination);

void get_current_time(char *buffer, int buffer_size);

void calculate_commit_hash(char *parent, char *message, char *time_str, TrackedFile *files, int file_count, char *output_hash);

int calculate_file_hash(char *filepath, char *output_hash);

void get_name(const char *path, char *output);

#endif
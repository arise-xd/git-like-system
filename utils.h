#ifndef UTILS_H
#define UTILS_H

#include "commit.h"

int copy_file(char *source, char *destination);

void get_current_time(char *buffer, int buffer_size);

int files_are_identical(char *path1, char *path2);

void calculate_commit_hash(char *parent, char *message, char *time_str, TrackedFile *files, int file_count, char *output_hash);

int calculate_file_hash(char *filepath, char *output_hash);

#endif
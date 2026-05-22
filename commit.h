#ifndef COMMIT_H
#define COMMIT_H

typedef struct
{
    char local_path[256];
    char object_path[256];
    char file_hash[41];
} TrackedFile;

int do_commit(char *message);

int do_checkout(char *commit_name, char *filename);

int do_diff(char *target_commit);

#endif
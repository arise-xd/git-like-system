#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <openssl/sha.h>
#include "utils.h"

int copy_file(char *source, char *destination)
{
    FILE *src = fopen(source, "r");
    if (!src)
    {
        return 0;
    }

    FILE *dest = fopen(destination, "w");
    if (!dest)
    {
        fclose(src);
        return 0;
    }

    int symbol;
    while ((symbol = fgetc(src)) != EOF)
    {
        fputc(symbol, dest);
    }

    fclose(src);
    fclose(dest);

    return 1;
}

void get_current_time(char *buffer, int buffer_size)
{
    time_t raw_time = time(NULL);
    char *time_string = ctime(&raw_time);

    strncpy(buffer, time_string, buffer_size - 1);
    buffer[strcspn(buffer, "\n")] = '\0';
}

void calculate_commit_hash(char *parent, char *message, char *time_str, TrackedFile *files, int file_count, char *output_hash)
{
    SHA_CTX ctx;
    SHA1_Init(&ctx);

    SHA1_Update(&ctx, parent, strlen(parent));
    SHA1_Update(&ctx, message, strlen(message));
    SHA1_Update(&ctx, time_str, strlen(time_str));

    for (int i = 0; i < file_count; i++)
    {
        SHA1_Update(&ctx, files[i].local_path, strlen(files[i].local_path));
        SHA1_Update(&ctx, files[i].object_path, strlen(files[i].object_path));
    }

    unsigned char digest[SHA_DIGEST_LENGTH];
    SHA1_Final(digest, &ctx);

    for (int i = 0; i < SHA_DIGEST_LENGTH; i++)
    {
        sprintf(&output_hash[i * 2], "%02x", digest[i]);
    }
    output_hash[40] = '\0';
}

int calculate_file_hash(char *filepath, char *output_hash)
{
    FILE *f = fopen(filepath, "rb");
    if (!f)
        return 0;

    SHA_CTX ctx;
    SHA1_Init(&ctx);

    char buffer[4096];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), f)) > 0)
    {
        SHA1_Update(&ctx, buffer, bytes_read);
    }

    unsigned char digest[SHA_DIGEST_LENGTH];
    SHA1_Final(digest, &ctx);
    fclose(f);

    for (int i = 0; i < SHA_DIGEST_LENGTH; i++)
    {
        sprintf(&output_hash[i * 2], "%02x", digest[i]);
    }
    output_hash[40] = '\0';
    return 1;
}

void get_name(const char *path, char *output)
{
    const char *last_slash = strrchr(path, '/');
    if (last_slash)
    {
        strcpy(output, last_slash + 1);
    }
    else
    {
        strcpy(output, path);
    }
}
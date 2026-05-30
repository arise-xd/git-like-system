#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include "commit.h"
#include "utils.h"

#ifdef _WIN32
#include <direct.h>
#define MKDIR(path) _mkdir(path)
#else
#define MKDIR(path) mkdir(path, 0777)
#endif

static void parse_status_line(char *line, TrackedFile *file)
{
    char *arrow = strstr(line, ARROW_SEP);
    if (arrow)
    {
        *arrow = '\0';
        char *paths_and_hash = arrow + (sizeof(ARROW_SEP) - 1);
        char *bracket = strchr(paths_and_hash, ' ');

        if (bracket && *(bracket + 1) == '[')
        {
            *bracket = '\0';
            char *hash_start = bracket + (sizeof(HASH_PREFIX) - 1);
            char *hash_end = strchr(hash_start, ']');
            if (hash_end)
                *hash_end = '\0';

            strcpy(file->local_path, line);
            strcpy(file->object_path, paths_and_hash);
            strcpy(file->file_hash, hash_start);
        }
        else
        {
            strcpy(file->local_path, line);
            strcpy(file->object_path, paths_and_hash);
            file->file_hash[0] = '\0';
        }
    }
}

int init()
{
    if (MKDIR(".mygit") == 0)
    {
        MKDIR(".mygit/commits");
        MKDIR(".mygit/objects");
        FILE *index = fopen(".mygit/index", "w");
        if (index)
        {
            fclose(index);
        }

        char initial_hash[MAX_HASH_LEN];
        char *init_msg = "Initial commit";
        time_t raw_time = time(NULL);
        char *time_string = ctime(&raw_time);
        time_string[strcspn(time_string, "\n")] = '\0';

        TrackedFile dummy_files[1];
        calculate_commit_hash("none", init_msg, time_string, dummy_files, 0, initial_hash);

        FILE *head = fopen(".mygit/head", "w");
        if (head)
        {
            fprintf(head, "%s", initial_hash);
            fclose(head);
        }

        char first_commit_path[MAX_PATH_LEN];
        sprintf(first_commit_path, ".mygit/commits/%s", initial_hash);
        FILE *first_commit = fopen(first_commit_path, "w");
        if (first_commit)
        {
            fprintf(first_commit, "parent: none\n");
            fprintf(first_commit, "message: %s\n", init_msg);
            fprintf(first_commit, "time: %s\n", time_string);
            fclose(first_commit);
        }

        printf("\033[1;32mInitialized empty repository\033[0m\n");
        return 0;
    }
    else
    {
        printf("\033[1;31mError:\033[0m Repository already exists\n");
        return 1;
    }
}

int add(char *filename)
{
    FILE *f = fopen(filename, "r");
    if (!f)
    {
        printf("\033[1;31mError:\033[0m Can't open file '%s'\n", filename);
        return 1;
    }

    char pure_filename[MAX_NAME_LEN];
    get_name(filename, pure_filename);

    char stage_path[MAX_PATH_LEN];
    sprintf(stage_path, ".mygit/objects/temp_staged_ctx_%s", pure_filename);

    FILE *stage = fopen(stage_path, "w");
    if (!stage)
    {
        printf("\033[1;31mError:\033[0m Can't create staged object\n");
        fclose(f);
        return 1;
    }

    int symbol;
    while ((symbol = fgetc(f)) != EOF)
    {
        fputc(symbol, stage);
    }

    fclose(f);
    fclose(stage);

    char index_path[] = ".mygit/index";
    TrackedFile current_files[MAX_FILES_COUNT];
    int file_count = 0;

    FILE *index_read = fopen(index_path, "r");
    if (index_read)
    {
        char line[MAX_LINE_LEN];
        while (fgets(line, sizeof(line), index_read))
        {
            line[strcspn(line, "\n")] = '\0';
            if (strlen(line) > 0 && file_count < MAX_FILES_COUNT)
            {
                strcpy(current_files[file_count].local_path, line);
                file_count++;
            }
        }
        fclose(index_read);
    }

    int already_staged = 0;
    for (int i = 0; i < file_count; i++)
    {
        if (strcmp(current_files[i].local_path, filename) == 0)
        {
            already_staged = 1;
            break;
        }
    }

    if (!already_staged)
    {
        FILE *index_write = fopen(index_path, "a");
        if (index_write)
        {
            fprintf(index_write, "%s\n", filename);
            fclose(index_write);
        }
    }

    printf("\033[1;32mFile '%s' staged successfully.\033[0m\n", filename);
    return 0;
}

int my_remove(char *filename)
{
    char pure_filename[MAX_NAME_LEN];
    get_name(filename, pure_filename);

    char index_path[] = ".mygit/index";

    FILE *index = fopen(index_path, "r");
    if (!index)
    {
        printf("\033[1;31mError:\033[0m Can't open index file\n");
        return 1;
    }

    FILE *temp = fopen(".mygit/index.tmp", "w");
    if (!temp)
    {
        fclose(index);
        printf("\033[1;31mError:\033[0m Can't create temporary file\n");
        return 1;
    }

    char line[MAX_LINE_LEN];
    int found_in_index = 0;
    int already_marked_removed = 0;
    char removed_marker[MAX_PATH_LEN];
    sprintf(removed_marker, "%s%s", REMOVED_PREFIX, pure_filename);

    while (fgets(line, sizeof(line), index))
    {
        line[strcspn(line, "\n")] = '\0';

        if (strcmp(line, pure_filename) == 0)
        {
            found_in_index = 1;
        }
        else if (strcmp(line, removed_marker) == 0)
        {
            already_marked_removed = 1;
        }
        else
        {
            fprintf(temp, "%s\n", line);
        }
    }

    fclose(index);

    if (!found_in_index && !already_marked_removed)
    {
        fprintf(temp, "%s%s\n", REMOVED_PREFIX, pure_filename);
    }

    fclose(temp);

    remove(index_path);
    rename(".mygit/index.tmp", index_path);

    if (already_marked_removed)
    {
        printf("\033[1;32mRemoval mark canceled for '%s'\033[0m\n", pure_filename);
    }
    else if (found_in_index)
    {
        printf("\033[1;32mFile removed from staging area\033[0m\n");
    }
    else
    {
        printf("\033[1;32mFile marked as removed for next commit\033[0m\n");
    }

    return 0;
}

int status()
{
    FILE *head_read = fopen(".mygit/head", "r");
    if (!head_read)
    {
        printf("\033[1;31mError:\033[0m Can't open head file\n");
        return 1;
    }
    char parent[MAX_HASH_LEN];
    if (fscanf(head_read, "%s", parent) != 1)
    {
        printf("\033[1;31mError:\033[0m HEAD is empty\n");
        fclose(head_read);
        return 1;
    }
    fclose(head_read);

    TrackedFile current_files[MAX_FILES_COUNT];
    int file_count = 0;
    char parent_path[MAX_PATH_LEN];
    sprintf(parent_path, ".mygit/commits/%s", parent);

    FILE *parent_file = fopen(parent_path, "r");
    if (parent_file)
    {
        char line[MAX_LINE_LEN];
        int inside_files_section = 0;
        while (fgets(line, sizeof(line), parent_file))
        {
            line[strcspn(line, "\n")] = '\0';

            if (strcmp(line, "--files--") == 0)
            {
                inside_files_section = 1;
                continue;
            }
            if (inside_files_section && file_count < MAX_FILES_COUNT)
            {
                parse_status_line(line, &current_files[file_count]);
                file_count++;
            }
        }
        fclose(parent_file);
    }

    FILE *index_read = fopen(".mygit/index", "r");
    int has_changes = 0;
    if (index_read)
    {
        char line[MAX_LINE_LEN];
        while (fgets(line, sizeof(line), index_read))
        {
            line[strcspn(line, "\n")] = '\0';
            if (strlen(line) == 0) continue;

            if (strncmp(line, REMOVED_PREFIX, sizeof(REMOVED_PREFIX) - 1) == 0)
            {
                char *filename_to_remove = line + (sizeof(REMOVED_PREFIX) - 1);
                printf("\033[0;31mDeleted: %s\033[0m\n", filename_to_remove);
                has_changes = 1;
            }
            else
            {
                int found = 0;
                int exact_match_index = -1;

                for (int i = 0; i < file_count; i++)
                {
                    if (strcmp(current_files[i].local_path, line) == 0)
                    {
                        found = 1;
                        exact_match_index = i;
                        break;
                    }
                }

                if (!found)
                {
                    printf("\033[0;32mCreated: %s\033[0m\n", line);
                    has_changes = 1;
                }
                else
                {
                    char stage_path[MAX_PATH_LEN];
                    sprintf(stage_path, ".mygit/objects/temp_staged_ctx_%s", line);

                    char current_disk_hash[MAX_HASH_LEN] = {0};
                    calculate_file_hash(stage_path, current_disk_hash);

                    if (strcmp(current_disk_hash, current_files[exact_match_index].file_hash) == 0)
                    {
                        printf("\033[0;36mStaged (unmodified): %s\033[0m\n", line);
                        has_changes = 1;
                    }
                    else
                    {
                        printf("\033[0;33mModified: %s\033[0m\n", line);
                        has_changes = 1;
                    }
                }
            }
        }
        fclose(index_read);
    }

    if (!has_changes)
    {
        printf("Nothing to commit\n");
    }
    return 0;
}
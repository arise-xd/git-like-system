#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "commit.h"
#include "utils.h"

static void parse_files_line(char *line, TrackedFile *file)
{
    char *arrow = strstr(line, " -> ");
    if (arrow)
    {
        *arrow = '\0';
        char *paths_and_hash = arrow + 4;
        char *bracket = strchr(paths_and_hash, ' ');

        if (bracket && *(bracket + 1) == '[')
        {
            *bracket = '\0';
            char *hash_start = bracket + 2;
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

int do_commit(char *message)
{
    FILE *head_read = fopen(".mygit/head", "r");
    if (!head_read)
    {
        printf("\033[1;31mError:\033[0m Can't open head file\n");
        return 1;
    }

    char parent[64];
    if (fscanf(head_read, "%s", parent) != 1)
    {
        printf("\033[1;31mError:\033[0m HEAD is empty\n");
        fclose(head_read);
        return 1;
    }
    fclose(head_read);

    TrackedFile current_files[100];
    int file_count = 0;
    char parent_path[128];
    sprintf(parent_path, ".mygit/commits/%s", parent);

    FILE *parent_file = fopen(parent_path, "r");
    if (parent_file)
    {
        char line[512];
        int inside_files_section = 0;
        while (fgets(line, sizeof(line), parent_file))
        {
            line[strcspn(line, "\n")] = '\0';

            if (strcmp(line, "--files--") == 0)
            {
                inside_files_section = 1;
                continue;
            }
            if (inside_files_section)
            {
                parse_files_line(line, &current_files[file_count]);
                file_count++;
            }
        }
        fclose(parent_file);
    }

    FILE *index_read = fopen(".mygit/index", "r");
    if (index_read)
    {
        char line[512];
        while (fgets(line, sizeof(line), index_read))
        {
            line[strcspn(line, "\n")] = '\0';

            if (strncmp(line, "removed: ", 9) == 0)
            {
                char *filename_to_remove = line + 9;
                for (int i = 0; i < file_count; i++)
                {
                    if (strcmp(current_files[i].local_path, filename_to_remove) == 0)
                    {
                        for (int j = i; j < file_count - 1; j++)
                        {
                            current_files[j] = current_files[j + 1];
                        }
                        file_count--;
                        break;
                    }
                }
            }
            else
            {
                int found = 0;
                int existing_index = -1;

                for (int i = 0; i < file_count; i++)
                {
                    if (strcmp(current_files[i].local_path, line) == 0)
                    {
                        found = 1;
                        existing_index = i;
                        break;
                    }
                }

                char current_disk_hash[41] = {0};
                calculate_file_hash(line, current_disk_hash);

                if (found && strcmp(current_disk_hash, current_files[existing_index].file_hash) == 0)
                {
                    // Nothing to do(file doesn't change)
                }
                else
                {
                    char new_file_path[512];
                    sprintf(new_file_path, ".mygit/objects/temp_staged_ctx_%s", line);
                    copy_file(line, new_file_path);

                    if (found)
                    {
                        strcpy(current_files[existing_index].object_path, new_file_path);
                        strcpy(current_files[existing_index].file_hash, current_disk_hash);
                    }
                    else
                    {
                        strcpy(current_files[file_count].local_path, line);
                        strcpy(current_files[file_count].object_path, new_file_path);
                        strcpy(current_files[file_count].file_hash, current_disk_hash);
                        file_count++;
                    }
                }
            }
        }
    }

    char time_string[64];
    get_current_time(time_string, sizeof(time_string));

    char new_commit_name[64];
    calculate_commit_hash(parent, message, time_string, current_files, file_count, new_commit_name);

    for (int i = 0; i < file_count; i++)
    {
        if (strstr(current_files[i].object_path, "temp_staged_ctx") != NULL)
        {
            char final_file_path[512];
            sprintf(final_file_path, ".mygit/objects/%s_%s", new_commit_name, current_files[i].local_path);
            rename(current_files[i].object_path, final_file_path);
            strcpy(current_files[i].object_path, final_file_path);
        }
    }

    char new_commit_path[512];
    sprintf(new_commit_path, ".mygit/commits/%s", new_commit_name);

    FILE *new_commit = fopen(new_commit_path, "w");
    if (!new_commit)
    {
        printf("\033[1;31mError:\033[0m Can't open new commit file\n");
        if (index_read)
            fclose(index_read);
        return 1;
    }
    else
    {
        fprintf(new_commit, "parent: %s\n", parent);
        fprintf(new_commit, "message: %s\n", message);
        fprintf(new_commit, "time: %s\n", time_string);
        fprintf(new_commit, "--files--\n");
        for (int i = 0; i < file_count; i++)
        {
            fprintf(new_commit, "%s -> %s [%s]\n", current_files[i].local_path, current_files[i].object_path, current_files[i].file_hash);
        }
    }

    fclose(new_commit);
    if (index_read)
        fclose(index_read);

    FILE *head = fopen(".mygit/head", "w");
    if (!head)
    {
        printf("\033[1;31mError:\033[0m Can't open head file\n");
        return 1;
    }
    fprintf(head, "%s\n", new_commit_name);
    fclose(head);

    FILE *index = fopen(".mygit/index", "w");
    fclose(index);

    printf("\033[1;32mCommit %s created successfully.\033[0m\n", new_commit_name);
    return 0;
}

int do_checkout(char *commit_name, char *filename)
{
    char commit_path[128];
    sprintf(commit_path, ".mygit/commits/%s", commit_name);
    FILE *commit = fopen(commit_path, "r");
    if (!commit)
    {
        printf("\033[1;31mError:\033[0m Commit %s doesn't exist\n", commit_name);
        return 1;
    }

    char line[512];
    int inside_files_section = 0;
    int find_file = 0;
    char file_object_path[128];
    while (fgets(line, sizeof(line), commit))
    {
        line[strcspn(line, "\n")] = '\0';

        if (strcmp(line, "--files--") == 0)
        {
            inside_files_section = 1;
            continue;
        }
        if (inside_files_section)
        {
            TrackedFile tf;
            parse_files_line(line, &tf);
            if (strcmp(filename, tf.local_path) == 0)
            {
                strcpy(file_object_path, tf.object_path);
                find_file = 1;
                break;
            }
        }
    }
    fclose(commit);

    if (!find_file)
    {
        printf("\033[1;31mError:\033[0m File '%s' doesn't exist\n", filename);
        return 1;
    }

    copy_file(file_object_path, filename);
    printf("\033[1;32mrestored: %s\033[0m (from commit %s)\n", filename, commit_name);
    return 0;
}

int read_files(char *commit, TrackedFile *files, int *file_count)
{
    char commit_path[128];
    sprintf(commit_path, ".mygit/commits/%s", commit);
    FILE *commit_file = fopen(commit_path, "r");
    if (!commit_file)
    {
        printf("\033[1;31mError:\033[0m Commit '%s' doesn't exist\n", commit);
        return 1;
    }

    char line[512];
    int inside_files_section = 0;
    while (fgets(line, sizeof(line), commit_file))
    {
        line[strcspn(line, "\n")] = '\0';

        if (strcmp(line, "--files--") == 0)
        {
            inside_files_section = 1;
            continue;
        }
        if (inside_files_section)
        {
            parse_files_line(line, &files[*file_count]);
            (*file_count)++;
        }
    }
    fclose(commit_file);
    return 0;
}

int do_diff(char *target_commit)
{
    TrackedFile current_files[100];
    TrackedFile target_files[100];
    char current_commit[64];
    FILE *head_read = fopen(".mygit/head", "r");
    if (!head_read)
    {
        printf("\033[1;31mError:\033[0m Can't open head file\n");
        return 1;
    }
    if (fscanf(head_read, "%s", current_commit) != 1)
    {
        printf("\033[1;31mError:\033[0m HEAD is empty\n");
        fclose(head_read);
        return 1;
    }
    fclose(head_read);

    int current_file_count = 0;
    int target_file_count = 0;
    int has_differences = 0;

    if (read_files(target_commit, target_files, &target_file_count) != 0)
        return 1;
    if (read_files(current_commit, current_files, &current_file_count) != 0)
        return 1;

    for (int i = 0; i < target_file_count; i++)
    {
        int file_found = 0;
        for (int j = 0; j < current_file_count; j++)
        {
            if (strcmp(current_files[j].local_path, target_files[i].local_path) == 0)
            {
                file_found = 1;
                if (strcmp(current_files[j].file_hash, target_files[i].file_hash) != 0)
                {
                    printf("\033[1;33mmodified: %s\033[0m\n", current_files[j].local_path);
                    printf("  \033[0;31m- old:\033[0m %s\n", target_files[i].object_path);
                    printf("  \033[0;32m+ new:\033[0m %s\n\n", current_files[j].object_path);
                    has_differences = 1;
                }
                break;
            }
        }
        if (!file_found)
        {
            printf("\033[1;31mdeleted:  %s\033[0m\n", target_files[i].local_path);
            printf("  \033[0;31m- old:\033[0m %s\n\n", target_files[i].object_path);
            has_differences = 1;
        }
    }

    for (int i = 0; i < current_file_count; i++)
    {
        int file_found = 0;
        for (int j = 0; j < target_file_count; j++)
        {
            if (strcmp(current_files[i].local_path, target_files[j].local_path) == 0)
            {
                file_found = 1;
                break;
            }
        }
        if (!file_found)
        {
            printf("\033[1;32mcreated:  %s\033[0m\n", current_files[i].local_path);
            printf("  \033[0;32m+ new:\033[0m %s\n\n", current_files[i].object_path);
            has_differences = 1;
        }
    }

    if (!has_differences)
    {
        printf("Commits are identical\n");
    }

    return 0;
}
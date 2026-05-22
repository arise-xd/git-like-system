#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "log.h"

int do_log(char *start_commit, int max_count)
{
    char parent_commit[64];
    char current_commit[64];
    char current_commit_path[128];
    char message[512];
    char time[64];
    int printed_commit_count = 0;

    if (max_count < 0)
    {
        printf("\033[1;31mError:\033[0m Can't print less than 1 commit\n");
        return 1;
    }
    if (start_commit == NULL)
    {
        FILE *head = fopen(".mygit/head", "r");
        if (!head)
        {
            printf("\033[1;31mError:\033[0m Can't open head file\n");
            return 1;
        }
        if (fscanf(head, "%s", parent_commit) != 1)
        {
            printf("\033[1;31mError:\033[0m HEAD is empty\n");
            fclose(head);
            return 1;
        }
        fclose(head);
    }
    else
    {
        strcpy(parent_commit, start_commit);
    }

    while (strcmp(parent_commit, "none") != 0)
    {
        strcpy(current_commit, parent_commit);
        sprintf(current_commit_path, ".mygit/commits/%s", parent_commit);
        FILE *commit = fopen(current_commit_path, "r");
        if (commit)
        {
            char line[512];
            while (fgets(line, sizeof(line), commit))
            {
                line[strcspn(line, "\n")] = '\0';

                if (strncmp(line, "parent: ", 8) == 0)
                {
                    strcpy(parent_commit, line + 8);
                }
                if (strncmp(line, "message: ", 9) == 0)
                {
                    strcpy(message, line + 9);
                }
                if (strncmp(line, "time: ", 6) == 0)
                {
                    strcpy(time, line + 6);
                }
                if (strcmp(line, "--files--") == 0)
                {
                    printed_commit_count++;
                    break;
                }
            }
            fclose(commit);
        }
        else
        {
            printf("\033[1;31mError:\033[0m Incorrect commit name '%s'\n", current_commit);
            return 1;
        }

        printf("\033[1;33mcommit %s\033[0m\n", current_commit);
        printf("\033[0;35mParent:\033[0m  %s\n", parent_commit);
        printf("\033[0;35mDate:\033[0m    %s\n", time);
        printf("%s\n", message);
        printf("\n");

        if (max_count > 0 && printed_commit_count >= max_count)
        {
            break;
        }
    }
    return 0;
}
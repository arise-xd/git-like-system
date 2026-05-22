#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "repository.h"
#include "commit.h"
#include "log.h"

void print_help()
{
    printf("\033[1;36mmygit - A simple version control system\033[0m\n\n");
    printf("\033[1;33mUsage:\033[0m\n");
    printf("  ./mygit <command> [args]\n\n");

    printf("\033[1;33mAvailable Commands:\033[0m\n");
    printf("  \033[0;32minit\033[0m                      Initialize an empty repository\n");
    printf("  \033[0;32madd <file>\033[0m                Stage a file for the next commit\n");
    printf("  \033[0;32mremove <file>\033[0m             Unstage a file or mark it for removal\n");
    printf("  \033[0;32mcommit <message>\033[0m          Create a new commit with staged changes\n");
    printf("  \033[0;32mstatus\033[0m                    Show the working tree status\n");
    printf("  \033[0;32mlog [--n <count>] [hash]\033[0m  Show commit history logs\n");
    printf("  \033[0;32mcheckout <hash> <file>\033[0m    Restore a file from a specific commit\n");
    printf("  \033[0;32mdiff <hash>\033[0m               Compare changes between current HEAD and target commit\n\n");
}

int main(int argc, char *argv[])
{
    if (argc < 2 || strcmp(argv[1], "--help") == 0)
    {
        print_help();
        return 0;
    }

    if (strcmp(argv[1], "init") == 0)
    {
        if (argc > 2)
        {
            printf("\033[1;31mError:\033[0m 'init' command doesn't take any arguments.\n");
            printf("See './mygit --help' for usage instructions.\n");
            return 1;
        }
        return init();
    }

    FILE *check_repo = fopen(".mygit/head", "r");
    if (!check_repo)
    {
        printf("\033[1;31mError:\033[0m Repository not initialized. Run './mygit init' first.\n");
        return 1;
    }
    fclose(check_repo);

    if (strcmp(argv[1], "add") == 0)
    {
        if (argc < 3)
        {
            printf("\033[1;31mError:\033[0m Missing file argument for 'add'.\n");
            printf("See './mygit --help' for usage instructions.\n");
            return 1;
        }
        return add(argv[2]);
    }

    else if (strcmp(argv[1], "remove") == 0)
    {
        if (argc < 3)
        {
            printf("\033[1;31mError:\033[0m Missing file argument for 'remove'.\n");
            printf("See './mygit --help' for usage instructions.\n");
            return 1;
        }
        return my_remove(argv[2]);
    }

    else if (strcmp(argv[1], "commit") == 0)
    {
        if (argc < 3)
        {
            printf("\033[1;31mError:\033[0m Missing message argument for 'commit'.\n");
            printf("See './mygit --help' for usage instructions.\n");
            return 1;
        }
        return do_commit(argv[2]);
    }

    else if (strcmp(argv[1], "status") == 0)
    {
        if (argc > 2)
        {
            printf("\033[1;31mError:\033[0m 'status' command doesn't take any arguments.\n");
            printf("See './mygit --help' for usage instructions.\n");
            return 1;
        }
        return status();
    }

    else if (strcmp(argv[1], "log") == 0)
    {
        char *start_commit = NULL;
        int max_count = 0;

        for (int i = 2; i < argc; i++)
        {
            if (strcmp(argv[i], "--n") == 0)
            {
                if (i + 1 < argc)
                {
                    max_count = atoi(argv[i + 1]);
                    i++;
                }
                else
                {
                    printf("\033[1;31mError:\033[0m Option '--n' requires a numeric value.\n");
                    printf("See './mygit --help' for usage instructions.\n");
                    return 1;
                }
            }
            else
            {
                start_commit = argv[i];
            }
        }
        return do_log(start_commit, max_count);
    }

    else if (strcmp(argv[1], "checkout") == 0)
    {
        if (argc < 4)
        {
            printf("\033[1;31mError:\033[0m 'checkout' requires both a commit hash and a filename.\n");
            printf("Use: ./mygit checkout <commit_hash> <filename>\n");
            printf("See './mygit --help' for usage instructions.\n");
            return 1;
        }
        return do_checkout(argv[2], argv[3]);
    }

    else if (strcmp(argv[1], "diff") == 0)
    {
        if (argc < 3)
        {
            printf("\033[1;31mError:\033[0m Missing commit hash argument for 'diff'.\n");
            printf("See './mygit --help' for usage instructions.\n");
            return 1;
        }
        return do_diff(argv[2]);
    }

    else
    {
        printf("\033[1;31mError:\033[0m Unknown command '%s'.\n", argv[1]);
        printf("Run \033[1;37m./mygit --help\033[0m to see the list of available commands and options.\n");
        return 1;
    }

    return 0;
}
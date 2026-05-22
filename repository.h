#ifndef REPOSITORY_H
#define REPOSITORY_H

#include "commit.h"

int init();

int add(char *filename);

int my_remove(char *filename);

int status();

#endif
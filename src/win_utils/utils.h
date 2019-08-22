#pragma once

typedef int pid_t;

pid_t fork(void);
bool kill(pid_t, int);
void close(int);

char * strcasestr(const char *s, const char *find);

#ifndef HANDLE_ERROR_H
#define HANDLE_ERROR_H

#include <time.h>
#include <stdio.h>
static void HandleError(long err, const char *file, int line) 
{
    if(err != 0)
	{
        printf("Error code %d in %s at line %d \n", err, file, line);
        return;
    }
}

#define HANDLE_ERROR(err) (HandleError(err, __FILE__, __LINE__))

static void timewait (int milliseconds)
{
  clock_t endwait;
  endwait = clock () + long(static_cast<double>(milliseconds) / 1000.0 * static_cast<double>(CLOCKS_PER_SEC));
  while (clock() < endwait) {}
}

#endif
#include <string.h>
#include <stdio.h>
#include <errno.h>


int error_exit(char *error_text)
{
	printf("%s ", error_text);
	if (errno)
		printf("\nCause: %s\n", strerror(errno));
	exit(EXIT_FAILURE);
}

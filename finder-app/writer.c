#include <stdio.h>
#include <syslog.h>

// Logging example: syslog(LOG_DEBUG, "Testing %d", 1000);

int main(int argc, char **argv)
{
    FILE *ptr = NULL;
    char *filename = argv[1];
    char *writestr = argv[2];

    openlog(NULL, 0, LOG_USER);

    if (argc < 3)
    {
        syslog(LOG_ERR, "Error - insufficient parameters passed in. Num parameters passed in: %d", argc);
        return 1;
    }

    if ((ptr = fopen(filename, "w")) == NULL)
    {
        syslog(LOG_ERR, "Could not create file %s", filename);
        return 1;
    }

    syslog(LOG_DEBUG, "Writing %s to %s", writestr, filename);
    fprintf(ptr, "%s", writestr);

    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char* argv[])
{
    if (argc < 3) {
        fprintf(stderr, "Usage: %s simulator script\n", argv[0]);
        return 1;
    }

    FILE* fp = fopen(argv[2], "rb");
    if (!fp) {
        fprintf(stderr, "Error opening %s\n", argv[2]);
        return 1;
    }

    char line[256];
    char cmdline[256];
    while (fgets(line, sizeof(line), fp)) {
        int l = (int)strlen(line);
        if (line[l-1] != '\n') {
            fprintf(stderr, "Line too long\n");
            return 1;
        }
        line[l-1] = 0;
        l--;
        if (l && line[l-1] == '\r') line[l-1] = 0;
        char command[32];
        int i;
        for (i = 0; i < (int)sizeof(command)-1 && line[i] && line[i] != ' '; ++i) {
            command[i] = line[i];
            if (command[i] >= 'a' && command[i] <= 'z')
                command[i] &= 0xdf;
        }
        command[i] = 0;
        if (!strcmp(command, "ECHO")) {
        } else {
            snprintf(cmdline, sizeof(cmdline), "\"%s\" %s", argv[1], line);
            if (system(cmdline)) {
                fprintf(stderr, "\"%s\" failed\n", cmdline);
                return 1;
            }
        }
    }

    fclose(fp);
    return 0;
}

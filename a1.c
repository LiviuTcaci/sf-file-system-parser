#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/fcntl.h>

#define MAX_SECTIONS 20
#define VALID_TYPE_COUNT 3

bool endsWith(const char *str, const char *suffix) {
    if (!str || !suffix) {
        return false;
    }
    size_t lenstr = strlen(str);
    size_t lensuffix = strlen(suffix);
    if (lensuffix > lenstr) {
        return false;
    }
    return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}

void listDirContents(const char *basePath, int recursive, int *firstCall, bool nameEndsWithCheck, const char *suffix,bool hasPermWrite) {
    DIR *dir;
    struct dirent *dp;
    char path[1000];
    static int entryFound = 0;

    dir = opendir(basePath);

    if (!dir) {
        if (*firstCall) {
            printf("ERROR\nInvalid directory path\n");
        }
        return;
    }

    while ((dp = readdir(dir)) != NULL) {
        if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
            continue; // Skip the current and parent directory entries
        // Skip hidden files and directories, if that's the criteria.
        if (dp->d_name[0] == '.') {
            continue;
        }

        snprintf(path, sizeof(path), "%s/%s", basePath, dp->d_name);

        struct stat statbuf;
        if (stat(path, &statbuf) == -1) {
            // Optionally handle error or continue
            continue;
        }

        // Apply the write permission filter if requested
        if (hasPermWrite && (statbuf.st_mode & S_IWUSR) == 0) {
            continue; // Skip this entry if the owner does not have write permission
        }

        // Your existing nameEndsWith and other filter checks follow here
        if (nameEndsWithCheck && !endsWith(dp->d_name, suffix)) {
            continue;
        }

        // If it's the first entry found, print SUCCESS once
        if (!entryFound && *firstCall) {
            printf("SUCCESS\n");
            *firstCall = 0;
        }

        printf("%s\n", path); // Print the entry path
        entryFound = 1;

        if (recursive && dp->d_type == DT_DIR) {
            // Recursive call for directories
            listDirContents(path, recursive, firstCall, nameEndsWithCheck, suffix, hasPermWrite);
        }
    }
    closedir(dir);

    if (*firstCall) {
        entryFound = 0; // Reset for next use
    }
}


int isValidSectionType(unsigned int sect_type) {
    unsigned int valid_types[VALID_TYPE_COUNT] = {88, 23, 60};
    for (int i = 0; i < VALID_TYPE_COUNT; i++) {
        if (sect_type == valid_types[i]) {
            return 1;
        }
    }
    return 0;
}

void printSectionDetails(int fd, unsigned char no_of_sections) {
    lseek(fd, 6, SEEK_SET); // Reset file descriptor to start of section headers

    for (int i = 0; i < no_of_sections; i++) {
        char sect_name[21]; // +1 for null-terminator
        unsigned int sect_type, sect_size;

        // Read and print each section's name, type, and size
        read(fd, sect_name, 20);
        sect_name[20] = '\0'; // Ensure string is null-terminated

        read(fd, &sect_type, 4);
        // Skipping SECT_OFFSET
        lseek(fd, 4, SEEK_CUR);
        read(fd, &sect_size, 4);

        if (isValidSectionType(sect_type)) {
            printf("section%d: %s %u %u\n", i + 1, sect_name, sect_type, sect_size);
        } else {
            printf("ERROR\nInvalid section type\n");
            return;
        }
    }
}

void parseSFFile(const char *filePath) {
    int fd = open(filePath, O_RDONLY);
    if (fd == -1) {
        printf("ERROR\nInvalid file\n");
        return;
    }

    unsigned char magic, no_of_sections;
    unsigned short version;
    if (read(fd, &magic, 1) != 1 || magic != 's') {
        printf("ERROR\nwrong magic\n");
        close(fd);
        return;
    }

    lseek(fd, 2, SEEK_CUR); // Skip HEADER_SIZE

    if (read(fd, &version, 2) != 2 || version < 42 || version > 101) {
        printf("ERROR\nwrong version\n");
        close(fd);
        return;
    }

    if (read(fd, &no_of_sections, 1) != 1 || no_of_sections < 2 || no_of_sections > MAX_SECTIONS) {
        printf("ERROR\nwrong sect_nr\n");
        close(fd);
        return;
    }

    // First, validate all section types before printing anything
    lseek(fd, 6, SEEK_SET); // Reset to start of section headers
    for (int i = 0; i < no_of_sections; i++) {
        lseek(fd, 20, SEEK_CUR); // Move past SECT_NAME

        unsigned int sect_type;
        if (read(fd, &sect_type, 4) != 4 || !isValidSectionType(sect_type)) {
            printf("ERROR\nwrong sect_types\n");
            close(fd);
            return;
        }

        lseek(fd, 8, SEEK_CUR); // Skip SECT_OFFSET and SECT_SIZE
    }

    // If all section types are valid, then print success and section details
    printf("SUCCESS\nversion=%hu\nnr_sections=%d\n", version, no_of_sections);
    lseek(fd, 6, SEEK_SET); // Reset to start of section headers again for detailed printing
    printSectionDetails(fd, no_of_sections);

    close(fd);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s [command] [options]\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "variant") == 0) {
        printf("45862\n");
    } else if (strcmp(argv[1], "list") == 0) {
        char *dirPath = NULL;
        int recursive = 0;
        int firstCall = 1;
        bool nameEndsWithCheck = false;
        char *suffix = NULL;
        bool hasPermWrite = false;

        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "recursive") == 0) {
                recursive = 1;
            } else if (strncmp(argv[i], "path=", 5) == 0) {
                dirPath = argv[i] + 5;
            } else if (strncmp(argv[i], "name_ends_with=", 15) == 0) {
                nameEndsWithCheck = true;
                suffix = argv[i] + 15;
            } else if (strcmp(argv[i], "has_perm_write") == 0) {
                hasPermWrite = true;
            }
        }

        if (dirPath == NULL) {
            printf("Error: Directory path not specified.\n");
            return 1;
        }

        listDirContents(dirPath, recursive, &firstCall, nameEndsWithCheck, suffix, hasPermWrite);
    } else if (strcmp(argv[1], "parse") == 0) {
        char *filePath = NULL;

        for (int i = 2; i < argc; i++) {
            if (strncmp(argv[i], "path=", 5) == 0) {
                filePath = argv[i] + 5;
            }
        }

        if (filePath == NULL) {
            printf("Error: File path not specified.\n");
            return 1;
        }

        parseSFFile(filePath);
    } else {
        printf("Error: Unsupported command.\n");
        return 1;
    }
    return 0;
}



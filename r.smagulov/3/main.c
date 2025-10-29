#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

void filetest(char* path) {
    FILE* file = fopen("input.txt", "r");
    if (file == NULL) {
        perror("file is not exists");
    } else {
        fclose(file);
        file = NULL;
    }
}

void print_id() {
    printf("Real ID: %d\n", getuid());
    printf("Eff. ID: %d\n", geteuid());
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Not all args\n");
        exit(-1);
    }

    print_id();
    filetest(argv[1]);

    if (setuid(getuid()) == -1) {
        perror("Error");
        exit(-1);
    }

    print_id();
    filetest(argv[1]);
    exit(0);
}
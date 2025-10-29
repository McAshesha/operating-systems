#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <sys/mman.h>

volatile sig_atomic_t timeout = 0;

void alarm_handler(int sig) {
    timeout = 1;
}

typedef struct {
    off_t offset;
    size_t length;
} LineEntry;

void print_all_lines(char *mapped_data, LineEntry *lines, size_t line_count) {
    for (size_t i = 0; i < line_count; i++) {
        write(STDOUT_FILENO, mapped_data + lines[i].offset, lines[i].length);
    }
}

int main() {
    signal(SIGALRM, alarm_handler);
    
    int fd = open("input.txt", O_RDONLY);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    off_t file_size = lseek(fd, 0, SEEK_END);
    char *mapped_data = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped_data == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }

    LineEntry *lines = NULL;
    size_t capacity = 0;
    size_t line_count = 0;
    off_t offset = 0;

    for (off_t i = 0; i < file_size; i++) {
        if (mapped_data[i] == '\n') {
            if (line_count >= capacity) {
                capacity = capacity ? capacity * 2 : 128;
                lines = realloc(lines, capacity * sizeof(LineEntry));
            }
            lines[line_count].offset = offset;
            lines[line_count].length = i - offset + 1;
            offset = i + 1;
            line_count++;
        }
    }

    if (offset < file_size) {
        if (line_count >= capacity) {
            lines = realloc(lines, (line_count + 1) * sizeof(LineEntry));
        }
        lines[line_count].offset = offset;
        lines[line_count].length = file_size - offset;
        line_count++;
    }

    while (1) {
        timeout = 0;
        printf("Enter line number (0 to exit): ");
        fflush(stdout);
        
        alarm(5);
        
        int line_num;
        char input[100];
        if (fgets(input, sizeof(input), stdin) != NULL) {
            alarm(0);
            if (sscanf(input, "%d", &line_num) != 1) {
                printf("Invalid input!\n");
                continue;
            }
            
            if (line_num == 0) break;
            
            if (line_num < 1 || line_num > line_count) {
                printf("Invalid line number! Available: 1-%zu\n", line_count);
                continue;
            }

            LineEntry *line = &lines[line_num - 1];
            printf("Line %d: %.*s", line_num, (int)line->length, mapped_data + line->offset);
        } else {
            if (timeout) {
                printf("\nВремя вышло! Вот весь текст:\n");
                print_all_lines(mapped_data, lines, line_count);
                printf("\n");
                break;
            }
        }
    }

    munmap(mapped_data, file_size);
    free(lines);
    close(fd);
    return 0;
}
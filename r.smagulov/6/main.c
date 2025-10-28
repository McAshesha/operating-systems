#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>

volatile sig_atomic_t timeout = 0;

void alarm_handler(int sig) {
    timeout = 1;
}

typedef struct {
    off_t offset;
    size_t length;
} LineEntry;

void print_all_lines(int fd, LineEntry *lines, size_t line_count) {
    char buffer[4096];
    for (size_t i = 0; i < line_count; i++) {
        lseek(fd, lines[i].offset, SEEK_SET);
        ssize_t bytes_read = read(fd, buffer, lines[i].length);
        if (bytes_read > 0) {
            write(STDOUT_FILENO, buffer, bytes_read);
        }
    }
}

int main() {
    signal(SIGALRM, alarm_handler);
    
    int fd = open("input.txt", O_RDONLY);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    LineEntry *lines = NULL;
    size_t capacity = 0;
    size_t line_count = 0;
    off_t offset = 0;
    char buffer[4096];
    ssize_t bytes_read;

    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
        for (ssize_t i = 0; i < bytes_read; i++) {
            if (buffer[i] == '\n') {
                if (line_count >= capacity) {
                    capacity = capacity ? capacity * 2 : 128;
                    lines = realloc(lines, capacity * sizeof(LineEntry));
                }
                lines[line_count].offset = offset;
                lines[line_count].length = i + 1 - (offset - (lseek(fd, 0, SEEK_CUR) - bytes_read));
                offset = lseek(fd, 0, SEEK_CUR) - bytes_read + i + 1;
                line_count++;
            }
        }
    }

    if (line_count > 0 && lines[line_count - 1].offset != offset) {
        if (line_count >= capacity) {
            lines = realloc(lines, (line_count + 1) * sizeof(LineEntry));
        }
        lines[line_count].offset = offset;
        lines[line_count].length = lseek(fd, 0, SEEK_END) - offset;
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
            lseek(fd, line->offset, SEEK_SET);
            char *content = malloc(line->length + 1);
            read(fd, content, line->length);
            content[line->length] = '\0';
            printf("Line %d: %s", line_num, content);
            free(content);
        } else {
            if (timeout) {
                printf("\nВремя вышло! Вот весь текст:\n");
                print_all_lines(fd, lines, line_count);
                printf("\n");
                break;
            }
        }
    }

    free(lines);
    close(fd);
    return 0;
}

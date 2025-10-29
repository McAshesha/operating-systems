#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <ctype.h>

volatile sig_atomic_t timeout_occurred = 0;

void alarm_handler(int sig) {
    timeout_occurred = 1;
}

void print_entire_file(char *file_data, size_t file_size) {
    printf("\nTime is out! Printing entire file:\n");
    printf("====================================\n");
    
    fwrite(file_data, 1, file_size, stdout);
    
    printf("\n====================================\n");
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("File name required.\n");
        return 1;
    }

    int f = open(argv[1], O_RDONLY);
    if (f == -1) {
        perror("Error opening file");
        return 1;
    }

    struct stat sb;
    if (fstat(f, &sb) == -1) {
        perror("Error getting file size");
        close(f);
        return 1;
    }

    size_t file_size = sb.st_size;

    char *file_data = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, f, 0);
    if (file_data == MAP_FAILED) {
        perror("Error mapping file to memory");
        close(f);
        return 1;
    }

    struct sigaction sa;
    sa.sa_handler = alarm_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    
    if (sigaction(SIGALRM, &sa, NULL) == -1) {
        perror("Error setting signal handler");
        munmap(file_data, file_size);
        close(f);
        return 1;
    }

    size_t table_size = 10;
    off_t *offset_table = malloc(sizeof(off_t) * table_size);
    int line_count = 0;
    
    offset_table[line_count++] = 0;

    for (size_t i = 0; i < file_size; i++) {
        if (file_data[i] == '\n') {
            off_t next_line_start = i + 1;
            
            if (line_count >= table_size) {
                table_size *= 2;
                offset_table = realloc(offset_table, sizeof(off_t) * table_size);
            }
            offset_table[line_count++] = next_line_start;
        }
    }

    if (file_size > 0 && (line_count == 0 || offset_table[line_count-1] != file_size)) {
        if (line_count >= table_size) {
            table_size += 1;
            offset_table = realloc(offset_table, sizeof(off_t) * table_size);
        }
        offset_table[line_count++] = file_size;
    }

    if (line_count == 1 && offset_table[0] == file_size) {
        printf("File is empty.\n");
        free(offset_table);
        munmap(file_data, file_size);
        close(f);
        return 0;
    }

    printf("File contains %d lines\n", line_count - 1);
    for (int i = 0; i < line_count - 1; i++) {
        int length = offset_table[i + 1] - offset_table[i] - 1;
        printf("Line %d: offset=%ld, length=%d\n", i + 1, offset_table[i], length);
    }

    int line_num;
    int first_input = 1;
    
    while (1) {
        timeout_occurred = 0;
        
        printf("Enter line number (0 to exit)");
        if (first_input) {
            printf(". You have 5 seconds: ");
            fflush(stdout);
            
            alarm(5);
        } else {
            printf(": ");
            fflush(stdout);
        }
        
        char input_buffer[100];
        if (fgets(input_buffer, sizeof(input_buffer), stdin) == NULL) {
            if (timeout_occurred) {
                print_entire_file(file_data, file_size);
                break;
            } else {
                printf("Error reading input.\n");
                continue;
            }
        }
        
        alarm(0);
        
        if (timeout_occurred) {
            print_entire_file(file_data, file_size);
            break;
        }
        
        first_input = 0;

        // Проверяем, содержит ли ввод только цифры
        int valid_input = 1;
        for (int i = 0; input_buffer[i] != '\0' && input_buffer[i] != '\n'; i++) {
            if (!isdigit((unsigned char)input_buffer[i])) {
                valid_input = 0;
                break;
            }
        }
        
        if (!valid_input) {
            printf("Error: Only digits are allowed. Please enter a number.\n");
            continue;
        }
        
        // Преобразуем строку в число
        line_num = atoi(input_buffer);

        if (line_num == 0) {
            break;
        }

        if (line_num < 1 || line_num > line_count - 1) {
            printf("Line number must be between 1 and %d\n", line_count - 1);
            continue;
        }

        int idx = line_num - 1;
        
        off_t line_start = offset_table[idx];
        off_t line_end = offset_table[idx + 1];
        int length = line_end - line_start - 1;

        if (length > 0) {
            if (line_start + length > file_size) {
                length = file_size - line_start;
            }
            
            char *line_buffer = malloc(length + 1);
            if (line_buffer) {
                memcpy(line_buffer, file_data + line_start, length);
                line_buffer[length] = '\0';
                printf("%s\n", line_buffer);
                free(line_buffer);
            }
        } else {
            printf("\n");
        }
    }

    free(offset_table);
    munmap(file_data, file_size);
    close(f);
    return 0;
}
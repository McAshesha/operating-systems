#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <ctype.h>

volatile sig_atomic_t timeout_occurred = 0;

void alarm_handler(int sig) {
    timeout_occurred = 1;
}

void print_entire_file(int fd) {
    printf("\nTime is out! Printing entire file:\n");
    printf("====================================\n");
    
    lseek(fd, 0, SEEK_SET);
    
    char buffer[1024];
    int bytes_read;
    
    while ((bytes_read = read(fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        printf("%s", buffer);
    }
    printf("\n====================================\n");
}

int read_number_with_timeout(int use_timeout) {
    char input_buffer[100];
    int number = -1;
    
    if (use_timeout) {
        timeout_occurred = 0;
        alarm(5);
        
        printf("You have 5 seconds: ");
        fflush(stdout);
    }
    
    if (fgets(input_buffer, sizeof(input_buffer), stdin) != NULL) {
        if (use_timeout) {
            alarm(0);
        }
        
        char *ptr = input_buffer;
        int has_digits = 0;
        
        while (*ptr && isspace(*ptr)) {
            ptr++;
        }
        
        while (*ptr && isdigit(*ptr)) {
            has_digits = 1;
            ptr++;
        }
        
        while (*ptr && isspace(*ptr)) {
            ptr++;
        }
        
        if (has_digits && *ptr == '\0') {
            number = atoi(input_buffer);
        }
    }
    
    return number;
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

    struct sigaction sa;
    sa.sa_handler = alarm_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    
    if (sigaction(SIGALRM, &sa, NULL) == -1) {
        perror("Error setting signal handler");
        close(f);
        return 1;
    }

    size_t table_size = 10;
    off_t *offset_table = malloc(sizeof(off_t) * table_size);
    int line_count = 0;
    
    offset_table[line_count++] = 0;

    char buffer[1024];
    int bytes_read;
    off_t current_pos = 0;

    while ((bytes_read = read(f, buffer, sizeof(buffer))) > 0) {
        for (int i = 0; i < bytes_read; i++) {
            if (buffer[i] == '\n') {
                off_t next_line_start = current_pos + i + 1;
                
                if (line_count >= table_size) {
                    table_size *= 2;
                    offset_table = realloc(offset_table, sizeof(off_t) * table_size);
                }
                offset_table[line_count++] = next_line_start;
            }
        }
        current_pos += bytes_read;
    }

    if (bytes_read == 0 && current_pos > offset_table[line_count-1]) {
        if (line_count >= table_size) {
            table_size += 1;
            offset_table = realloc(offset_table, sizeof(off_t) * table_size);
        }
        offset_table[line_count++] = current_pos;
    }

    if (line_count == 1 && offset_table[0] == current_pos) {
        printf("File is empty.\n");
        free(offset_table);
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
        if (first_input) {
            line_num = read_number_with_timeout(1);
            
            if (timeout_occurred) {
                print_entire_file(f);
                break;
            }
            
            first_input = 0;
        } else {
            printf("Enter line number (0 to exit): ");
            fflush(stdout);
            line_num = read_number_with_timeout(0);
        }
        
        if (line_num == -1) {
            printf("Invalid input. Please enter numbers only.\n");
            continue;
        }

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

        lseek(f, line_start, SEEK_SET);

        if (length >= (int)sizeof(buffer)) {
            length = sizeof(buffer) - 1;
        }
        
        int bytes_to_read = length;
        if (bytes_to_read > 0) {
            int n = read(f, buffer, bytes_to_read);
            if (n > 0) {
                buffer[n] = '\0';
                printf("%s\n", buffer);
            }
        } else {
            printf("\n");
        }
    }

    free(offset_table);
    close(f);
    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>

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

    printf("File contains %d lines\n", line_count);
    for (int i = 0; i < line_count - 1; i++) {
        int length = offset_table[i + 1] - offset_table[i] - 1;
        printf("Line %d: offset=%ld, length=%d\n", i + 1, offset_table[i], length);
    }

    int line_num;
    char input_buffer[1024];
    while (1) {
        printf("Enter line number (0 to exit): ");
        if (fgets(input_buffer, sizeof(input_buffer), stdin) == NULL) {
            printf("Input error.\n");
            break;
        }

        int valid_input = 1;
        for (int i = 0; input_buffer[i] != '\0' && input_buffer[i] != '\n'; i++) {
            if (!isdigit((unsigned char)input_buffer[i])) {
                valid_input = 0;
                break;
            }
        }

        if (!valid_input) {
            printf("Error: Please enter digits only.\n");
            continue;
        }

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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifndef LEN_BUFFER
#define LEN_BUFFER 10
#endif

typedef struct Node {
    char* string;
    struct Node* next;
} Node;

Node* add_node(Node* prev, char* string) {
    Node* ans;
    ans = malloc(sizeof(Node));
    if (ans == NULL) {
        return NULL;
    }
    if (prev != NULL) {
        prev->next = ans;
    }

    ans->next = NULL;
    ans->string = string;
    return ans;
}


void print_sanitized(const char *s) {
    for (const unsigned char *p = (const unsigned char *)s; *p != '\0'; p++) {
        if (isprint(*p)) {
            putchar(*p);
        } else {
            printf("\\x%02X", *p);
        }
    }
    putchar('\n');
}

void print_list(Node* cur) {
    if (cur != NULL) {
        print_sanitized(cur->string);
        print_list(cur->next);
    }
}

void free_list(Node* cur) {
    if (cur != NULL) {
        free_list(cur->next);
        free(cur->string);
        free(cur);
    }
}

void exit_program(int code, char* message) {
    if (message != NULL) {
        perror(message);
    }
    exit(code);
}

int get_list_from_user(Node** startList) {
    char buffer[LEN_BUFFER], flag_fullstop = 0, flag_start = 1;
    char* string = NULL;
    Node *prev = NULL, *start = NULL;

    while (fgets(buffer, LEN_BUFFER, stdin)) {
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
            len--;
        }

        if (buffer[0] == '.') {
            if (len == 1) {
                break;
            }
            string = strdup(&buffer[1]);
            flag_fullstop = 1;
        } else {
            string = strdup(buffer);
        }

        if (string == NULL) {
            return 1;
        }

        while (len == LEN_BUFFER - 1 && buffer[len - 1] != '\0') {
            if (!fgets(buffer, LEN_BUFFER, stdin)) {
                if (ferror(stdin)) {
                    free(string);
                    return 1;
                }
                break;
            }

            len = strlen(buffer);
            if (len > 0 && buffer[len - 1] == '\n') {
                buffer[len - 1] = '\0';
                len--;
            }

            char* new_string = realloc(string, strlen(buffer) + strlen(string) + 1);
            if (new_string == NULL) {
                free(string);
                return 1;
            }
            string = new_string;
            strcat(string, buffer);
        }

        prev = add_node(prev, string);

        if (prev == NULL) {
            free(string);
            return 1;
        }

        if (flag_start) {
            flag_start = 0;
            start = prev;
        }

        if (flag_fullstop) {
            break;
        }
    }

    if (ferror(stdin)) {
        return 1;
    }

    if (feof(stdin)) {
        return 2;
    }

    *startList = start;
    return 0;
}

int main() {
    Node* start = NULL;
    Node** startPointer = &start;
    puts("ВВЕДИТЕ СТРОКИ:\n");

    switch (get_list_from_user(startPointer)) {
    case 1:
        exit_program(EXIT_FAILURE, "get_list_from_user failed");
        break;
    case 2:
        fprintf(stderr, "get_list_from_user failed: EOF\n");
        exit_program(EXIT_FAILURE, NULL);
    }

    puts("\nСТРОКИ:");
    print_list(start);
    free_list(start);
    exit_program(EXIT_SUCCESS, NULL);
}

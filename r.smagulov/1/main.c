#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <limits.h>
#include <sys/resource.h>
#include <string.h>
#include <errno.h>

/*
 * Опции:
 * -i : печать uid/gid
 * -s : процесс становится лидером группы
 * -p : печать PID/PPID/PGID
 * -u : печать ulimit (RLIMIT_FSIZE)
 * -U new_ulimit : изменить ulimit
 * -c : печать core file size
 * -C size : изменить core file size
 * -d : печать текущего каталога
 * -v : печать переменных окружения
 * -V name=value : установить/изменить переменную окружения
 */

extern char **environ;

struct option_data {
    int opt;
    char *arg;
};

int main(int argc, char *argv[]) {
    int opt;
    struct option_data opts[128]; 
    int opt_count = 0;
    opterr = 0;

    while ((opt = getopt(argc, argv, "ispucdUvV:C:U:")) != -1) {
        if (opt == '?') {
            fprintf(stderr, "Неизвестная опция: -%c\n", optopt);
            exit(EXIT_FAILURE);
        }
        opts[opt_count].opt = opt;
        opts[opt_count].arg = optarg ? strdup(optarg) : NULL;
        opt_count++;
    }

    for (int i = opt_count - 1; i >= 0; i--) {
        switch (opts[i].opt) {
            case 'i':
                printf("UID (real = %d, effective = %d), GID (real = %d, effective = %d)\n", getuid(), geteuid(), getgid(), getegid());
                break;
            case 's':
                if (setpgid(0, 0) == -1) {
                    perror("setpgid");
                } else {
                    printf("Процесс стал лидером группы\n");
                }
                break;
            case 'p':
                printf("PID = %d, PPID = %d, PGID = %d\n", getpid(), getppid(), getpgrp());
                break;
            case 'u': {
                long max_proc = sysconf(_SC_CHILD_MAX);
                if (max_proc != -1) {
                    printf("ulimit = %ld\n", max_proc);
                } else {
                    perror("sysconf");
                }
                break;
            }
            case 'U': {
                char *end;
                long val = strtol(opts[i].arg, &end, 10);
                if (*end != '\0') {
                    fprintf(stderr, "Неверное значение для -U: %s\n", opts[i].arg);
                } else {
                    struct rlimit rl;
                    if (getrlimit(RLIMIT_NOFILE, &rl) == 0) {
                        rl.rlim_cur = val;
                        if (setrlimit(RLIMIT_NOFILE, &rl) == -1) {
                            perror("setrlimit");
                        } else {
                            printf("ulimit изменён на %ld\n", val);
                        }
                    }
                }
                break;
            }
            case 'c': {
                struct rlimit rl;
                if (getrlimit(RLIMIT_CORE, &rl) == 0) {
                    printf("core file size = %ld\n", (long)rl.rlim_cur);
                }
                break;
            }
            case 'C': {
                char *end;
                long val = strtol(opts[i].arg, &end, 10);
                if (*end != '\0') {
                    fprintf(stderr, "Неверное значение для -C: %s\n", opts[i].arg);
                } else {
                    struct rlimit rl;
                    if (getrlimit(RLIMIT_CORE, &rl) == 0) {
                        rl.rlim_cur = val;
                        if (setrlimit(RLIMIT_CORE, &rl) == -1) {
                            perror("setrlimit");
                        } else {
                            printf("core file size изменён на %ld\n", val);
                        }
                    }
                }
                break;
            }
            case 'd': {
                char cwd[PATH_MAX];
                if (getcwd(cwd, sizeof(cwd)) != NULL) {
                    printf("cwd = %s\n", cwd);
                } else {
                    perror("getcwd");
                }
                break;
            }
            case 'v': {
                for (char **env = environ; *env; env++) {
                    printf("%s\n", *env);
                }
                break;
            }
            case 'V': {
                char *arg = opts[i].arg;
                char *eq = strchr(arg, '=');
                if (!eq) {
                    fprintf(stderr, "Неверный формат для -V: %s (нужно name=value)\n", arg);
                } else {
                    if (putenv(arg) != 0) {
                        perror("putenv");
                    } else {
                        printf("Переменная окружения установлена: %s\n", arg);
                    }
                }
                break;
            }
            default:
                fprintf(stderr, "Неизвестная опция: %c\n", opts[i].opt);
                break;
        }
    }

    for (int i = 0; i < opt_count; i++) {
        free(opts[i].arg);
    }

    return 0;
}

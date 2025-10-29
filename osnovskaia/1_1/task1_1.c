
#include <sys/stat.h>
#include <sys/param.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <string.h>

void print_usage()
{
    printf("-i Печатает реальные и эффективные идентификаторы пользователя и группы.\n-s Процесс становится лидером группы. Подсказка: смотри setpgid(2).\n-p Печатает идентификаторы процесса, процесса-родителя и группы процессов.\n-u Печатает значение ulimit\n");
    printf("-Unew_ulimit Изменяет значение ulimit. Подсказка: смотри atol(3C) на странице руководства strtol(3C)\n-c Печатает размер в байтах core-файла, который может быть создан.\n-Csize Изменяет размер core-файла");
    printf("-d Печатает текущую рабочую директорию\n-v Распечатывает переменные среды и их значения\n-Vname=value Вносит новую переменную в среду или изменяет значение существующей переменной.\n\n\n");
}

void set_environment_variable(const char *name, const char *value)
{
    printf("\n");
    if (setenv(name, value, 1) == -1)
    {
        printf("Couldn't set env var\n");
        return;
    }
    printf("Environment variable %s set to %s\n", name, value);
}
int parse_name_value(const char *input, char **name, char **value)
{
    char *equals = strchr(input, '=');
    if (equals == NULL)
    {
        printf("Invalid format for -V. Use: name=value\n");
        return -1;
    }
    size_t name_len = equals - input;
    *name = malloc(name_len + 1);
    *value = strdup(equals + 1);
    if (*name == NULL || *value == NULL)
    {
        printf("malloc/strdup failure\n");
        free(*name);
        *name = NULL;
        free(*value);
        *value = NULL;
        return -1;
    }
    strncpy(*name, input, name_len);
    (*name)[name_len] = '\0';
    return 0;
}
extern char **environ;
int main(int argc, char *argv[])
{
    print_usage();
    int c;
    while ((c = getopt(argc, argv, "ispuU:cC:dvV:")) != -1)
    {
        switch (c)
        {
        case 'i':
        {
            uid_t effective_uid = geteuid();
            gid_t effective_gid = getegid();
            uid_t real_uid = getuid();
            gid_t real_gid = getgid();
            printf("effective id's: uid=%d, gid=%d \nreal id's: uid=%d, gid=%d\n", (int)effective_uid, (int)effective_gid, (int)real_uid, (int)real_gid);
        }
        break;
        case 's':
        {
            if (setpgid(0, 0) == -1)
            {
                printf("error setting group leader\n");
                return 1;
            }
            printf("Process became new group leader\n");
        }
        break;
        case 'p':
        {
            pid_t pid = getpid();
            printf("Process id: %d, parent process id:%d, group process id:%d\n", (int)pid, (int)getppid(), (int)getpgid(pid));
        }
        break;
        case 'u':
        {
        long max_proc = sysconf(_SC_CHILD_MAX);
        printf("sysconf(_SC_CHILD_MAX): %ld\n", max_proc);
        }
        break;
        case 'U':
        {
            if (optarg != NULL)
            {
                long new_limit = atol(optarg);
                struct rlimit rlp;
                if (getrlimit(RLIMIT_FSIZE, &rlp) == -1)
                {
                    perror("getrlimit");
                    return 1;
                }
                rlp.rlim_cur = new_limit;
                if (setrlimit(RLIMIT_FSIZE, &rlp) == -1)
                {
                    perror("setrlimit");
                    return 1;
                }
                printf("New ulimit set to: %ld\n", new_limit);
            }
            else
            {
                fprintf(stderr, "Option -U requires an argument\n");
                return 1;
            }
        }
        break;
        case 'c':
        {
            struct rlimit rlim;
            if (getrlimit(RLIMIT_CORE, &rlim) == -1)
            {
                perror("getrlimit failed");
                return 1;
            }
            printf("Current core file size limit: ");
            if (rlim.rlim_cur == RLIM_INFINITY)
            {
                printf("unlimited\n");
            }
            else
            {
                printf("%ld bytes\n", (long)rlim.rlim_cur);
            }
        }
        break;
        case 'C':
        {
            if (optarg == NULL)
            {
                fprintf(stderr, "Option -C requires an argument\n");
                return 1;
            }
            // Преобразуем строку в число
            char *endptr;
            long size = strtol(optarg, &endptr, 10);
            // Проверяем корректность преобразования
            if (endptr == optarg || *endptr != '\0')
            {
                fprintf(stderr, "Invalid number for -C: %s\n", optarg);
                return 1;
            }
            // Проверяем на отрицательные значения
            if (size < 0)
            {
                fprintf(stderr, "Core file size cannot be negative: %ld\n", size);
                return 1;
            }
            struct rlimit rlim;
            if (getrlimit(RLIMIT_CORE, &rlim) == -1)
            {
                perror("getrlimit failed");
                return 1;
            }
            rlim.rlim_cur = (rlim_t)size;
            if (setrlimit(RLIMIT_CORE, &rlim) == -1)
            {
                perror("setrlimit failed");
                return 1;
            }
            printf("Core file size limit set to: ");
            if (rlim.rlim_cur == RLIM_INFINITY)
            {
                printf("unlimited\n");
            }
            else
            {
                printf("%ld bytes\n", (long)rlim.rlim_cur);
            }
        }
        break;
        case 'd':
        {
            // #ifdef MAXPATHLEN // Для Solaris
            //           char cwd[MAXPATHLEN];
            // #elif defined(PATH_MAX) // Для Linux и других
            char cwd[PATH_MAX];
            // #else
            //           char cwd[4096]; // Fallback значение
            // #endif
            if (getcwd(cwd, sizeof(cwd)) != NULL)
            {
                printf("Current directory: %s\n", cwd);
            }
            else
            {
                perror("getcwd() error");
            }
        }
        break;
        case 'v':
        {
            printf("Environment variables:\n");
            printf("=====================\n");
            for (char **env = environ; *env != NULL; env++)
            {
                printf("%s\n", *env);
            }
            printf("=====================\n");
        }
        break;
        case 'V':
        {
            char *name, *value;
            if (parse_name_value(optarg, &name, &value) == -1)
            {
                break;
            }
            set_environment_variable(name, value);
            free(name);
            free(value);
        }
        break;
        case '?':
            fprintf(stderr, "Invalid option: %c\n", optopt);
            break;
        default:
            fprintf(stderr, "Unexpected error\n");
            break;
        }
    }
    if (optind == 1)
    {
        printf("No options provided.\n");
    }
    return 0;
}

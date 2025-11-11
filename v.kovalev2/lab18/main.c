#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <stdint.h>

static const char* base_name(const char* path)
{
    const char* slash = strrchr(path, '/');
    return (slash ? slash + 1 : path);
}

static void print_mode_and_perms(mode_t mode)
{
    /* тип файла */
    if (S_ISDIR(mode))
    {
        printf("d");
    }
    else if (S_ISREG(mode))
    {
        printf("-");
    }
    else
    {
        printf("?");
    }

    /* rwx для owner/group/other */
    const int shifts[3] = {0, 3, 6}; /* будем сдвигать от прав пользователя */
    for (int j = 0; j < 3; j++)
    {
        int s = shifts[j];
        printf("%c", (mode & (S_IRUSR >> s)) ? 'r' : '-');
        printf("%c", (mode & (S_IWUSR >> s)) ? 'w' : '-');
        printf("%c", (mode & (S_IXUSR >> s)) ? 'x' : '-');
    }
}

static void print_owner_group(uid_t uid, gid_t gid)
{
    struct passwd* pwd = getpwuid(uid);
    struct group* grp = getgrgid(gid);

    if (pwd)
    {
        printf(" %s", pwd->pw_name);
    }
    else
    {
        printf(" %u", (unsigned)uid);
    }

    if (grp)
    {
        printf(" %s", grp->gr_name);
    }
    else
    {
        printf(" %u", (unsigned)gid);
    }
}

static void print_mtime(time_t t)
{
    struct tm tm_info;
    localtime_r(&t, &tm_info);
    char buf[32];
    /* Пример: "Nov 11 21:07" */
    strftime(buf, sizeof(buf), "%b %e %H:%M", &tm_info);
    printf(" %s", buf);
}

static void print_entry_line(const char* path, const struct stat* st)
{
    print_mode_and_perms(st->st_mode);

    /* количество ссылок */
    printf(" %lu", (unsigned long)st->st_nlink);

    /* владелец/группа */
    print_owner_group(st->st_uid, st->st_gid);

    /* размер — только для обычного файла */
    if (S_ISREG(st->st_mode))
    {
        printf(" %jd", (intmax_t)st->st_size);
    }
    else
    {
        printf(""); /* поле пустое для не-regular — по условию */
    }

    /* время */
    print_mtime(st->st_mtime);

    /* имя (только базовое) */
    printf(" %s\n", base_name(path));
}

static void process_path(const char* path);

static void process_directory(const char* dirpath)
{
    DIR* dir = opendir(dirpath);
    if (!dir)
    {
        perror(dirpath);
        return;
    }

    struct dirent* ent;
    while ((ent = readdir(dir)) != NULL)
    {
        const char* name = ent->d_name;
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
        {
            continue;
        }

        /* склеиваем путь: dirpath + "/" + name */
        size_t len_dir = strlen(dirpath);
        size_t len_name = strlen(name);
        int needs_slash = (len_dir > 0 && dirpath[len_dir - 1] != '/');

        size_t full_len = len_dir + (needs_slash ? 1 : 0) + len_name + 1;
        char* full = (char*)malloc(full_len);
        if (!full)
        {
            fprintf(stderr, "malloc failed\n");
            closedir(dir);
            return;
        }

        if (needs_slash)
        {
            snprintf(full, full_len, "%s/%s", dirpath, name);
        }
        else
        {
            snprintf(full, full_len, "%s%s", dirpath, name);
        }

        process_path(full);
        free(full);
    }

    closedir(dir);
}

static void process_path(const char* path)
{
    struct stat st;
    if (lstat(path, &st) == -1)
    {
        perror(path);
        return;
    }

    /* печатаем строку для самого объекта */
    print_entry_line(path, &st);

    /* если это каталог — рекурсивно обходим содержимое */
    if (S_ISDIR(st.st_mode))
    {
        process_directory(path);
    }
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "No filename\n");
        return 1;
    }

    for (int i = 1; i < argc; i++)
    {
        process_path(argv[i]);
    }

    return 0;
}

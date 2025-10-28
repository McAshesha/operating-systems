#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
extern char *tzname[];

int main() {
    time_t now;
    struct tm *sp;
    
    setenv("TZ", "PST8", 1);
    tzset();
    (void) time(&now);
    printf("Время в Калифорнии:\n");
    printf("%s", ctime(&now));

    sp = localtime(&now);
    printf("%d/%d/%02d %d:%02d %s\n",
        sp->tm_mon + 1, sp->tm_mday,
        sp->tm_year + 1900, sp->tm_hour + 1,
        sp->tm_min, tzname[sp->tm_isdst]);
    exit(0);
}

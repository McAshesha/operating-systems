#include <unistd.h>
#include <stdio.h>
#include <termios.h>
#include <fcntl.h>
#include <stdlib.h>

#define LineLen 40

int main()
{
    int fd;
    struct termios tty, save;
    char line[LineLen+1] = {0};
    int curEnd = 0;

    if (isatty(fileno(stdout)) == 0)
    {
        perror("Stdout is not terminal");
        exit(1);
    }

    fd = open("/dev/tty", O_RDONLY);
    tcgetattr(fd, &tty);
    save = tty;
    tty.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(fd, TCSAFLUSH, &tty);

    char c[2];
    c[0] = 0;
    c[1] = 0;

    read(fd, c, 1);

    while(c[0] != tty.c_cc[VEOF])
    {
        if(c[0] == tty.c_cc[VERASE])
        {

        }
        else
        {
            line[curEnd] = c[0];
            curEnd++;
            line[curEnd] = 0;
            write(1, line, LineLen+1);
        }
        read(fd, c, 1);
    }

    tcsetattr(fd, TCSAFLUSH, &save);
    exit(0);
}

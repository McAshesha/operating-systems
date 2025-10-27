#include <unistd.h>
#include <stdio.h>
#include <termios.h>
#include <fcntl.h>
#include <stdlib.h>
#include <ctype.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

#define LINE_LEN 40
#define WORD_BUF_CAP 1024

static int term_fd = -1;
static struct termios saved_tio;

static void restore_tty(void)
{
    if (term_fd >= 0)
    {
        tcsetattr(term_fd, TCSAFLUSH, &saved_tio);
    }
}

static void on_signal(int signo)
{
    (void)signo;
    restore_tty();
    _exit(128 + signo);
}

static void put_char(int fd, char ch)
{
    (void)write(fd, &ch, 1);
}

static void put_str(int fd, const char *s)
{
    (void)write(fd, s, strlen(s));
}

static void erase_one(int fd, char *line_buf, int *col)
{
    if (*col <= 0)
    {
        return;
    }
    put_str(fd, "\b \b");
    (*col)--;
    line_buf[*col] = '\0';
}

static void recompute_current_word(const char *line_buf, int col, int *word_start_col, int *word_len)
{
    int i = col;
    while (i > 0 && line_buf[i - 1] == ' ')
    {
        i--;
    }
    int j = i;
    while (j > 0 && line_buf[j - 1] != ' ')
    {
        j--;
    }
    *word_start_col = j;
    *word_len = i - j;
}

int main(void)
{
    term_fd = open("/dev/tty", O_RDWR);
    if (term_fd < 0)
    {
        perror("open /dev/tty");
        return 1;
    }

    if (tcgetattr(term_fd, &saved_tio) < 0)
    {
        perror("tcgetattr");
        return 1;
    }

    struct termios raw = saved_tio;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(term_fd, TCSAFLUSH, &raw) < 0)
    {
        perror("tcsetattr");
        return 1;
    }

    atexit(restore_tty);
    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);

    unsigned char ch_erase = saved_tio.c_cc[VERASE];
    unsigned char ch_kill  = saved_tio.c_cc[VKILL];
    unsigned char ch_eof   = saved_tio.c_cc[VEOF];
#ifdef VWERASE
    unsigned char ch_werase = saved_tio.c_cc[VWERASE];
#else
    unsigned char ch_werase = 0x17;
#endif

    char line_buf[LINE_LEN + 1] = {0};
    int col = 0;

    char word_buf[WORD_BUF_CAP];
    int word_len = 0;
    int word_start_col = 0;

    for (;;)
    {
        unsigned char ch;
        ssize_t n = read(term_fd, &ch, 1);
        if (n <= 0)
        {
            if (n < 0 && errno == EINTR)
            {
                continue;
            }
            break;
        }

        if (ch == ch_eof)
        {
            if (col == 0)
            {
                put_str(term_fd, "\r\n");
                break;
            }
            else
            {
                put_char(term_fd, '\a');
                continue;
            }
        }

        if (ch == ch_erase)
        {
            if (col > 0)
            {
                erase_one(term_fd, line_buf, &col);
                recompute_current_word(line_buf, col, &word_start_col, &word_len);
            }
            else
            {
                put_char(term_fd, '\a');
            }
            continue;
        }

        if (ch == ch_kill)
        {
            while (col > 0)
            {
                erase_one(term_fd, line_buf, &col);
            }
            word_len = 0;
            word_start_col = 0;
            continue;
        }

        if (ch == ch_werase || ch == 0x17)
        {
            while (col > 0 && line_buf[col - 1] == ' ')
            {
                erase_one(term_fd, line_buf, &col);
            }
            while (col > 0 && line_buf[col - 1] != ' ')
            {
                erase_one(term_fd, line_buf, &col);
            }
            recompute_current_word(line_buf, col, &word_start_col, &word_len);
            continue;
        }

        if (isprint(ch))
        {
            if (ch == ' ')
            {
                if (col == LINE_LEN)
                {
                    put_char(term_fd, '\n');
                    col = 0;
                    line_buf[0] = '\0';
                }
                put_char(term_fd, ch);
                if (col < LINE_LEN)
                {
                    line_buf[col++] = ' ';
                    line_buf[col] = '\0';
                }
                else
                {
                    col = 0;
                    line_buf[0] = '\0';
                }
                word_len = 0;
                word_start_col = col;
            }
            else
            {
                if (word_len == 0)
                {
                    word_start_col = col;
                }

                put_char(term_fd, ch);

                if (col < LINE_LEN)
                {
                    line_buf[col++] = (char)ch;
                    line_buf[col] = '\0';
                }
                else
                {
                    col++;
                }

                if (word_len < WORD_BUF_CAP - 1)
                {
                    word_buf[word_len++] = (char)ch;
                }

                if (col > LINE_LEN)
                {
                    for (int i = 0; i < word_len; ++i)
                    {
                        erase_one(term_fd, line_buf, &col);
                    }

                    put_char(term_fd, '\n');
                    col = 0;
                    line_buf[0] = '\0';

                    for (int i = 0; i < word_len; ++i)
                    {
                        char wc = word_buf[i];
                        put_char(term_fd, wc);
                        if (col < LINE_LEN)
                        {
                            line_buf[col++] = wc;
                            line_buf[col] = '\0';
                        }
                        else
                        {
                            col++;
                        }
                    }
                    word_start_col = 0;
                }
            }
            continue;
        }

        put_char(term_fd, '\a');
    }

    return 0;
}

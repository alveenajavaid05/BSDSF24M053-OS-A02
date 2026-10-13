#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <limits.h>

void do_ls(const char *dir, int display_mode);

void permissions(mode_t mode, char *p)
{
    p[0] = S_ISDIR(mode) ? 'd' : S_ISLNK(mode) ? 'l' :
           S_ISCHR(mode) ? 'c' : S_ISBLK(mode) ? 'b' :
           S_ISFIFO(mode) ? 'p' : S_ISSOCK(mode) ? 's' : '-';
    p[1] = mode & S_IRUSR ? 'r' : '-';
    p[2] = mode & S_IWUSR ? 'w' : '-';
    p[3] = mode & S_IXUSR ? 'x' : '-';
    p[4] = mode & S_IRGRP ? 'r' : '-';
    p[5] = mode & S_IWGRP ? 'w' : '-';
    p[6] = mode & S_IXGRP ? 'x' : '-';
    p[7] = mode & S_IROTH ? 'r' : '-';
    p[8] = mode & S_IWOTH ? 'w' : '-';
    p[9] = mode & S_IXOTH ? 'x' : '-';
    if (mode & S_ISUID) p[3] = mode & S_IXUSR ? 's' : 'S';
    if (mode & S_ISGID) p[6] = mode & S_IXGRP ? 's' : 'S';
    if (mode & S_ISVTX) p[9] = mode & S_IXOTH ? 't' : 'T';
    p[10] = '\0';
}

void print_long(const char *path, const char *name)
{
    struct stat info;

    if (lstat(path, &info) == -1)
    {
        perror(path);
        return;
    }

    char perm[11], date[20];
    permissions(info.st_mode, perm);

    struct passwd *owner = getpwuid(info.st_uid);
    struct group *group = getgrgid(info.st_gid);
    struct tm *time_info = localtime(&info.st_mtime);

    if (time_info)
        strftime(date, sizeof(date), "%b %e %H:%M", time_info);
    else
        strcpy(date, "unknown time");

    printf("%s %2lu %-8s %-8s %8lld %s %s\n",
           perm, (unsigned long)info.st_nlink,
           owner ? owner->pw_name : "?",
           group ? group->gr_name : "?",
           (long long)info.st_size, date, name);
}

int terminal_width(void)
{
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0)
        return ws.ws_col;

    return 80;
}

void print_columns(char **names, int count, size_t max_len)
{
    if (count == 0)
        return;

    int width = terminal_width();
    int col_width = (int)max_len + 2;

    int columns = width / col_width;
    if (columns < 1) columns = 1;
    if (columns > count) columns = count;

    int rows = (count + columns - 1) / columns;

    for (int row = 0; row < rows; row++)
    {
        for (int col = 0; col < columns; col++)
        {
            int index = col * rows + row;

            if (index < count)
            {
                printf("%s", names[index]);

                if (col < columns - 1 && index + rows < count)
                    printf("%*s",
                           col_width - (int)strlen(names[index]), "");
            }
        }
        putchar('\n');
    }
}

void print_horizontal(char **names, int count, size_t max_len)
{
    if (count == 0)
        return;

    int width = terminal_width();
    int col_width = (int)max_len + 2;
    int line_pos = 0;

    for (int i = 0; i < count; i++)
    {
        if (line_pos > 0 && line_pos + col_width > width)
        {
            putchar('\n');
            line_pos = 0;
        }

        printf("%s", names[i]);
        line_pos += (int)strlen(names[i]);

        if (i < count - 1)
        {
            int spaces = col_width - (int)strlen(names[i]);
            printf("%*s", spaces, "");
            line_pos += spaces;
        }
    }

    putchar('\n');
}

void do_ls(const char *dir, int display_mode)
{
    DIR *dp = opendir(dir);

    if (!dp)
    {
        perror(dir);
        return;
    }

    char **names = NULL;
    int count = 0;
    int capacity = 0;
    size_t max_len = 0;
    struct dirent *entry;

    errno = 0;

    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;

        if (display_mode == 1)
        {
            char path[PATH_MAX];
            int length = snprintf(path, sizeof(path), "%s/%s",
                                  dir, entry->d_name);

            if (length < 0 || length >= (int)sizeof(path))
            {
                fprintf(stderr, "Path too long: %s/%s\n",
                        dir, entry->d_name);
                continue;
            }

            print_long(path, entry->d_name);
            errno = 0;
            continue;
        }

        if (count == capacity)
        {
            int new_capacity = capacity == 0 ? 16 : capacity * 2;
            char **temp = realloc(names, new_capacity * sizeof(char *));

            if (!temp)
            {
                perror("realloc");
                break;
            }

            names = temp;
            capacity = new_capacity;
        }

        names[count] = strdup(entry->d_name);

        if (!names[count])
        {
            perror("strdup");
            break;
        }

        size_t len = strlen(names[count]);

        if (len > max_len)
            max_len = len;

        count++;
        errno = 0;
    }

    if (errno != 0)
        perror("readdir");

    closedir(dp);

    if (display_mode == 2)
        print_horizontal(names, count, max_len);
    else if (display_mode == 0)
        print_columns(names, count, max_len);

    for (int i = 0; i < count; i++)
        free(names[i]);

    free(names);
}

int main(int argc, char *argv[])
{
    int display_mode = 0;
    int opt;

    while ((opt = getopt(argc, argv, "lx")) != -1)
    {
        if (opt == 'l')
            display_mode = 1;
        else if (opt == 'x')
            display_mode = 2;
        else
        {
            fprintf(stderr, "Usage: %s [-l | -x] [directory ...]\n",
                    argv[0]);
            return 1;
        }
    }

    if (optind == argc)
        do_ls(".", display_mode);
    else
    {
        for (int i = optind; i < argc; i++)
        {
            if (argc - optind > 1)
                printf("Directory listing of %s:\n", argv[i]);

            do_ls(argv[i], display_mode);

            if (i < argc - 1)
                putchar('\n');
        }
    }

    return 0;
}

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

#define COLOR_RESET   "\033[0m"
#define COLOR_BLUE    "\033[1;34m"
#define COLOR_GREEN   "\033[1;32m"
#define COLOR_RED     "\033[1;31m"
#define COLOR_PINK    "\033[1;35m"
#define COLOR_REVERSE "\033[7m"

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

    if (mode & S_ISUID)
        p[3] = mode & S_IXUSR ? 's' : 'S';

    if (mode & S_ISGID)
        p[6] = mode & S_IXGRP ? 's' : 'S';

    if (mode & S_ISVTX)
        p[9] = mode & S_IXOTH ? 't' : 'T';

    p[10] = '\0';
}

int has_suffix(const char *name, const char *suffix)
{
    size_t name_len = strlen(name);
    size_t suffix_len = strlen(suffix);

    if (name_len < suffix_len)
        return 0;

    return strcmp(name + name_len - suffix_len, suffix) == 0;
}

const char *get_file_color(const char *path, const char *name)
{
    struct stat info;

    if (lstat(path, &info) == -1)
        return NULL;

    if (S_ISLNK(info.st_mode))
        return COLOR_PINK;

    if (S_ISDIR(info.st_mode))
        return COLOR_BLUE;

    if (S_ISCHR(info.st_mode) || S_ISBLK(info.st_mode) ||
        S_ISFIFO(info.st_mode) || S_ISSOCK(info.st_mode))
        return COLOR_REVERSE;

    if (has_suffix(name, ".tar") ||
        has_suffix(name, ".gz") ||
        has_suffix(name, ".zip"))
        return COLOR_RED;

    if (info.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))
        return COLOR_GREEN;

    return NULL;
}

void print_colored_name(const char *dir, const char *name)
{
    char path[PATH_MAX];

    int length = snprintf(path, sizeof(path), "%s/%s", dir, name);

    if (length < 0 || length >= (int)sizeof(path))
    {
        printf("%s", name);
        return;
    }

    const char *color = get_file_color(path, name);

    if (color != NULL)
        printf("%s%s%s", color, name, COLOR_RESET);
    else
        printf("%s", name);
}

void print_long(const char *path, const char *name)
{
    struct stat info;

    if (lstat(path, &info) == -1)
    {
        perror(path);
        return;
    }

    char perm[11];
    char date[20];

    permissions(info.st_mode, perm);

    struct passwd *owner = getpwuid(info.st_uid);
    struct group *group = getgrgid(info.st_gid);
    struct tm *time_info = localtime(&info.st_mtime);

    if (time_info)
        strftime(date, sizeof(date), "%b %e %H:%M", time_info);
    else
        strcpy(date, "unknown time");

    printf("%s %2lu %-8s %-8s %8lld %s ",
           perm,
           (unsigned long)info.st_nlink,
           owner ? owner->pw_name : "?",
           group ? group->gr_name : "?",
           (long long)info.st_size,
           date);

    const char *color = get_file_color(path, name);

    if (color != NULL)
        printf("%s%s%s", color, name, COLOR_RESET);
    else
        printf("%s", name);

    putchar('\n');
}

int terminal_width(void)
{
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 &&
        ws.ws_col > 0)
        return ws.ws_col;

    return 80;
}

void print_columns(const char *dir, char **names,
                   int count, size_t max_len)
{
    if (count == 0)
        return;

    int width = terminal_width();
    int col_width = (int)max_len + 2;

    int columns = width / col_width;

    if (columns < 1)
        columns = 1;

    if (columns > count)
        columns = count;

    int rows = (count + columns - 1) / columns;

    for (int row = 0; row < rows; row++)
    {
        for (int col = 0; col < columns; col++)
        {
            int index = col * rows + row;

            if (index < count)
            {
                print_colored_name(dir, names[index]);

                if (col < columns - 1 && index + rows < count)
                {
                    int spaces =
                        col_width - (int)strlen(names[index]);

                    printf("%*s", spaces, "");
                }
            }
        }

        putchar('\n');
    }
}

void print_horizontal(const char *dir, char **names,
                      int count, size_t max_len)
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

        print_colored_name(dir, names[i]);

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

int compare_names(const void *a, const void *b)
{
    const char *name_a = *(const char **)a;
    const char *name_b = *(const char **)b;

    return strcmp(name_a, name_b);
}

void do_ls(const char *dir, int display_mode)
{
    DIR *dp = opendir(dir);

    if (dp == NULL)
    {
        perror(dir);
        return;
    }

    char **names = NULL;
    int count = 0;
    int capacity = 0;
    int max_len = 0;
    struct dirent *entry;

    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;

        if (count == capacity)
        {
            int new_capacity =
                (capacity == 0) ? 16 : capacity * 2;

            char **temp = realloc(
                names, new_capacity * sizeof(char *));

            if (temp == NULL)
            {
                perror("realloc");
                break;
            }

            names = temp;
            capacity = new_capacity;
        }

        names[count] = strdup(entry->d_name);

        if (names[count] == NULL)
        {
            perror("strdup");
            break;
        }

        int length = (int)strlen(names[count]);

        if (length > max_len)
            max_len = length;

        count++;
    }

    closedir(dp);

    if (count > 1)
        qsort(names, count, sizeof(char *), compare_names);

    if (display_mode == 1)
    {
        for (int i = 0; i < count; i++)
        {
            char path[PATH_MAX];

            int length = snprintf(
                path, sizeof(path), "%s/%s", dir, names[i]);

            if (length < 0 || length >= (int)sizeof(path))
            {
                fprintf(stderr, "Path too long: %s/%s\n",
                        dir, names[i]);
                continue;
            }

            print_long(path, names[i]);
        }
    }
    else if (display_mode == 2)
    {
        print_horizontal(dir, names, count, (size_t)max_len);
    }
    else
    {
        print_columns(dir, names, count, (size_t)max_len);
    }

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
            fprintf(stderr,
                    "Usage: %s [-l | -x] [directory ...]\n",
                    argv[0]);
            return 1;
        }
    }

    if (optind == argc)
    {
        do_ls(".", display_mode);
    }
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

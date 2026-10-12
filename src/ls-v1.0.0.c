
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <limits.h>

void do_ls(const char *dir, int long_format);

void permissions(mode_t mode, char *p)
{
    p[0] = S_ISDIR(mode) ? 'd' :
           S_ISLNK(mode) ? 'l' :
           S_ISCHR(mode) ? 'c' :
           S_ISBLK(mode) ? 'b' :
           S_ISFIFO(mode) ? 'p' :
           S_ISSOCK(mode) ? 's' : '-';

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
        p[3] = (mode & S_IXUSR) ? 's' : 'S';
    if (mode & S_ISGID)
        p[6] = (mode & S_IXGRP) ? 's' : 'S';
    if (mode & S_ISVTX)
        p[9] = (mode & S_IXOTH) ? 't' : 'T';

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

    char perm[11];
    char date[20];
    permissions(info.st_mode, perm);

    struct passwd *owner = getpwuid(info.st_uid);
    struct group *group = getgrgid(info.st_gid);

    struct tm *time_info = localtime(&info.st_mtime);

    if (time_info != NULL)
        strftime(date, sizeof(date), "%b %e %H:%M", time_info);
    else
        strcpy(date, "unknown time");

    printf("%s %2lu %-8s %-8s %8lld %s %s\n",
           perm,
           (unsigned long)info.st_nlink,
           owner ? owner->pw_name : "?",
           group ? group->gr_name : "?",
           (long long)info.st_size,
           date,
           name);
}

void do_ls(const char *dir, int long_format)
{
    struct dirent *entry;
    DIR *dp = opendir(dir);

    if (dp == NULL)
    {
        perror(dir);
        return;
    }

    errno = 0;

    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;

        if (long_format)
        {
            char path[PATH_MAX];

            if (snprintf(path, sizeof(path), "%s/%s",
                         dir, entry->d_name) >= (int)sizeof(path))
            {
                fprintf(stderr, "Path too long: %s/%s\n",
                        dir, entry->d_name);
                continue;
            }

            print_long(path, entry->d_name);
        }
        else
        {
            printf("%s\n", entry->d_name);
        }

        errno = 0;
    }

    if (errno != 0)
        perror("readdir");

    closedir(dp);
}

int main(int argc, char *argv[])
{
    int long_format = 0;
    int opt;

    while ((opt = getopt(argc, argv, "l")) != -1)
    {
        if (opt == 'l')
            long_format = 1;
        else
        {
            fprintf(stderr, "Usage: %s [-l] [directory ...]\n",
                    argv[0]);
            return 1;
        }
    }

    if (optind == argc)
    {
        do_ls(".", long_format);
    }
    else
    {
        int i;

        for (i = optind; i < argc; i++)
        {
            if (argc - optind > 1)
                printf("Directory listing of %s:\n", argv[i]);

            do_ls(argv[i], long_format);

            if (i < argc - 1)
                puts("");
        }
    }

    return 0;
}


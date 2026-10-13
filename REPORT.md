# Operating Systems Programming Assignment 02
## Feature 6: Colorized Output Based on File Type (v1.5.0)

### Question 1: What are ANSI escape codes? Give the escape code for green color.

ANSI escape codes are special character sequences used to control terminal output formatting. They can change text color, background color, brightness, and other display properties.

In C, the escape character is written as `\033`. The general format for setting a text color is:

`"\033[code m"`

Examples used in this program:

- Blue: `"\033[1;34m"`
- Green: `"\033[1;32m"`
- Red: `"\033[1;31m"`
- Pink/Magenta: `"\033[1;35m"`
- Reverse video: `"\033[7m"`
- Reset formatting: `"\033[0m"`

The green color escape sequence used in this program is `"\033[1;32m"`. The reset sequence `"\033[0m"` restores the terminal's default formatting after the filename is printed.

### Question 2: Which permission bits should be checked to determine whether a file is executable?

In Linux, execute permissions are stored separately for the file owner, group, and other users.

The three permission bits are:

- `S_IXUSR`: execute permission for the file owner.
- `S_IXGRP`: execute permission for users in the file's group.
- `S_IXOTH`: execute permission for other users.

In C, these bits can be checked together using a bitwise AND operation:

`info.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)`

If the result is nonzero, at least one of these execute permission bits is set. In this assignment, such a file is displayed in green unless another file-type rule takes priority.

### Implementation Summary

The program uses `lstat()` to obtain file information without following symbolic links. It checks the file type with macros such as `S_ISDIR()` and `S_ISLNK()`, and checks execute permissions using `S_IXUSR`, `S_IXGRP`, and `S_IXOTH`.

The program displays directories in blue, executable files in green, `.tar`, `.gz`, and `.zip` files in red, symbolic links in pink/magenta, and special files using reverse video. Other files are displayed in the terminal's default color.

The same color rules are applied to the default column display, horizontal display (`-x`), and long listing (`-l`).

# Feature 2: Long Listing Format (-l)

## 1. Difference between stat() and lstat()

Both stat() and lstat() retrieve file metadata, including file type, permissions, size, and modification time. The difference is how they handle symbolic links. stat() follows a symbolic link and returns information about its target, whereas lstat() returns information about the symbolic link itself. For an ls utility, lstat() is useful when we need to identify and display symbolic links separately from their targets.

## 2. Understanding st_mode and Bitwise Operators

The st_mode field in struct stat stores the file type and permission bits. Macros such as S_ISDIR(mode) and S_ISREG(mode) identify directories and regular files. Permission macros such as S_IRUSR, S_IWUSR, and S_IXUSR represent the owner's read, write, and execute permissions. The bitwise AND operator (&) can test whether a permission bit is set. For example, (mode & S_IRUSR) checks the owner's read permission.

# Feature 3: Column Display (Down Then Across)

## 1. Down-Then-Across Printing Logic

In down-then-across formatting, filenames are placed vertically in each column before moving to the next column. The program first collects the filenames and determines the maximum filename length. It calculates the number of columns and rows that fit within the terminal width. It then uses indexed access to print entries belonging to each row, such as filenames[i], filenames[i + rows], and filenames[i + 2 * rows]. A simple sequential loop is insufficient because it would print entries from left to right rather than filling each column from top to bottom.

## 2. Purpose of ioctl()

The ioctl() system call can retrieve terminal information, including its width, using the TIOCGWINSZ request. This helps the program arrange filenames to fit the current terminal size. A fixed width of 80 characters is simpler but may cause poor alignment, unnecessary wrapping, or wasted space when the actual terminal is wider or narrower.

# Feature 4: Horizontal Column Display (-x)

## 1. Vertical Versus Horizontal Printing

Down-then-across formatting requires more pre-calculation because the program must determine the number of rows and calculate which entries belong in each column. Horizontal formatting processes the sorted filenames from left to right and moves to a new line when the next filename would exceed the terminal width. Both modes need filename lengths and terminal width, but vertical formatting requires additional indexing and row/column calculations.

## 2. Managing Display Modes

The program uses command-line argument parsing with getopt() to identify options. A display-mode variable records whether long listing (-l), horizontal display (-x), or the default column display should be used. After reading and sorting the filenames, the program checks the selected mode and calls the corresponding printing function. The -R option independently enables recursive traversal.

# Feature 5: Alphabetical Sorting

## 1. Why Read All Entries Before Sorting?

The program must collect directory entries into memory before sorting because qsort() needs access to the complete array. Once the entries have been collected, qsort() arranges them alphabetically before the display function prints them. For directories containing millions of files, storing every filename can consume substantial memory, increase allocation overhead, and make sorting slower. Very large directories may therefore require more memory-efficient or external sorting strategies.

## 2. qsort() Comparison Function

The qsort() function sorts an array using a comparison function supplied by the programmer. Its general signature is:

    void qsort(void *base, size_t nmemb, size_t size,
               int (*compar)(const void *, const void *));

For an array of strings, the comparison function receives pointers to two array elements, converts them to the appropriate string-pointer types, and uses strcmp() to compare the strings. It returns a negative value if the first item comes before the second, zero if they are equal, and a positive value if the first comes after the second. The arguments are const void * because qsort() supports arrays of different data types and the comparison function must accept generic pointers without modifying the compared elements.

# Feature 7: Recursive Listing (-R)

## 1. Base Case in Recursion

A base case is the condition that stops a recursive function from making further recursive calls. In this ls implementation, recursion stops when the current directory has no eligible subdirectories to visit. The program also skips the special entries "." and ".." to prevent traversal back into the current or parent directory. Each recursive call processes a deeper directory until no further subdirectories remain.

## 2. Why Construct the Full Path?

The program constructs the full path, such as "parent_dir/subdir", so it can locate the correct child directory relative to the current directory. If it called do_ls("subdir") instead, the program would interpret that relative path from the process's current working directory, not necessarily from "parent_dir". It could therefore fail to open the directory or list the wrong location. Building the full path allows each recursive call to access the correct nested directory.

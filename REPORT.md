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

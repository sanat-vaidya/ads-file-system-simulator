// main.c - the shell: reads a line, splits it into words, runs the command.
//
// Interactive: shows a prompt.
// Input from a file (./fs < demo.txt): echoes every line with the prompt,
// so the output reads like a recorded session. Lines starting with # are
// comments and are echoed as they are.

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include "fs.h"

#define MAX_WORDS (LINE_SIZE / 2 + 1)   // a line can never have more words than this

static bool is_space(char c) {
    return c == ' ' || c == '\t';
}

// Split line into words, in place: the spaces after each word become '\0'.
// Returns the number of words.
static int split_words(char *line, char *words[]) {
    int n = 0;
    int i = 0;
    while (line[i] != '\0') {
        while (is_space(line[i])) i++;
        if (line[i] == '\0') break;
        words[n] = line + i;
        n++;
        while (line[i] != '\0' && !is_space(line[i])) i++;
        if (line[i] != '\0') {
            line[i] = '\0';
            i++;
        }
    }
    return n;
}

// The raw text after the first `skip` words.
// text_after("write a.txt hello  world", 2) -> "hello  world"
static const char *text_after(const char *line, int skip) {
    int i = 0;
    for (int w = 0; w < skip; w++) {
        while (is_space(line[i])) i++;
        while (line[i] != '\0' && !is_space(line[i])) i++;
    }
    while (is_space(line[i])) i++;
    return line + i;
}

static void usage(const char *text) {
    printf("usage: %s\n", text);
}

static void print_help(void) {
    printf(
        "files and directories\n"
        "  mkdir <path>...        make directories\n"
        "  touch <path>...        make empty files\n"
        "  ls [path]              list a directory (sorted by its B-tree)\n"
        "  cd [path]              change directory (no path = /)\n"
        "  pwd                    print the current directory\n"
        "  write <file> <text>    replace a file's contents (creates it if needed)\n"
        "  append <file> <text>   add a line to the end of a file\n"
        "  cat <file>             print a file\n"
        "  stat <path>            show an inode\n"
        "inside the data structures\n"
        "  df                     disk and inode usage\n"
        "other\n"
        "  help                   this list\n"
        "  exit                   quit\n");
}

int main(void) {
    fs_init();
    bool interactive = isatty(STDIN_FILENO);
    if (interactive) printf("File System Simulator. Type 'help' for commands.\n");

    char line[LINE_SIZE];
    char copy[LINE_SIZE];
    char cwd[PATH_SIZE];
    char *args[MAX_WORDS];

    while (true) {
        if (interactive) {
            fs_cwd_path(cwd);
            printf("fs:%s$ ", cwd);
            fflush(stdout);
        }
        if (fgets(line, LINE_SIZE, stdin) == NULL) {
            if (interactive) printf("\n");
            break;
        }

        // drop the newline, trailing spaces and Windows line endings
        int len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r' || is_space(line[len - 1]))) {
            len--;
            line[len] = '\0';
        }

        strcpy(copy, line);
        int count = split_words(copy, args);
        bool skip = (count == 0 || args[0][0] == '#');

        if (!interactive) {
            if (skip) {
                printf("%s\n", line);
            } else {
                fs_cwd_path(cwd);
                printf("fs:%s$ %s\n", cwd, line);
            }
        }
        if (skip) continue;

        char *cmd = args[0];
        int n = count - 1;   // number of arguments

        if (strcmp(cmd, "exit") == 0 || strcmp(cmd, "quit") == 0) {
            break;
        } else if (strcmp(cmd, "help") == 0) {
            print_help();
        } else if (strcmp(cmd, "mkdir") == 0) {
            if (n < 1) usage("mkdir <path>...");
            for (int i = 1; i <= n; i++) fs_mkdir(args[i]);
        } else if (strcmp(cmd, "touch") == 0) {
            if (n < 1) usage("touch <path>...");
            for (int i = 1; i <= n; i++) fs_touch(args[i]);
        } else if (strcmp(cmd, "ls") == 0) {
            if (n > 1) usage("ls [path]");
            else fs_ls(n == 0 ? "." : args[1]);
        } else if (strcmp(cmd, "cd") == 0) {
            if (n > 1) usage("cd [path]");
            else fs_cd(n == 0 ? "/" : args[1]);
        } else if (strcmp(cmd, "pwd") == 0) {
            fs_pwd();
        } else if (strcmp(cmd, "write") == 0) {
            if (n < 1) usage("write <file> <text>");
            else fs_write(args[1], text_after(line, 2));
        } else if (strcmp(cmd, "append") == 0) {
            if (n < 1) usage("append <file> <text>");
            else fs_append(args[1], text_after(line, 2));
        } else if (strcmp(cmd, "cat") == 0) {
            if (n != 1) usage("cat <file>");
            else fs_cat(args[1]);
        } else if (strcmp(cmd, "stat") == 0) {
            if (n != 1) usage("stat <path>");
            else fs_stat(args[1]);
        } else if (strcmp(cmd, "df") == 0) {
            fs_df();
        } else {
            printf("error: unknown command: %s (type 'help')\n", cmd);
        }
    }

    fs_shutdown();
    return 0;
}

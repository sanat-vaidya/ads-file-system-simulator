#include "fs.h"
#include "disk.h"
#include "inode_table.h"
#include "btree.h"
#include "stack.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int cwd;       // id of the current directory
static int next_id;   // ids are never reused

static void error(const char *msg, const char *what) {
    printf("error: %s: %s\n", msg, what);
}

void fs_init(void) {
    disk_init();

    Inode *root = malloc(sizeof(Inode));
    root->id = ROOT_ID;
    strcpy(root->name, "/");
    root->is_dir = true;
    root->parent_id = ROOT_ID;
    root->size = 0;
    root->first_block = FAT_END;
    root->children = btree_new();
    table_insert(root);

    next_id = ROOT_ID + 1;
    cwd = ROOT_ID;
}

void fs_shutdown(void) {
    table_free_all();   // every inode and every directory's B-tree
}

// =====================================================================
// paths
// =====================================================================

// Walk parent links up to the root, pushing names on a stack,
// then pop them to get the path in root-to-leaf order.
static void path_of(int id, char *out) {
    if (id == ROOT_ID) {
        strcpy(out, "/");
        return;
    }
    Stack names;
    stack_init(&names);
    while (id != ROOT_ID) {
        Inode *in = table_find(id);
        stack_push(&names, in->name);
        id = in->parent_id;
    }
    out[0] = '\0';
    while (!stack_empty(&names)) {
        strcat(out, "/");
        strcat(out, stack_pop(&names));
    }
}

void fs_cwd_path(char *out) {
    path_of(cwd, out);
}

// Turn any path into a clean list of names from the root, using a stack:
// a name is pushed, ".." pops, "." and empty parts are skipped.
// "/a/b/../c" -> [a, c]. Relative paths start from the current directory.
//
// The names are cut out of buf in place (each '/' becomes '\0') and the
// stack points into buf, so buf must stay alive while the stack is used.
static bool normalize(const char *path, char *buf, Stack *parts) {
    if (strlen(path) >= LINE_SIZE) return false;
    if (path[0] == '/') {
        strcpy(buf, path);
    } else {
        path_of(cwd, buf);
        strcat(buf, "/");
        strcat(buf, path);
    }

    stack_init(parts);
    char *name = buf;
    int len = strlen(buf);
    for (int i = 0; i <= len; i++) {
        if (buf[i] != '/' && buf[i] != '\0') continue;
        buf[i] = '\0';   // the current name ends here
        if (strcmp(name, "..") == 0) {
            stack_pop(parts);
        } else if (strcmp(name, "") != 0 && strcmp(name, ".") != 0) {
            if (stack_full(parts)) return false;
            stack_push(parts, name);
        }
        name = buf + i + 1;
    }
    return true;
}

// Follow the names from the root down. Returns the inode id, or -1.
// The stack has the last name on top, so first reverse it into a second
// stack, which puts the first name on top.
static int walk(Stack *parts) {
    Stack order;
    stack_init(&order);
    while (!stack_empty(parts)) stack_push(&order, stack_pop(parts));

    int cur = ROOT_ID;
    while (!stack_empty(&order)) {
        Inode *dir = table_find(cur);
        if (!dir->is_dir) return -1;
        int next = btree_find(dir->children, stack_pop(&order));
        if (next == -1) return -1;
        cur = next;
    }
    return cur;
}

static int resolve(const char *path) {
    char buf[PATH_SIZE];
    Stack parts;
    if (!normalize(path, buf, &parts)) return -1;
    return walk(&parts);
}

// For creating things: split "/a/b/new" into the directory /a/b and the name "new".
// name needs LINE_SIZE chars (it may be too long to be valid; the caller checks).
static bool resolve_parent(const char *path, int *parent_id, char *name) {
    char buf[PATH_SIZE];
    Stack parts;
    if (!normalize(path, buf, &parts) || stack_empty(&parts)) return false;
    strcpy(name, stack_pop(&parts));
    *parent_id = walk(&parts);
    return *parent_id != -1 && table_find(*parent_id)->is_dir;
}

static bool valid_name(const char *name) {
    int len = strlen(name);
    if (len == 0 || len > MAX_NAME) return false;
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) return false;
    for (int i = 0; i < len; i++) {
        char c = name[i];
        bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')
                  || c == '.' || c == '_' || c == '-';
        if (!ok) return false;
    }
    return true;
}

// =====================================================================
// helpers
// =====================================================================

// ids of a directory's entries, sorted by name. Returns how many.
static int children_of(int id, int out[]) {
    return btree_list(table_find(id)->children, out);
}

// Make a new file or directory. The caller has checked the path is not taken.
// Returns the new id, or -1 after printing an error.
static int create(const char *path, bool is_dir) {
    int parent_id;
    char name[LINE_SIZE];
    if (!resolve_parent(path, &parent_id, name)) {
        error("no such directory", path);
        return -1;
    }
    if (!valid_name(name)) {
        error("invalid name", name);
        return -1;
    }
    if (table_size() >= MAX_INODES) {
        error("no free inodes", path);
        return -1;
    }

    Inode *in = malloc(sizeof(Inode));
    in->id = next_id;
    next_id++;
    strcpy(in->name, name);
    in->is_dir = is_dir;
    in->parent_id = parent_id;
    in->size = 0;
    in->first_block = FAT_END;
    in->children = NULL;
    if (is_dir) in->children = btree_new();

    table_insert(in);
    btree_insert(table_find(parent_id)->children, name, in->id);
    return in->id;
}

// =====================================================================
// namespace commands
// =====================================================================

void fs_mkdir(const char *path) {
    if (resolve(path) != -1) {
        error("already exists", path);
        return;
    }
    create(path, true);
}

void fs_touch(const char *path) {
    if (resolve(path) != -1) return;   // already there, nothing to do
    create(path, false);
}

void fs_ls(const char *path) {
    int id = resolve(path);
    if (id == -1) {
        error("no such file or directory", path);
        return;
    }
    Inode *in = table_find(id);
    if (!in->is_dir) {
        printf("%s\n", in->name);
        return;
    }
    int kids[MAX_INODES];
    int n = children_of(id, kids);   // already sorted by the B-tree
    for (int i = 0; i < n; i++) {
        Inode *c = table_find(kids[i]);
        printf("%s%s\n", c->name, c->is_dir ? "/" : "");
    }
}

void fs_cd(const char *path) {
    int id = resolve(path);
    if (id == -1) {
        error("no such directory", path);
        return;
    }
    if (!table_find(id)->is_dir) {
        error("not a directory", path);
        return;
    }
    cwd = id;
}

void fs_pwd(void) {
    char p[PATH_SIZE];
    path_of(cwd, p);
    printf("%s\n", p);
}

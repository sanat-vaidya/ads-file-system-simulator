#include "fs.h"
#include "disk.h"
#include "inode_table.h"
#include "btree.h"
#include "trie.h"
#include "stack.h"
#include "queue.h"
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
    trie_init();

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
    trie_free_all();
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
            if (!stack_empty(parts)) stack_pop(parts);
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

// True if a is b, or a is one of b's parent directories.
static bool is_ancestor(int a, int b) {
    while (true) {
        if (b == a) return true;
        if (b == ROOT_ID) return false;
        b = table_find(b)->parent_id;
    }
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
    trie_insert(name, in->id);
    return in->id;
}

// Free an inode and everything under it. DFS: children first, then itself.
static void delete_tree(int id) {
    Inode *in = table_find(id);
    if (in->is_dir) {
        int kids[MAX_INODES];
        int n = children_of(id, kids);
        for (int i = 0; i < n; i++) delete_tree(kids[i]);
    } else {
        disk_free_chain(in->first_block);
    }
    trie_remove(in->name, id);
    table_remove(id);   // also frees the directory's B-tree
}

static void print_chain(int first) {
    if (first == FAT_END) {
        printf("(no blocks)");
        return;
    }
    int chain[NUM_BLOCKS];
    int n = disk_chain(first, chain);
    for (int i = 0; i < n; i++) printf("%d -> ", chain[i]);
    printf("END");
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

void fs_mv(const char *src, const char *dst) {
    int id = resolve(src);
    if (id == -1) {
        error("no such file or directory", src);
        return;
    }
    if (id == ROOT_ID) {
        error("cannot move", "/");
        return;
    }
    Inode *in = table_find(id);

    // work out the new parent directory and the new name
    int new_parent;
    char new_name[LINE_SIZE];
    int dst_id = resolve(dst);
    if (dst_id != -1 && table_find(dst_id)->is_dir) {
        new_parent = dst_id;   // "mv a.txt docs" moves a.txt inside docs
        strcpy(new_name, in->name);
    } else if (dst_id != -1) {
        error("already exists", dst);
        return;
    } else {
        if (!resolve_parent(dst, &new_parent, new_name)) {
            error("no such directory", dst);
            return;
        }
        if (!valid_name(new_name)) {
            error("invalid name", new_name);
            return;
        }
    }

    if (new_parent == in->parent_id && strcmp(new_name, in->name) == 0) return;   // nothing to do

    Inode *target = table_find(new_parent);
    if (btree_find(target->children, new_name) != -1) {
        char where[PATH_SIZE] = "";
        if (new_parent != ROOT_ID) path_of(new_parent, where);
        strcat(where, "/");
        strcat(where, new_name);
        error("already exists", where);
        return;
    }

    // only names move, the disk blocks stay where they are
    btree_remove(table_find(in->parent_id)->children, in->name);
    btree_insert(target->children, new_name, id);
    if (strcmp(new_name, in->name) != 0) {
        trie_remove(in->name, id);
        trie_insert(new_name, id);
        strcpy(in->name, new_name);
    }
    in->parent_id = new_parent;
}

void fs_search(const char *prefix) {
    int ids[MAX_INODES];
    int n = trie_search(prefix, ids);
    char p[PATH_SIZE];
    for (int i = 0; i < n; i++) {
        path_of(ids[i], p);
        printf("%s\n", p);
    }
    if (n == 0) printf("no matches\n");
}

// Breadth-first search with a queue: nearest matches come out first.
void fs_find(const char *dir, const char *name) {
    int start = resolve(dir);
    if (start == -1) {
        error("no such directory", dir);
        return;
    }
    if (!table_find(start)->is_dir) {
        error("not a directory", dir);
        return;
    }

    Queue q;
    queue_init(&q);
    enqueue(&q, start);
    int visited = 0;
    int found = 0;
    char p[PATH_SIZE];
    while (!queue_empty(&q)) {
        int id = dequeue(&q);
        visited++;
        Inode *in = table_find(id);
        if (strcmp(in->name, name) == 0) {
            path_of(id, p);
            printf("%s\n", p);
            found++;
        }
        if (in->is_dir) {
            int kids[MAX_INODES];
            int n = children_of(id, kids);
            for (int i = 0; i < n; i++) enqueue(&q, kids[i]);
        }
    }
    if (found == 0) printf("no matches\n");
    printf("(visited %d inodes)\n", visited);
}

// Depth-first: print each child, and go into it right away if it is a directory.
// One prefix buffer is shared by all levels: add 4 columns before going down,
// cut them off again after.
static void print_tree(int id, char *prefix, int *dirs, int *files) {
    int kids[MAX_INODES];
    int n = children_of(id, kids);
    for (int i = 0; i < n; i++) {
        Inode *c = table_find(kids[i]);
        bool last = (i == n - 1);
        printf("%s%s%s%s\n", prefix, last ? "└── " : "├── ", c->name, c->is_dir ? "/" : "");
        if (c->is_dir) {
            (*dirs)++;
            int len = strlen(prefix);
            strcat(prefix, last ? "    " : "│   ");
            print_tree(kids[i], prefix, dirs, files);
            prefix[len] = '\0';
        } else {
            (*files)++;
        }
    }
}

void fs_tree(const char *path) {
    int id = resolve(path);
    if (id == -1) {
        error("no such directory", path);
        return;
    }
    if (!table_find(id)->is_dir) {
        error("not a directory", path);
        return;
    }
    char p[PATH_SIZE];
    path_of(id, p);
    printf("%s\n", p);

    char prefix[MAX_INODES * 8] = "";   // up to 6 bytes per level ("│" is 3 bytes)
    int dirs = 0;
    int files = 0;
    print_tree(id, prefix, &dirs, &files);
    printf("%d directories, %d files\n", dirs, files);
}

void fs_btree(const char *path) {
    int id = resolve(path);
    if (id == -1) {
        error("no such directory", path);
        return;
    }
    Inode *in = table_find(id);
    if (!in->is_dir) {
        error("not a directory", path);
        return;
    }
    char p[PATH_SIZE];
    path_of(id, p);
    printf("B-tree of %s: %d entries, height %d, max %d keys per node\n",
           p, btree_size(in->children), btree_height(in->children), MAX_KEYS);
    btree_print(in->children);
}

// =====================================================================
// storage commands
// =====================================================================

void fs_write(const char *path, const char *text) {
    int len = strlen(text);
    int needed = blocks_for(len);
    int id = resolve(path);
    if (id == -1) {
        id = create(path, false);
        if (id == -1) return;
    }

    Inode *in = table_find(id);
    if (in->is_dir) {
        error("is a directory", path);
        return;
    }
    // the old blocks get freed first, so they count as available
    if (needed > disk_free_count() + blocks_for(in->size)) {
        error("disk full", path);
        return;
    }

    disk_free_chain(in->first_block);
    in->first_block = disk_allocate(needed);
    disk_write_bytes(in->first_block, 0, text, len);
    in->size = len;
}

// Adds text as a new line at the end of the file.
void fs_append(const char *path, const char *text) {
    int id = resolve(path);
    if (id == -1) {
        fs_write(path, text);   // new file: same as write
        return;
    }
    Inode *in = table_find(id);
    if (in->is_dir) {
        error("is a directory", path);
        return;
    }

    // read the old contents, add the new line, write it all back
    char content[DISK_BYTES + LINE_SIZE + 2];
    disk_read_bytes(in->first_block, in->size, content);
    if (in->size > 0) strcat(content, "\n");
    strcat(content, text);
    int len = strlen(content);

    int needed = blocks_for(len);
    if (needed > disk_free_count() + blocks_for(in->size)) {
        error("disk full", path);
        return;
    }
    disk_free_chain(in->first_block);
    in->first_block = disk_allocate(needed);
    disk_write_bytes(in->first_block, 0, content, len);
    in->size = len;
}

void fs_cat(const char *path) {
    int id = resolve(path);
    if (id == -1) {
        error("no such file", path);
        return;
    }
    Inode *in = table_find(id);
    if (in->is_dir) {
        error("is a directory", path);
        return;
    }
    if (in->size > 0) {
        char content[DISK_BYTES + 1];
        disk_read_bytes(in->first_block, in->size, content);
        printf("%s\n", content);
    }
}

void fs_stat(const char *path) {
    int id = resolve(path);
    if (id == -1) {
        error("no such file or directory", path);
        return;
    }
    Inode *in = table_find(id);
    char p[PATH_SIZE];

    path_of(id, p);
    printf("path:    %s\n", p);
    printf("type:    %s\n", in->is_dir ? "directory" : "file");
    printf("inode:   %d (hash bucket %d)\n", id, table_bucket_of(id));
    path_of(in->parent_id, p);
    printf("parent:  %d (%s)\n", in->parent_id, p);
    if (in->is_dir) {
        printf("entries: %d (B-tree height %d)\n", btree_size(in->children), btree_height(in->children));
    } else {
        printf("size:    %d bytes, %d blocks\n", in->size, blocks_for(in->size));
        printf("blocks:  ");
        print_chain(in->first_block);
        printf("\n");
    }
}

void fs_df(void) {
    int used = NUM_BLOCKS - disk_free_count();
    int stored = 0;
    int ids[MAX_INODES];
    int n = table_all_ids(ids);
    for (int i = 0; i < n; i++) {
        Inode *in = table_find(ids[i]);
        if (!in->is_dir) stored += in->size;
    }
    printf("blocks: %d used, %d free, %d total (%d bytes each)\n",
           used, disk_free_count(), NUM_BLOCKS, BLOCK_SIZE);
    printf("bytes:  %d stored in %d bytes of blocks (%d unused in last blocks)\n",
           stored, used * BLOCK_SIZE, used * BLOCK_SIZE - stored);
    printf("inodes: %d used, %d free, %d total\n",
           table_size(), MAX_INODES - table_size(), MAX_INODES);
}

// Files in tree order (DFS, sorted by name). Returns the new count.
static int collect_files(int id, int out[], int n) {
    Inode *in = table_find(id);
    if (!in->is_dir) {
        out[n] = id;
        return n + 1;
    }
    int kids[MAX_INODES];
    int k = children_of(id, kids);
    for (int i = 0; i < k; i++) n = collect_files(kids[i], out, n);
    return n;
}

#define LETTERS "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"
#define NUM_LETTERS 62

// Each file with data gets a letter. One row = 8 blocks = one byte of the bitmap.
void fs_diskmap(void) {
    char owner[NUM_BLOCKS];
    for (int b = 0; b < NUM_BLOCKS; b++) owner[b] = '.';

    int files[MAX_INODES];
    int num_files = collect_files(ROOT_ID, files, 0);

    int shown[MAX_INODES];   // files that own blocks, in letter order
    int num_shown = 0;
    for (int i = 0; i < num_files; i++) {
        Inode *in = table_find(files[i]);
        if (in->first_block == FAT_END) continue;
        char letter = num_shown < NUM_LETTERS ? LETTERS[num_shown] : '*';
        shown[num_shown] = files[i];
        num_shown++;
        int chain[NUM_BLOCKS];
        int n = disk_chain(in->first_block, chain);
        for (int j = 0; j < n; j++) owner[chain[j]] = letter;
    }

    printf("disk: %d/%d blocks used ('.' = free)\n", NUM_BLOCKS - disk_free_count(), NUM_BLOCKS);
    printf("           blocks    bitmap\n");
    for (int row = 0; row < NUM_BLOCKS; row += 8) {
        printf("  %2d-%2d:  ", row, row + 7);
        for (int b = row; b < row + 8; b++) printf("%c", owner[b]);
        printf("  ");
        for (int b = row; b < row + 8; b++) printf("%c", disk_is_used(b) ? '1' : '0');
        printf("\n");
    }
    char p[PATH_SIZE];
    for (int i = 0; i < num_shown; i++) {
        path_of(shown[i], p);
        printf("  %c = %s\n", i < NUM_LETTERS ? LETTERS[i] : '*', p);
    }
}

void fs_blocks(const char *path) {
    int id = resolve(path);
    if (id == -1) {
        error("no such file", path);
        return;
    }
    Inode *in = table_find(id);
    if (in->is_dir) {
        error("is a directory", path);
        return;
    }
    char p[PATH_SIZE];
    path_of(id, p);
    printf("%s: %d bytes, chain ", p, in->size);
    print_chain(in->first_block);
    printf("\n");

    // show what each block holds (newlines shown as \n)
    char content[DISK_BYTES + 1];
    disk_read_bytes(in->first_block, in->size, content);
    int chain[NUM_BLOCKS];
    int n = disk_chain(in->first_block, chain);
    for (int i = 0; i < n; i++) {
        printf("  block %2d: \"", chain[i]);
        for (int j = i * BLOCK_SIZE; j < (i + 1) * BLOCK_SIZE && j < in->size; j++) {
            if (content[j] == '\n') printf("\\n");
            else printf("%c", content[j]);
        }
        printf("\"\n");
    }
}

void fs_hashmap(void) {
    table_print();
}

// =====================================================================
// rm (both sides)
// =====================================================================

void fs_rm(const char *path, bool recursive) {
    int id = resolve(path);
    if (id == -1) {
        error("no such file or directory", path);
        return;
    }
    if (id == ROOT_ID) {
        error("cannot remove", "/");
        return;
    }
    if (is_ancestor(id, cwd)) {
        error("cannot remove the current directory or its parents", path);
        return;
    }
    Inode *in = table_find(id);
    if (in->is_dir && btree_size(in->children) > 0 && !recursive) {
        error("directory not empty (use rm -r)", path);
        return;
    }
    btree_remove(table_find(in->parent_id)->children, in->name);
    delete_tree(id);
}

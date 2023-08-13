// reference: https://gist.github.com/jserv/4dfaf78cf516cc20f4bc55ce388c195d
/*
 * S-Tree: A self-balancing binary search tree.
 *
 * AVL-trees promise a close-to-optimal tree layout for lookup, but they
 * consume a significant amount of memory and require relatively slow
 * balancing operations. Red-black trees offer quicker manipulation with
 * a slightly less optimal tree layout, and the proposed S-Tree offers
 * fast insertion and deletion by balancing trees during lookup.
 *
 * S-trees rely on four fundamental Binary Search Tree (BST) operations:
 * rotate_left, rotate_right, replace_right, and replace_left. The latter
 * two, replace_right and replace_left, are exclusively employed during node
 * removal, following the conventional BST approach. They identify the
 * next/last node in the right/left subtree, respectively, and perform the
 * substitution of the node scheduled for deletion with the identified node.
 *
 * In contrast, rotate_left and rotate_right are integral to a dedicated update
 * phase aimed at rebalancing the tree. This update phase follows both insert
 * and remove phases in the current implementation. Nonetheless, it is
 * theoretically possible to have arbitrary sequences comprising insert,
 * remove, lookup, and update operations. Notably, the frequency of updates
 * directly influences the extent to which the tree layout approaches
 * optimality. However, it is important to consider that each update operation
 * incurs a certain time penalty.
 *
 * The update function exhibits a relatively straightforward process: When a
 * specific node leans to the right or left beyond a defined threshold, a left
 * or right rotation is performed on the node, respectively. Concurrently, the
 * node's hint is consistently updated. Additionally, if the node's hint becomes
 * zero or experiences a change compared to its previous state during the
 * update, modifications are made to the node's parent, as it existed before
 * these update operations.
 */

/* S-Tree uses hints to decide whether to perform a balancing operation or not.
 * Hints are similar to AVL-trees' height property, but they are not
 * required to be absolutely accurate. A hint provides an approximation
 * of the longest chain of nodes under the node to which the hint is attached.
 */
struct st_node {
    short hint;
    struct st_node *parent;
    struct st_node *left, *right;
};

struct st_root {
    struct st_node *root;
};

enum st_dir { LEFT, RIGHT };

#define st_root(r) (r->root)
#define st_left(n) (n->left)
#define st_right(n) (n->right)
#define st_rparent(n) (st_right(n)->parent)
#define st_lparent(n) (st_left(n)->parent)
#define st_parent(n) (n->parent)

struct st_node *st_first(struct st_node *n)
{
    if (!st_left(n))
        return n;

    return st_first(st_left(n));
}

struct st_node *st_last(struct st_node *n)
{
    if (!st_right(n))
        return n;

    return st_last(st_right(n));
}

static inline void st_rotate_left(struct st_node *n)
{
    struct st_node *l = st_left(n), *p = st_parent(n);

    st_parent(l) = st_parent(n);
    st_left(n) = st_right(l);
    st_parent(n) = l;
    st_right(l) = n;

    if (p && st_left(p) == n)
        st_left(p) = l;
    else if (p)
        st_right(p) = l;

    if (st_left(n))
        st_lparent(n) = n;
}

static inline void st_rotate_right(struct st_node *n)
{
    struct st_node *r = st_right(n), *p = st_parent(n);

    st_parent(r) = st_parent(n);
    st_right(n) = st_left(r);
    st_parent(n) = r;
    st_left(r) = n;

    if (p && st_left(p) == n)
        st_left(p) = r;
    else if (p)
        st_right(p) = r;

    if (st_right(n))
        st_rparent(n) = n;
}

static inline int st_balance(struct st_node *n)
{
    int l = 0, r = 0;

    if (st_left(n))
        l = st_left(n)->hint + 1;

    if (st_right(n))
        r = st_right(n)->hint + 1;

    return l - r;
}

static inline int st_max_hint(struct st_node *n)
{
    int l = 0, r = 0;

    if (st_left(n))
        l = st_left(n)->hint + 1;

    if (st_right(n))
        r = st_right(n)->hint + 1;

    return l > r ? l : r;
}

static inline void st_update(struct st_node **root, struct st_node *n)
{
    if (!n)
        return;

    int b = st_balance(n);
    int prev_hint = n->hint;
    struct st_node *p = st_parent(n);

    if (b < -1) {
        /* leaning to the right */
        if (n == *root)
            *root = st_right(n);
        st_rotate_right(n);
    }

    else if (b > 1) {
        /* leaning to the left */
        if (n == *root)
            *root = st_left(n);
        st_rotate_left(n);
    }

    n->hint = st_max_hint(n);
    if (n->hint == 0 || n->hint != prev_hint)
        st_update(root, p);
}

/* The process of insertion is straightforward and follows the standard approach
 * used in any BST. After inserting a new node into the tree using conventional
 * BST insertion techniques, an update operation is invoked on the newly
 * inserted node.
 */
void st_insert(struct st_node **root,
               struct st_node *p,
               struct st_node *n,
               enum st_dir d)
{
    if (d == LEFT)
        st_left(p) = n;
    else
        st_right(p) = n;

    st_parent(n) = p;
    st_update(root, n);
}

static inline void st_replace_right(struct st_node *n, struct st_node *r)
{
    struct st_node *p = st_parent(n), *rp = st_parent(r);

    if (st_left(rp) == r) {
        st_left(rp) = st_right(r);
        if (st_right(r))
            st_rparent(r) = rp;
    }

    if (st_parent(rp) == n)
        st_parent(rp) = r;

    st_parent(r) = p;
    st_left(r) = st_left(n);

    if (st_right(n) != r) {
        st_right(r) = st_right(n);
        st_rparent(n) = r;
    }

    if (p && st_left(p) == n)
        st_left(p) = r;
    else if (p)
        st_right(p) = r;

    if (st_left(n))
        st_lparent(n) = r;
}

static inline void st_replace_left(struct st_node *n, struct st_node *l)
{
    struct st_node *p = st_parent(n), *lp = st_parent(l);

    if (st_right(lp) == l) {
        st_right(lp) = st_left(l);
        if (st_left(l))
            st_lparent(l) = lp;
    }

    if (st_parent(lp) == n)
        st_parent(lp) = l;

    st_parent(l) = p;
    st_right(l) = st_right(n);

    if (st_left(n) != l) {
        st_left(l) = st_left(n);
        st_lparent(n) = l;
    }

    if (p && st_left(p) == n)
        st_left(p) = l;
    else if (p)
        st_right(p) = l;

    if (st_right(n))
        st_rparent(n) = l;
}

/* The process of deletion in this tree structure is relatively more intricate,
 * although it shares similarities with deletion methods employed in other BST.
 * When removing a node, if the node to be deleted has a right child, the
 * deletion process entails replacing the node to be removed with the first node
 * encountered in the right subtree. Following this replacement, an update
 * operation is invoked on the right child of the newly inserted node.
 *
 * Similarly, if the node to be deleted does not have a right child, the
 * replacement process involves utilizing the first node found in the left
 * subtree. Subsequently, an update operation is called on the left child of th
 * replacement node.
 *
 * In scenarios where the node to be deleted has no children (neither left nor
 * right), it can be directly removed from the tree, and an update operation is
 * invoked on the parent node of the deleted node.
 */
void st_remove(struct st_node **root, struct st_node *del)
{
    if (st_right(del)) {
        struct st_node *least = st_first(st_right(del));
        if (del == *root)
            *root = least;

        // TODO: AAAA;
        st_replace_right(del, least);
        // TODO: BBBB;
        st_update(root, least);
        return;
    }

    if (st_left(del)) {
        struct st_node *most = st_last(st_left(del));
        if (del == *root)
            *root = most;

        // TODO: CCCC;
        st_replace_left(del, most);
        // TODO: DDDD;
        st_update(root, most);
        return;
    }

    if (del == *root) {
        *root = 0;
        return;
    }

    /* empty node */
    struct st_node *parent = st_parent(del);

    if (st_left(parent) == del)
        st_left(parent) = 0;
    else
        st_right(parent) = 0;

    // TODO: EEEE;
    st_update(root, parent);
}

/* Test program */

#include <assert.h>
#include <stddef.h> /* offsetof */
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <time.h>

#define container_of(ptr, type, member) \
    ((type *) ((char *) (ptr) - (offsetof(type, member))))

#define treeint_entry(ptr) container_of(ptr, struct treeint, st_n)

struct treeint {
    int value;
    struct st_node st_n;
};

static struct st_root *tree;

/*
    初始化 tree 內容
*/
int treeint_init()
{
    tree = calloc(sizeof(struct st_root), 1);
    assert(tree);
    return 0;
}

/*
    將 n 的節點依序先左後右，存在時清除節點
*/
static void __treeint_destroy(struct st_node *n)
{
    if (st_left(n))
        __treeint_destroy(st_left(n));

    if (st_right(n))
        __treeint_destroy(st_right(n));

    struct treeint *i = treeint_entry(n);
    free(i);
}

/*
    從 root 開始清除所有節點
*/
int treeint_destroy()
{
    assert(tree);
    if (st_root(tree))
        __treeint_destroy(st_root(tree));

    free(tree);
    return 0;
}

/*
    從 root 開始查詢，有以下 3 種情境：
        1. 當 insert value: a 已經在 tree 中時返回該節點
        2. 當 a < t->value，改找 left 節點
        3. 當 a > t->value，改找 right 節點
    直到沒有下一個節點為止，並且 d 會存儲 a 與 t->value 的關係，當 a < t->value
    時 d 為 LEFT，當 a > t->value 時 d 為 RIGHT;
    接著建立新節點，有以下 2 種情境：
        1. 當目前還沒有任何節點時，將 tree 的 root 指向新節點
        2. 或是將新節點根據 d 值插入到 p (前一個節點) 的 LEFT / RIGHT
    返回新節點
*/
struct treeint *treeint_insert(int a)
{
    struct st_node *p = NULL;
    enum st_dir d;
    for (struct st_node *n = st_root(tree); n;) {
        struct treeint *t = container_of(n, struct treeint, st_n);
        if (a == t->value)
            return t;

        p = n;

        if (a < t->value) {
            n = st_left(n);
            d = LEFT;
        } else if (a > t->value) {
            n = st_right(n);
            d = RIGHT;
        }
    }

    struct treeint *i = calloc(sizeof(struct treeint), 1);
    if (st_root(tree))
        st_insert(&st_root(tree), p, &i->st_n, d);
    else
        st_root(tree) = &i->st_n;

    i->value = a;
    return i;
}

/*
    從 root 開始查詢，有以下 3 種情境：
        1. 當 insert value: a 已經在 tree 中時返回該節點
        2. 當 a < t->value，改找 left 節點
        3. 當 a > t->value，改找 right 節點
    直到沒有下一個節點為止，沒有找到時返回 0;
*/
struct treeint *treeint_find(int a)
{
    struct st_node *n = st_root(tree);
    while (n) {
        struct treeint *t = treeint_entry(n);
        if (a == t->value)
            return t;

        if (a < t->value)
            n = st_left(n);
        else if (a > t->value)
            n = st_right(n);
    }

    return 0;
}

/*
    先透過 a 查詢節點，當節點不存在時返回 -1，存在時從 tree 中移除節點
*/
int treeint_remove(int a)
{
    struct treeint *n = treeint_find(a);
    if (!n)
        return -1;

    st_remove(&st_root(tree), &n->st_n);
    free(n);
    return 0;
}

/* ascending order */
/*
    將 tree 的內容都顯示出來，因為是 ASC
   的方式，因此從最小開始顯示，先顯示左節點再顯示右節點
*/
static void __treeint_dump(struct st_node *n, int depth)
{
    if (!n)
        return;

    // TODO: __treeint_dump(FFFF, depth + 1);
    __treeint_dump(st_left(n), depth + 1);

    struct treeint *v = treeint_entry(n);
    printf("%d\n", v->value);

    // TODO: __treeint_dump(GGGG, depth + 1);
    __treeint_dump(st_right(n), depth + 1);
}

void treeint_dump()
{
    __treeint_dump(st_root(tree), 0);
}

#define interval(title, block)                                         \
    do {                                                               \
        clock_t start_insert = clock();                                \
        block;                                                         \
        clock_t end_insert = clock();                                  \
        printf("%s: %f seconds\n", title,                              \
               (double) (end_insert - start_insert) / CLOCKS_PER_SEC); \
    } while (0);

int main()
{
    struct timeval start, end;
    struct rusage ru;
    srand(time(0));
    struct treeint treelist[2];
    printf("struct size: %ldB\n", sizeof(treelist[0]));
    printf("struct size: %x - %x = %ldB\n", &treelist[1], &treelist[0],
           (long) &treelist[1] - (long) &treelist[0]);

    int node_num = 1000000;

    getrusage(RUSAGE_SELF, &ru);
    printf("%s: %ldK\n", "ru_maxrss", ru.ru_maxrss);
    gettimeofday(&start, NULL);
    treeint_init();

    for (int i = 0; i < node_num; ++i) {
        int v = rand() % node_num;
        interval("Insertion Time", treeint_insert(v));
    }

    printf("[ After insertions ]\n");
    treeint_dump();

    printf("Searching...\n");
    for (int i = 0; i < node_num; ++i) {
        int v = rand() % node_num;
        interval("Searching Time", treeint_find(v);)
    }

    printf("Removing...\n");
    for (int i = 0; i < node_num; ++i) {
        int v = rand() % node_num;
        interval("Remove Time", treeint_remove(v);)
    }

    printf("[ After removals ]\n");
    treeint_dump();

    treeint_destroy();
    gettimeofday(&end, NULL);
    getrusage(RUSAGE_SELF, &ru);
    printf("%s: %ldK\n", "ru_maxrss", ru.ru_maxrss);
    printf("%.3g %.3g %.3g\n",
           (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6,
           ru.ru_utime.tv_sec + ru.ru_utime.tv_usec / 1e6,
           ru.ru_stime.tv_sec + ru.ru_stime.tv_usec / 1e6);

    return 0;
}

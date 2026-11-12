#include "avl.h"
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int hgt(const AVLNode *n) { return n ? n->height : 0; }

static int bal(const AVLNode *n) { return n ? hgt(n->left) - hgt(n->right) : 0; }

static void upd(AVLNode *n)
{
    int l = hgt(n->left), r = hgt(n->right);
    n->height = 1 + (l > r ? l : r);
}

static AVLNode *new_node(AVLTree *t, int key)
{
    AVLNode *n = malloc(sizeof *n);
    if (!n) return NULL;
    n->id = t->next_id++;
    n->key = key;
    n->height = 1;
    n->left = n->right = NULL;
    return n;
}

static void relink(AVLTree *t, AVLNode *parent, AVLNode *old, AVLNode *nw)
{
    if (!parent)                 t->root = nw;
    else if (parent->left == old) parent->left = nw;
    else                          parent->right = nw;
}

static int count_nodes(const AVLNode *n)
{
    return n ? 1 + count_nodes(n->left) + count_nodes(n->right) : 0;
}

typedef struct {
    AVLSnapshot *s;
    int next;
    int order;
} SnapCtx;

static int snap_build(SnapCtx *c, const AVLNode *n, int depth)
{
    if (!n) return -1;

    int idx = c->next++;
    AVLSnapNode *sn = &c->s->nodes[idx];
    sn->id      = n->id;
    sn->key     = n->key;
    sn->height  = n->height;
    sn->balance = bal(n);
    sn->depth   = depth;
    if (depth > c->s->max_depth) c->s->max_depth = depth;

    int l = snap_build(c, n->left, depth + 1);
    c->s->nodes[idx].order = c->order++;
    int r = snap_build(c, n->right, depth + 1);

    c->s->nodes[idx].left  = l;
    c->s->nodes[idx].right = r;
    return idx;
}

void avl_snapshot_take(const AVLTree *t, AVLSnapshot *out)
{
    int n = count_nodes(t->root);
    out->nodes = n ? calloc((size_t)n, sizeof *out->nodes) : NULL;
    out->count = out->nodes ? n : 0;
    out->root = -1;
    out->max_depth = 0;
    if (!out->nodes) return;

    SnapCtx c = { out, 0, 0 };
    out->root = snap_build(&c, t->root, 0);
}

void avl_snapshot_free(AVLSnapshot *s)
{
    free(s->nodes);
    s->nodes = NULL;
    s->count = 0;
    s->root = -1;
    s->max_depth = 0;
}

void avl_trace_init(AVLTrace *tr)
{
    memset(tr, 0, sizeof *tr);
}

void avl_trace_clear(AVLTrace *tr)
{
    for (int i = 0; i < tr->count; i++)
        avl_snapshot_free(&tr->steps[i].snap);
    free(tr->steps);
    memset(tr, 0, sizeof *tr);
}

static void trace_begin(AVLTrace *tr, AVLOp op, int key)
{
    if (!tr) return;
    avl_trace_clear(tr);
    tr->op = op;
    tr->key = key;
}

#if defined(__GNUC__)
__attribute__((format(printf, 6, 7)))
#endif
static void emit(const AVLTree *t, AVLTrace *tr, AVLStepType type,
                 const AVLNode *a, const AVLNode *b, const char *fmt, ...)
{
    if (!tr) return;

    if (tr->count == tr->capacity) {
        int nc = tr->capacity ? tr->capacity * 2 : 32;
        AVLStep *ns = realloc(tr->steps, (size_t)nc * sizeof *ns);
        if (!ns) return;               /* sem memória: para de gravar */
        tr->steps = ns;
        tr->capacity = nc;
    }

    AVLStep *s = &tr->steps[tr->count++];
    s->type = type;
    s->node_id  = a ? a->id : -1;
    s->node_id2 = b ? b->id : -1;

    va_list ap;
    va_start(ap, fmt);
    vsnprintf(s->msg, AVL_MSG_LEN, fmt, ap);
    va_end(ap);

    avl_snapshot_take(t, &s->snap);
}

static bool finish(AVLTree *t, AVLTrace *tr, bool ok, const char *msg)
{
    emit(t, tr, AVL_STEP_DONE, NULL, NULL, "%s", msg);
    if (tr) tr->success = ok;
    return ok;
}

const char *avl_step_type_name(AVLStepType type)
{
    switch (type) {
    case AVL_STEP_START:        return "START";
    case AVL_STEP_COMPARE:      return "COMPARE";
    case AVL_STEP_FOUND:        return "FOUND";
    case AVL_STEP_NOT_FOUND:    return "NOT_FOUND";
    case AVL_STEP_DUPLICATE:    return "DUPLICATE";
    case AVL_STEP_INSERT:       return "INSERT";
    case AVL_STEP_SUCCESSOR:    return "SUCCESSOR";
    case AVL_STEP_COPY_KEY:     return "COPY_KEY";
    case AVL_STEP_REMOVE:       return "REMOVE";
    case AVL_STEP_UNLINK:       return "UNLINK";
    case AVL_STEP_UPDATE:       return "UPDATE";
    case AVL_STEP_UNBALANCED:   return "UNBALANCED";
    case AVL_STEP_CASE:         return "CASE";
    case AVL_STEP_ROTATE_LEFT:  return "ROTATE_LEFT";
    case AVL_STEP_ROTATE_RIGHT: return "ROTATE_RIGHT";
    case AVL_STEP_BALANCED:     return "BALANCED";
    case AVL_STEP_DONE:         return "DONE";
    }
    return "?";
}

static AVLNode *rotate_left(AVLTree *t, AVLTrace *tr, AVLNode *x, AVLNode *parent)
{
    AVLNode *y = x->right;
    x->right = y->left;
    y->left = x;
    upd(x);
    upd(y);
    relink(t, parent, x, y);
    emit(t, tr, AVL_STEP_ROTATE_LEFT, y, x,
         "Rotação à esquerda em %d: %d sobe e %d desce", x->key, y->key, x->key);
    return y;
}

static AVLNode *rotate_right(AVLTree *t, AVLTrace *tr, AVLNode *y, AVLNode *parent)
{
    AVLNode *x = y->left;
    y->left = x->right;
    x->right = y;
    upd(y);
    upd(x);
    relink(t, parent, y, x);
    emit(t, tr, AVL_STEP_ROTATE_RIGHT, x, y,
         "Rotação à direita em %d: %d sobe e %d desce", y->key, x->key, y->key);
    return x;
}

static AVLNode *fix_unbalanced(AVLTree *t, AVLTrace *tr, AVLNode *n, AVLNode *parent)
{
    if (bal(n) > 1) {
        AVLNode *c = n->left;
        if (bal(c) >= 0) {
            emit(t, tr, AVL_STEP_CASE, n, c,
                 "Caso LL (esquerda-esquerda): rotação simples à direita em %d", n->key);
            return rotate_right(t, tr, n, parent);
        }
        /* LR */
        emit(t, tr, AVL_STEP_CASE, n, c,
             "Caso LR (esquerda-direita): rotação à esquerda em %d e depois à direita em %d",
             c->key, n->key);
        rotate_left(t, tr, c, n);
        return rotate_right(t, tr, n, parent);
    } else {
        AVLNode *c = n->right;
        if (bal(c) <= 0) {
            emit(t, tr, AVL_STEP_CASE, n, c,
                 "Caso RR (direita-direita): rotação simples à esquerda em %d", n->key);
            return rotate_left(t, tr, n, parent);
        }
        emit(t, tr, AVL_STEP_CASE, n, c,
             "Caso RL (direita-esquerda): rotação à direita em %d e depois à esquerda em %d",
             c->key, n->key);
        rotate_right(t, tr, c, n);
        return rotate_left(t, tr, n, parent);
    }
}

static void rebalance_up(AVLTree *t, AVLTrace *tr, AVLNode **path, int len)
{
    for (int i = len - 1; i >= 0; i--) {
        AVLNode *n = path[i];
        AVLNode *parent = i > 0 ? path[i - 1] : NULL;
        int old_h = n->height;

        upd(n);
        emit(t, tr, AVL_STEP_UPDATE, n, NULL,
             "Atualiza o nó %d: altura = %d, FB = %+d", n->key, n->height, bal(n));

        AVLNode *sub = n;
        if (bal(n) > 1 || bal(n) < -1) {
            emit(t, tr, AVL_STEP_UNBALANCED, n, NULL,
                 "Nó %d desbalanceado (FB = %+d)", n->key, bal(n));
            sub = fix_unbalanced(t, tr, n, parent);
        }

        if (sub->height == old_h) {
            if (i > 0)
                emit(t, tr, AVL_STEP_BALANCED, sub, NULL,
                     "A altura da subárvore em %d não mudou: os ancestrais não são afetados",
                     sub->key);
            break;
        }
    }
}

void avl_init(AVLTree *t)
{
    t->root = NULL;
    t->size = 0;
    t->next_id = 1;
}

static void free_rec(AVLNode *n)
{
    if (!n) return;
    free_rec(n->left);
    free_rec(n->right);
    free(n);
}

void avl_clear(AVLTree *t)
{
    free_rec(t->root);
    avl_init(t);
}

bool avl_search(AVLTree *t, int key, AVLTrace *tr)
{
    trace_begin(tr, AVL_OP_SEARCH, key);
    emit(t, tr, AVL_STEP_START, NULL, NULL, "Buscar %d", key);

    AVLNode *cur = t->root, *last = NULL;
    while (cur) {
        if (key == cur->key) {
            emit(t, tr, AVL_STEP_FOUND, cur, NULL, "Encontrou %d", key);
            return finish(t, tr, true, "Busca concluída: chave encontrada");
        }
        bool left = key < cur->key;
        emit(t, tr, AVL_STEP_COMPARE, cur, NULL, "%d %s %d: vai para a %s",
             key, left ? "<" : ">", cur->key, left ? "esquerda" : "direita");
        last = cur;
        cur = left ? cur->left : cur->right;
    }

    emit(t, tr, AVL_STEP_NOT_FOUND, NULL, last,
         "Chegou a um ponto vazio: %d não está na árvore", key);
    return finish(t, tr, false, "Busca concluída: chave não encontrada");
}

bool avl_insert(AVLTree *t, int key, AVLTrace *tr)
{
    trace_begin(tr, AVL_OP_INSERT, key);
    emit(t, tr, AVL_STEP_START, NULL, NULL, "Inserir %d", key);

    AVLNode *path[AVL_MAX_HEIGHT];
    int len = 0;
    AVLNode *cur = t->root;

    while (cur) {
        path[len++] = cur;
        if (key == cur->key) {
            emit(t, tr, AVL_STEP_DUPLICATE, cur, NULL,
                 "%d já existe na árvore: inserção ignorada", key);
            return finish(t, tr, false, "Inserção cancelada (chave duplicada)");
        }
        bool left = key < cur->key;
        emit(t, tr, AVL_STEP_COMPARE, cur, NULL, "%d %s %d: vai para a %s",
             key, left ? "<" : ">", cur->key, left ? "esquerda" : "direita");
        cur = left ? cur->left : cur->right;
    }

    AVLNode *node = new_node(t, key);
    if (!node) return finish(t, tr, false, "Erro: sem memória");

    AVLNode *parent = len ? path[len - 1] : NULL;
    if (!parent)                  t->root = node;
    else if (key < parent->key)   parent->left = node;
    else                          parent->right = node;
    t->size++;

    emit(t, tr, AVL_STEP_INSERT, node, parent, "Cria o nó %d como %s", key,
         !parent ? "raiz" : (key < parent->key ? "filho esquerdo" : "filho direito"));

    rebalance_up(t, tr, path, len);
    return finish(t, tr, true, "Inserção concluída: árvore balanceada");
}

bool avl_remove(AVLTree *t, int key, AVLTrace *tr)
{
    trace_begin(tr, AVL_OP_REMOVE, key);
    emit(t, tr, AVL_STEP_START, NULL, NULL, "Remover %d", key);

    AVLNode *path[AVL_MAX_HEIGHT];
    int len = 0;
    AVLNode *cur = t->root;

    while (cur && cur->key != key) {
        path[len++] = cur;
        bool left = key < cur->key;
        emit(t, tr, AVL_STEP_COMPARE, cur, NULL, "%d %s %d: vai para a %s",
             key, left ? "<" : ">", cur->key, left ? "esquerda" : "direita");
        cur = left ? cur->left : cur->right;
    }

    if (!cur) {
        emit(t, tr, AVL_STEP_NOT_FOUND, NULL, len ? path[len - 1] : NULL,
             "Chegou a um ponto vazio: %d não está na árvore", key);
        return finish(t, tr, false, "Remoção cancelada (chave não encontrada)");
    }

    emit(t, tr, AVL_STEP_FOUND, cur, NULL, "Encontrou o nó %d", key);

    AVLNode *victim = cur;

    if (cur->left && cur->right) {
        path[len++] = cur;
        AVLNode *s = cur->right;
        emit(t, tr, AVL_STEP_SUCCESSOR, s, cur,
             "Dois filhos: procura o sucessor (menor chave da subárvore direita)");
        while (s->left) {
            path[len++] = s;
            s = s->left;
            emit(t, tr, AVL_STEP_SUCCESSOR, s, cur, "Desce à esquerda: %d", s->key);
        }
        emit(t, tr, AVL_STEP_SUCCESSOR, s, cur, "Sucessor encontrado: %d", s->key);

        int old_key = cur->key;
        cur->key = s->key;
        emit(t, tr, AVL_STEP_COPY_KEY, cur, s,
             "Copia a chave %d do sucessor para o nó que continha %d", s->key, old_key);
        victim = s;
    }

    AVLNode *child  = victim->left ? victim->left : victim->right;
    AVLNode *parent = len ? path[len - 1] : NULL;

    emit(t, tr, AVL_STEP_REMOVE, victim, parent,
         child ? "Remove o nó %d (um filho): o filho assume seu lugar"
               : "Remove o nó %d (folha)", victim->key);

    relink(t, parent, victim, child);
    free(victim);
    t->size--;

    emit(t, tr, AVL_STEP_UNLINK, child, parent, "Nó removido da árvore");

    rebalance_up(t, tr, path, len);
    return finish(t, tr, true, "Remoção concluída: árvore balanceada");
}

int avl_height(const AVLTree *t) { return hgt(t->root); }

static bool validate_rec(const AVLNode *n, long lo, long hi, int *out_h)
{
    if (!n) { *out_h = 0; return true; }
    if (n->key <= lo || n->key >= hi) return false;

    int hl, hr;
    if (!validate_rec(n->left, lo, n->key, &hl)) return false;
    if (!validate_rec(n->right, n->key, hi, &hr)) return false;

    int h = 1 + (hl > hr ? hl : hr);
    if (n->height != h) return false;
    if (hl - hr > 1 || hr - hl > 1) return false;

    *out_h = h;
    return true;
}

bool avl_validate(const AVLTree *t)
{
    int h;
    return validate_rec(t->root, LONG_MIN, LONG_MAX, &h) && count_nodes(t->root) == t->size;
}

static int inorder_rec(const AVLNode *n, int *out, int cap, int pos)
{
    if (!n) return pos;
    pos = inorder_rec(n->left, out, cap, pos);
    if (pos < cap) out[pos] = n->key;
    pos++;
    return inorder_rec(n->right, out, cap, pos);
}

int avl_inorder(const AVLTree *t, int *out, int cap)
{
    return inorder_rec(t->root, out, cap, 0);
}

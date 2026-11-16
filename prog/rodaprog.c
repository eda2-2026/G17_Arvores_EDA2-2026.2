#define _POSIX_C_SOURCE 200809L
#include "avl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define C_RESET  "\033[0m"
#define C_DIM    "\033[2m"
#define C_BOLD   "\033[1m"
#define C_YELLOW "\033[1;33m"
#define C_CYAN   "\033[1;36m"
#define C_GREEN  "\033[32m"
#define C_RED    "\033[31m"

enum { MODE_STEP, MODE_AUTO, MODE_FAST };
static int mode = MODE_STEP;
static int delay_ms = 700;

static void sleep_ms(int ms)
{
    struct timespec ts = { ms / 1000, (ms % 1000) * 1000000L };
    nanosleep(&ts, NULL);
}

static void clear_screen(void) { printf("\033[H\033[2J"); }

static void draw_rec(const AVLSnapshot *s, int idx, int depth, int side,
                     int hl1, int hl2)
{
    if (idx < 0) return;
    const AVLSnapNode *n = &s->nodes[idx];

    draw_rec(s, n->right, depth + 1, +1, hl1, hl2);

    for (int i = 0; i < depth; i++) printf("      ");
    if (depth > 0) printf(C_DIM "%s" C_RESET, side > 0 ? "┌─" : "└─");

    const char *col = (n->id == hl1) ? C_YELLOW : (n->id == hl2) ? C_CYAN : C_BOLD;
    const char *fbc = (n->balance > 1 || n->balance < -1) ? C_RED : C_GREEN;
    printf("%s%d" C_RESET " " C_DIM "(h%d," C_RESET "%s%+d" C_RESET C_DIM ")" C_RESET "\n",
           col, n->key, n->height, fbc, n->balance);

    draw_rec(s, n->left, depth + 1, -1, hl1, hl2);
}

static void draw_snapshot(const AVLSnapshot *s, int hl1, int hl2)
{
    if (s->count == 0) { printf(C_DIM "  (árvore vazia)\n" C_RESET); return; }
    draw_rec(s, s->root, 0, 0, hl1, hl2);
}

static void draw_tree(const AVLTree *t)
{
    AVLSnapshot s = {0};
    avl_snapshot_take(t, &s);
    printf("\n");
    draw_snapshot(&s, -1, -1);
    avl_snapshot_free(&s);
    printf("\n  nós: %d | altura: %d | válida: %s\n\n", t->size, avl_height(t),
           avl_validate(t) ? C_GREEN "sim" C_RESET : C_RED "NÃO" C_RESET);
}

static const char *op_name(AVLOp op)
{
    switch (op) {
    case AVL_OP_SEARCH: return "BUSCA";
    case AVL_OP_INSERT: return "INSERÇÃO";
    case AVL_OP_REMOVE: return "REMOÇÃO";
    }
    return "?";
}

static void play_trace(const AVLTrace *tr)
{
    for (int i = 0; i < tr->count; i++) {
        const AVLStep *st = &tr->steps[i];

        if (mode != MODE_FAST) {
            clear_screen();
            printf(C_BOLD "%s %d" C_RESET "   passo %d/%d\n\n",
                   op_name(tr->op), tr->key, i + 1, tr->count);
            draw_snapshot(&st->snap, st->node_id, st->node_id2);
            printf("\n  [%s] %s\n\n", avl_step_type_name(st->type), st->msg);
            printf(C_DIM "  amarelo = nó principal | ciano = nó secundário | "
                   "(h = altura, FB = fator de balanço)\n" C_RESET);
        } else {
            printf("  [%s] %s\n", avl_step_type_name(st->type), st->msg);
        }

        if (mode == MODE_STEP) {
            printf("\n  Enter = próximo passo, 'a' + Enter = pular animação: ");
            fflush(stdout);
            char buf[16];
            if (!fgets(buf, sizeof buf, stdin)) return;
            if (buf[0] == 'a') mode = MODE_FAST;
        } else if (mode == MODE_AUTO) {
            fflush(stdout);
            sleep_ms(delay_ms);
        }
    }
}

static void help(void)
{
    printf(
        "\nComandos:\n"
        "  i <n> [n2 ...]   inserir chave(s)\n"
        "  r <n> [n2 ...]   remover chave(s)\n"
        "  b <n>            buscar chave\n"
        "  p                mostrar árvore atual\n"
        "  e                mostrar em ordem (in-order)\n"
        "  c                limpar a árvore\n"
        "  rand <k>         inserir k chaves aleatórias\n"
        "  modo <passo|auto|rapido>   como as operações são animadas\n"
        "  vel <ms>         atraso no modo auto (padrão 700)\n"
        "  h                ajuda\n"
        "  q                sair\n\n");
}

static void run_op(AVLTree *t, AVLTrace *tr, char op, int key)
{
    bool ok = op == 'i' ? avl_insert(t, key, tr)
            : op == 'r' ? avl_remove(t, key, tr)
                        : avl_search(t, key, tr);
    play_trace(tr);

    printf("\n  Resultado: %s\n", ok ? C_GREEN "sucesso" C_RESET : C_RED "falhou" C_RESET);
    if (mode != MODE_FAST || op != 'b') draw_tree(t);
}

int main(void)
{
    AVLTree t;
    AVLTrace tr;
    avl_init(&t);
    avl_trace_init(&tr);
    srand((unsigned)time(NULL));

    printf(C_BOLD "Visualizador AVL (terminal)" C_RESET "  — digite 'h' para ajuda\n");
    help();

    char line[256];
    for (;;) {
        printf("avl> ");
        fflush(stdout);
        if (!fgets(line, sizeof line, stdin)) break;

        char cmd[32] = {0};
        char *rest = line;
        int off = 0;
        if (sscanf(line, "%31s%n", cmd, &off) < 1) continue;
        rest += off;

        if (!strcmp(cmd, "q")) break;
        else if (!strcmp(cmd, "h")) help();
        else if (!strcmp(cmd, "p")) draw_tree(&t);
        else if (!strcmp(cmd, "c")) { avl_clear(&t); printf("  árvore limpa\n"); }
        else if (!strcmp(cmd, "e")) {
            int *buf = malloc((size_t)(t.size + 1) * sizeof *buf);
            int n = buf ? avl_inorder(&t, buf, t.size) : 0;
            printf("  in-order:");
            for (int i = 0; i < n; i++) printf(" %d", buf[i]);
            printf("\n");
            free(buf);
        }
        else if (!strcmp(cmd, "modo")) {
            char m[16] = {0};
            sscanf(rest, "%15s", m);
            if (!strcmp(m, "passo"))       mode = MODE_STEP;
            else if (!strcmp(m, "auto"))   mode = MODE_AUTO;
            else if (!strcmp(m, "rapido")) mode = MODE_FAST;
            else { printf("  use: modo passo|auto|rapido\n"); continue; }
            printf("  modo = %s\n", m);
        }
        else if (!strcmp(cmd, "vel")) {
            int v = atoi(rest);
            if (v >= 0) delay_ms = v;
            printf("  atraso = %d ms\n", delay_ms);
        }
        else if (!strcmp(cmd, "rand")) {
            int k = atoi(rest), saved = mode;
            mode = MODE_FAST;
            for (int i = 0; i < k; i++) avl_insert(&t, rand() % 100, NULL);
            mode = saved;
            draw_tree(&t);
        }
        else if (!strcmp(cmd, "i") || !strcmp(cmd, "r") || !strcmp(cmd, "b")) {
            char *p = rest, *end;
            int any = 0;
            for (;;) {
                long v = strtol(p, &end, 10);
                if (end == p) break;
                any = 1;
                run_op(&t, &tr, cmd[0], (int)v);
                p = end;
                if (cmd[0] == 'b') break;
            }
            if (!any) printf("  use: %s <número>\n", cmd);
        }
        else printf("  comando desconhecido (h = ajuda)\n");
    }

    avl_trace_clear(&tr);
    avl_clear(&t);
    return 0;
}

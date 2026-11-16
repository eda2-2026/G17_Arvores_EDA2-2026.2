/*
 * AVL Tree Visualizer (raylib)
 * Single file: includes its own AVL implementation + animated frontend.
 *
 * Build:  make            (or: gcc avl_visualizer.c -o avl_visualizer -lraylib -lm)
 * Run:    ./avl_visualizer
 *
 * Controls:
 *   Type a number in the box, then:
 *     ENTER or [Insert]  insert
 *     DELETE or [Delete] remove
 *     [Search]           animated search path
 */
#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

#define WIN_W 1200
#define WIN_H 760
#define TOP_BAR 70
#define LOG_H 130
#define NODE_R 22
#define MAX_LOG 6
#define MAX_PATH 128

/* ------------------------------ AVL core ------------------------------ */

typedef struct Node {
    int key, height;
    struct Node *l, *r;
    float x, y;    /* current (animated) position */
    float tx, ty;  /* target position */
} Node;

static Node *root = NULL;
static int rotations = 0;

/* ------------------------------ log ------------------------------ */

static char logbuf[MAX_LOG][112];
static Color logcol[MAX_LOG];

static void log_add(Color c, const char *fmt, ...) {
    for (int i = 0; i < MAX_LOG - 1; i++) {
        strcpy(logbuf[i], logbuf[i + 1]);
        logcol[i] = logcol[i + 1];
    }
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(logbuf[MAX_LOG - 1], sizeof(logbuf[0]), fmt, ap);
    va_end(ap);
    logcol[MAX_LOG - 1] = c;
}

/* ------------------------------ AVL ops ------------------------------ */

static int height(Node *n) { return n ? n->height : 0; }
static int maxi(int a, int b) { return a > b ? a : b; }
static void upd(Node *n) { n->height = 1 + maxi(height(n->l), height(n->r)); }
static int balance(Node *n) { return n ? height(n->l) - height(n->r) : 0; }

static Node *new_node(int key) {
    Node *n = calloc(1, sizeof(Node));
    n->key = key;
    n->height = 1;
    n->x = WIN_W / 2.0f;
    n->y = TOP_BAR;
    return n;
}

static Node *rot_right(Node *y) {
    Node *x = y->l;
    y->l = x->r;
    x->r = y;
    upd(y);
    upd(x);
    rotations++;
    return x;
}

static Node *rot_left(Node *x) {
    Node *y = x->r;
    x->r = y->l;
    y->l = x;
    upd(x);
    upd(y);
    rotations++;
    return y;
}

static Node *rebalance(Node *n) {
    upd(n);
    int bf = balance(n);
    if (bf > 1) {
        if (balance(n->l) < 0) {
            log_add(ORANGE, "Node %d: LR case -> rotate left on %d, then right on %d", n->key, n->l->key, n->key);
            n->l = rot_left(n->l);
        } else {
            log_add(ORANGE, "Node %d: LL case -> rotate right", n->key);
        }
        return rot_right(n);
    }
    if (bf < -1) {
        if (balance(n->r) > 0) {
            log_add(ORANGE, "Node %d: RL case -> rotate right on %d, then left on %d", n->key, n->r->key, n->key);
            n->r = rot_right(n->r);
        } else {
            log_add(ORANGE, "Node %d: RR case -> rotate left", n->key);
        }
        return rot_left(n);
    }
    return n;
}

static Node *insert(Node *n, int key, int *added) {
    if (!n) { *added = 1; return new_node(key); }
    if (key < n->key) n->l = insert(n->l, key, added);
    else if (key > n->key) n->r = insert(n->r, key, added);
    else return n;
    return rebalance(n);
}

static Node *min_node(Node *n) { while (n->l) n = n->l; return n; }

static Node *remove_key(Node *n, int key, int *removed) {
    if (!n) return NULL;
    if (key < n->key) n->l = remove_key(n->l, key, removed);
    else if (key > n->key) n->r = remove_key(n->r, key, removed);
    else {
        *removed = 1;
        if (!n->l || !n->r) {
            Node *c = n->l ? n->l : n->r;
            free(n);
            return c;  /* NULL or the only child */
        }
        Node *s = min_node(n->r);
        n->key = s->key;
        int dummy = 0;
        n->r = remove_key(n->r, s->key, &dummy);
    }
    return rebalance(n);
}

static void free_tree(Node *n) {
    if (!n) return;
    free_tree(n->l);
    free_tree(n->r);
    free(n);
}

static int count_nodes(Node *n) { return n ? 1 + count_nodes(n->l) + count_nodes(n->r) : 0; }

/* ------------------------------ layout ------------------------------ */

static float spacing;
static int total_nodes;
static float depth_step;

static void layout(Node *n, int depth, int *idx) {
    if (!n) return;
    layout(n->l, depth + 1, idx);
    n->tx = WIN_W / 2.0f + ((*idx) - (total_nodes - 1) / 2.0f) * spacing;
    n->ty = TOP_BAR + 55 + depth * depth_step;
    (*idx)++;
    layout(n->r, depth + 1, idx);
}

static void relayout(void) {
    total_nodes = count_nodes(root);
    spacing = total_nodes > 1 ? (WIN_W - 120.0f) / (total_nodes - 1) : 0;
    if (spacing > 70) spacing = 70;
    int h = height(root);
    float avail = WIN_H - TOP_BAR - LOG_H - 110.0f;
    depth_step = h > 1 ? avail / (h - 1) : 70;
    if (depth_step > 80) depth_step = 80;
    int idx = 0;
    layout(root, 0, &idx);
}

static void animate(Node *n, float dt) {
    if (!n) return;
    float k = 1.0f - expf(-9.0f * dt);
    n->x += (n->tx - n->x) * k;
    n->y += (n->ty - n->y) * k;
    animate(n->l, dt);
    animate(n->r, dt);
}

/* ------------------------------ search state ------------------------------ */

static Node *path[MAX_PATH];
static int path_len = 0, path_shown = 0, path_found = 0, searching = 0;
static float path_timer = 0;

static void clear_path(void) { path_len = path_shown = path_found = searching = 0; }

static int in_path(Node *n) {
    for (int i = 0; i < path_shown; i++) if (path[i] == n) return 1;
    return 0;
}

static void start_search(int key) {
    clear_path();
    Node *c = root;
    while (c && path_len < MAX_PATH) {
        path[path_len++] = c;
        if (key == c->key) { path_found = 1; break; }
        c = key < c->key ? c->l : c->r;
    }
    if (path_len == 0) { log_add(RED, "Search %d: tree is empty", key); return; }
    searching = 1;
    path_timer = 0;
    path_shown = 1;
    log_add(SKYBLUE, "Searching for %d...", key);
}

/* ------------------------------ drawing ------------------------------ */

static void draw_edges(Node *n) {
    if (!n) return;
    if (n->l) { DrawLineEx((Vector2){n->x, n->y}, (Vector2){n->l->x, n->l->y}, 2, GRAY); draw_edges(n->l); }
    if (n->r) { DrawLineEx((Vector2){n->x, n->y}, (Vector2){n->r->x, n->r->y}, 2, GRAY); draw_edges(n->r); }
}

static void draw_nodes(Node *n) {
    if (!n) return;
    draw_nodes(n->l);
    draw_nodes(n->r);

    Color fill = (Color){52, 96, 168, 255};
    int bf = balance(n);
    if (bf == 1 || bf == -1) fill = (Color){92, 74, 168, 255};

    int hit = in_path(n);
    if (hit) fill = (Color){220, 140, 30, 255};
    if (path_found && path_shown == path_len && n == path[path_len - 1]) fill = (Color){40, 170, 90, 255};

    DrawCircleV((Vector2){n->x, n->y}, NODE_R + 2, RAYWHITE);
    DrawCircleV((Vector2){n->x, n->y}, NODE_R, fill);

    char buf[16];
    snprintf(buf, sizeof(buf), "%d", n->key);
    int fs = 20;
    int w = MeasureText(buf, fs);
    DrawText(buf, (int)(n->x - w / 2), (int)(n->y - fs / 2), fs, WHITE);

    snprintf(buf, sizeof(buf), "bf %+d", bf);
    int w2 = MeasureText(buf, 12);
    DrawText(buf, (int)(n->x - w2 / 2), (int)(n->y + NODE_R + 4), 12, LIGHTGRAY);

    snprintf(buf, sizeof(buf), "h%d", n->height);
    DrawText(buf, (int)(n->x + NODE_R - 2), (int)(n->y - NODE_R - 10), 12, GRAY);
}

static int button(Rectangle r, const char *label, Color base) {
    Vector2 m = GetMousePosition();
    int hover = CheckCollisionPointRec(m, r);
    int down = hover && IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    Color c = base;
    if (hover) c = ColorBrightness(base, 0.2f);
    if (down) c = ColorBrightness(base, -0.2f);
    DrawRectangleRounded(r, 0.3f, 6, c);
    int fs = 18;
    int w = MeasureText(label, fs);
    DrawText(label, (int)(r.x + (r.width - w) / 2), (int)(r.y + (r.height - fs) / 2), fs, WHITE);
    return hover && IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
}

/* ------------------------------ actions ------------------------------ */

static void do_insert(int key) {
    clear_path();
    int added = 0, before = rotations;
    root = insert(root, key, &added);
    if (!added) { log_add(RED, "Insert %d: key already exists", key); return; }
    relayout();
    log_add(GREEN, "Inserted %d (%d rotation%s)", key, rotations - before, rotations - before == 1 ? "" : "s");
}

static void do_remove(int key) {
    clear_path();
    int removed = 0, before = rotations;
    root = remove_key(root, key, &removed);
    if (!removed) { log_add(RED, "Delete %d: key not found", key); return; }
    relayout();
    log_add(GREEN, "Deleted %d (%d rotation%s)", key, rotations - before, rotations - before == 1 ? "" : "s");
}

/* ------------------------------ main ------------------------------ */

int main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(WIN_W, WIN_H, "AVL Tree Visualizer");
    SetTargetFPS(60);

    char input[8] = "";
    int input_len = 0;
    log_add(LIGHTGRAY, "Type a number and press ENTER to insert it.");

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        /* text input */
        int ch;
        while ((ch = GetCharPressed()) > 0) {
            int ok = (ch >= '0' && ch <= '9') || (ch == '-' && input_len == 0);
            if (ok && input_len < 5) { input[input_len++] = (char)ch; input[input_len] = '\0'; }
        }
        if ((IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) && input_len > 0)
            input[--input_len] = '\0';

        int have_key = input_len > 0 && strcmp(input, "-") != 0;
        int key = have_key ? atoi(input) : 0;

        if (IsKeyPressed(KEY_ENTER) && have_key) { do_insert(key); input[0] = '\0'; input_len = 0; }
        if (IsKeyPressed(KEY_DELETE) && have_key) { do_remove(key); input[0] = '\0'; input_len = 0; }

        /* search animation */
        if (searching) {
            path_timer += dt;
            int shown = (int)(path_timer / 0.5f) + 1;
            if (shown > path_len) shown = path_len;
            path_shown = shown;
            if (path_shown == path_len) {
                searching = 0;
                if (path_found) log_add(GREEN, "Found %d after visiting %d node%s", path[path_len - 1]->key, path_len, path_len == 1 ? "" : "s");
                else log_add(RED, "Not found (visited %d node%s)", path_len, path_len == 1 ? "" : "s");
            }
        }

        animate(root, dt);

        /* ---------- draw ---------- */
        BeginDrawing();
        ClearBackground((Color){24, 26, 33, 255});

        DrawRectangle(0, 0, WIN_W, TOP_BAR, (Color){34, 37, 48, 255});
        DrawText("AVL Tree", 20, 12, 26, RAYWHITE);

        Rectangle box = {250, 16, 120, 38};
        DrawRectangleRounded(box, 0.2f, 6, (Color){18, 19, 25, 255});
        DrawRectangleRoundedLines(box, 0.2f, 6, (Color){90, 100, 130, 255});
        const char *shown_txt = input_len ? input : "key";
        DrawText(shown_txt, (int)box.x + 12, (int)box.y + 9, 20, input_len ? WHITE : GRAY);
        if (((int)(GetTime() * 2)) % 2 == 0) {
            int w = input_len ? MeasureText(input, 20) : 0;
            DrawRectangle((int)box.x + 14 + w, (int)box.y + 8, 2, 22, RAYWHITE);
        }

        float bx = 390;
        if (button((Rectangle){bx, 16, 90, 38}, "Insert", (Color){40, 130, 80, 255}) && have_key) {
            do_insert(key); input[0] = '\0'; input_len = 0;
        }
        bx += 100;
        if (button((Rectangle){bx, 16, 90, 38}, "Delete", (Color){170, 60, 60, 255}) && have_key) {
            do_remove(key); input[0] = '\0'; input_len = 0;
        }
        bx += 100;
        if (button((Rectangle){bx, 16, 90, 38}, "Search", (Color){50, 100, 170, 255}) && have_key) {
            start_search(key);
        }

        if (!root) {
            const char *msg = "Empty tree";
            DrawText(msg, WIN_W / 2 - MeasureText(msg, 28) / 2, WIN_H / 2 - 40, 28, GRAY);
        }
        draw_edges(root);
        draw_nodes(root);

        /* legend */
        DrawCircle(WIN_W - 250, TOP_BAR + 24, 7, (Color){52, 96, 168, 255});
        DrawText("balanced", WIN_W - 238, TOP_BAR + 17, 14, LIGHTGRAY);
        DrawCircle(WIN_W - 160, TOP_BAR + 24, 7, (Color){92, 74, 168, 255});
        DrawText("bf = +/-1", WIN_W - 148, TOP_BAR + 17, 14, LIGHTGRAY);

        /* log panel */
        DrawRectangle(0, WIN_H - LOG_H, WIN_W, LOG_H, (Color){34, 37, 48, 255});
        for (int i = 0; i < MAX_LOG; i++) {
            if (logbuf[i][0]) DrawText(logbuf[i], 20, WIN_H - LOG_H + 10 + i * 20, 16, logcol[i]);
        }

        EndDrawing();
    }

    free_tree(root);
    CloseWindow();
    return 0;
}

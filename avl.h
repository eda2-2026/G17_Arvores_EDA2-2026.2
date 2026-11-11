/*

Ideia central:

Cada operação (inserir, remover, buscar) pode gravar um "trace":
uma lista de passos (AVLStep). Cada passo tem uma mensagem explicativa,
os nós destacados e uma FOTO (snapshot) da árvore naquele instante.
A interface gráfica só precisa tocar essa lista de passos, sem conhecer
nada da lógica da AVL. Cada nó tem um 'id' único e estável: use-o para animar (interpolar a
posição de um mesmo nó entre dois snapshots consecutivos).

Convenções:
- altura de folha = 1, altura de NULL = 0
- fator de balanceamento (FB) = altura(esq) - altura(dir)
- chaves duplicadas são rejeitadas

*/

#ifndef AVL_H
#define AVL_H

#include <stdbool.h>

#define AVL_MAX_HEIGHT 64
#define AVL_MSG_LEN    128

typedef struct AVLNode {
    int id;
    int key;
    int height;
    struct AVLNode *left;
    struct AVLNode *right;
} AVLNode;

typedef struct {
    AVLNode *root;
    int size;
    int next_id;
} AVLTree;

typedef struct {
    int id;
    int key;
    int height;
    int balance;
    int left;
    int right;
    int depth;
    int order;
} AVLSnapNode;

typedef struct {
    AVLSnapNode *nodes;
    int count;
    int root;
    int max_depth;
} AVLSnapshot;

typedef enum {
    AVL_STEP_START,
    AVL_STEP_COMPARE,
    AVL_STEP_FOUND,
    AVL_STEP_NOT_FOUND,
    AVL_STEP_DUPLICATE,
    AVL_STEP_INSERT,
    AVL_STEP_SUCCESSOR,
    AVL_STEP_COPY_KEY,
    AVL_STEP_REMOVE,
    AVL_STEP_UNLINK,
    AVL_STEP_UPDATE,
    AVL_STEP_UNBALANCED,
    AVL_STEP_CASE,
    AVL_STEP_ROTATE_LEFT,
    AVL_STEP_ROTATE_RIGHT,
    AVL_STEP_BALANCED,
    AVL_STEP_DONE
} AVLStepType;

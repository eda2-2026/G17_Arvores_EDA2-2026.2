# G17_Arvores_EDA2-2026.2

Visualizador animado de árvore AVL em C, feito com [raylib](https://www.raylib.com/).
Permite **inserir**, **remover** e **buscar** chaves e acompanhar a árvore na tela.

## Integrantes

- João Gabriel Rolim Veiga (242015432)
- Christian Silva Rolim (242004573)

## Estrutura do repositório
 
| Arquivo | Descrição |
| --- | --- |
| `avl_visualizer.c` | Programa principal. Arquivo único, com a implementação da AVL embutida e a interface gráfica em raylib. |
| `avl.c` / `avl.h` | Backend da AVL em C puro, que grava um _trace_ passo a passo de cada operação (com snapshots da árvore). **Não é usado pela build atual**: o visualizador tem sua própria AVL. |
| `Makefile` | Compila o `avl_visualizer.c`. |

## Controles

- Digite um número na caixa de texto e use:
  - 'ENTER' ou botão 'Insert': inserir
  - 'DELETE' ou botão 'Delete': remover
  - botão 'Search': busca animada

## Requisitos

- Compilador C (preferivelmente o `gcc`)
- `make`
- [raylib](https://github.com/raysan5/raylib)
- Ambiente com janela gráfica (no WSL, o WSLg do Windows 11)

## Como compilar e executar

### Linux / WSL (Ubuntu)
 
**1. Instale as dependências:**
 
```bash
sudo apt update
sudo apt install -y build-essential git pkg-config unzip \
  libasound2-dev libx11-dev libxrandr-dev libxi-dev \
  libgl1-mesa-dev libglu1-mesa-dev libxcursor-dev libxinerama-dev \
  libwayland-dev libxkbcommon-dev
```
 
**2. Instale o raylib.** Em algumas versões do Ubuntu o pacote `libraylib-dev` não existe. Se `sudo apt install libraylib-dev` funcionar, pule este passo. Caso contrário, compile do fonte:
 
```bash
cd ~
git clone --depth 1 https://github.com/raysan5/raylib.git
cd raylib/src
make PLATFORM=PLATFORM_DESKTOP
sudo make install
sudo ldconfig
```
 
**3. Compile e rode o projeto** (dentro da pasta do repositório):
 
```bash
make LDLIBS="-lraylib -lGL -lm -lpthread -ldl -lrt -lX11"
./avl_visualizer
```
 
> O `LDLIBS` é necessário quando o raylib foi instalado como biblioteca estática (caso do passo 2). Se foi instalado pelo `apt`, um simples `make` costuma bastar. Depois de compilar, `make run` também executa o programa.
 
**Usando WSL:** no Windows 11 as janelas gráficas funcionam pelo WSLg. Se necessário, rode `wsl --update` no PowerShell e reabra o Ubuntu. Confira com `echo $DISPLAY` (deve aparecer algo como `:0`). Evite compilar dentro de `/mnt/c/...`; copie o projeto para a pasta do Linux (`~`).

### Windows (sem WSL) 
 
1. Baixe o arquivo `avl_visualizer.exe`, que está na raiz deste repositório
   (no GitHub: clique no arquivo e depois no botão de download).
2. Dê dois cliques nele para abrir.

> Na primeira execução, o Windows pode mostrar o aviso "O Windows protegeu seu
> computador" (SmartScreen), porque o executável não é assinado. Clique em
> **Mais informações** e depois em **Executar assim mesmo**.

## Problemas Comuns
 
| Erro | Solução |
| --- | --- |
| `Unable to locate package libraylib-dev` | Compile o raylib do fonte (passo 2 acima). |
| `undefined reference to 'XOpenDisplay'`, `XSync`, `XInternAtom`... | Compile com `LDLIBS="-lraylib -lGL -lm -lpthread -ldl -lrt -lX11"`. |
| `raylib.h: No such file or directory` | O raylib não está instalado; refaça o passo 2. |
| `error while loading shared libraries: libraylib.so` | Rode `sudo ldconfig`. |
| `cannot open display` / a janela não abre (WSL) | Atualize o WSL (`wsl --update`) e verifique `echo $DISPLAY`. |
| Janela preta ou erro de OpenGL (WSL) | Tente `LIBGL_ALWAYS_SOFTWARE=1 ./avl_visualizer`. |
| `make` diz que já está atualizado | Rode `make clean` e compile de novo. |
 
## Limpar
 
```bash
make clean
```

## Screenshots do Funcionamento do Projeto

### 1. Inserção

*   <img src="https://github.com/user-attachments/assets/e09d78a2-dc99-41fd-bb16-d41779b7945a" width="500">
*   <img src="https://github.com/user-attachments/assets/b3cf2cd8-2f6e-43e6-a096-974192d68576" width="500">

### 2. Remoção

*   <img src="https://github.com/user-attachments/assets/0b5ad1c4-5cbe-4a7f-9d5b-134a5ae770b8" width="500">
*   <img src="https://github.com/user-attachments/assets/b2ba198e-1127-445b-93b4-a142e62e3172" width="500">

### 3. Busca

*   <img src="https://github.com/user-attachments/assets/cbf621e5-5acf-40d9-86ea-03b033907237" width="500">
*   <img src="https://github.com/user-attachments/assets/a02a6a0f-c283-4fa6-a727-77b4cb82e60b" width="500">

## Link do Vídeo da nosso projeto no YouTube

* 


















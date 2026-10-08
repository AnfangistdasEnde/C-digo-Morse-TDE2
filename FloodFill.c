#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

typedef struct No {
    int x, y;
    struct No *proximo;
} No;

typedef struct {
    No *topo;
} Pilha;

typedef struct {
    No *inicio, *fim;
} Fila;

typedef struct {
    unsigned char *dados;
    size_t tamanho, inicio, passo;
    int largura, altura, invertida;
} Imagem;

static int empilhar(Pilha *p, int x, int y) {
    No *n = malloc(sizeof *n);
    if (!n) return 0;

    n->x = x;
    n->y = y;
    n->proximo = p->topo;
    p->topo = n;
    return 1;
}

static int desempilhar(Pilha *p, int *x, int *y) {
    No *n = p->topo;
    if (!n) return 0;

    *x = n->x;
    *y = n->y;
    p->topo = n->proximo;
    free(n);
    return 1;
}

static int enfileirar(Fila *f, int x, int y) {
    No *n = malloc(sizeof *n);
    if (!n) return 0;

    n->x = x;
    n->y = y;
    n->proximo = NULL;

    if (f->fim) f->fim->proximo = n;
    else f->inicio = n;

    f->fim = n;
    return 1;
}

static int desenfileirar(Fila *f, int *x, int *y) {
    No *n = f->inicio;
    if (!n) return 0;

    *x = n->x;
    *y = n->y;
    f->inicio = n->proximo;

    if (!f->inicio) f->fim = NULL;

    free(n);
    return 1;
}

static uint32_t ler32(const unsigned char *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static int ler_linha(char *texto, int tamanho) {
    int c;

    if (!fgets(texto, tamanho, stdin)) exit(0);

    if (!strchr(texto, '\n') && !feof(stdin)) {
        while ((c = getchar()) != '\n' && c != EOF) {}
        puts("Entrada muito longa.");
        return 0;
    }

    texto[strcspn(texto, "\r\n")] = '\0';
    return 1;
}

static int numero(const char *mensagem, int min, int max) {
    char texto[100], *fim;
    long valor;

    for (;;) {
        printf("%s", mensagem);
        if (!ler_linha(texto, sizeof texto)) continue;

        errno = 0;
        valor = strtol(texto, &fim, 10);

        if (fim == texto) {
            puts("Digite um numero inteiro.");
            continue;
        }

        while (*fim == ' ' || *fim == '\t') fim++;

        if (!errno && !*fim && valor >= min && valor <= max)
            return (int)valor;

        printf("Digite um inteiro entre %d e %d.\n", min, max);
    }
}

static int abrir(const char *nome, Imagem *img) {
    FILE *f = fopen(nome, "rb");
    Imagem nova = {0};
    long tamanho;
    int32_t altura;

    if (!f) {
        puts("Nao foi possivel abrir o arquivo.");
        return 0;
    }

    if (fseek(f, 0, SEEK_END) != 0 ||
        (tamanho = ftell(f)) < 54 ||
        fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        puts("Arquivo invalido.");
        return 0;
    }

    nova.tamanho = (size_t)tamanho;
    nova.dados = malloc(nova.tamanho);

    if (!nova.dados) {
        fclose(f);
        puts("Memoria insuficiente.");
        return 0;
    }

    if (fread(nova.dados, 1, nova.tamanho, f) != nova.tamanho) {
        fclose(f);
        free(nova.dados);
        puts("Erro de leitura.");
        return 0;
    }

    fclose(f);

    nova.inicio = ler32(nova.dados + 10);
    nova.largura = (int32_t)ler32(nova.dados + 18);
    altura = (int32_t)ler32(nova.dados + 22);

    if (nova.dados[0] != 'B' || nova.dados[1] != 'M' ||
        ler32(nova.dados + 14) != 40 ||
        nova.dados[26] != 1 || nova.dados[27] != 0 ||
        nova.dados[28] != 24 || nova.dados[29] != 0 ||
        ler32(nova.dados + 30) != 0 ||
        nova.inicio < 54 || nova.inicio > nova.tamanho ||
        nova.largura <= 0 || nova.largura == INT_MAX ||
        altura == 0 || altura == INT32_MIN ||
        altura == INT32_MAX) {
        free(nova.dados);
        puts("Use BMP de 24 bits, sem compressao, com cabecalho de 40 bytes.");
        return 0;
    }

    nova.invertida = altura > 0;
    nova.altura = altura < 0 ? -altura : altura;

    if ((size_t)nova.largura > (SIZE_MAX - 3) / 3) {
        free(nova.dados);
        puts("Imagem muito grande.");
        return 0;
    }

    nova.passo = ((size_t)nova.largura * 3 + 3) & ~(size_t)3;

    if ((size_t)nova.altura >
        (nova.tamanho - nova.inicio) / nova.passo) {
        free(nova.dados);
        puts("Imagem incompleta.");
        return 0;
    }

    free(img->dados);
    *img = nova;
    printf("Imagem: %d x %d pixels.\n", img->largura, img->altura);
    return 1;
}

static unsigned char *pixel(Imagem *img, int x, int y) {
    int linha = img->invertida ? img->altura - 1 - y : y;

    return img->dados + img->inicio +
           (size_t)linha * img->passo + (size_t)x * 3;
}

static int salvar(Imagem *img, const char *nome) {
    FILE *f = fopen(nome, "wb");
    int ok;

    if (!f) {
        puts("Erro ao criar arquivo.");
        return 0;
    }

    ok = fwrite(img->dados, 1, img->tamanho, f) == img->tamanho;
    if (fclose(f) != 0) ok = 0;
    if (!ok) puts("Erro ao salvar imagem.");

    return ok;
}

static void preencher(Imagem *original, int x, int y,
                      int modo, unsigned rodada) {
    Imagem img = *original;
    Pilha p = {0};
    Fila f = {0};
    unsigned char antiga[3], nova[3], *atual;
    int dx[4] = {0, 0, -1, 1};
    int dy[4] = {-1, 1, 0, 0};
    int intervalo, ok;
    size_t pintados = 0, etapa = 0;
    char nome[100], prefixo[40];

    img.dados = malloc(img.tamanho);
    if (!img.dados) {
        puts("Memoria insuficiente.");
        return;
    }

    memcpy(img.dados, original->dados, img.tamanho);
    memcpy(antiga, pixel(&img, x, y), 3);

    nova[2] = (unsigned char)numero("Vermelho: ", 0, 255);
    nova[1] = (unsigned char)numero("Verde: ", 0, 255);
    nova[0] = (unsigned char)numero("Azul: ", 0, 255);

    if (memcmp(antiga, nova, 3) == 0) {
        puts("A cor ja e a mesma.");
        free(img.dados);
        return;
    }

    intervalo = numero("Salvar a cada quantos pixels? ", 1, INT_MAX);

    snprintf(prefixo, sizeof prefixo, "%s_%u",
             modo == 1 ? "pilha" : "fila", rodada);
    snprintf(nome, sizeof nome, "%s_original.bmp", prefixo);

    if (!salvar(&img, nome)) {
        free(img.dados);
        return;
    }

    ok = modo == 1 ? empilhar(&p, x, y) : enfileirar(&f, x, y);

    while (ok && (modo == 1 ? desempilhar(&p, &x, &y)
                            : desenfileirar(&f, &x, &y))) {
        if (x < 0 || y < 0 || x >= img.largura || y >= img.altura)
            continue;

        atual = pixel(&img, x, y);
        if (memcmp(atual, antiga, 3) != 0) continue;

        memcpy(atual, nova, 3);
        pintados++;

        if (pintados == 1 || pintados % (size_t)intervalo == 0) {
            snprintf(nome, sizeof nome, "%s_passo_%04zu.bmp",
                     prefixo, ++etapa);
            if (!salvar(&img, nome)) {
                ok = 0;
                break;
            }
        }

        for (int i = 0; i < 4; i++) {
            ok = modo == 1
                ? empilhar(&p, x + dx[i], y + dy[i])
                : enfileirar(&f, x + dx[i], y + dy[i]);

            if (!ok) break;
        }
    }

    while (desempilhar(&p, &x, &y)) {}
    while (desenfileirar(&f, &x, &y)) {}

    if (ok) {
        snprintf(nome, sizeof nome, "%s_final.bmp", prefixo);
        if (salvar(&img, nome))
            printf("Concluido: %zu pixels. Arquivo: %s\n", pintados, nome);
    } else {
        puts("Execucao interrompida por erro de memoria ou gravacao.");
    }

    free(img.dados);
}

int main(void) {
    Imagem img = {0};
    char caminho[1024];
    int opcao, x = 0, y = 0, coordenada = 0;
    unsigned rodada = 0;

    do {
        puts("\n1 - Executar com pilha");
        puts("2 - Executar com fila");
        puts("3 - Escolher imagem");
        puts("4 - Escolher coordenada");
        puts("0 - Encerrar");

        opcao = numero("Opcao: ", 0, 4);

        switch (opcao) {
            case 3:
                puts("Caminho do BMP, sem aspas:");
                if (ler_linha(caminho, sizeof caminho) &&
                    abrir(caminho, &img))
                    coordenada = 0;
                break;

            case 4:
                if (!img.dados) {
                    puts("Escolha a imagem primeiro.");
                    break;
                }

                x = numero("X (coluna, a partir de 0): ", 0, img.largura - 1);
                y = numero("Y (linha, a partir de 0): ", 0, img.altura - 1);
                coordenada = 1;
                break;

            case 1:
            case 2:
                if (!img.dados || !coordenada) {
                    puts("Escolha a imagem e a coordenada primeiro.");
                    break;
                }

                preencher(&img, x, y, opcao, ++rodada);
                break;
        }
    } while (opcao != 0);

    free(img.dados);
    return 0;
}
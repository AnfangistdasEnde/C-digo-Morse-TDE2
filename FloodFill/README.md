cat > README.md <<'EOF'
# Preenchimento de imagens BMP com pilha e fila

## Objetivo

Implementar o algoritmo Flood Fill em C utilizando duas estruturas de dados: pilha e fila.

O programa preenche uma região da imagem a partir de uma coordenada escolhida. Apenas pixels conectados que possuem a mesma cor do pixel inicial são alterados.

## Estruturas utilizadas

- **Pilha:** utiliza a lógica LIFO, em que o último elemento inserido é o primeiro a ser removido. Realiza o preenchimento em profundidade.
- **Fila:** utiliza a lógica FIFO, em que o primeiro elemento inserido é o primeiro a ser removido. Realiza o preenchimento em largura.

As duas estruturas são implementadas com listas encadeadas e alocação dinâmica de memória.

Um vetor de controle impede que o mesmo pixel seja adicionado mais de uma vez.

## Requisitos

- Compilador C com suporte a C11, como GCC.
- Imagem BMP de 24 bits, sem compressão e com cabeçalho DIB de 40 bytes.

## Compilação

No terminal, dentro da pasta do projeto:

```bash
gcc -std=c11 -Wall -Wextra -Wpedantic floodfill.c -o floodfill
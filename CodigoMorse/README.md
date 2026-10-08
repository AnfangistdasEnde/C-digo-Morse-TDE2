Código Morse com Árvore Binária

Programa em C++ que constrói dinamicamente uma árvore binária com as letras (A–Z) e os números (0–9) do Código Morse e a usa para codificar e decodificar textos, digitados ou lidos de arquivos .txt.

Como a árvore funciona

A partir da raiz, cada símbolo da sequência Morse indica um caminho:

ponto (.) → filho esquerdo
traço (-) → filho direito

Compilação e execução   
g++ morse.cpp -o morse
./morse          # Linux/macOS
morse.exe        # Windows


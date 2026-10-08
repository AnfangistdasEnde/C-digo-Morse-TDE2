#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cctype>
using namespace std;

int menu () {
    int escolha;
    cout << "\n1 - Codificar texto digitado." << endl;
    cout << "2 - Decodificar Morse digitado." << endl;
    cout << "3 - Codificar arquivo de texto." << endl;
    cout << "4 - Decodificar arquivo Morse." << endl;
    cout << "5 - Mostrar árvore." << endl;
    cout << "0 - Encerrar." << endl;
    cin >> escolha;
    if (cin.fail()) { // entrada que não é número
        cin.clear();
        cin.ignore(10000, '\n');
        return -1;
    }
    cin.ignore(10000, '\n'); // descarta o resto da linha para o getline funcionar depois
    return escolha;
}

class MorseNode {
public:

    char caracter; //guarda o caractere correspondente ao código Morse
    MorseNode* esquerda; //ponteiro para o nó da esquerda (representa o ponto)
    MorseNode* direita; //ponteiro para o nó da direita (representa o traço)

    MorseNode(char c) : caracter(c), esquerda(nullptr), direita(nullptr) {} // Construtor que inicializa o caractere e os ponteiros para nullptr na lista de nós da árvore de Morse
};

void inserir(MorseNode*& raiz, const string& codigo, char caracter) {
    if (!raiz) {
        raiz = new MorseNode('\0'); // Cria um nó raiz vazio se ainda não existir
    }
    MorseNode* atual = raiz;
    for (char c : codigo) {
        if (c == '.') {
            if (!atual->esquerda) {
                atual->esquerda = new MorseNode('\0');
            }
            atual = atual->esquerda;
        } else if (c == '-') {
            if (!atual->direita) {
                atual->direita = new MorseNode('\0');
            }
            atual = atual->direita;
        }
    }
    atual->caracter = caracter;
}

void buscar(MorseNode* raiz, const string& codigo, char& caracter) {
    MorseNode* atual = raiz;
    for (char c : codigo) {
        if (!atual) {
            caracter = '\0'; // Código Morse inválido
            return;
        }
        if (c == '.') {
            atual = atual->esquerda;
        } else if (c == '-') {
            atual = atual->direita;
        }
    }
    if (atual) {
        caracter = atual->caracter; // Retorna o caractere correspondente ao código Morse
    } else {
        caracter = '\0'; // Código Morse inválido
    }
}

// Percorre a árvore procurando o caractere e monta o caminho (ponto/traço) até ele.
// Retorna true se achou. Usada na codificação (caractere -> Morse).
bool buscarCodigo(MorseNode* no, char alvo, string& caminho) {
    if (!no) 
    return false;
    if (no->caracter == alvo) 
    return true;

    caminho.push_back('.');
    if (buscarCodigo(no->esquerda, alvo, caminho)) return true;
    caminho.pop_back();

    caminho.push_back('-');
    if (buscarCodigo(no->direita, alvo, caminho)) return true;
    caminho.pop_back();

    return false;
}

// Codifica texto comum para Morse. Letras separadas por espaço, palavras por " / ".
// Símbolos que não existem na árvore são ignorados e contados em "invalidos".
string codificarTexto(MorseNode* raiz, const string& texto, int& invalidos) {
    string resultado = "";
    bool espacoPendente = false;
    invalidos = 0;

    for (char c : texto) {
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            espacoPendente = true; // vários espaços seguidos viram uma só barra
            continue;
        }
        char maiuscula = toupper((unsigned char)c); 
        string codigo = "";
        if (maiuscula == '\0' || !buscarCodigo(raiz, maiuscula, codigo) || codigo.empty()) {
            invalidos++; // símbolo sem representação nesta tabela
            continue;
        }
        if (!resultado.empty()) {
            resultado += espacoPendente ? " / " : " ";
        }
        resultado += codigo;
        espacoPendente = false;
    }
    return resultado;
}

// Decodifica Morse para texto. Aceita somente '.', '-', '/' e espaço.
// ok = false se houver qualquer erro (símbolo inválido ou código inexistente).
string decodificarMorse(MorseNode* raiz, const string& entrada, bool& ok) {
    ok = true;

    // 1) valida os símbolos permitidos
    for (char c : entrada) {
        if (c != '.' && c != '-' && c != '/' && c != ' ') {
            cout << "Erro: símbolo inválido na entrada Morse. Use apenas '.', '-', '/' e espaço." << endl;
            ok = false;
            return "";
        }
    }

    // 2) separa em tokens (espaços extras são ignorados)
    string resultado = "";
    istringstream fluxo(entrada);
    string token;
    while (fluxo >> token) {
        if (token == "/") {
            resultado += ' '; // barra volta a ser espaço entre palavras
            continue;
        }
        if (token.find('/') != string::npos) {
            cout << "Erro: a barra deve ficar separada por espaços (token \"" << token << "\")." << endl;
            ok = false;
            resultado += '?';
            continue;
        }
        char c;
        buscar(raiz, token, c);
        if (c == '\0') {
            cout << "Erro: código Morse inexistente \"" << token << "\"." << endl;
            ok = false;
            resultado += '?';
        } else {
            resultado += c;
        }
    }
    return resultado;
}

// Lê arquivos de texto e de Morse.
class FileService {
public:
    // Lê o arquivo de texto inteiro (quebras de linha viram espaços).
    static bool lerTexto(const string& caminho, string& conteudo) {
        ifstream arq(caminho);
        if (!arq.is_open()) return false;
        conteudo = "";
        string linha;
        while (getline(arq, linha)) {
            conteudo += linha + " ";
        }
        return true;
    }

    // Lê o arquivo Morse: precisa ter UMA única linha.
    // Retorna false e preenche "erro" se não abrir ou se tiver mais de uma linha.
    static bool lerMorse(const string& caminho, string& linha, string& erro) {
        ifstream arq(caminho);
        if (!arq.is_open()) {
            erro = "Não foi possível abrir o arquivo.";
            return false;
        }
        getline(arq, linha);
        if (!linha.empty() && linha.back() == '\r') linha.pop_back();

        string extra;
        while (getline(arq, extra)) {
            if (!extra.empty() && extra.back() == '\r') extra.pop_back();
            if (!extra.empty()) {
                erro = "O arquivo Morse deve conter uma única linha.";
                return false;
            }
        }
        return true;
    }
};

// Exibe a árvore "deitada": raiz à esquerda, traço (-) em cima, ponto (.) embaixo.
// '*' indica nó intermediário sem caractere.
void exibirArvore(MorseNode* no, int nivel, const string& rotulo) {
    if (!no) return;
    exibirArvore(no->direita, nivel + 1, "-");

    cout << string(nivel * 7, ' ');
    if (nivel == 0) {
        cout << "RAIZ" << endl;
    } else {
        cout << "(" << rotulo << ")--" << (no->caracter == '\0' ? '*' : no->caracter) << endl;
    }

    exibirArvore(no->esquerda, nivel + 1, ".");
}

void liberar(MorseNode* no) {
    if (!no) return;
    liberar(no->esquerda);
    liberar(no->direita);
    delete no;
}

int main() {
    MorseNode* raiz = nullptr; // Inicializa a raiz da árvore de Morse como nula
    // Inserção de códigos Morse na árvore
    // Letras
    inserir(raiz, ".-", 'A');
    inserir(raiz, "-...", 'B');
    inserir(raiz, "-.-.", 'C');
    inserir(raiz, "-..", 'D');
    inserir(raiz, ".", 'E');
    inserir(raiz, "..-.", 'F');
    inserir(raiz, "--.", 'G');
    inserir(raiz, "....", 'H');
    inserir(raiz, "..", 'I');
    inserir(raiz, ".---", 'J');
    inserir(raiz, "-.-", 'K');
    inserir(raiz, ".-..", 'L');
    inserir(raiz, "--", 'M');
    inserir(raiz, "-.", 'N');
    inserir(raiz, "---", 'O');
    inserir(raiz, ".--.", 'P');
    inserir(raiz, "--.-", 'Q');
    inserir(raiz, ".-.", 'R');
    inserir(raiz, "...", 'S');
    inserir(raiz, "-", 'T');
    inserir(raiz, "..-", 'U');
    inserir(raiz, "...-", 'V');
    inserir(raiz, ".--", 'W');
    inserir(raiz, "-..-", 'X');
    inserir(raiz, "-.--", 'Y');
    inserir(raiz, "--..", 'Z');
    // Números
    inserir(raiz, "-----", '0');
    inserir(raiz, ".----", '1');
    inserir(raiz, "..---", '2');
    inserir(raiz, "...--", '3');
    inserir(raiz, "....-", '4');
    inserir(raiz, ".....", '5');
    inserir(raiz, "-....", '6');
    inserir(raiz, "--...", '7');
    inserir(raiz, "---..", '8');
    inserir(raiz, "----.", '9');

    int opcao;
    do {
        opcao = menu();
        string entrada, caminho, erro, saida;
        int invalidos;
        bool ok;

        switch (opcao) {
        case 1:
            cout << "Digite o texto: ";
            getline(cin, entrada);
            saida = codificarTexto(raiz, entrada, invalidos);
            cout << "Morse: " << saida << endl;
            if (invalidos > 0)
                cout << "Aviso: " << invalidos << " símbolo(s) sem código Morse foram ignorados." << endl;
            break;

        case 2:
            cout << "Digite o Morse (use . - / e espaco): ";
            getline(cin, entrada);
            saida = decodificarMorse(raiz, entrada, ok);
            if (ok || !saida.empty()) cout << "Texto: " << saida << endl;
            break;

        case 3:
            cout << "Caminho do arquivo de texto: ";
            getline(cin, caminho);
            if (!FileService::lerTexto(caminho, entrada)) {
                cout << "Erro: não foi possível abrir o arquivo." << endl;
                break;
            }
            saida = codificarTexto(raiz, entrada, invalidos);
            cout << "Morse: " << saida << endl;
            if (invalidos > 0)
                cout << "Aviso: " << invalidos << " símbolo(s) sem código Morse foram ignorados." << endl;
            break;

        case 4:
            cout << "Caminho do arquivo Morse: ";
            getline(cin, caminho);
            if (!FileService::lerMorse(caminho, entrada, erro)) {
                cout << "Erro: " << erro << endl;
                break;
            }
            saida = decodificarMorse(raiz, entrada, ok);
            if (ok || !saida.empty()) cout << "Texto: " << saida << endl;
            break;

        case 5:
            cout << "\nÁrvore Morse: (.) = esquerda/ponto (embaixo), (-) = direita/traço (em cima), * = sem caractere\n" << endl;
            exibirArvore(raiz, 0, "");
            break;

        case 0:
            cout << "Encerrando." << endl;
            break;

        default:
            cout << "Opção inválida." << endl;
        }
    } while (opcao != 0);

    liberar(raiz);
    return 0;
}
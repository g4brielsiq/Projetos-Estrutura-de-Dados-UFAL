#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <stdbool.h>

// Estrutura para a pilha
typedef struct No {
    double valor;
    struct No* proximo;
} No;

No* topo = NULL;

// Funções básicas da pilha
void InicializarPilha() {
    topo = NULL;
}

int PilhaVazia() {
    return topo == NULL;
}

void Empilhar(double valor) {
    No* novo_no = (No*)malloc(sizeof(No));
    novo_no->valor = valor;
    novo_no->proximo = topo;
    topo = novo_no;
}

double Desempilhar() {
    if (PilhaVazia()) {
        printf("Erro: Pilha vazia\n");
        return 0;
    }
    
    No* temp = topo;
    double valor = temp->valor;
    topo = topo->proximo;
    free(temp);
    return valor;
}

// Função para calcular radical
double CalcularRadical(double indice, double base, double expoente) {
    return pow(pow(base, expoente), 1.0 / indice);
}

// Função para obter precedência do operador
int Precedencia(char operador) {
    switch (operador) {
        case '+':
        case '-':
            return 1;
        case '*':
        case '/':
            return 2;
        default:
            return 0;
    }
}

// Função para aplicar operação
double AplicarOperacao(double a, double b, char operador) {
    switch (operador) {
        case '+': return a + b;
        case '-': return a - b;
        case '*': return a * b;
        case '/': 
            if (b == 0) {
                printf("Erro: Divisão por zero\n");
                return 0;
            }
            return a / b;
        default: return 0;
    }
}

// Função principal para processar a expressão
double ProcessarExpressao(char* expr) {
    double pilha_numeros[100];
    char pilha_operadores[100];
    int topo_numeros = -1;
    int topo_operadores = -1;
    
    int i = 0;
    int len = strlen(expr);
    
    while (i < len) {
        // Ignorar espaços
        if (isspace(expr[i])) {
            i++;
            continue;
        }
        
        // Processar radical: (indice,base,expoente)
        if (expr[i] == '(') {
            i++; // Pular '('
            
            char temp_str[50];
            int pos_temp = 0;
            int contador_virgulas = 0;
            
            double indice = 0, base = 0, expoente = 0;
            
            // Ler indice
            while (expr[i] != ',' && expr[i] != '\0') {
                temp_str[pos_temp++] = expr[i++];
            }
            temp_str[pos_temp] = '\0';
            indice = atof(temp_str);
            
            i++; // Pular ','
            pos_temp = 0;
            contador_virgulas++;
            
            // Ler base
            while (expr[i] != ',' && expr[i] != '\0') {
                temp_str[pos_temp++] = expr[i++];
            }
            temp_str[pos_temp] = '\0';
            base = atof(temp_str);
            
            i++; // Pular ','
            pos_temp = 0;
            contador_virgulas++;
            
            // Ler expoente
            while (expr[i] != ')' && expr[i] != '\0') {
                temp_str[pos_temp++] = expr[i++];
            }
            temp_str[pos_temp] = '\0';
            expoente = atof(temp_str);
            
            i++; // Pular ')'
            
            double resultado_radical = CalcularRadical(indice, base, expoente);
            pilha_numeros[++topo_numeros] = resultado_radical;
        }
        // Processar operador
        else if (expr[i] == '+' || expr[i] == '-' || expr[i] == '*' || expr[i] == '/') {
            while (topo_operadores >= 0 && 
                   Precedencia(pilha_operadores[topo_operadores]) >= Precedencia(expr[i])) {
                // Aplicar operação com precedência maior ou igual
                char op = pilha_operadores[topo_operadores--];
                double b = pilha_numeros[topo_numeros--];
                double a = pilha_numeros[topo_numeros--];
                double resultado = AplicarOperacao(a, b, op);
                pilha_numeros[++topo_numeros] = resultado;
            }
            pilha_operadores[++topo_operadores] = expr[i];
            i++;
        }
        else {
            printf("Erro: Caractere inválido '%c'\n", expr[i]);
            return 0;
        }
    }
    
    // Aplicar operações restantes
    while (topo_operadores >= 0) {
        char op = pilha_operadores[topo_operadores--];
        double b = pilha_numeros[topo_numeros--];
        double a = pilha_numeros[topo_numeros--];
        double resultado = AplicarOperacao(a, b, op);
        pilha_numeros[++topo_numeros] = resultado;
    }
    
    if (topo_numeros != 0) {
        printf("Erro: Expressão mal formada\n");
        return 0;
    }
    
    return pilha_numeros[0];
}

int main() {
    char expressao[1000];
    double resultado;
    
    printf("Digite a expressão matemática:\n");
    printf("Exemplo: (3,2,3)-(3,3,4)/(2,3,2)+(2,2,1)\n");
    
    fgets(expressao, sizeof(expressao), stdin);
    
    // Remover quebra de linha se existir
    expressao[strcspn(expressao, "\n")] = '\0';
    
    resultado = ProcessarExpressao(expressao);
    
    printf("Resultado: %.6f\n", resultado);
    
    return 0;
}
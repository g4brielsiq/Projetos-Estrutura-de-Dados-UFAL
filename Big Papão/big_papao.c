#include <stdio.h>
#define NUM_FUNCIONARIOS 13
#define MAX_HABILIDADES 3

typedef struct {
    int id;
    char nome[50];
    int num_funcoes;
    char funcoes[MAX_HABILIDADES][30];
    int ocupado;
} Funcionario;


typedef struct {
    int qtdBatataFrita;
    int qtdSandwichSimples;
    int qtdSandwichMedio;
    int qtdSandwichElab;
    int qtdRefrigerante;
    int qtdSuco;
    int qtdMilkShake;
    float tempoTotal;
} Pedido;

Funcionario* mapa_habilidades() {
    static Funcionario equipe[NUM_FUNCIONARIOS] = {
        {1, "Funcionario1", 3, {"sandwich", "batata_frita", "suco"}, 0},
        {2, "Funcionario2", 2, {"sandwich", "batata_frita"}, 0},
        {3, "Funcionario3", 1, {"sandwich"}, 0},
        {4, "Funcionario4", 1, {"sandwich"}, 0},
        {5, "Funcionario5", 1, {"sandwich"}, 0},
        {6, "Funcionario6", 2, {"sandwich", "batata_frita"}, 0},
        {7, "Funcionario7", 1, {"batata_frita"}, 0},
        {8, "Funcionario8", 2, {"bebidas", "montar_bandeja"}, 0},
        {9, "Funcionario9", 1, {"montar_bandeja"}, 0},
        {10, "Funcionario10", 2, {"separacao", "caixa"}, 0},
        {11, "Funcionario11", 2, {"separacao", "sandwich"}, 0},
        {12, "Funcionario12", 2, {"caixa", "bebidas"}, 0},
        {13, "Funcionario13", 1, {"caixa"}, 0}
    };
    return equipe;
}


int main()
{
    Funcionario* Quadro = mapa_habilidades();

}

#include <stdio.h>
#define NUM_FUNCIONARIOS 13


typedef struct {
    int id;
    char nome[50];
    char funcao[30];
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

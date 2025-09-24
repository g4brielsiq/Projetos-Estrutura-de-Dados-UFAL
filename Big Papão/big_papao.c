#include <stdio.h>
#include <string.h>
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
    int qtdsanduicheSimples;
    int qtdsanduicheMedio;
    int qtdsanduicheElab;
    int qtdRefrigerante;
    int qtdSuco;
    int qtdMilkShake;
    float tempoTotal;
} Pedido;

Funcionario* MapaHabilidades() {
    static Funcionario equipe[NUM_FUNCIONARIOS] = {
        {1, "Funcionario1", 3, {"sanduiche", "batata_frita", "suco"}, 0},
        {2, "Funcionario2", 2, {"sanduiche", "batata_frita"}, 0},
        {3, "Funcionario3", 1, {"sanduiche"}, 0},
        {4, "Funcionario4", 1, {"sanduiche"}, 0},
        {5, "Funcionario5", 1, {"sanduiche"}, 0},
        {6, "Funcionario6", 2, {"sanduiche", "batata_frita"}, 0},
        {7, "Funcionario7", 1, {"batata_frita"}, 0},
        {8, "Funcionario8", 2, {"bebidas", "montar_bandeja"}, 0},
        {9, "Funcionario9", 1, {"montar_bandeja"}, 0},
        {10, "Funcionario10", 2, {"separacao", "caixa"}, 0},
        {11, "Funcionario11", 2, {"separacao", "sanduiche"}, 0},
        {12, "Funcionario12", 2, {"caixa", "bebidas"}, 0},
        {13, "Funcionario13", 1, {"caixa"}, 0}
    };
    return equipe;
}

void PrintFuncionarios(Funcionario* quadro) {

    printf("\n");
    for (int i = 0; i < NUM_FUNCIONARIOS; i++) {
        printf("ID: %d\n", quadro[i].id);
        printf("Nome: %s\n", quadro[i].nome);
        printf("Funcoes: ");
        for (int j = 0; j < quadro[i].num_funcoes; j++) {
            printf("%s", quadro[i].funcoes[j]);
            if (j < quadro[i].num_funcoes - 1) {
                printf(", ");
            }
        }
        printf("\nOcupado: %s\n", quadro[i].ocupado ? "Sim" : "Nao");
        printf("------------------------\n");
    }
}

int OcuparFuncionarioPorHabilidade(Funcionario* quadro, char* habilidade)
{
    for(int i = 0; i < NUM_FUNCIONARIOS; i++)
    {
        if(!quadro[i].ocupado)
        {
            for(int j; j < quadro[i].num_funcoes;j++)
            {
                 if (strcmp(quadro[i].funcoes[j], habilidade) == 0) {
                    return i; 
                }
            }
        }
    }
    return -1;
}
void TesteBusca(Funcionario* Quadro)
{
    int indice = OcuparFuncionarioPorHabilidade(Quadro, "sanduiche");
    if (indice != -1) {
        Quadro[indice].ocupado = 1; 
        printf("Funcionário %s alocado para preparo de sanduíche.\n", Quadro[indice].nome);
    } else {
        printf("Nenhum funcionário disponível para sanduíche.\n");
    }
}

int main()
{
    Funcionario* Quadro = MapaHabilidades();
    //PrintFuncionarios(Quadro);
    TesteBusca(Quadro);
    PrintFuncionarios(Quadro);

    return 0;
}

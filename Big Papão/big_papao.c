#include <stdio.h>
#include <string.h>
#include <stdlib.h> 
#define NUM_FUNCIONARIOS 13
#define MAX_HABILIDADES 2
// Definição dos tempos de preparo (em segundos):
#define TEMPO_BATATA_FRITA      190   
#define TEMPO_SANDUICHE_SIMPLES 58    
#define TEMPO_SANDUICHE_MEDIO   88    
#define TEMPO_SANDUICHE_ELAB    105   
#define TEMPO_REFRIGERANTE      5     
#define TEMPO_SUCO              38    
#define TEMPO_MILK_SHAKE        60    
// Definição de capacidade máxima:
#define CHAPA 3
#define FRITADEIRA 2
#define LIQUIDIFICADOR 4 
 
// Estruturas:
typedef struct {
    int id;
    char nome[50];
    int num_funcoes;
    char funcoes[MAX_HABILIDADES][30];
    int ocupado;
} Funcionario;

typedef struct {
    int id;
    int qtdBatataFrita;
    int qtdsanduicheSimples;
    int qtdsanduicheMedio;
    int qtdsanduicheElab;
    int qtdRefrigerante;
    int qtdSuco;
    int qtdMilkShake;
    float tempoTotal;
} Pedido;

typedef struct No {
    Pedido pedido;
    struct No* proximo;
} No;

typedef struct Fila {
    No* inicio;
    No* fim;
    int tamanho;
} Fila;


// Funções relacionadas aos funcionários:
Funcionario* MapaHabilidades() {
    static Funcionario equipe[NUM_FUNCIONARIOS] = {
        {1, "Funcionario1", 1, {"sanduiche"}, 0},
        {2, "Funcionario2", 1, {"sanduiche"}, 0},
        {3, "Funcionario3", 1, {"caixa"}, 0},
        {4, "Funcionario4", 1, {"batata_frita"}, 0},
        {5, "Funcionario5", 1, {"montar_bandeja"}, 0},
        {6, "Funcionario6", 2, {"sanduiche","suco"}, 0},
        {7, "Funcionario7", 2, {"sanduiche", "batata_frita"}, 0},
        {8, "Funcionario8", 2, {"sanduiche", "batata_frita"}, 0},
        {9, "Funcionario9", 2, {"bebidas", "montar_bandeja"}, 0},
        {10, "Funcionario10", 2, {"separacao", "caixa"}, 0},
        {11, "Funcionario11", 2, {"separacao", "sanduiche"}, 0},
        {12, "Funcionario12", 2, {"caixa", "bebidas"}, 0},
        {13, "Funcionario13", 2, {"sanduiche", "batata_frita"}, 0}
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

void PrintFuncionariosLivres(Funcionario* quadro) {

    printf("\nFuncionarios Livres:\n");
    for (int i = 0; i < NUM_FUNCIONARIOS; i++) {
        if(!quadro[i].ocupado)
        {
            printf("ID: %d\n", quadro[i].id);
            printf("Nome: %s\n", quadro[i].nome);
            printf("Funcoes: ");
            for (int j = 0; j < quadro[i].num_funcoes; j++) {
                printf("%s", quadro[i].funcoes[j]);
                if (j < quadro[i].num_funcoes - 1) {
                    printf(", ");
                }
            }
            printf("\n------------------------\n");
        }
        
    }
}

void PrintFuncionariosOcupados(Funcionario* quadro) {

    printf("\nFuncionarios Ocupados:\n");
    for (int i = 0; i < NUM_FUNCIONARIOS; i++) {
        if(quadro[i].ocupado)
        {
            printf("ID: %d\n", quadro[i].id);
            printf("Nome: %s\n", quadro[i].nome);
            printf("Funcoes: ");
            for (int j = 0; j < quadro[i].num_funcoes; j++) {
                printf("%s", quadro[i].funcoes[j]);
                if (j < quadro[i].num_funcoes - 1) {
                    printf(", ");
                }
            }
            printf("\n------------------------\n");
        }
        
    }
}

int OcuparFuncionarioPorHabilidade(Funcionario* quadro, char* habilidade)
{
    for(int i = 0; i < NUM_FUNCIONARIOS; i++)
    {
        if(!quadro[i].ocupado)
        {
            for(int j=0; j < quadro[i].num_funcoes;j++)
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

// Funções de Fila:
Fila* criar_fila() {
    Fila* nova_fila = (Fila*) malloc(sizeof(Fila));
    if (nova_fila != NULL) {
        nova_fila->inicio = NULL;
        nova_fila->fim = NULL;
        nova_fila->tamanho = 0;
    }
    return nova_fila;
}

void enfileirar(Fila* fila, Pedido novo_pedido) {
    No* novo_no = (No*) malloc(sizeof(No));
    if (novo_no == NULL) {
        printf("Erro: Falha na alocacao de memoria para o novo pedido!\n");
        return;
    }
    novo_no->pedido = novo_pedido;
    novo_no->proximo = NULL;

    if (fila->fim == NULL) { 
        fila->inicio = novo_no;
        fila->fim = novo_no;
    } else { 
        fila->fim->proximo = novo_no;
        fila->fim = novo_no;
    }
    fila->tamanho++;
}

Pedido desenfileirar(Fila* fila) {
    Pedido pedido_vazio = {0};
    if (fila->inicio == NULL) {
        printf("Aviso: A fila de pedidos esta vazia!\n");
        return pedido_vazio; 
    }

    No* no_removido = fila->inicio;
    Pedido pedido_retornado = no_removido->pedido;

    fila->inicio = fila->inicio->proximo;
    if (fila->inicio == NULL) { 
        fila->fim = NULL;
    }

    free(no_removido);
    fila->tamanho--;

    return pedido_retornado;
}

void liberar_fila(Fila* fila) {
    No* atual = fila->inicio;
    while (atual != NULL) {
        No* proximo_no = atual->proximo;
        free(atual);
        atual = proximo_no;
    }
    free(fila);
}

// Funções menu:
static int proximo_id_pedido = 1; // Contador global para o ID do pedido

void imprimir_pedido(Pedido p) {
    // Verifica se o pedido é válido (se tem algum item)
    if (p.id == 0) {
        printf("Pedido invalido ou vazio.\n");
        return;
    }

    printf("\n--- Detalhes do Pedido ID: %d ---\n", p.id);
    printf("Tempo Total Estimado: %.0f segundos\n", p.tempoTotal);
    printf("Itens do Pedido:\n");

    if (p.qtdBatataFrita > 0) {
        printf("  - Batata Frita: %d\n", p.qtdBatataFrita);
    }
    if (p.qtdsanduicheSimples > 0) {
        printf("  - Sanduiche Simples: %d\n", p.qtdsanduicheSimples);
    }
    if (p.qtdsanduicheMedio > 0) {
        printf("  - Sanduiche Medio: %d\n", p.qtdsanduicheMedio);
    }
    if (p.qtdsanduicheElab > 0) {
        printf("  - Sanduiche Elaborado: %d\n", p.qtdsanduicheElab);
    }
    if (p.qtdRefrigerante > 0) {
        printf("  - Refrigerante: %d\n", p.qtdRefrigerante);
    }
    if (p.qtdSuco > 0) {
        printf("  - Suco: %d\n", p.qtdSuco);
    }
    if (p.qtdMilkShake > 0) {
        printf("  - Milk Shake: %d\n", p.qtdMilkShake);
    }
    printf("----------------------------------\n\n");
}


void adicionar_pedido_menu(Fila* fila) {
    Pedido novo_pedido = {0}; // Inicializa todos os campos com 0
    int escolha = -1;
    
    novo_pedido.id = proximo_id_pedido++;

    printf("\n--- Adicionar Novo Pedido (ID: %d) ---\n", novo_pedido.id);
    do
    {
        int quantidade = 0;
        puts("----------- CARDAPIO -----------");
        puts("Por favor escolha o item a ser pedido:");
        puts("1- Batata Frita");
        puts("2- Sanduiche Simples");
        puts("3- Sanduiche Medio");
        puts("4- Sanduiche Elaborado");
        puts("5- Refrigerante");
        puts("6- Suco");
        puts("7- Milk Shake");
        puts("0- Sair");
        puts("------------------------------");
        printf("Número do Item: ");
        scanf("%d",&escolha);
        switch(escolha)
        {
            case 1:
                printf("Quantidade de Batata Frita: ");
                scanf("%d", &quantidade);
                novo_pedido.qtdBatataFrita += quantidade;
                printf("%d un. de Batata Frita adicionadas.\n\n", quantidade);
                break;
            case 2:
                printf("Quantidade de Sanduiche Simples: ");
                scanf("%d", &quantidade);
                novo_pedido.qtdsanduicheSimples += quantidade;
                printf("%d un. de Sanduiche Simples adicionados.\n\n", quantidade);
                break;
            case 3:
                printf("Quantidade de Sanduiche Medio: ");
                scanf("%d", &quantidade);
                novo_pedido.qtdsanduicheMedio += quantidade;
                printf("%d un. de Sanduiche Medio adicionados.\n\n", quantidade);
                break;
            case 4:
                printf("Quantidade de Sanduiche Elaborado: ");
                scanf("%d", &quantidade);
                novo_pedido.qtdsanduicheElab += quantidade;
                printf("%d un. de Sanduiche Elaborado adicionados.\n\n", quantidade);
                break;
            case 5:
                printf("Quantidade de Refrigerante: ");
                scanf("%d", &quantidade);
                novo_pedido.qtdRefrigerante += quantidade;
                printf("%d un. de Refrigerante adicionados.\n\n", quantidade);
                break;
            case 6:
                printf("Quantidade de Suco: ");
                scanf("%d", &quantidade);
                novo_pedido.qtdSuco += quantidade;
                printf("%d un. de Suco adicionados.\n\n", quantidade);
                break;
            case 7:
                printf("Quantidade de Milk Shake: ");
                scanf("%d", &quantidade);
                novo_pedido.qtdMilkShake += quantidade;
                printf("%d un. de Milk Shake adicionados.\n\n", quantidade);
                break;
            case 0:
                enfileirar(fila, novo_pedido);
                printf("Finalizando adicao de itens...\n");
                break;
            default:
                
                printf("Opcao invalida! Por favor, escolha um item do menu.\n\n");
                break;
        }

    } while (escolha != 0);
    puts("----------- PEDIDO -----------");
    imprimir_pedido(novo_pedido);
}


int main()
{
    
    int escolha = -1;
    Fila* fila_de_pedidos = criar_fila();
    Funcionario* Quadro = MapaHabilidades();
    //PrintFuncionarios(Quadro);
    //TesteBusca(Quadro);
    //PrintFuncionarios(Quadro);
    //PrintFuncionariosOcupados(Quadro);
    //PrintFuncionariosLivres(Quadro);
    do {
        printf("========== MENU RESTAURANTE ==========\n");
        printf("1. Adicionar novo pedido a fila\n");
        printf("2. Visualizar fila de pedidos\n");
        printf("0. Sair\n");
        printf("======================================\n");
        printf("Escolha uma opcao: ");
        scanf("%d", &escolha);

        switch (escolha) {
            case 1:
                adicionar_pedido_menu(fila_de_pedidos);
                break;
            case 2:
                puts("A ser implementada");
                break;
            case 0:
                printf("\nEncerrando o sistema...\n");
                break;
            default:
                printf("\nOpcao invalida! Tente novamente.\n\n");
                break;
        }

    } while (escolha != 0);
    liberar_fila(fila_de_pedidos);
    return 0;
}

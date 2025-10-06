#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Definições em português para estruturas de dados
// Tipos de itens
typedef enum {
    BATATA_FRITA,
    SANDUINCHE_SIMPLES,
    SANDUINCHE_MEDIO,
    SANDUINCHE_ELABORADO,
    REFRIGERANTE,
    SUCO,
    MILK_SHAKE
} TipoItem;

// Habilidades dos funcionários (usando bitmask para simplicidade)
typedef enum {
    HABILIDADE_SANDUICHE = 1 << 0,
    HABILIDADE_BATATA = 1 << 1,
    HABILIDADE_SUCOS = 1 << 2,
    HABILIDADE_BEBDAS = 1 << 3,
    HABILIDADE_MONTAGEM = 1 << 4,
    HABILIDADE_SEPARACAO = 1 << 5,
    HABILIDADE_CAIXA = 1 << 6
} Habilidade;

// Struct para Item do pedido
typedef struct Item {
    TipoItem tipo;
    int quantidade;
    int tempoPreparo; // Calculado baseado no tipo
    struct Item* proximo;
} Item;

// Struct para Pedido (fila de itens)
typedef struct Pedido {
    int id;
    int tempoChegada; // Em segundos
    Item* itens; // Lista ligada de itens
    int tempoTotalPreparo; // Tempo estimado total
    int tempoMontagem; // 30s fixo se aplicável
    int entregue; // 1 se entregue em 5 min, 0 caso contrário
    struct Pedido* proximo;
} Pedido;

// Struct para Funcionário
typedef struct Funcionario {
    int id;
    unsigned int habilidades; // Bitmask de habilidades
    int ocupadoAte; // Tempo em que fica livre (em segundos)
    struct Funcionario* proximo;
} Funcionario;

// Struct para Fila de Pedidos (usando lista ligada como fila)
typedef struct FilaPedidos {
    Pedido* inicio;
    Pedido* fim;
    int tamanho;
} FilaPedidos;

// Struct para Lista de Funcionários Disponíveis
typedef struct ListaFuncionarios {
    Funcionario* inicio;
    int tamanho;
} ListaFuncionarios;

// Nomes dos itens para exibição
char* nomesItens[] = {
    "Batata Frita",
    "Sanduiche Simples",
    "Sanduiche Medio",
    "Sanduiche Elaborado",
    "Refrigerante",
    "Suco",
    "Milk Shake"
};

// Tempos de preparo em segundos (conforme especificação)
int temposPreparo[] = {
    190, // BATATA_FRITA
    58,  // SANDUINCHE_SIMPLES
    88,  // SANDUINCHE_MEDIO
    105, // SANDUINCHE_ELABORADO
    5,   // REFRIGERANTE
    38,  // SUCO
    60   // MILK_SHAKE
};

// Limites de preparo simultâneo
#define MAX_SANDUICHES_CHAPA 3
#define MAX_BATATAS_PENEIRA 2
#define MAX_MILK_SHAKES_LIQUIDIFICADOR 4
#define TEMPO_MAX_ATENDIMENTO 300 // 5 minutos em segundos
#define TEMPO_MONTAGEM 30

// Funções para manipular fila de pedidos
FilaPedidos* criarFilaPedidos() {
    FilaPedidos* fila = (FilaPedidos*)malloc(sizeof(FilaPedidos));
    fila->inicio = NULL;
    fila->fim = NULL;
    fila->tamanho = 0;
    return fila;
}

void enfileirarPedido(FilaPedidos* fila, Pedido* pedido) {
    if (!fila->inicio) {
        fila->inicio = pedido;
        fila->fim = pedido;
    } else {
        fila->fim->proximo = pedido;
        fila->fim = pedido;
    }
    fila->tamanho++;
    pedido->proximo = NULL;
}

Pedido* desenfileirarPedido(FilaPedidos* fila) {
    if (!fila->inicio) return NULL;
    Pedido* pedido = fila->inicio;
    fila->inicio = fila->inicio->proximo;
    fila->tamanho--;
    if (!fila->inicio) fila->fim = NULL;
    return pedido;
}

// Função para criar item
Item* criarItem(TipoItem tipo, int quantidade) {
    Item* item = (Item*)malloc(sizeof(Item));
    item->tipo = tipo;
    item->quantidade = quantidade;
    item->tempoPreparo = temposPreparo[tipo] * quantidade;
    item->proximo = NULL;
    return item;
}

// Função para adicionar item ao pedido
void adicionarItem(Pedido* pedido, Item* item) {
    if (!pedido->itens) {
        pedido->itens = item;
    } else {
        Item* atual = pedido->itens;
        while (atual->proximo) {
            atual = atual->proximo;
        }
        atual->proximo = item;
    }
    pedido->tempoTotalPreparo += item->tempoPreparo;
}

// Função para imprimir o conteúdo de um pedido
void imprimirPedido(Pedido* pedido) {
    printf("Processando Pedido %d (chegada em %d s): ", pedido->id, pedido->tempoChegada);
    Item* atual = pedido->itens;
    if (!atual) {
        printf("Vazio\n");
        return;
    }
    while (atual) {
        printf("%s x%d", nomesItens[atual->tipo], atual->quantidade);
        if (atual->proximo) printf(", ");
        atual = atual->proximo;
    }
    printf("\n");
}

// Função para calcular tempo de montagem (simplificado: 30s se aplicável)
int calcularTempoMontagem(Item* itens) {
    int itensComer = 0, itensBeber = 0;
    Item* atual = itens;
    while (atual) {
        if (atual->tipo == BATATA_FRITA || (atual->tipo >= SANDUINCHE_SIMPLES && atual->tipo <= SANDUINCHE_ELABORADO)) {
            itensComer += atual->quantidade;
        } else {
            itensBeber += atual->quantidade;
        }
        atual = atual->proximo;
    }
    // Verifica limite da bandeja: max 2 comer + 2 beber
    if (itensComer > 2 || itensBeber > 2) {
        return -1; // Inválido
    }
    return TEMPO_MONTAGEM;
}

// Função para criar lista de funcionários (13 conforme especificação)
ListaFuncionarios* criarFuncionarios() {
    ListaFuncionarios* lista = (ListaFuncionarios*)malloc(sizeof(ListaFuncionarios));
    lista->inicio = NULL;
    lista->tamanho = 0;

    Funcionario* funcs[13];
    int i;
    for (i = 0; i < 13; i++) {
        funcs[i] = (Funcionario*)malloc(sizeof(Funcionario));
        funcs[i]->id = i + 1;
        funcs[i]->ocupadoAte = 0;
        funcs[i]->proximo = NULL;
    }

    // 5 sanduíches básicos
    for (i = 0; i < 5; i++) {
        funcs[i]->habilidades = HABILIDADE_SANDUICHE;
    }
    // 2 deles também batata
    funcs[0]->habilidades |= HABILIDADE_BATATA;
    funcs[1]->habilidades |= HABILIDADE_BATATA;
    // 1 também sucos (usando um dos sanduíches)
    funcs[2]->habilidades |= HABILIDADE_SUCOS;

    // 2 batata adicionais (um já coberto acima)
    funcs[5]->habilidades = HABILIDADE_BATATA;
    funcs[6]->habilidades = HABILIDADE_BATATA | HABILIDADE_SANDUICHE;

    // 1 bebidas também montagem
    funcs[7]->habilidades = HABILIDADE_BEBDAS | HABILIDADE_MONTAGEM;

    // 1 montagem
    funcs[8]->habilidades = HABILIDADE_MONTAGEM;

    // 2 separação
    funcs[9]->habilidades = HABILIDADE_SEPARACAO | HABILIDADE_CAIXA;
    funcs[10]->habilidades = HABILIDADE_SEPARACAO | HABILIDADE_SANDUICHE;

    // 2 caixa
    funcs[11]->habilidades = HABILIDADE_CAIXA;
    funcs[12]->habilidades = HABILIDADE_CAIXA | HABILIDADE_BEBDAS;

    // Adiciona à lista
    for (i = 0; i < 13; i++) {
        if (!lista->inicio) {
            lista->inicio = funcs[i];
        } else {
            Funcionario* atual = lista->inicio;
            while (atual->proximo) atual = atual->proximo;
            atual->proximo = funcs[i];
        }
        lista->tamanho++;
    }

    return lista;
}

// Função para encontrar funcionário disponível com habilidade necessária
Funcionario* encontrarFuncionario(ListaFuncionarios* lista, unsigned int habilidadeNecessaria, int tempoAtual) {
    Funcionario* atual = lista->inicio;
    Funcionario* candidato = NULL;
    int menorOcupado = -1;

    while (atual) {
        if ((atual->habilidades & habilidadeNecessaria) && atual->ocupadoAte <= tempoAtual) {
            if (menorOcupado == -1 || atual->ocupadoAte < menorOcupado) {
                candidato = atual;
                menorOcupado = atual->ocupadoAte;
            }
        }
        atual = atual->proximo;
    }
    return candidato;
}

// Função para alocar funcionário a uma tarefa
int alocarFuncionario(Funcionario* func, int tempoTarefa, int tempoAtual) {
    if (func && func->ocupadoAte <= tempoAtual) {
        func->ocupadoAte = tempoAtual + tempoTarefa;
        return 1;
    }
    return 0;
}

// Função para processar um item (simplificado, considerando limites)
int processarItem(Item* item, ListaFuncionarios* lista, int tempoAtual, int* tempoProcessado) {
    unsigned int habilidadeNecessaria;
    int capacidadeMax = 1; // Padrão

    switch (item->tipo) {
        case BATATA_FRITA:
            habilidadeNecessaria = HABILIDADE_BATATA;
            capacidadeMax = MAX_BATATAS_PENEIRA;
            break;
        case SANDUINCHE_SIMPLES:
        case SANDUINCHE_MEDIO:
        case SANDUINCHE_ELABORADO:
            habilidadeNecessaria = HABILIDADE_SANDUICHE;
            capacidadeMax = MAX_SANDUICHES_CHAPA;
            break;
        case REFRIGERANTE:
        case SUCO:
        case MILK_SHAKE:
            habilidadeNecessaria = HABILIDADE_BEBDAS;
            if (item->tipo == MILK_SHAKE) capacidadeMax = MAX_MILK_SHAKES_LIQUIDIFICADOR;
            break;
        default:
            return 0;
    }

    // Verifica se quantidade excede capacidade (processa em lotes)
    int lotes = (item->quantidade + capacidadeMax - 1) / capacidadeMax;
    *tempoProcessado = (temposPreparo[item->tipo] * lotes); // Tempo por lote (simplificado)

    Funcionario* func = encontrarFuncionario(lista, habilidadeNecessaria, tempoAtual);
    if (func && alocarFuncionario(func, *tempoProcessado, tempoAtual)) {
        return 1;
    }
    return 0; // Não pôde alocar
}

// Função principal de simulação
void simularCozinha(FilaPedidos* filaPedidos, ListaFuncionarios* listaFuncs) {
    int tempoAtual = 0;
    int perdas = 0;
    int atendidos = 0;

    printf("=== INÍCIO DA SIMULAÇÃO DA COZINHA ===\n");

    while (filaPedidos->tamanho > 0) {
        Pedido* pedido = desenfileirarPedido(filaPedidos);
        imprimirPedido(pedido);
        int tempoInicio = tempoAtual;

        // Ajusta tempo atual para chegada do pedido (se atrasado)
        if (pedido->tempoChegada > tempoAtual) {
            tempoAtual = pedido->tempoChegada;
        }

        pedido->tempoMontagem = calcularTempoMontagem(pedido->itens);
        if (pedido->tempoMontagem == -1) {
            printf("  -> Pedido %d inválido: excede limite da bandeja (mais de 2 itens para comer ou beber). ATRASADO/Perda!\n", pedido->id);
            perdas++;
            // Libera itens
            Item* atualItem = pedido->itens;
            while (atualItem) {
                Item* temp = atualItem;
                atualItem = atualItem->proximo;
                free(temp);
            }
            free(pedido);
            continue;
        }

        // Processa cada item sequencialmente (simplificado)
        Item* atualItem = pedido->itens;
        int sucesso = 1;
        int tempoPreparoTotal = 0;
        while (atualItem) {
            int tempoItem;
            if (!processarItem(atualItem, listaFuncs, tempoAtual, &tempoItem)) {
                printf("  -> Falha ao processar %s no Pedido %d (sem funcionário disponível). ATRASADO/Perda!\n", nomesItens[atualItem->tipo], pedido->id);
                sucesso = 0;
                break;
            }
            tempoPreparoTotal += tempoItem;
            tempoAtual += tempoItem; // Avança tempo por item (sem paralelismo total)
            atualItem = atualItem->proximo;
        }

        int tempoTotal = 0;
        if (sucesso) {
            // Adiciona montagem
            Funcionario* funcMontagem = encontrarFuncionario(listaFuncs, HABILIDADE_MONTAGEM, tempoAtual);
            if (funcMontagem && alocarFuncionario(funcMontagem, pedido->tempoMontagem, tempoAtual)) {
                tempoAtual += pedido->tempoMontagem;
                tempoTotal = tempoAtual - tempoInicio;
            } else {
                printf("  -> Falha na montagem do Pedido %d (sem funcionário para bandeja). ATRASADO/Perda!\n", pedido->id);
                sucesso = 0;
            }
        }

        if (sucesso && tempoTotal <= TEMPO_MAX_ATENDIMENTO) {
            pedido->entregue = 1;
            atendidos++;
            printf("  -> Pedido %d ATENDIDO em %d segundos (dentro dos 5 minutos).\n", pedido->id, tempoTotal);
        } else if (sucesso) {
            pedido->entregue = 0;
            perdas++;
            printf("  -> Pedido %d ATRASADO: tempo total %d segundos > 300s.\n", pedido->id, tempoTotal);
        }

        // Avança tempo para simular intervalo entre processamentos (se houver mais pedidos)
        if (filaPedidos->tamanho > 0) {
            tempoAtual += 10; // Pequeno intervalo
        }

        // Libera itens do pedido
        atualItem = pedido->itens;
        while (atualItem) {
            Item* temp = atualItem;
            atualItem = atualItem->proximo;
            free(temp);
        }
        free(pedido);
        printf("\n");
    }

    printf("=== FIM DA SIMULAÇÃO ===\n");
    printf("Pedidos Atendidos: %d\n", atendidos);
    printf("Pedidos Atrasados/Perdas: %d\n", perdas);
}

// Função principal
int main() {
    srand(time(NULL));

    // Cria fila de pedidos de exemplo (6 pedidos variados para cobrir casos)
    FilaPedidos* fila = criarFilaPedidos();
    ListaFuncionarios* funcs = criarFuncionarios();

    // Pedido 1: Simples - 1 batata, 1 sanduiche simples, 1 refrigerante (deve atender)
    Pedido* p1 = (Pedido*)malloc(sizeof(Pedido));
    p1->id = 1;
    p1->tempoChegada = 0;
    p1->itens = NULL;
    p1->tempoTotalPreparo = 0;
    p1->tempoMontagem = 0;
    p1->entregue = 0;
    adicionarItem(p1, criarItem(BATATA_FRITA, 1));
    adicionarItem(p1, criarItem(SANDUINCHE_SIMPLES, 1));
    adicionarItem(p1, criarItem(REFRIGERANTE, 1));
    enfileirarPedido(fila, p1);

    // Pedido 2: Médio - 2 sanduiches médios, 1 suco (deve atender)
    Pedido* p2 = (Pedido*)malloc(sizeof(Pedido));
    p2->id = 2;
    p2->tempoChegada = 60; // Chega após 1 min
    p2->itens = NULL;
    p2->tempoTotalPreparo = 0;
    p2->tempoMontagem = 0;
    p2->entregue = 0;
    adicionarItem(p2, criarItem(SANDUINCHE_MEDIO, 2));
    adicionarItem(p2, criarItem(SUCO, 1));
    enfileirarPedido(fila, p2);

    // Pedido 3: Com milk shake - 1 milk shake, 1 sanduiche elaborado (deve atender)
    Pedido* p3 = (Pedido*)malloc(sizeof(Pedido));
    p3->id = 3;
    p3->tempoChegada = 120; // Chega após 2 min
    p3->itens = NULL;
    p3->tempoTotalPreparo = 0;
    p3->tempoMontagem = 0;
    p3->entregue = 0;
    adicionarItem(p3, criarItem(MILK_SHAKE, 1));
    adicionarItem(p3, criarItem(SANDUINCHE_ELABORADO, 1));
    enfileirarPedido(fila, p3);

    // Pedido 4: Complexo - 2 batatas, 3 sanduiches simples, 2 refrigerantes (excede bandeja: 5 comer >2, atrasado)
    Pedido* p4 = (Pedido*)malloc(sizeof(Pedido));
    p4->id = 4;
    p4->tempoChegada = 180; // Chega após 4 min
    p4->itens = NULL;
    p4->tempoTotalPreparo = 0;
    p4->tempoMontagem = 0;
    p4->entregue = 0;
    adicionarItem(p4, criarItem(BATATA_FRITA, 2));
    adicionarItem(p4, criarItem(SANDUINCHE_SIMPLES, 3));
    adicionarItem(p4, criarItem(REFRIGERANTE, 2));
    enfileirarPedido(fila, p4);

    // Executa simulação
    simularCozinha(fila, funcs);

    // Libera memória dos funcionários (simplificado)
    Funcionario* atualFunc = funcs->inicio;
    while (atualFunc) {
        Funcionario* temp = atualFunc;
        atualFunc = atualFunc->proximo;
        free(temp);
    }
    free(funcs);
    free(fila);

    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Definição dos tempos de preparo em segundos
#define TEMPO_BATATA 190
#define TEMPO_SANDUICHE_SIMPLES 58
#define TEMPO_SANDUICHE_MEDIO 88
#define TEMPO_SANDUICHE_ELABORADO 105
#define TEMPO_REFRIGERANTE 5
#define TEMPO_SUCO 38
#define TEMPO_MILKSHAKE 60
#define TEMPO_MONTAGEM 30
#define TEMPO_MAXIMO_ATENDIMENTO 300 // 5 minutos em segundos

// Estruturas de dados
typedef struct {
    int id;
    char tipo[20];
    int tempo_preparo;
    int tempo_chegada;
    int tempo_inicio_preparo;
    int tempo_conclusao;
} Sanduiche;

typedef struct {
    int id;
    int tempo_preparo;
    int tempo_chegada;
    int tempo_inicio_preparo;
    int tempo_conclusao;
} BatataFrita;

typedef struct {
    int id;
    char tipo[20];
    int tempo_preparo;
    int tempo_chegada;
    int tempo_inicio_preparo;
    int tempo_conclusao;
} Bebida;

typedef struct {
    int id;
    Sanduiche* sanduiche;
    BatataFrita* batata;
    Bebida* bebida1;
    Bebida* bebida2;
    int tempo_chegada;
    int tempo_conclusao;
    int atrasado;
} Pedido;

typedef struct NoFila {
    Pedido* pedido;
    struct NoFila* proximo;
} NoFila;

typedef struct {
    NoFila* inicio;
    NoFila* fim;
    int tamanho;
} FilaPedidos;

typedef struct {
    int id;
    char habilidades[5][30];
    int num_habilidades;
    int ocupado;
    Pedido* pedido_atual;
    int tempo_termino;
} Funcionario;

typedef struct {
    char tipo[30];
    int capacidade_maxima;
    int em_uso;
    int tempo_termino;
} Equipamento;

// Protótipos das funções
FilaPedidos* criar_fila();
void enfileirar(FilaPedidos* fila, Pedido* pedido);
Pedido* desenfileirar(FilaPedidos* fila);
int fila_vazia(FilaPedidos* fila);

void inicializar_funcionarios(Funcionario funcionarios[]);
void inicializar_equipamentos(Equipamento equipamentos[]);
Pedido* criar_pedido_pre_definido(int id, int caso, int tempo_atual);
void processar_pedidos(FilaPedidos* fila_pedidos, Funcionario funcionarios[], Equipamento equipamentos[], int tempo_atual);
void simular_casos_pre_definidos();

int main() {
    srand(time(NULL));
    printf("=== SIMULADOR BIGPAPÃO LANCHONETE - CASOS PRÉ-DEFINIDOS ===\n\n");
    simular_casos_pre_definidos();
    return 0;
}

// Implementação das funções da fila
FilaPedidos* criar_fila() {
    FilaPedidos* fila = (FilaPedidos*)malloc(sizeof(FilaPedidos));
    fila->inicio = fila->fim = NULL;
    fila->tamanho = 0;
    return fila;
}

void enfileirar(FilaPedidos* fila, Pedido* pedido) {
    NoFila* novo_no = (NoFila*)malloc(sizeof(NoFila));
    novo_no->pedido = pedido;
    novo_no->proximo = NULL;
    
    if (fila->fim == NULL) {
        fila->inicio = fila->fim = novo_no;
    } else {
        fila->fim->proximo = novo_no;
        fila->fim = novo_no;
    }
    fila->tamanho++;
}

Pedido* desenfileirar(FilaPedidos* fila) {
    if (fila->inicio == NULL) return NULL;
    
    NoFila* temp = fila->inicio;
    Pedido* pedido = temp->pedido;
    fila->inicio = fila->inicio->proximo;
    
    if (fila->inicio == NULL) {
        fila->fim = NULL;
    }
    
    free(temp);
    fila->tamanho--;
    return pedido;
}

int fila_vazia(FilaPedidos* fila) {
    return fila->inicio == NULL;
}

void inicializar_funcionarios(Funcionario funcionarios[]) {
    // 5 funcionários habilitados a fazer sanduíches
    for (int i = 0; i < 5; i++) {
        funcionarios[i].id = i + 1;
        funcionarios[i].num_habilidades = 1;
        strcpy(funcionarios[i].habilidades[0], "sanduiche");
        funcionarios[i].ocupado = 0;
        funcionarios[i].pedido_atual = NULL;
        funcionarios[i].tempo_termino = 0;
    }
    
    strcpy(funcionarios[0].habilidades[1], "batata");
    strcpy(funcionarios[1].habilidades[1], "batata");
    funcionarios[0].num_habilidades = funcionarios[1].num_habilidades = 2;
    
    strcpy(funcionarios[2].habilidades[1], "bebida");
    funcionarios[2].num_habilidades = 2;
    
    // 2 funcionários habilitados a fazer batatas fritas
    funcionarios[5].id = 6;
    strcpy(funcionarios[5].habilidades[0], "batata");
    funcionarios[5].num_habilidades = 1;
    funcionarios[5].ocupado = 0;
    
    funcionarios[6].id = 7;
    strcpy(funcionarios[6].habilidades[0], "batata");
    strcpy(funcionarios[6].habilidades[1], "sanduiche");
    funcionarios[6].num_habilidades = 2;
    funcionarios[6].ocupado = 0;
    
    // 1 funcionário habilitado a fazer bebidas
    funcionarios[7].id = 8;
    strcpy(funcionarios[7].habilidades[0], "bebida");
    strcpy(funcionarios[7].habilidades[1], "montagem");
    funcionarios[7].num_habilidades = 2;
    funcionarios[7].ocupado = 0;
    
    // 1 funcionário habilitado a montar bandeja
    funcionarios[8].id = 9;
    strcpy(funcionarios[8].habilidades[0], "montagem");
    funcionarios[8].num_habilidades = 1;
    funcionarios[8].ocupado = 0;
    
    // 2 funcionários habilitados a separar pedidos
    funcionarios[9].id = 10;
    strcpy(funcionarios[9].habilidades[0], "separacao");
    strcpy(funcionarios[9].habilidades[1], "caixa");
    funcionarios[9].num_habilidades = 2;
    funcionarios[9].ocupado = 0;
    
    funcionarios[10].id = 11;
    strcpy(funcionarios[10].habilidades[0], "separacao");
    strcpy(funcionarios[10].habilidades[1], "sanduiche");
    funcionarios[10].num_habilidades = 2;
    funcionarios[10].ocupado = 0;
    
    // 2 funcionários habilitados a ser caixa
    funcionarios[11].id = 12;
    strcpy(funcionarios[11].habilidades[0], "caixa");
    strcpy(funcionarios[11].habilidades[1], "bebida");
    funcionarios[11].num_habilidades = 2;
    funcionarios[11].ocupado = 0;
    
    funcionarios[12].id = 13;
    strcpy(funcionarios[12].habilidades[0], "caixa");
    funcionarios[12].num_habilidades = 1;
    funcionarios[12].ocupado = 0;
}

void inicializar_equipamentos(Equipamento equipamentos[]) {
    strcpy(equipamentos[0].tipo, "chapa_sanduiche");
    equipamentos[0].capacidade_maxima = 3;
    equipamentos[0].em_uso = 0;
    equipamentos[0].tempo_termino = 0;
    
    strcpy(equipamentos[1].tipo, "peneira_batata");
    equipamentos[1].capacidade_maxima = 2;
    equipamentos[1].em_uso = 0;
    equipamentos[1].tempo_termino = 0;
    
    strcpy(equipamentos[2].tipo, "liquidificador_milkshake");
    equipamentos[2].capacidade_maxima = 4;
    equipamentos[2].em_uso = 0;
    equipamentos[2].tempo_termino = 0;
}

Pedido* criar_pedido_pre_definido(int id, int caso, int tempo_atual) {
    Pedido* pedido = (Pedido*)malloc(sizeof(Pedido));
    pedido->id = id;
    pedido->tempo_chegada = tempo_atual;
    pedido->atrasado = 0;
    
    printf("\n=== CRIANDO PEDIDO %d - CASO %d ===\n", id, caso);
    
    switch(caso) {
        case 1: // Pedido simples - dentro do prazo
            printf("Caso 1: Pedido SIMPLES (dentro do prazo)\n");
            printf("- Sanduíche simples + Refrigerante\n");
            
            pedido->sanduiche = (Sanduiche*)malloc(sizeof(Sanduiche));
            pedido->sanduiche->id = id * 10 + 1;
            strcpy(pedido->sanduiche->tipo, "simples");
            pedido->sanduiche->tempo_preparo = TEMPO_SANDUICHE_SIMPLES;
            pedido->sanduiche->tempo_chegada = tempo_atual;
            
            pedido->batata = NULL;
            
            pedido->bebida1 = (Bebida*)malloc(sizeof(Bebida));
            pedido->bebida1->id = id * 10 + 2;
            strcpy(pedido->bebida1->tipo, "refrigerante");
            pedido->bebida1->tempo_preparo = TEMPO_REFRIGERANTE;
            pedido->bebida1->tempo_chegada = tempo_atual;
            
            pedido->bebida2 = NULL;
            break;
            
        case 2: // Pedido médio - limite do prazo
            printf("Caso 2: Pedido MÉDIO (limite do prazo)\n");
            printf("- Sanduíche médio + Batata + Suco\n");
            
            pedido->sanduiche = (Sanduiche*)malloc(sizeof(Sanduiche));
            pedido->sanduiche->id = id * 10 + 1;
            strcpy(pedido->sanduiche->tipo, "medio");
            pedido->sanduiche->tempo_preparo = TEMPO_SANDUICHE_MEDIO;
            pedido->sanduiche->tempo_chegada = tempo_atual;
            
            pedido->batata = (BatataFrita*)malloc(sizeof(BatataFrita));
            pedido->batata->id = id * 10 + 2;
            pedido->batata->tempo_preparo = TEMPO_BATATA;
            pedido->batata->tempo_chegada = tempo_atual;
            
            pedido->bebida1 = (Bebida*)malloc(sizeof(Bebida));
            pedido->bebida1->id = id * 10 + 3;
            strcpy(pedido->bebida1->tipo, "suco");
            pedido->bebida1->tempo_preparo = TEMPO_SUCO;
            pedido->bebida1->tempo_chegada = tempo_atual;
            
            pedido->bebida2 = NULL;
            break;
            
        case 3: // Pedido complexo - provável atraso
            printf("Caso 3: Pedido COMPLEXO (provável atraso)\n");
            printf("- Sanduíche elaborado + Batata + Milk Shake\n");
            
            pedido->sanduiche = (Sanduiche*)malloc(sizeof(Sanduiche));
            pedido->sanduiche->id = id * 10 + 1;
            strcpy(pedido->sanduiche->tipo, "elaborado");
            pedido->sanduiche->tempo_preparo = TEMPO_SANDUICHE_ELABORADO;
            pedido->sanduiche->tempo_chegada = tempo_atual;
            
            pedido->batata = (BatataFrita*)malloc(sizeof(BatataFrita));
            pedido->batata->id = id * 10 + 2;
            pedido->batata->tempo_preparo = TEMPO_BATATA;
            pedido->batata->tempo_chegada = tempo_atual;
            
            pedido->bebida1 = (Bebida*)malloc(sizeof(Bebida));
            pedido->bebida1->id = id * 10 + 3;
            strcpy(pedido->bebida1->tipo, "milkshake");
            pedido->bebida1->tempo_preparo = TEMPO_MILKSHAKE;
            pedido->bebida1->tempo_chegada = tempo_atual;
            
            pedido->bebida2 = NULL;
            break;
            
        case 4: // Pedido familiar - muito complexo
            printf("Caso 4: Pedido FAMILIAR (muito complexo)\n");
            printf("- 2 Sanduíches elaborados + Batata + 2 Milk Shakes\n");
            
            pedido->sanduiche = (Sanduiche*)malloc(sizeof(Sanduiche));
            pedido->sanduiche->id = id * 10 + 1;
            strcpy(pedido->sanduiche->tipo, "elaborado");
            pedido->sanduiche->tempo_preparo = TEMPO_SANDUICHE_ELABORADO;
            pedido->sanduiche->tempo_chegada = tempo_atual;
            
            pedido->batata = (BatataFrita*)malloc(sizeof(BatataFrita));
            pedido->batata->id = id * 10 + 2;
            pedido->batata->tempo_preparo = TEMPO_BATATA;
            pedido->batata->tempo_chegada = tempo_atual;
            
            pedido->bebida1 = (Bebida*)malloc(sizeof(Bebida));
            pedido->bebida1->id = id * 10 + 3;
            strcpy(pedido->bebida1->tipo, "milkshake");
            pedido->bebida1->tempo_preparo = TEMPO_MILKSHAKE;
            pedido->bebida1->tempo_chegada = tempo_atual;
            
            pedido->bebida2 = (Bebida*)malloc(sizeof(Bebida));
            pedido->bebida2->id = id * 10 + 4;
            strcpy(pedido->bebida2->tipo, "milkshake");
            pedido->bebida2->tempo_preparo = TEMPO_MILKSHAKE;
            pedido->bebida2->tempo_chegada = tempo_atual;
            break;
            
        case 5: // Pedido rápido - certamente no prazo
            printf("Caso 5: Pedido RÁPIDO (certamente no prazo)\n");
            printf("- Sanduíche simples + Refrigerante (sem batata)\n");
            
            pedido->sanduiche = (Sanduiche*)malloc(sizeof(Sanduiche));
            pedido->sanduiche->id = id * 10 + 1;
            strcpy(pedido->sanduiche->tipo, "simples");
            pedido->sanduiche->tempo_preparo = TEMPO_SANDUICHE_SIMPLES;
            pedido->sanduiche->tempo_chegada = tempo_atual;
            
            pedido->batata = NULL;
            
            pedido->bebida1 = (Bebida*)malloc(sizeof(Bebida));
            pedido->bebida1->id = id * 10 + 2;
            strcpy(pedido->bebida1->tipo, "refrigerante");
            pedido->bebida1->tempo_preparo = TEMPO_REFRIGERANTE;
            pedido->bebida1->tempo_chegada = tempo_atual;
            
            pedido->bebida2 = NULL;
            break;
    }
    
    // Calcular tempo estimado do pedido
    int tempo_total = TEMPO_MONTAGEM;
    if (pedido->sanduiche) tempo_total += pedido->sanduiche->tempo_preparo;
    if (pedido->batata) tempo_total += pedido->batata->tempo_preparo;
    if (pedido->bebida1) tempo_total += pedido->bebida1->tempo_preparo;
    if (pedido->bebida2) tempo_total += pedido->bebida2->tempo_preparo;
    
    printf("Tempo estimado: %d segundos | Prazo: %d segundos\n", 
           tempo_total, TEMPO_MAXIMO_ATENDIMENTO);
    printf("Previsão: %s\n\n", 
           tempo_total > TEMPO_MAXIMO_ATENDIMENTO ? "ATRASO" : "DENTRO DO PRAZO");
    
    return pedido;
}

void processar_pedidos(FilaPedidos* fila_pedidos, Funcionario funcionarios[], Equipamento equipamentos[], int tempo_atual) {
    // Verificar funcionários livres e alocar tarefas
    for (int i = 0; i < 13; i++) {
        if (!funcionarios[i].ocupado && !fila_vazia(fila_pedidos)) {
            Pedido* pedido = desenfileirar(fila_pedidos);
            funcionarios[i].ocupado = 1;
            funcionarios[i].pedido_atual = pedido;
            
            // Calcular tempo total do pedido
            int tempo_total = TEMPO_MONTAGEM;
            if (pedido->sanduiche) tempo_total += pedido->sanduiche->tempo_preparo;
            if (pedido->batata) tempo_total += pedido->batata->tempo_preparo;
            if (pedido->bebida1) tempo_total += pedido->bebida1->tempo_preparo;
            if (pedido->bebida2) tempo_total += pedido->bebida2->tempo_preparo;
            
            funcionarios[i].tempo_termino = tempo_atual + tempo_total;
            pedido->tempo_conclusao = funcionarios[i].tempo_termino;
            
            // Verificar se vai atrasar
            pedido->atrasado = (pedido->tempo_conclusao - pedido->tempo_chegada > TEMPO_MAXIMO_ATENDIMENTO);
            
            printf("⏱️  Funcionário %d iniciou pedido %d\n", funcionarios[i].id, pedido->id);
            printf("   Chegada: %ds | Término previsto: %ds | Duração: %ds\n", 
                   pedido->tempo_chegada, funcionarios[i].tempo_termino, tempo_total);
        }
    }
    
    // Liberar funcionários que terminaram
    for (int i = 0; i < 13; i++) {
        if (funcionarios[i].ocupado && tempo_atual >= funcionarios[i].tempo_termino) {
            Pedido* pedido = funcionarios[i].pedido_atual;
            int tempo_total = pedido->tempo_conclusao - pedido->tempo_chegada;
            
            printf("\n✅ PEDIDO %d CONCLUÍDO:\n", pedido->id);
            printf("   Tempo total: %d segundos\n", tempo_total);
            printf("   Status: %s\n", pedido->atrasado ? "❌ ATRASADO (reembolso devido)" : "✅ DENTRO DO PRAZO");
            printf("   Funcionário %d liberado\n\n", funcionarios[i].id);
            
            // Liberar memória
            free(pedido->sanduiche);
            if (pedido->batata) free(pedido->batata);
            free(pedido->bebida1);
            if (pedido->bebida2) free(pedido->bebida2);
            free(pedido);
            
            funcionarios[i].ocupado = 0;
            funcionarios[i].pedido_atual = NULL;
        }
    }
}

void simular_casos_pre_definidos() {
    FilaPedidos* fila_pedidos = criar_fila();
    Funcionario funcionarios[13];
    Equipamento equipamentos[3];
    
    inicializar_funcionarios(funcionarios);
    inicializar_equipamentos(equipamentos);
    
    int tempo_simulacao = 1000; // 1000 segundos de simulação
    int pedidos_criados = 0;
    int pedidos_concluidos = 0;
    int pedidos_atrasados = 0;
    
    printf("🎯 SIMULAÇÃO COM 5 CASOS PRÉ-DEFINIDOS\n");
    printf("=========================================\n\n");
    
    // Criar os 5 casos predefinidos em tempos específicos
    int tempos_chegada[] = {0, 100, 200, 300, 400}; // Chegada espaçada
    
    for (int i = 0; i < 5; i++) {
        Pedido* pedido = criar_pedido_pre_definido(i + 1, i + 1, tempos_chegada[i]);
        enfileirar(fila_pedidos, pedido);
        pedidos_criados++;
    }
    
    printf("📊 INICIANDO PROCESSAMENTO DOS PEDIDOS...\n\n");
    
    for (int tempo_atual = 0; tempo_atual < tempo_simulacao; tempo_atual++) {
        // Processar pedidos a cada segundo
        processar_pedidos(fila_pedidos, funcionarios, equipamentos, tempo_atual);
        
        // Contar pedidos concluídos
        for (int i = 0; i < 13; i++) {
            if (funcionarios[i].ocupado && tempo_atual == funcionarios[i].tempo_termino) {
                pedidos_concluidos++;
                if (funcionarios[i].pedido_atual && funcionarios[i].pedido_atual->atrasado) {
                    pedidos_atrasados++;
                }
            }
        }
        
        // Parar se todos os pedidos foram concluídos
        if (pedidos_concluidos >= 5 && fila_vazia(fila_pedidos)) {
            printf("\n🎉 TODOS OS PEDIDOS FORAM CONCLUÍDOS!\n");
            break;
        }
    }
    
    // Relatório final detalhado
    printf("\n📈 RELATÓRIO FINAL DETALHADO\n");
    printf("===============================\n");
    printf("Total de pedidos criados: %d\n", pedidos_criados);
    printf("Total de pedidos concluídos: %d\n", pedidos_concluidos);
    printf("Pedidos atrasados: %d\n", pedidos_atrasados);
    printf("Taxa de sucesso: %.1f%%\n", ((pedidos_criados - pedidos_atrasados) * 100.0) / pedidos_criados);
    
    if (pedidos_atrasados > 0) {
        printf("\n💸 PREJUÍZO ESTIMADO POR ATRASOS:\n");
        printf("   Supondo valor médio de R$ 25,00 por pedido:\n");
        printf("   Prejuízo total: R$ %.2f\n", pedidos_atrasados * 25.0);
    }
    
    // Liberar memória de pedidos não processados
    while (!fila_vazia(fila_pedidos)) {
        Pedido* pedido = desenfileirar(fila_pedidos);
        free(pedido->sanduiche);
        if (pedido->batata) free(pedido->batata);
        free(pedido->bebida1);
        if (pedido->bebida2) free(pedido->bebida2);
        free(pedido);
    }
    free(fila_pedidos);
    
    printf("\n🏁 SIMULAÇÃO CONCLUÍDA!\n");
}
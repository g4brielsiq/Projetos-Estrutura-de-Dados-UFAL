#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_PEDIDOS 100
#define MAX_ITENS 10
#define MAX_FUNCIONARIOS 13
#define TEMPO_MAXIMO 300

// Estruturas atualizadas
typedef struct {
    char nome[50];
    int tempo_preparo;
    char tipo;
    int em_preparo;
    int tempo_restante;
    int pronto; // 0 - não pronto, 1 - pronto
} Item;

typedef struct {
    int numero;
    Item itens[MAX_ITENS];
    int num_itens;
    int tempo_chegada;
    int tempo_pronto;
    int status;
    int itens_prontos;
    int em_montagem;
} Pedido;

typedef struct NoFila {
    Pedido *pedido;
    Item *item;
    struct NoFila *prox;
} NoFila;

typedef struct {
    NoFila *inicio;
    NoFila *fim;
    int tamanho;
} Fila;

typedef struct {
    int id;
    char habilidades[100];
    int ocupado;
    Pedido *pedido_atual;
    Item *item_atual;
    int tempo_restante;
    char tarefa_atual[50];
} Funcionario;

// Variáveis globais
Fila fila_sanduiches;
Fila fila_batatas;
Fila fila_bebidas;
Fila fila_montagem;
Pedido pedidos[MAX_PEDIDOS];
int num_pedidos = 0;
int tempo_atual = 0;
Funcionario funcionarios[MAX_FUNCIONARIOS];

// Protótipos
void inicializar_fila(Fila *f);
void enfileirar(Fila *f, Pedido *pedido, Item *item);
NoFila* desenfileirar(Fila *f);
void inicializar_funcionarios();
void processar_tarefas();
void mostrar_estado_cozinha();
int pedido_pronto(Pedido *pedido);
void finalizar_pedido(Pedido *pedido);
void criar_pedido_predefinido(int numero);
void menu_interativo();
void verificar_montagem_pedidos();

// Implementação das funções
void inicializar_fila(Fila *f) {
    f->inicio = NULL;
    f->fim = NULL;
    f->tamanho = 0;
}

void enfileirar(Fila *f, Pedido *pedido, Item *item) {
    NoFila *novo = (NoFila*)malloc(sizeof(NoFila));
    novo->pedido = pedido;
    novo->item = item;
    novo->prox = NULL;
    
    if (f->fim == NULL) {
        f->inicio = novo;
        f->fim = novo;
    } else {
        f->fim->prox = novo;
        f->fim = novo;
    }
    f->tamanho++;
}

NoFila* desenfileirar(Fila *f) {
    if (f->inicio == NULL) return NULL;
    
    NoFila *removido = f->inicio;
    f->inicio = f->inicio->prox;
    
    if (f->inicio == NULL) {
        f->fim = NULL;
    }
    
    f->tamanho--;
    return removido;
}

void inicializar_funcionarios() {
    // 5 funcionários habilitados a fazer sanduíches
    strcpy(funcionarios[0].habilidades, "Sanduiche,Batata");
    strcpy(funcionarios[1].habilidades, "Sanduiche,Batata");
    strcpy(funcionarios[2].habilidades, "Sanduiche,Suco");
    strcpy(funcionarios[3].habilidades, "Sanduiche");
    strcpy(funcionarios[4].habilidades, "Sanduiche");
    
    // 2 funcionários habilitados a fazer batatas fritas
    strcpy(funcionarios[5].habilidades, "Batata");
    strcpy(funcionarios[6].habilidades, "Batata,Sanduiche");
    
    // 1 funcionário habilitado a fazer bebidas
    strcpy(funcionarios[7].habilidades, "Bebida,Montagem");
    
    // 1 funcionário habilitado a montar bandeja
    strcpy(funcionarios[8].habilidades, "Montagem");
    
    // 2 funcionários habilitados a separação
    strcpy(funcionarios[9].habilidades, "Separacao,Caixa");
    strcpy(funcionarios[10].habilidades, "Separacao,Sanduiche");
    
    // 2 funcionários habilitados a caixa
    strcpy(funcionarios[11].habilidades, "Caixa");
    strcpy(funcionarios[12].habilidades, "Caixa,Bebida");
    
    for (int i = 0; i < MAX_FUNCIONARIOS; i++) {
        funcionarios[i].id = i + 1;
        funcionarios[i].ocupado = 0;
        funcionarios[i].pedido_atual = NULL;
        funcionarios[i].item_atual = NULL;
        funcionarios[i].tempo_restante = 0;
        strcpy(funcionarios[i].tarefa_atual, "Livre");
    }
}

int tem_habilidade(Funcionario *f, const char *habilidade) {
    return strstr(f->habilidades, habilidade) != NULL;
}

// FUNÇÃO CRÍTICA ATUALIZADA - Agora com lógica de priorização por pedido
void processar_tarefas() {
    // Primeiro: processar funcionários ocupados
    for (int i = 0; i < MAX_FUNCIONARIOS; i++) {
        if (funcionarios[i].ocupado) {
            funcionarios[i].tempo_restante--;
            
            if (funcionarios[i].tempo_restante <= 0) {
                // Tarefa concluída!
                if (funcionarios[i].item_atual != NULL) {
                    funcionarios[i].item_atual->pronto = 1;
                    funcionarios[i].item_atual->em_preparo = 0;
                    funcionarios[i].pedido_atual->itens_prontos++;
                    
                    printf("✅ Item '%s' do Pedido %d pronto!\n", 
                           funcionarios[i].item_atual->nome, 
                           funcionarios[i].pedido_atual->numero);
                }
                
                // Se era montagem, finalizar pedido
                if (strstr(funcionarios[i].tarefa_atual, "Montando")) {
                    finalizar_pedido(funcionarios[i].pedido_atual);
                }
                
                // Liberar funcionário
                funcionarios[i].ocupado = 0;
                funcionarios[i].pedido_atual = NULL;
                funcionarios[i].item_atual = NULL;
                strcpy(funcionarios[i].tarefa_atual, "Livre");
            }
        }
    }
    
    // Segundo: atribuir novas tarefas a funcionários livres
    // Estratégia: tentar completar pedidos mais antigos primeiro
    
    // Para cada pedido em andamento, verificar itens pendentes
    for (int p = 0; p < num_pedidos; p++) {
        if (pedidos[p].status == 1) { // Pedido em preparo
            for (int i = 0; i < pedidos[p].num_itens; i++) {
                if (!pedidos[p].itens[i].pronto && !pedidos[p].itens[i].em_preparo) {
                    // Item precisa ser preparado
                    char *tipo_tarefa = "";
                    Fila *fila_alvo = NULL;
                    
                    if (strstr(pedidos[p].itens[i].nome, "Sanduiche")) {
                        tipo_tarefa = "Sanduiche";
                        fila_alvo = &fila_sanduiches;
                    } else if (strstr(pedidos[p].itens[i].nome, "Batata")) {
                        tipo_tarefa = "Batata";
                        fila_alvo = &fila_batatas;
                    } else {
                        tipo_tarefa = "Bebida";
                        fila_alvo = &fila_bebidas;
                    }
                    
                    // Verificar limites de equipamentos
                    int limite_atingido = 0;
                    if (strcmp(tipo_tarefa, "Sanduiche") == 0) {
                        int sanduiches_na_chapa = 0;
                        for (int f = 0; f < MAX_FUNCIONARIOS; f++) {
                            if (funcionarios[f].ocupado && strstr(funcionarios[f].tarefa_atual, "Sanduiche")) {
                                sanduiches_na_chapa++;
                            }
                        }
                        if (sanduiches_na_chapa >= 3) limite_atingido = 1;
                    } else if (strcmp(tipo_tarefa, "Batata") == 0) {
                        int batatas_na_peneira = 0;
                        for (int f = 0; f < MAX_FUNCIONARIOS; f++) {
                            if (funcionarios[f].ocupado && strstr(funcionarios[f].tarefa_atual, "Batata")) {
                                batatas_na_peneira++;
                            }
                        }
                        if (batatas_na_peneira >= 2) limite_atingido = 1;
                    }
                    
                    if (!limite_atingido) {
                        // Procurar funcionário livre com habilidade
                        for (int f = 0; f < MAX_FUNCIONARIOS; f++) {
                            if (!funcionarios[f].ocupado && tem_habilidade(&funcionarios[f], tipo_tarefa)) {
                                // Atribuir tarefa
                                funcionarios[f].ocupado = 1;
                                funcionarios[f].pedido_atual = &pedidos[p];
                                funcionarios[f].item_atual = &pedidos[p].itens[i];
                                funcionarios[f].tempo_restante = pedidos[p].itens[i].tempo_preparo;
                                
                                if (strcmp(tipo_tarefa, "Sanduiche") == 0) {
                                    strcpy(funcionarios[f].tarefa_atual, "Fazendo Sanduiche");
                                } else if (strcmp(tipo_tarefa, "Batata") == 0) {
                                    strcpy(funcionarios[f].tarefa_atual, "Fritando Batatas");
                                } else {
                                    strcpy(funcionarios[f].tarefa_atual, "Preparando Bebida");
                                }
                                
                                pedidos[p].itens[i].em_preparo = 1;
                                printf("👨‍🍳 Funcionário %d começou: %s (Pedido %d)\n", 
                                       funcionarios[f].id, funcionarios[f].tarefa_atual, pedidos[p].numero);
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
    
    // Terceiro: verificar se pedidos estão prontos para montagem
    verificar_montagem_pedidos();
}

// Nova função para verificar montagem
void verificar_montagem_pedidos() {
    for (int p = 0; p < num_pedidos; p++) {
        if (pedidos[p].status == 1 && !pedidos[p].em_montagem && pedido_pronto(&pedidos[p])) {
            // Pedido pronto para montagem
            int montador_encontrado = 0;
            
            // Procurar montador livre
            for (int f = 0; f < MAX_FUNCIONARIOS; f++) {
                if (!funcionarios[f].ocupado && tem_habilidade(&funcionarios[f], "Montagem")) {
                    funcionarios[f].ocupado = 1;
                    funcionarios[f].pedido_atual = &pedidos[p];
                    funcionarios[f].item_atual = NULL;
                    funcionarios[f].tempo_restante = 30; // 30 segundos para montar
                    strcpy(funcionarios[f].tarefa_atual, "Montando Bandeja");
                    pedidos[p].em_montagem = 1;
                    montador_encontrado = 1;
                    
                    printf("📦 Montador %d começou montagem do Pedido %d\n", 
                           funcionarios[f].id, pedidos[p].numero);
                    break;
                }
            }
            
            if (!montador_encontrado) {
                // Se não encontrou montador livre, coloca na fila de montagem
                enfileirar(&fila_montagem, &pedidos[p], NULL);
                printf("⏳ Pedido %d aguardando montagem (fila: %d)\n", 
                       pedidos[p].numero, fila_montagem.tamanho);
            }
        }
    }
    
    // Processar fila de montagem se houver montadores livres
    if (fila_montagem.tamanho > 0) {
        for (int f = 0; f < MAX_FUNCIONARIOS; f++) {
            if (!funcionarios[f].ocupado && tem_habilidade(&funcionarios[f], "Montagem")) {
                NoFila *tarefa = desenfileirar(&fila_montagem);
                if (tarefa != NULL && tarefa->pedido->status == 1 && !tarefa->pedido->em_montagem) {
                    funcionarios[f].ocupado = 1;
                    funcionarios[f].pedido_atual = tarefa->pedido;
                    funcionarios[f].item_atual = NULL;
                    funcionarios[f].tempo_restante = 30;
                    strcpy(funcionarios[f].tarefa_atual, "Montando Bandeja");
                    tarefa->pedido->em_montagem = 1;
                    
                    printf("📦 Montador %d começou montagem do Pedido %d (da fila)\n", 
                           funcionarios[f].id, tarefa->pedido->numero);
                }
                free(tarefa);
            }
        }
    }
}

int pedido_pronto(Pedido *pedido) {
    for (int i = 0; i < pedido->num_itens; i++) {
        if (!pedido->itens[i].pronto) {
            return 0;
        }
    }
    return 1;
}

void finalizar_pedido(Pedido *pedido) {
    pedido->status = 2;
    pedido->tempo_pronto = tempo_atual;
    int tempo_total = pedido->tempo_pronto - pedido->tempo_chegada;
    
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║                     🎉 PEDIDO PRONTO! 🎉                    ║\n");
    printf("╠══════════════════════════════════════════════════════════════╣\n");
    printf("║ Pedido: %3d                                                ║\n", pedido->numero);
    printf("║ Tempo de preparo: %3d segundos                            ║\n", tempo_total);
    
    if (tempo_total > TEMPO_MAXIMO) {
        printf("║ ⚠️  ATENÇÃO: Pedido excedeu 5 minutos! Reembolso necessário. ║\n");
    } else {
        printf("║ ✅ Pedido entregue dentro do prazo!                        ║\n");
    }
    
    printf("║ Itens:                                                    ║\n");
    for (int i = 0; i < pedido->num_itens; i++) {
        printf("║   • %-50s ║\n", pedido->itens[i].nome);
    }
    printf("╚══════════════════════════════════════════════════════════════╝\n");
}

// Resto das funções permanecem similares, mas atualizadas para a nova lógica
void mostrar_estado_cozinha() {
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║                 🏪 COZINHA BIG PAPÃO 🏪                    ║\n");
    printf("╠══════════════════════════════════════════════════════════════╣\n");
    printf("║ Tempo: %3d segundos                                      ║\n", tempo_atual);
    printf("╠══════════════════════════════════════════════════════════════╣\n");
    printf("║ 📊 ESTATÍSTICAS:                                           ║\n");
    printf("║   Pedidos ativos: %2d   Em montagem: %2d                   ║\n", 
           num_pedidos, fila_montagem.tamanho);
    printf("╠══════════════════════════════════════════════════════════════╣\n");
    printf("║ 👨‍🍳 FUNCIONÁRIOS:                                          ║\n");
    
    for (int i = 0; i < MAX_FUNCIONARIOS; i++) {
        char detalhes[50];
        if (funcionarios[i].ocupado) {
            if (funcionarios[i].pedido_atual != NULL) {
                sprintf(detalhes, "Ped %d - %ds", 
                        funcionarios[i].pedido_atual->numero, 
                        funcionarios[i].tempo_restante);
            } else {
                strcpy(detalhes, "???");
            }
            printf("║ %2d. %-20s %-15s           ║\n", 
                   funcionarios[i].id, funcionarios[i].tarefa_atual, detalhes);
        } else {
            printf("║ %2d. %-20s %-15s           ║\n", 
                   funcionarios[i].id, "Livre", funcionarios[i].habilidades);
        }
    }
    printf("╠══════════════════════════════════════════════════════════════╣\n");
    printf("║ 📦 PEDIDOS:                                                ║\n");
    
    for (int i = 0; i < num_pedidos; i++) {
        char status[20];
        switch (pedidos[i].status) {
            case 0: strcpy(status, "⏳ Esperando"); break;
            case 1: 
                if (pedidos[i].em_montagem) {
                    strcpy(status, "📦 Em montagem");
                } else {
                    sprintf(status, "👨‍🍳 %d/%d itens", 
                           pedidos[i].itens_prontos, pedidos[i].num_itens);
                }
                break;
            case 2: strcpy(status, "✅ Pronto"); break;
        }
        
        printf("║ Pedido %2d: %-20s | Chegada: %3ds           ║\n", 
               pedidos[i].numero, status, pedidos[i].tempo_chegada);
    }
    printf("╚══════════════════════════════════════════════════════════════╝\n");
}

void criar_pedido_predefinido(int numero) {
    if (num_pedidos >= MAX_PEDIDOS) return;
    
    Pedido *p = &pedidos[num_pedidos];
    p->numero = numero;
    p->tempo_chegada = tempo_atual;
    p->status = 1; // Agora vai direto para "em preparo"
    p->num_itens = 0;
    p->itens_prontos = 0;
    p->em_montagem = 0;
    
    // Inicializar itens
    switch (numero) {
        case 1:
            strcpy(p->itens[0].nome, "Sanduiche Simples");
            p->itens[0].tempo_preparo = 58;
            p->itens[0].tipo = 'C';
            p->itens[0].em_preparo = 0;
            p->itens[0].pronto = 0;
            p->num_itens++;
            
            strcpy(p->itens[1].nome, "Refrigerante");
            p->itens[1].tempo_preparo = 5;
            p->itens[1].tipo = 'B';
            p->itens[1].em_preparo = 0;
            p->itens[1].pronto = 0;
            p->num_itens++;
            break;
            
        case 2:
            strcpy(p->itens[0].nome, "Sanduiche Medio");
            p->itens[0].tempo_preparo = 88;
            p->itens[0].tipo = 'C';
            p->itens[0].em_preparo = 0;
            p->itens[0].pronto = 0;
            p->num_itens++;
            
            strcpy(p->itens[1].nome, "Batata Frita");
            p->itens[1].tempo_preparo = 190;
            p->itens[1].tipo = 'C';
            p->itens[1].em_preparo = 0;
            p->itens[1].pronto = 0;
            p->num_itens++;
            
            strcpy(p->itens[2].nome, "Suco");
            p->itens[2].tempo_preparo = 38;
            p->itens[2].tipo = 'B';
            p->itens[2].em_preparo = 0;
            p->itens[2].pronto = 0;
            p->num_itens++;
            break;
            
        // ... outros casos similares
    }
    
    num_pedidos++;
    printf("✅ Pedido %d criado com %d itens! (Chegou em %ds)\n", numero, p->num_itens, tempo_atual);
}

// ... (menu_interativo e main permanecem similares, mas adaptadas)

int main() {
    inicializar_fila(&fila_sanduiches);
    inicializar_fila(&fila_batatas);
    inicializar_fila(&fila_bebidas);
    inicializar_fila(&fila_montagem);
    inicializar_funcionarios();
    
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║           🏪 BIG PAPÃO - SIMULAÇÃO INTELIGENTE 🏪          ║\n");
    printf("║        (Funcionários trabalham por pedido completo)         ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    
    // Criar alguns pedidos de exemplo
    for (int i = 1; i <= 3; i++) {
        criar_pedido_predefinido(i);
    }
    
    // Simular
    while (tempo_atual < 400) {
        tempo_atual++;
        processar_tarefas();
        
        if (tempo_atual % 30 == 0) {
            mostrar_estado_cozinha();
        }
        
        // Parar se todos os pedidos estiverem prontos
        int todos_prontos = 1;
        for (int i = 0; i < num_pedidos; i++) {
            if (pedidos[i].status != 2) {
                todos_prontos = 0;
                break;
            }
        }
        if (todos_prontos) break;
    }
    
    printf("\n=== SIMULAÇÃO CONCLUÍDA ===\n");
    return 0;
}
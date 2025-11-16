#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

// =============================================================================
// 1. DEFINIÇÕES E ESTRUTURAS DE DADOS
// (A única mudança aqui é o 'loja_id' na struct Cozinha)
// =============================================================================

#define NUM_FUNCIONARIOS 13
#define TEMPO_MAX_ATENDIMENTO 300
// O TEMPO_ATENDIMENTO_CAIXA foi removido, pois agora simulamos o tempo ocioso com a entrada "0".

typedef enum {
    ITEM_SANDUICHE_SIMPLES, ITEM_SANDUICHE_MEDIO, ITEM_SANDUICHE_ELABORADO,
    ITEM_BATATA_FRITA, ITEM_REFRIGERANTE, ITEM_SUCO, ITEM_MILK_SHAKE,
    ITEM_MONTAGEM
} TipoItem;

const char *NOMES_ITENS[] = {
    "Sanduiche Simples", "Sanduiche Medio", "Sanduiche Elaborado",
    "Batata Frita", "Refrigerante", "Suco", "Milk Shake", "Montagem de Bandeja"
};

typedef enum {
    STATUS_NA_FILA, STATUS_EM_PREPARO, STATUS_AGUARDANDO_MONTAGEM,
    STATUS_EM_MONTAGEM, STATUS_CONCLUIDO_NO_PRAZO, STATUS_CONCLUIDO_ATRASADO
} StatusPedido;

typedef enum {
    HABILIDADE_SANDUICHE = 1 << 0,
    HABILIDADE_BATATA    = 1 << 1,
    HABILIDADE_BEBIDAS   = 1 << 2,
    HABILIDADE_MONTAGEM  = 1 << 3
} Habilidade;

const int TEMPOS_DE_PREPARO[] = {58, 88, 105, 190, 5, 38, 60, 30};

typedef struct Tarefa {
    int pedido_id;
    TipoItem tipo_item;
    int tempo_conclusao;
} Tarefa;

typedef struct Pedido {
    int id;
    int tempo_chegada;
    StatusPedido status;
    int tarefas_preparo_restantes;
    TipoItem *itens;
    int num_itens;
    struct Pedido *proximo;
} Pedido;

typedef struct {
    int id;
    unsigned int habilidades;
    int livre_a_partir_de;
} Funcionario;

typedef struct {
    int capacidade_por_funcionario;
    int validade_produto_min;
} Equipamento;

typedef struct {
    int loja_id; // NOVO: Identificador (1 ou 2) para os logs de impressão
    int tempo_atual;
    Funcionario funcionarios[NUM_FUNCIONARIOS];
    Equipamento chapa;
    Equipamento fritadeira;
    Equipamento liquidificador;
    Pedido *pedidos_na_fila_espera;
    Pedido **pedidos_em_andamento;
    int num_pedidos_em_andamento;
    Tarefa *tarefas_na_fila_preparo;
    int num_tarefas_na_fila_preparo;
    Tarefa *tarefas_em_execucao;
    int num_tarefas_em_execucao;
    int total_pedidos_criados;
    int atendidos_no_prazo;
    int atendidos_com_atraso;
} Cozinha;

// =============================================================================
// 2. FUNÇÕES DE GERENCIAMENTO DA COZINHA (Refatoradas)
// =============================================================================

// Modificada para aceitar um ID de loja para clareza nos logs
void inicializar_cozinha(Cozinha *cozinha, int loja_id)
{
    memset(cozinha, 0, sizeof(Cozinha));
    cozinha->loja_id = loja_id;

    printf("--- Configurando a Cozinha da Loja %d ---\n", loja_id);
    cozinha->chapa.capacidade_por_funcionario = 3;
    cozinha->fritadeira.capacidade_por_funcionario = 2;
    cozinha->fritadeira.validade_produto_min = 60;
    cozinha->liquidificador.capacidade_por_funcionario = 4;
    cozinha->liquidificador.validade_produto_min = 30;
    printf("Configuracao da Loja %d concluida.\n\n", loja_id);

    // Mapeamento dos 13 funcionários (idêntico para ambas as lojas)
    cozinha->funcionarios[0] = (Funcionario){(loja_id * 100) + 1, HABILIDADE_SANDUICHE | HABILIDADE_BATATA, 0};
    cozinha->funcionarios[1] = (Funcionario){(loja_id * 100) + 2, HABILIDADE_SANDUICHE | HABILIDADE_BATATA, 0};
    cozinha->funcionarios[2] = (Funcionario){(loja_id * 100) + 3, HABILIDADE_SANDUICHE | HABILIDADE_BEBIDAS, 0};
    cozinha->funcionarios[3] = (Funcionario){(loja_id * 100) + 4, HABILIDADE_SANDUICHE, 0};
    cozinha->funcionarios[4] = (Funcionario){(loja_id * 100) + 5, HABILIDADE_SANDUICHE, 0};
    cozinha->funcionarios[5] = (Funcionario){(loja_id * 100) + 6, HABILIDADE_SANDUICHE | HABILIDADE_BATATA, 0};
    cozinha->funcionarios[6] = (Funcionario){(loja_id * 100) + 7, HABILIDADE_BATATA, 0};
    cozinha->funcionarios[7] = (Funcionario){(loja_id * 100) + 8, HABILIDADE_BEBIDAS | HABILIDADE_MONTAGEM, 0};
    cozinha->funcionarios[8] = (Funcionario){(loja_id * 100) + 9, HABILIDADE_MONTAGEM, 0};
    cozinha->funcionarios[9] = (Funcionario){(loja_id * 100) + 10, 0, 0};
    cozinha->funcionarios[10] = (Funcionario){(loja_id * 100) + 11, HABILIDADE_SANDUICHE, 0};
    cozinha->funcionarios[11] = (Funcionario){(loja_id * 100) + 12, HABILIDADE_BEBIDAS, 0};
    cozinha->funcionarios[12] = (Funcionario){(loja_id * 100) + 13, 0, 0};
}

// (Função idêntica à anterior)
void adicionar_pedido_na_fila_espera(Cozinha *c, Pedido *novo_pedido)
{
    novo_pedido->proximo = NULL;
    if (c->pedidos_na_fila_espera == NULL) {
        c->pedidos_na_fila_espera = novo_pedido;
    } else {
        Pedido *atual = c->pedidos_na_fila_espera;
        while (atual->proximo != NULL)
            atual = atual->proximo;
        atual->proximo = novo_pedido;
    }
}

// (Função idêntica à anterior)
void limpar_cozinha(Cozinha *c)
{
    free(c->tarefas_em_execucao);
    free(c->tarefas_na_fila_preparo);
    for (int i = 0; i < c->num_pedidos_em_andamento; i++) {
        free(c->pedidos_em_andamento[i]->itens);
        free(c->pedidos_em_andamento[i]);
    }
    free(c->pedidos_em_andamento);
    Pedido *atual = c->pedidos_na_fila_espera;
    while (atual != NULL) {
        Pedido *temp = atual;
        atual = atual->proximo;
        free(temp->itens);
        free(temp);
    }
}

// (Função idêntica à anterior)
Pedido *encontrar_pedido_em_andamento(Cozinha *c, int pedido_id)
{
    for (int i = 0; i < c->num_pedidos_em_andamento; i++) {
        if (c->pedidos_em_andamento[i]->id == pedido_id)
            return c->pedidos_em_andamento[i];
    }
    return NULL;
}

// (Função idêntica à anterior)
void imprimir_composicao_bandejas(Pedido *pedido)
{
    printf("\n--- Composicao do Pedido #%d ---\n", pedido->id);

    TipoItem itens_comer[pedido->num_itens];
    TipoItem itens_beber[pedido->num_itens];
    int count_comer = 0;
    int count_beber = 0;

    for (int i = 0; i < pedido->num_itens; i++) {
        if (pedido->itens[i] <= ITEM_BATATA_FRITA) {
            itens_comer[count_comer++] = pedido->itens[i];
        } else {
            itens_beber[count_beber++] = pedido->itens[i];
        }
    }

    int bandeja_num = 1;
    int idx_comer_atual = 0;
    int idx_beber_atual = 0;

    while (idx_comer_atual < count_comer || idx_beber_atual < count_beber) {
        printf("Bandeja %d:\n", bandeja_num++);
        for (int i = 0; i < 2 && idx_comer_atual < count_comer; i++) {
            printf("  - %s\n", NOMES_ITENS[itens_comer[idx_comer_atual++]]);
        }
        for (int i = 0; i < 2 && idx_beber_atual < count_beber; i++) {
            printf("  - %s\n", NOMES_ITENS[itens_beber[idx_beber_atual++]]);
        }
    }
    printf("---------------------------------\n");
}

// =============================================================================
// 3. LÓGICA DE ENTRADA E ROTEAMENTO (NOVAS FUNÇÕES)
// =============================================================================

// Esta é uma versão modificada da antiga 'coletar_pedidos'.
// Ela não tem mais o loop de "quantos pedidos", apenas cria UM pedido e o retorna.
Pedido* criar_pedido_interativo(int id_pedido, int tempo_chegada)
{
    printf("\n--- Registrando Pedido #%d (chegada em t=%ds) ---\n", id_pedido, tempo_chegada);

    TipoItem *itens_do_pedido = NULL;
    int qtd_itens = 0;
    int escolha = -1;

    do {
        printf("Escolha os itens para o pedido #%d:\n", id_pedido);
        for (int j = 0; j < 7; j++) // Mostra os 7 itens do cardápio
            printf("  %d. %s\n", j + 1, NOMES_ITENS[j]);
        printf("  0. Finalizar Pedido\n");
        printf("Digite o numero do item: ");
        scanf("%d", &escolha);

        if (escolha >= 1 && escolha <= 7) {
            int quantidade;
            printf("Digite a quantidade do item: ");
            scanf("%d", &quantidade);
            if (quantidade < 1) {
                printf("Quantidade invalida, adicionando 1 unidade.\n");
                quantidade = 1;
            }
            for (int k = 0; k < quantidade; k++) {
                qtd_itens++;
                itens_do_pedido = (TipoItem *)realloc(itens_do_pedido, sizeof(TipoItem) * qtd_itens);
                itens_do_pedido[qtd_itens - 1] = (TipoItem)(escolha - 1);
            }
            printf(">> (%d) - '%s' adicionado(s) ao pedido.\n\n", quantidade, NOMES_ITENS[escolha - 1]);
        } else if (escolha != 0) {
            printf("Opcao invalida! Tente novamente.\n\n");
        }
    } while (escolha != 0);

    if (qtd_itens > 0) {
        Pedido *novo_pedido = (Pedido *)malloc(sizeof(Pedido));
        novo_pedido->id = id_pedido;
        novo_pedido->tempo_chegada = tempo_chegada; // O tempo de chegada é o tempo global atual
        novo_pedido->status = STATUS_NA_FILA;
        novo_pedido->itens = itens_do_pedido;
        novo_pedido->num_itens = qtd_itens;
        novo_pedido->tarefas_preparo_restantes = qtd_itens;
        
        printf(">> Pedido #%d criado com %d itens.\n", id_pedido, qtd_itens);
        return novo_pedido; // Retorna o pedido criado
    } else {
        printf(">> Pedido #%d cancelado por nao ter itens.\n", id_pedido);
        return NULL; // Retorna nulo se o pedido foi cancelado
    }
}

// NOVA FUNÇÃO: O Roteador do iFood
// Decide para qual loja enviar o pedido com base na menor fila de tarefas pendentes.
void rotear_pedido_ifood(Pedido *pedido, Cozinha *loja1, Cozinha *loja2)
{
    if (pedido == NULL) return;

    // A Métrica de Decisão: qual cozinha tem menos tarefas na fila de preparo.
    int carga_loja_1 = loja1->num_tarefas_na_fila_preparo;
    int carga_loja_2 = loja2->num_tarefas_na_fila_preparo;

    printf("[Roteador iFood] Carga Loja 1: %d tarefas | Carga Loja 2: %d tarefas.\n", carga_loja_1, carga_loja_2);

    if (carga_loja_1 <= carga_loja_2) {
        printf("[Roteador iFood] Enviando Pedido #%d para a Loja 1 (menos ocupada).\n", pedido->id);
        adicionar_pedido_na_fila_espera(loja1, pedido);
    } else {
        printf("[Roteador iFood] Enviando Pedido #%d para a Loja 2 (menos ocupada).\n", pedido->id);
        adicionar_pedido_na_fila_espera(loja2, pedido);
    }
}

// =============================================================================
// 4. O NOVO MOTOR DA SIMULAÇÃO (Refatorado)
// =============================================================================

// (Função idêntica à anterior, mas com o printf modificado para mostrar a Loja)
void despachar_tarefas(Cozinha *c)
{
    Habilidade habilidades[] = {HABILIDADE_SANDUICHE, HABILIDADE_BATATA, HABILIDADE_BEBIDAS, HABILIDADE_MONTAGEM};
    Equipamento *equipamentos[] = {&c->chapa, &c->fritadeira, &c->liquidificador, NULL};

    for (int h = 0; h < 4; h++) {
        int funcs_livres[NUM_FUNCIONARIOS];
        int num_funcs_livres = 0;
        for (int i = 0; i < NUM_FUNCIONARIOS; i++) {
            if ((c->funcionarios[i].habilidades & habilidades[h]) && (c->funcionarios[i].livre_a_partir_de <= c->tempo_atual)) {
                funcs_livres[num_funcs_livres++] = i;
            }
        }

        if (num_funcs_livres == 0) continue;

        int capacidade_total = num_funcs_livres * (equipamentos[h] ? equipamentos[h]->capacidade_por_funcionario : 1);
        int tarefas_alocadas_nesta_rodada = 0;

        for (int i = c->num_tarefas_na_fila_preparo - 1; i >= 0; i--) {
            if (tarefas_alocadas_nesta_rodada >= capacidade_total) break;

            Tarefa *tarefa_atual = &c->tarefas_na_fila_preparo[i];
            int tarefa_corresponde = 0;

            switch (habilidades[h]) {
                case HABILIDADE_SANDUICHE:
                    if (tarefa_atual->tipo_item >= ITEM_SANDUICHE_SIMPLES && tarefa_atual->tipo_item <= ITEM_SANDUICHE_ELABORADO) tarefa_corresponde = 1;
                    break;
                case HABILIDADE_BATATA:
                    if (tarefa_atual->tipo_item == ITEM_BATATA_FRITA) tarefa_corresponde = 1;
                    break;
                case HABILIDADE_BEBIDAS:
                    if (tarefa_atual->tipo_item >= ITEM_REFRIGERANTE && tarefa_atual->tipo_item <= ITEM_MILK_SHAKE) tarefa_corresponde = 1;
                    break;
                case HABILIDADE_MONTAGEM:
                    if (tarefa_atual->tipo_item == ITEM_MONTAGEM) tarefa_corresponde = 1;
                    break;
            }

            if (tarefa_corresponde) {
                int id_func_alocado = funcs_livres[tarefas_alocadas_nesta_rodada % num_funcs_livres];
                tarefa_atual->tempo_conclusao = c->tempo_atual + TEMPOS_DE_PREPARO[tarefa_atual->tipo_item];
                c->funcionarios[id_func_alocado].livre_a_partir_de = tarefa_atual->tempo_conclusao;

                c->num_tarefas_em_execucao++;
                c->tarefas_em_execucao = (Tarefa *)realloc(c->tarefas_em_execucao, sizeof(Tarefa) * c->num_tarefas_em_execucao);
                c->tarefas_em_execucao[c->num_tarefas_em_execucao - 1] = *tarefa_atual;

                printf("[Loja %d | Tempo: %ds] Func. #%d iniciou %s do Pedido #%d (conclui em %ds).\n", 
                       c->loja_id, c->tempo_atual, c->funcionarios[id_func_alocado].id, 
                       NOMES_ITENS[tarefa_atual->tipo_item], tarefa_atual->pedido_id, tarefa_atual->tempo_conclusao);

                c->tarefas_na_fila_preparo[i] = c->tarefas_na_fila_preparo[c->num_tarefas_na_fila_preparo - 1];
                c->num_tarefas_na_fila_preparo--;
                tarefas_alocadas_nesta_rodada++;
            }
        }
    }
}

// NOVA FUNÇÃO: Substitui a antiga 'executar_simulacao'.
// Esta função processa TODOS os eventos de UMA cozinha que devem acontecer em UM SEGUNDO.
void atualizar_cozinha_um_passo(Cozinha *cozinha, int tempo_global_atual)
{
    // 1. Sincroniza o relógio desta cozinha com o relógio global.
    cozinha->tempo_atual = tempo_global_atual;

    // 2. Processa tarefas que terminaram NESTE SEGUNDO.
    for (int i = cozinha->num_tarefas_em_execucao - 1; i >= 0; i--) {
        Tarefa *tarefa_concluida = &cozinha->tarefas_em_execucao[i];
        
        if (tarefa_concluida->tempo_conclusao <= cozinha->tempo_atual) {
            Pedido *pedido_pai = encontrar_pedido_em_andamento(cozinha, tarefa_concluida->pedido_id);
            if (!pedido_pai) continue;

            printf("[Loja %d | Tempo: %ds] %s do Pedido #%d foi concluido.\n", 
                   cozinha->loja_id, cozinha->tempo_atual, 
                   NOMES_ITENS[tarefa_concluida->tipo_item], pedido_pai->id);

            if (tarefa_concluida->tipo_item == ITEM_MONTAGEM) {
                int tempo_de_producao = cozinha->tempo_atual - pedido_pai->tempo_chegada;
                if (tempo_de_producao <= TEMPO_MAX_ATENDIMENTO) {
                    cozinha->atendidos_no_prazo++;
                    pedido_pai->status = STATUS_CONCLUIDO_NO_PRAZO;
                    printf(">> [Loja %d] Pedido #%d FINALIZADO com SUCESSO. Tempo de producao: %ds.\n", 
                           cozinha->loja_id, pedido_pai->id, tempo_de_producao);
                } else {
                    cozinha->atendidos_com_atraso++;
                    pedido_pai->status = STATUS_CONCLUIDO_ATRASADO;
                    printf(">> [Loja %d] Pedido #%d FINALIZADO com ATRASO. Tempo de producao: %ds (PREJUIZO).\n", 
                           cozinha->loja_id, pedido_pai->id, tempo_de_producao);
                }
                imprimir_composicao_bandejas(pedido_pai);
            } else {
                pedido_pai->tarefas_preparo_restantes--;
                if (pedido_pai->tarefas_preparo_restantes == 0 && pedido_pai->status == STATUS_EM_PREPARO) {
                    pedido_pai->status = STATUS_AGUARDANDO_MONTAGEM;
                    Tarefa t = {pedido_pai->id, ITEM_MONTAGEM, 0};
                    cozinha->num_tarefas_na_fila_preparo++;
                    cozinha->tarefas_na_fila_preparo = (Tarefa *)realloc(cozinha->tarefas_na_fila_preparo, sizeof(Tarefa) * cozinha->num_tarefas_na_fila_preparo);
                    cozinha->tarefas_na_fila_preparo[cozinha->num_tarefas_na_fila_preparo - 1] = t;
                    printf("[Loja %d | Tempo: %ds] Pedido #%d pronto para montagem.\n", 
                           cozinha->loja_id, cozinha->tempo_atual, pedido_pai->id);
                }
            }
            // Remove a tarefa da lista de execução
            cozinha->tarefas_em_execucao[i] = cozinha->tarefas_em_execucao[cozinha->num_tarefas_em_execucao - 1];
            cozinha->num_tarefas_em_execucao--;
        }
    }

    // 3. Processa a chegada de novos pedidos da fila de espera interna (que foram roteados para cá).
    while (cozinha->pedidos_na_fila_espera != NULL && cozinha->pedidos_na_fila_espera->tempo_chegada <= cozinha->tempo_atual) {
        Pedido *pedido_iniciado = cozinha->pedidos_na_fila_espera;
        cozinha->pedidos_na_fila_espera = cozinha->pedidos_na_fila_espera->proximo;

        pedido_iniciado->status = STATUS_EM_PREPARO;
        cozinha->num_pedidos_em_andamento++;
        cozinha->pedidos_em_andamento = (Pedido **)realloc(cozinha->pedidos_em_andamento, sizeof(Pedido *) * cozinha->num_pedidos_em_andamento);
        cozinha->pedidos_em_andamento[cozinha->num_pedidos_em_andamento - 1] = pedido_iniciado;

        printf("[Loja %d | Tempo: %ds] Iniciando preparo do Pedido #%d.\n", 
               cozinha->loja_id, cozinha->tempo_atual, pedido_iniciado->id);
        
        cozinha->total_pedidos_criados++; // Incrementa o contador da loja
        for (int i = 0; i < pedido_iniciado->num_itens; i++) {
            Tarefa t = {pedido_iniciado->id, pedido_iniciado->itens[i], 0};
            cozinha->num_tarefas_na_fila_preparo++;
            cozinha->tarefas_na_fila_preparo = (Tarefa *)realloc(cozinha->tarefas_na_fila_preparo, sizeof(Tarefa) * cozinha->num_tarefas_na_fila_preparo);
            cozinha->tarefas_na_fila_preparo[cozinha->num_tarefas_na_fila_preparo - 1] = t;
        }
    }

    // 4. Tenta despachar novas tarefas para funcionários que acabaram de ficar livres.
    despachar_tarefas(cozinha);
}

// Função auxiliar para verificar se as cozinhas ainda têm trabalho pendente.
int cozinhas_estao_ocupadas(Cozinha *loja1, Cozinha *loja2)
{
    // Verifica se há pedidos em qualquer fila ou tarefas em qualquer estado em qualquer loja
    int loja1_ocupada = loja1->pedidos_na_fila_espera != NULL || 
                        loja1->num_tarefas_na_fila_preparo > 0 || 
                        loja1->num_tarefas_em_execucao > 0;
    
    int loja2_ocupada = loja2->pedidos_na_fila_espera != NULL || 
                        loja2->num_tarefas_na_fila_preparo > 0 || 
                        loja2->num_tarefas_em_execucao > 0;

    return loja1_ocupada || loja2_ocupada;
}


// =============================================================================
// 5. FUNÇÃO PRINCIPAL (Refatorada para ser o Motor Global)
// =============================================================================
int main()
{
    system("cls||clear");

    Cozinha cozinha_loja_1;
    Cozinha cozinha_loja_2;
    int proximo_id_pedido_global = 1;

    // 1. Inicializa as duas cozinhas com IDs diferentes
    inicializar_cozinha(&cozinha_loja_1, 1);
    inicializar_cozinha(&cozinha_loja_2, 2);

    int tempo_global = 0;
    int entrada_ativa = 1; // Controla se ainda estamos aceitando novos pedidos

    printf("\n=== INICIANDO SIMULACAO DO BIGPAPAO (Duas Lojas + iFood) ===\n");
    printf("Digite o destino (1=Loja 1, 2=Loja 2, 3=iFood)\n");
    printf("Digite 0 ou - para avancar 1 segundo (simular tempo ocioso).\n");
    printf("Digite -1 para encerrar a entrada de pedidos e deixar a simulacao terminar.\n");

    // O NOVO MOTOR DA SIMULAÇÃO: avança segundo a segundo.
    while (entrada_ativa || cozinhas_estao_ocupadas(&cozinha_loja_1, &cozinha_loja_2))
    {
        // 2. Processa a entrada do usuário, se ainda estiver ativa
        if (entrada_ativa) {
            printf("\n[Tempo Global: %ds] Destino (1=Loja1, 2=Loja2, 3=iFood, 0=Ocioso, -1=Sair): ", tempo_global);
            
            char input_str[10];
            scanf("%s", input_str);
            int destino = atoi(input_str); // Converte a string para número

            if (destino == -1) { // Encerrar entrada de pedidos
                entrada_ativa = 0;
                printf("[Sistema] Nao ha mais novos pedidos. Processando os pedidos restantes...\n");
            } 
            else if (destino == 0 || strcmp(input_str, "-") == 0) { // Simular tempo ocioso
                printf("[Sistema] Tempo ocioso no caixa. Cozinhas continuam trabalhando...\n");
            } 
            else if (destino == 1 || destino == 2 || destino == 3) {
                // Se a entrada for válida, cria um novo pedido
                Pedido *novo_pedido = criar_pedido_interativo(proximo_id_pedido_global++, tempo_global);
                
                if (novo_pedido) {
                    if (destino == 1) {
                        adicionar_pedido_na_fila_espera(&cozinha_loja_1, novo_pedido);
                    } else if (destino == 2) {
                        adicionar_pedido_na_fila_espera(&cozinha_loja_2, novo_pedido);
                    } else if (destino == 3) {
                        // Chama o roteador para decidir para onde o pedido iFood deve ir
                        rotear_pedido_ifood(novo_pedido, &cozinha_loja_1, &cozinha_loja_2);
                    }
                }
            } 
            else {
                printf("Entrada invalida. Digite 1, 2, 3, 0, ou -1.\n");
            }
        }

        // 3. Atualiza o estado de ambas as cozinhas para este segundo
        atualizar_cozinha_um_passo(&cozinha_loja_1, tempo_global);
        atualizar_cozinha_um_passo(&cozinha_loja_2, tempo_global);

        // 4. Avança o relógio global em 1 segundo
        tempo_global++;
    }

    // 5. Relatório Final
    printf("\n=== SIMULACAO FINALIZADA EM %d SEGUNDOS (TEMPO TOTAL DECORRIDO) ===\n", tempo_global - 1);
    
    printf("\n--- Resultados Loja 1 ---\n");
    printf("Total de Pedidos Processados: %d\n", cozinha_loja_1.total_pedidos_criados);
    printf("Atendidos no Prazo: %d\n", cozinha_loja_1.atendidos_no_prazo);
    printf("Atendidos com Atraso (Prejuizo): %d\n", cozinha_loja_1.atendidos_com_atraso);
    
    printf("\n--- Resultados Loja 2 ---\n");
    printf("Total de Pedidos Processados: %d\n", cozinha_loja_2.total_pedidos_criados);
    printf("Atendidos no Prazo: %d\n", cozinha_loja_2.atendidos_no_prazo);
    printf("Atendidos com Atraso (Prejuizo): %d\n", cozinha_loja_2.atendidos_com_atraso);

    // 6. Limpa a memória de ambas as cozinhas
    limpar_cozinha(&cozinha_loja_1);
    limpar_cozinha(&cozinha_loja_2);
    
    return 0;
}
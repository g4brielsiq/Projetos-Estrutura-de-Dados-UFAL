/*
  Simulação de cozinha (versão corrigida)
  Correções principais:
  - Removida a chamada a system("cls||clear") por motivos de portabilidade e segurança (CERT: evitar system) [CORREÇÃO].
  - Chegadas espaçadas no tempo: cada pedido recebe tempo_chegada incremental (modelo de next-event) [CORREÇÃO].
  - Despacho: cada funcionário recebe no máximo 1 tarefa por rodada (evita multitarefa simultânea não modelada) [CORREÇÃO].
  - realloc seguro: checagem de overflow em (count * elem_size) e uso de ponteiro temporário encapsulado [CORREÇÃO].
  - Impressão do ID real do funcionário (struct.id) em vez de índice+1 [CORREÇÃO].
  - Classificação comer/beber por função explícita em vez de depender da ordem do enum [CORREÇÃO].
  - Includes enxutos e comentários focados em regras/decisões.

  Compilação sugerida:
    gcc -std=c11 -Wall -Wextra -Wpedantic -O2 bigpapao.c -o bigpapao
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stddef.h>

// =============================================================================
// 0. FUNÇÕES UTILITÁRIAS (CORREÇÃO)
// =============================================================================

// Limpa stdin até '\n' ou EOF para estabilizar leituras subsequentes de scanf.
void limpar_buffer_entrada(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) { /* descarta */ }
}

// realloc seguro para vetores: previne overflow em count*elem_size e preserva ponteiro.
// Retorna 0 em sucesso; -1 em overflow; -2 em falha de alocação (sem alterar *pp).
static int safe_realloc_array(void **pp, size_t count, size_t elem_size) {
    if (count != 0 && elem_size > SIZE_MAX / count) {
        return -1; // overflow na multiplicação
    }
    void *tmp = realloc(*pp, count * elem_size);
    if (tmp == NULL && count != 0) {
        return -2; // falha de alocação
    }
    *pp = tmp; // ok (realloc(NULL,0) retorna NULL, é idempotente)
    return 0;
}

// =============================================================================
// 1. DEFINIÇÕES E ESTRUTURAS DE DADOS
// =============================================================================

#define NUM_FUNCIONARIOS 13
#define TEMPO_MAX_ATENDIMENTO 300
#define TEMPO_ATENDIMENTO_CAIXA 10

typedef enum {
    ITEM_SANDUICHE_SIMPLES,
    ITEM_SANDUICHE_MEDIO,
    ITEM_SANDUICHE_ELABORADO,
    ITEM_BATATA_FRITA,
    ITEM_REFRIGERANTE,
    ITEM_SUCO,
    ITEM_MILK_SHAKE,
    ITEM_MONTAGEM
} TipoItem;

const char *NOMES_ITENS[] = {
    "Sanduiche Simples", "Sanduiche Medio", "Sanduiche Elaborado",
    "Batata Frita", "Refrigerante", "Suco", "Milk Shake", "Montagem de Bandeja"
};

typedef enum {
    STATUS_NA_FILA,
    STATUS_EM_PREPARO,
    STATUS_AGUARDANDO_MONTAGEM,
    STATUS_EM_MONTAGEM,
    STATUS_CONCLUIDO_NO_PRAZO,
    STATUS_CONCLUIDO_ATRASADO
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

typedef struct Equipamento {
    int capacidade_por_funcionario; // permanece como metadado (não usado para paralelizar trabalhador)
    int validade_produto_min;
} Equipamento;

typedef struct Cozinha {
    int tempo_atual;

    // Recursos
    Funcionario funcionarios[NUM_FUNCIONARIOS];
    Equipamento chapa;
    Equipamento fritadeira;
    Equipamento liquidificador;

    // Filas/Listas
    Pedido *pedidos_na_fila_espera;
    Pedido **pedidos_em_andamento;
    int num_pedidos_em_andamento;
    Tarefa *tarefas_na_fila_preparo;
    int num_tarefas_na_fila_preparo;
    Tarefa *tarefas_em_execucao;
    int num_tarefas_em_execucao;

    // Métricas
    int total_pedidos_criados;
    int atendidos_no_prazo;
    int atendidos_com_atraso;

    // CORREÇÃO: agenda incremental de chegadas (modelo de caixa)
    int proximo_tempo_chegada;
} Cozinha;

// Helper explícito para classificar bebidas (evita depender da ordem do enum)
static int item_e_bebida(TipoItem t) {
    return (t == ITEM_REFRIGERANTE || t == ITEM_SUCO || t == ITEM_MILK_SHAKE);
}

// =============================================================================
// 2. FUNÇÕES DE GERENCIAMENTO DA COZINHA
// =============================================================================

void inicializar_cozinha(Cozinha *cozinha) {
    memset(cozinha, 0, sizeof(Cozinha));

    printf("--- Configurando a Cozinha com as Regras da Equipe ---\n");
    cozinha->chapa.capacidade_por_funcionario = 3;
    cozinha->fritadeira.capacidade_por_funcionario = 2;
    cozinha->fritadeira.validade_produto_min = 60;
    cozinha->liquidificador.capacidade_por_funcionario = 4;
    cozinha->liquidificador.validade_produto_min = 30;
    printf("Capacidade por funcionario: %d sanduiches, %d batatas, %d milk-shakes.\n",
           cozinha->chapa.capacidade_por_funcionario,
           cozinha->fritadeira.capacidade_por_funcionario,
           cozinha->liquidificador.capacidade_por_funcionario);
    printf("Tempo de atendimento no caixa (delay inicial): %d segundos.\n\n", TEMPO_ATENDIMENTO_CAIXA);

    cozinha->funcionarios[0]  = (Funcionario){1,  HABILIDADE_SANDUICHE | HABILIDADE_BATATA, 0};
    cozinha->funcionarios[1]  = (Funcionario){2,  HABILIDADE_SANDUICHE | HABILIDADE_BATATA, 0};
    cozinha->funcionarios[2]  = (Funcionario){3,  HABILIDADE_SANDUICHE | HABILIDADE_BEBIDAS, 0};
    cozinha->funcionarios[3]  = (Funcionario){4,  HABILIDADE_SANDUICHE, 0};
    cozinha->funcionarios[4]  = (Funcionario){5,  HABILIDADE_SANDUICHE, 0};
    cozinha->funcionarios[5]  = (Funcionario){6,  HABILIDADE_SANDUICHE | HABILIDADE_BATATA, 0};
    cozinha->funcionarios[6]  = (Funcionario){7,  HABILIDADE_BATATA, 0};
    cozinha->funcionarios[7]  = (Funcionario){8,  HABILIDADE_BEBIDAS | HABILIDADE_MONTAGEM, 0};
    cozinha->funcionarios[8]  = (Funcionario){9,  HABILIDADE_MONTAGEM, 0};
    cozinha->funcionarios[9]  = (Funcionario){10, 0, 0};
    cozinha->funcionarios[10] = (Funcionario){11, HABILIDADE_SANDUICHE, 0};
    cozinha->funcionarios[11] = (Funcionario){12, HABILIDADE_BEBIDAS, 0};
    cozinha->funcionarios[12] = (Funcionario){13, 0, 0};

    // CORREÇÃO: agenda incremental de chegadas
    cozinha->proximo_tempo_chegada = TEMPO_ATENDIMENTO_CAIXA;
}

void adicionar_pedido_na_fila_espera(Cozinha *c, Pedido *novo_pedido) {
    novo_pedido->proximo = NULL;
    if (c->pedidos_na_fila_espera == NULL) {
        c->pedidos_na_fila_espera = novo_pedido;
    } else {
        Pedido *atual = c->pedidos_na_fila_espera;
        while (atual->proximo != NULL) atual = atual->proximo;
        atual->proximo = novo_pedido;
    }
}

void limpar_cozinha(Cozinha *c) {
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

// =============================================================================
// 3. MENU INTERATIVO E IMPRESSÃO DE BANDEJAS
// =============================================================================

void imprimir_composicao_bandejas(Pedido *pedido) {
    printf("\n--- Composicao do Pedido #%d ---\n", pedido->id);

    TipoItem *itens_comer = (TipoItem *)malloc(sizeof(TipoItem) * (size_t)pedido->num_itens);
    TipoItem *itens_beber = (TipoItem *)malloc(sizeof(TipoItem) * (size_t)pedido->num_itens);

    if (itens_comer == NULL || itens_beber == NULL) {
        perror("Falha ao alocar memoria para impressao de bandejas");
        free(itens_comer);
        free(itens_beber);
        return;
    }

    int count_comer = 0, count_beber = 0;
    for (int i = 0; i < pedido->num_itens; i++) {
        if (item_e_bebida(pedido->itens[i])) {
            itens_beber[count_beber++] = pedido->itens[i];
        } else {
            itens_comer[count_comer++] = pedido->itens[i];
        }
    }

    int bandeja_num = 1;
    int idx_comer_atual = 0, idx_beber_atual = 0;

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

    free(itens_comer);
    free(itens_beber);
}

void coletar_pedidos_do_usuario(Cozinha *c) {
    int num_pedidos_iniciais;
    printf("Bem-vindo ao sistema de simulacao do BigPapao!\n");
    printf("Quantos pedidos iniciais deseja registrar? ");
    while (scanf("%d", &num_pedidos_iniciais) != 1) {
        printf("Entrada invalida. Por favor, digite um numero: ");
        limpar_buffer_entrada();
    }
    limpar_buffer_entrada();

    for (int i = 0; i < num_pedidos_iniciais; i++) {
        int id_pedido_atual = i + 1;
        printf("\n--- Registrando Pedido #%d ---\n", id_pedido_atual);

        TipoItem *itens_do_pedido = NULL;
        int qtd_itens = 0;
        int escolha = -1;

        do {
            printf("Escolha os itens para o pedido #%d:\n", id_pedido_atual);
            for (int j = 0; j < 7; j++)
                printf("  %d. %s\n", j + 1, NOMES_ITENS[j]);
            printf("  0. Finalizar Pedido\n");
            printf("Digite o numero do item: ");

            while (scanf("%d", &escolha) != 1) {
                printf("Entrada invalida. Por favor, digite um numero (0-7): ");
                limpar_buffer_entrada();
            }
            limpar_buffer_entrada();

            if (escolha >= 1 && escolha <= 7) {
                int quantidade;
                printf("Digite a quantidade do item: ");
                while (scanf("%d", &quantidade) != 1) {
                    printf("Entrada invalida. Por favor, digite um numero: ");
                    limpar_buffer_entrada();
                }
                limpar_buffer_entrada();

                if (quantidade < 1) {
                    printf("Quantidade invalida, adicionando 1 unidade.\n");
                    quantidade = 1;
                }
                for (int k = 0; k < quantidade; k++) {
                    int novo_qtd = qtd_itens + 1;
                    void *pp = itens_do_pedido;
                    int rc = safe_realloc_array(&pp, (size_t)novo_qtd, sizeof(TipoItem));
                    if (rc != 0) {
                        if (rc == -1) {
                            fprintf(stderr, "Overflow ao alocar itens do pedido\n");
                        } else {
                            perror("Falha critica de alocacao (realloc itens)");
                        }
                        free(itens_do_pedido);
                        exit(1);
                    }
                    itens_do_pedido = (TipoItem *)pp;
                    itens_do_pedido[novo_qtd - 1] = (TipoItem)(escolha - 1);
                    qtd_itens = novo_qtd;
                }
                printf(">> (%d) - '%s' adicionado(s) ao pedido.\n\n", quantidade, NOMES_ITENS[escolha - 1]);
            } else if (escolha != 0) {
                printf("Opcao invalida! Tente novamente.\n\n");
            }
        } while (escolha != 0);

        if (qtd_itens > 0) {
            Pedido *novo_pedido = (Pedido *)malloc(sizeof(Pedido));
            if (novo_pedido == NULL) {
                perror("Falha critica de alocacao (malloc pedido)");
                free(itens_do_pedido);
                exit(1);
            }

            novo_pedido->id = id_pedido_atual;

            // CORREÇÃO: cada pedido recebe um tempo de chegada incremental no "caixa"
            novo_pedido->tempo_chegada = c->proximo_tempo_chegada;
            c->proximo_tempo_chegada += TEMPO_ATENDIMENTO_CAIXA;

            novo_pedido->status = STATUS_NA_FILA;
            novo_pedido->itens = itens_do_pedido;
            novo_pedido->num_itens = qtd_itens;
            novo_pedido->tarefas_preparo_restantes = qtd_itens;

            adicionar_pedido_na_fila_espera(c, novo_pedido);
            c->total_pedidos_criados++;
            printf(">> Pedido #%d finalizado com %d itens. Chega em %ds.\n",
                   id_pedido_atual, qtd_itens, novo_pedido->tempo_chegada);
        } else {
            printf(">> Pedido #%d cancelado por nao ter itens.\n", id_pedido_atual);
        }
    }
}

// =============================================================================
// 4. O MOTOR DA SIMULAÇÃO
// =============================================================================

Pedido *encontrar_pedido_em_andamento(Cozinha *c, int pedido_id) {
    for (int i = 0; i < c->num_pedidos_em_andamento; i++) {
        if (c->pedidos_em_andamento[i]->id == pedido_id)
            return c->pedidos_em_andamento[i];
    }
    return NULL;
}

void despachar_tarefas(Cozinha *c) {
    Habilidade habilidades[] = {HABILIDADE_SANDUICHE, HABILIDADE_BATATA, HABILIDADE_BEBIDAS, HABILIDADE_MONTAGEM};

    for (int h = 0; h < 4; h++) {
        int funcs_livres[NUM_FUNCIONARIOS];
        int num_funcs_livres = 0;
        for (int i = 0; i < NUM_FUNCIONARIOS; i++) {
            if ((c->funcionarios[i].habilidades & habilidades[h]) &&
                (c->funcionarios[i].livre_a_partir_de <= c->tempo_atual)) {
                funcs_livres[num_funcs_livres++] = i;
            }
        }
        if (num_funcs_livres == 0) continue;

        // CORREÇÃO: cada funcionário inicia no máximo 1 tarefa por rodada
        int capacidade_total = num_funcs_livres;
        int tarefas_alocadas_nesta_rodada = 0;

        for (int i = c->num_tarefas_na_fila_preparo - 1; i >= 0; i--) {
            if (tarefas_alocadas_nesta_rodada >= capacidade_total) break;

            Tarefa *tarefa_atual = &c->tarefas_na_fila_preparo[i];
            int tarefa_corresponde = 0;
            switch (habilidades[h]) {
                case HABILIDADE_SANDUICHE:
                    if (tarefa_atual->tipo_item >= ITEM_SANDUICHE_SIMPLES &&
                        tarefa_atual->tipo_item <= ITEM_SANDUICHE_ELABORADO) tarefa_corresponde = 1;
                    break;
                case HABILIDADE_BATATA:
                    if (tarefa_atual->tipo_item == ITEM_BATATA_FRITA) tarefa_corresponde = 1;
                    break;
                case HABILIDADE_BEBIDAS:
                    if (item_e_bebida(tarefa_atual->tipo_item)) tarefa_corresponde = 1;
                    break;
                case HABILIDADE_MONTAGEM:
                    if (tarefa_atual->tipo_item == ITEM_MONTAGEM) tarefa_corresponde = 1;
                    break;
            }
            if (!tarefa_corresponde) continue;

            int id_func_alocado = funcs_livres[tarefas_alocadas_nesta_rodada]; // sem reutilizar o mesmo funcionário
            tarefa_atual->tempo_conclusao = c->tempo_atual + TEMPOS_DE_PREPARO[tarefa_atual->tipo_item];
            c->funcionarios[id_func_alocado].livre_a_partir_de = tarefa_atual->tempo_conclusao;

            c->num_tarefas_em_execucao++;
            void *pp_exec = c->tarefas_em_execucao;
            int rc = safe_realloc_array(&pp_exec, (size_t)c->num_tarefas_em_execucao, sizeof(Tarefa));
            if (rc != 0) {
                if (rc == -1) fprintf(stderr, "Overflow ao alocar tarefas_em_execucao\n");
                else perror("Falha critica de alocacao (tarefas_em_execucao)");
                exit(1);
            }
            c->tarefas_em_execucao = (Tarefa *)pp_exec;
            c->tarefas_em_execucao[c->num_tarefas_em_execucao - 1] = *tarefa_atual;

            printf("[Tempo: %ds] Func. #%d iniciou %s do Pedido #%d (conclui em %ds).\n",
                   c->tempo_atual,
                   c->funcionarios[id_func_alocado].id, // CORREÇÃO: usa ID real
                   NOMES_ITENS[tarefa_atual->tipo_item],
                   tarefa_atual->pedido_id,
                   tarefa_atual->tempo_conclusao);

            // swap-and-pop
            c->tarefas_na_fila_preparo[i] = c->tarefas_na_fila_preparo[c->num_tarefas_na_fila_preparo - 1];
            c->num_tarefas_na_fila_preparo--;
            tarefas_alocadas_nesta_rodada++;
        }
    }
}

void executar_simulacao(Cozinha *cozinha) {
    printf("\n=== INICIANDO SIMULACAO DA COZINHA BIGPAPAO ===\n");

    while (cozinha->total_pedidos_criados > (cozinha->atendidos_no_prazo + cozinha->atendidos_com_atraso)) {
        int alguma_acao_ocorreu;
        do {
            alguma_acao_ocorreu = 0;

            // Chegada de pedidos quando atingir tempo_chegada
            while (cozinha->pedidos_na_fila_espera != NULL &&
                   cozinha->pedidos_na_fila_espera->tempo_chegada <= cozinha->tempo_atual) {
                alguma_acao_ocorreu = 1;
                Pedido *pedido_iniciado = cozinha->pedidos_na_fila_espera;
                cozinha->pedidos_na_fila_espera = cozinha->pedidos_na_fila_espera->proximo;

                pedido_iniciado->status = STATUS_EM_PREPARO;
                cozinha->num_pedidos_em_andamento++;
                void *pp = cozinha->pedidos_em_andamento;
                int rc = safe_realloc_array(&pp, (size_t)cozinha->num_pedidos_em_andamento, sizeof(Pedido *));
                if (rc != 0) {
                    if (rc == -1) fprintf(stderr, "Overflow ao alocar pedidos_em_andamento\n");
                    else perror("Falha critica de alocacao (pedidos_em_andamento)");
                    exit(1);
                }
                cozinha->pedidos_em_andamento = (Pedido **)pp;
                cozinha->pedidos_em_andamento[cozinha->num_pedidos_em_andamento - 1] = pedido_iniciado;

                printf("[Tempo: %ds] Iniciando preparo do Pedido #%d.\n", cozinha->tempo_atual, pedido_iniciado->id);
                for (int i = 0; i < pedido_iniciado->num_itens; i++) {
                    Tarefa t = (Tarefa){pedido_iniciado->id, pedido_iniciado->itens[i], 0};
                    cozinha->num_tarefas_na_fila_preparo++;
                    void *pp2 = cozinha->tarefas_na_fila_preparo;
                    rc = safe_realloc_array(&pp2, (size_t)cozinha->num_tarefas_na_fila_preparo, sizeof(Tarefa));
                    if (rc != 0) {
                        if (rc == -1) fprintf(stderr, "Overflow ao alocar tarefas_na_fila_preparo\n");
                        else perror("Falha critica de alocacao (tarefas_na_fila_preparo)");
                        exit(1);
                    }
                    cozinha->tarefas_na_fila_preparo = (Tarefa *)pp2;
                    cozinha->tarefas_na_fila_preparo[cozinha->num_tarefas_na_fila_preparo - 1] = t;
                }
            }

            int num_tarefas_antes = cozinha->num_tarefas_em_execucao;
            despachar_tarefas(cozinha);
            if (cozinha->num_tarefas_em_execucao > num_tarefas_antes) {
                alguma_acao_ocorreu = 1;
            }

            for (int i = cozinha->num_tarefas_em_execucao - 1; i >= 0; i--) {
                Tarefa *tarefa_concluida = &cozinha->tarefas_em_execucao[i];
                if (tarefa_concluida->tempo_conclusao <= cozinha->tempo_atual) {
                    alguma_acao_ocorreu = 1;
                    Pedido *pedido_pai = encontrar_pedido_em_andamento(cozinha, tarefa_concluida->pedido_id);
                    if (!pedido_pai) {
                        // já removido ao finalizar montagem
                        cozinha->tarefas_em_execucao[i] = cozinha->tarefas_em_execucao[cozinha->num_tarefas_em_execucao - 1];
                        cozinha->num_tarefas_em_execucao--;
                        continue;
                    }

                    printf("[Tempo: %ds] %s do Pedido #%d foi concluido.\n",
                           cozinha->tempo_atual, NOMES_ITENS[tarefa_concluida->tipo_item], pedido_pai->id);

                    if (tarefa_concluida->tipo_item == ITEM_MONTAGEM) {
                        int tempo_de_producao = cozinha->tempo_atual - pedido_pai->tempo_chegada;
                        if (tempo_de_producao <= TEMPO_MAX_ATENDIMENTO) {
                            cozinha->atendidos_no_prazo++;
                            pedido_pai->status = STATUS_CONCLUIDO_NO_PRAZO;
                            printf(">> Pedido #%d FINALIZADO com SUCESSO. Tempo de producao: %ds.\n", pedido_pai->id, tempo_de_producao);
                        } else {
                            cozinha->atendidos_com_atraso++;
                            pedido_pai->status = STATUS_CONCLUIDO_ATRASADO;
                            printf(">> Pedido #%d FINALIZADO com ATRASO. Tempo de producao: %ds (PREJUIZO).\n", pedido_pai->id, tempo_de_producao);
                        }
                        imprimir_composicao_bandejas(pedido_pai);

                        // remove de em_andamento e libera memória
                        int indice_pedido_removido = -1;
                        for (int j = 0; j < cozinha->num_pedidos_em_andamento; j++) {
                            if (cozinha->pedidos_em_andamento[j]->id == pedido_pai->id) {
                                indice_pedido_removido = j;
                                break;
                            }
                        }
                        if (indice_pedido_removido != -1) {
                            free(pedido_pai->itens);
                            free(pedido_pai);
                            cozinha->pedidos_em_andamento[indice_pedido_removido] =
                                cozinha->pedidos_em_andamento[cozinha->num_pedidos_em_andamento - 1];
                            cozinha->num_pedidos_em_andamento--;
                        }
                    } else {
                        pedido_pai->tarefas_preparo_restantes--;
                        if (pedido_pai->tarefas_preparo_restantes == 0 && pedido_pai->status == STATUS_EM_PREPARO) {
                            alguma_acao_ocorreu = 1;
                            pedido_pai->status = STATUS_AGUARDANDO_MONTAGEM;
                            Tarefa t = (Tarefa){pedido_pai->id, ITEM_MONTAGEM, 0};
                            cozinha->num_tarefas_na_fila_preparo++;
                            void *pp3 = cozinha->tarefas_na_fila_preparo;
                            int rc = safe_realloc_array(&pp3, (size_t)cozinha->num_tarefas_na_fila_preparo, sizeof(Tarefa));
                            if (rc != 0) {
                                if (rc == -1) fprintf(stderr, "Overflow ao alocar tarefas_na_fila_preparo\n");
                                else perror("Falha critica de alocacao (tarefas_na_fila_preparo)");
                                exit(1);
                            }
                            cozinha->tarefas_na_fila_preparo = (Tarefa *)pp3;
                            cozinha->tarefas_na_fila_preparo[cozinha->num_tarefas_na_fila_preparo - 1] = t;
                            printf("[Tempo: %ds] Pedido #%d pronto para montagem.\n", cozinha->tempo_atual, pedido_pai->id);
                        }
                    }

                    // remove tarefa concluída (swap-and-pop)
                    cozinha->tarefas_em_execucao[i] = cozinha->tarefas_em_execucao[cozinha->num_tarefas_em_execucao - 1];
                    cozinha->num_tarefas_em_execucao--;
                }
            }
        } while (alguma_acao_ocorreu);

        int proximo_tempo_tarefa = INT_MAX;
        if (cozinha->num_tarefas_em_execucao > 0) {
            for (int i = 0; i < cozinha->num_tarefas_em_execucao; i++) {
                if (cozinha->tarefas_em_execucao[i].tempo_conclusao < proximo_tempo_tarefa) {
                    proximo_tempo_tarefa = cozinha->tarefas_em_execucao[i].tempo_conclusao;
                }
            }
        }

        int proximo_tempo_chegada = INT_MAX;
        if (cozinha->pedidos_na_fila_espera != NULL) {
            proximo_tempo_chegada = cozinha->pedidos_na_fila_espera->tempo_chegada; // cabeça é o próximo agendado
        }

        int proximo_evento = (proximo_tempo_tarefa < proximo_tempo_chegada)
                               ? proximo_tempo_tarefa
                               : proximo_tempo_chegada;

        if (proximo_evento == INT_MAX) {
            if (cozinha->total_pedidos_criados > (cozinha->atendidos_no_prazo + cozinha->atendidos_com_atraso)) {
                if (cozinha->num_tarefas_na_fila_preparo > 0) {
                    printf("AVISO: Simulacao parada. Existem %d tarefas na fila, mas nenhum funcionario disponivel/habilitado para executa-las (gargalo).\n",
                           cozinha->num_tarefas_na_fila_preparo);
                } else {
                    printf("AVISO: Simulacao parada, mas nem todos os pedidos foram concluidos. Verifique logica.\n");
                }
            }
            break;
        }

        if (proximo_evento > cozinha->tempo_atual) {
            cozinha->tempo_atual = proximo_evento;
        } else {
            cozinha->tempo_atual++;
        }
    }

    printf("\n=== SIMULACAO FINALIZADA EM %d SEGUNDOS (TEMPO TOTAL DECORRIDO) ===\n", cozinha->tempo_atual);
    printf("Total de Pedidos: %d\n", cozinha->total_pedidos_criados);
    printf("Atendidos no Prazo: %d\n", cozinha->atendidos_no_prazo);
    printf("Atendidos com Atraso (Prejuizo): %d\n", cozinha->atendidos_com_atraso);
}

// =============================================================================
// 5. FUNÇÃO PRINCIPAL
// =============================================================================

int main(void) {
    // CORREÇÃO: remover system("cls||clear") por portabilidade/segurança.

    Cozinha cozinha;
    inicializar_cozinha(&cozinha);
    coletar_pedidos_do_usuario(&cozinha);

    if (cozinha.total_pedidos_criados > 0) {
        executar_simulacao(&cozinha);
    } else {
        printf("\nNenhum pedido foi registrado. Encerrando o programa.\n");
    }

    limpar_cozinha(&cozinha);
    return 0;
}

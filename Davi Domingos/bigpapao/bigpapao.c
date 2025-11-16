/*
  Simulação de cozinha (duas lojas + envio em tempo real + entrada até -1 + menu)
  - Antes de cada pedido, pergunta: Loja 1 ou Loja 2 (ou -1 para encerrar).
  - 0 finaliza o pedido atual e envia IMEDIATAMENTE à produção da loja escolhida.
  - -1 encerra a coleta; se houver itens pendentes, o pedido é finalizado e enviado antes de sair.
  - Boas práticas: validação de entrada, realloc seguro, comentários explicativos.

  Compilação sugerida:
    gcc -std=c11 -Wall -Wextra -Wpedantic -O2 bigpapao.c -o bigpapao
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>

/* =============================================================================
   0. FUNÇÕES UTILITÁRIAS
   ============================================================================= */

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

/* =============================================================================
   1. DEFINIÇÕES E ESTRUTURAS DE DADOS
   ============================================================================= */

#define NUM_FUNCIONARIOS 13
#define TEMPO_MAX_ATENDIMENTO 300
#define TEMPO_ATENDIMENTO_CAIXA 10

// Tipos de item do cardápio e tarefa de montagem
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

// Estados do pedido
typedef enum {
    STATUS_NA_FILA,
    STATUS_EM_PREPARO,
    STATUS_AGUARDANDO_MONTAGEM,
    STATUS_EM_MONTAGEM,
    STATUS_CONCLUIDO_NO_PRAZO,
    STATUS_CONCLUIDO_ATRASADO
} StatusPedido;

// Habilidades (bitmask)
typedef enum {
    HABILIDADE_SANDUICHE = 1 << 0,
    HABILIDADE_BATATA    = 1 << 1,
    HABILIDADE_BEBIDAS   = 1 << 2,
    HABILIDADE_MONTAGEM  = 1 << 3
} Habilidade;

// Tempos de preparo por tipo de item
const int TEMPOS_DE_PREPARO[] = {58, 88, 105, 190, 5, 38, 60, 30};

// Unidade de trabalho (tarefa)
typedef struct Tarefa {
    int pedido_id;
    TipoItem tipo_item;
    int tempo_conclusao;
} Tarefa;

// Pedido do cliente
typedef struct Pedido {
    int id;
    int tempo_chegada;               // quando entrou na produção
    StatusPedido status;
    int tarefas_preparo_restantes;   // decrementa até 0; então cria montagem
    TipoItem *itens;
    int num_itens;
    struct Pedido *proximo;          // para fila (se usada)
} Pedido;

// Funcionário
typedef struct {
    int id;
    unsigned int habilidades; // bitmask
    int livre_a_partir_de;    // agenda (tempo quando fica livre)
} Funcionario;

// Equipamentos (metadados)
typedef struct Equipamento {
    int capacidade_por_funcionario; // metadado; não paraleliza trabalhador
    int validade_produto_min;
} Equipamento;

// Estado global da simulação (por loja)
typedef struct Cozinha {
    int tempo_atual;

    // Recursos
    Funcionario funcionarios[NUM_FUNCIONARIOS];
    Equipamento chapa;
    Equipamento fritadeira;
    Equipamento liquidificador;

    // Filas/Listas
    Pedido *pedidos_na_fila_espera;      // não usado no envio em tempo real, mas mantido
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

    // Identificação
    int loja; // 1 ou 2
} Cozinha;

// Helper explícito para classificar bebidas (robusto a alterações no enum)
static int item_e_bebida(TipoItem t) {
    return (t == ITEM_REFRIGERANTE || t == ITEM_SUCO || t == ITEM_MILK_SHAKE);
}

/* =============================================================================
   2. GERENCIAMENTO DA COZINHA
   ============================================================================= */

void inicializar_cozinha(Cozinha *cozinha, int loja) {
    memset(cozinha, 0, sizeof(Cozinha));
    cozinha->loja = loja;

    printf("--- Configurando a Cozinha da Loja %d ---\n", loja);
    cozinha->chapa.capacidade_por_funcionario = 3;
    cozinha->fritadeira.capacidade_por_funcionario = 2;
    cozinha->fritadeira.validade_produto_min = 60;
    cozinha->liquidificador.capacidade_por_funcionario = 4;
    cozinha->liquidificador.validade_produto_min = 30;
    printf("Capacidade por funcionario: %d sanduiches, %d batatas, %d milk-shakes.\n",
           cozinha->chapa.capacidade_por_funcionario,
           cozinha->fritadeira.capacidade_por_funcionario,
           cozinha->liquidificador.capacidade_por_funcionario);
    printf("Tempo de atendimento no caixa (informativo): %d segundos.\n\n", TEMPO_ATENDIMENTO_CAIXA);

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
}

// Enfileira no fim de uma lista ligada simples (mantido para compatibilidade)
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

/* =============================================================================
   3. MENU, IMPRESSÃO E ENVIO IMEDIATO
   ============================================================================= */

// Mostra o cardápio de forma clara, com instruções de controle.
static void imprimir_menu_itens(void) {
    printf("\n===== CARDAPIO BIGPAPAO =====\n");
    printf("  1. %s\n", NOMES_ITENS[ITEM_SANDUICHE_SIMPLES]);
    printf("  2. %s\n", NOMES_ITENS[ITEM_SANDUICHE_MEDIO]);
    printf("  3. %s\n", NOMES_ITENS[ITEM_SANDUICHE_ELABORADO]);
    printf("  4. %s\n", NOMES_ITENS[ITEM_BATATA_FRITA]);
    printf("  5. %s\n", NOMES_ITENS[ITEM_REFRIGERANTE]);
    printf("  6. %s\n", NOMES_ITENS[ITEM_SUCO]);
    printf("  7. %s\n", NOMES_ITENS[ITEM_MILK_SHAKE]);
    printf("  0. Finalizar pedido atual e enviar à produção\n");
    printf(" -1. Encerrar registro de pedidos\n");
    printf("==============================\n");
}

// Imprime a composição das bandejas, separando comer/beber (regra 2+2).
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

static void despachar_tarefas(Cozinha *c); // forward declaration

// Converte um pedido recém-criado em tarefas e inicia produção imediatamente (na loja c).
static void iniciar_pedido_imediato(Cozinha *c, Pedido *p) {
    // 1) Chegada agora e início de preparo
    p->tempo_chegada = c->tempo_atual;
    p->status = STATUS_EM_PREPARO;

    // 2) Adiciona a "em andamento"
    c->num_pedidos_em_andamento++;
    void *pp = c->pedidos_em_andamento;
    int rc = safe_realloc_array(&pp, (size_t)c->num_pedidos_em_andamento, sizeof(Pedido *));
    if (rc != 0) {
        if (rc == -1) fprintf(stderr, "Overflow ao alocar pedidos_em_andamento\n");
        else perror("Falha critica de alocacao (pedidos_em_andamento)");
        exit(1);
    }
    c->pedidos_em_andamento = (Pedido **)pp;
    c->pedidos_em_andamento[c->num_pedidos_em_andamento - 1] = p;

    printf("[Loja %d | Tempo: %ds] Iniciando preparo do Pedido #%d.\n", c->loja, c->tempo_atual, p->id);

    // 3) Gera tarefas de preparo
    for (int i = 0; i < p->num_itens; i++) {
        Tarefa t = (Tarefa){p->id, p->itens[i], 0};
        c->num_tarefas_na_fila_preparo++;
        void *pp2 = c->tarefas_na_fila_preparo;
        rc = safe_realloc_array(&pp2, (size_t)c->num_tarefas_na_fila_preparo, sizeof(Tarefa));
        if (rc != 0) {
            if (rc == -1) fprintf(stderr, "Overflow ao alocar tarefas_na_fila_preparo\n");
            else perror("Falha critica de alocacao (tarefas_na_fila_preparo)");
            exit(1);
        }
        c->tarefas_na_fila_preparo = (Tarefa *)pp2;
        c->tarefas_na_fila_preparo[c->num_tarefas_na_fila_preparo - 1] = t;
    }

    // 4) Tenta iniciar agora com funcionários livres
    despachar_tarefas(c);
}

/* =============================================================================
   3.2 COLETA DE PEDIDOS ATÉ -1 (DUAS LOJAS)
   ============================================================================= */

// Valida a escolha da loja: retorna 1, 2, ou -1 (encerra).
static int ler_escolha_loja(void) {
    int loja;
    for (;;) {
        printf("\nEscolha a Loja (1 ou 2) ou -1 para encerrar: ");
        if (scanf("%d", &loja) != 1) {
            printf("Entrada invalida. Digite -1, 1 ou 2.\n");
            limpar_buffer_entrada();
            continue;
        }
        limpar_buffer_entrada();
        if (loja == -1 || loja == 1 || loja == 2) return loja;
        printf("Opcao invalida. Digite -1, 1 ou 2.\n");
    }
}

// Registra pedidos continuamente; 0 finaliza pedido atual; -1 encerra toda a coleta.
// Pedidos finalizados são enviados imediatamente para produção na loja escolhida.
void coletar_pedidos_duas_lojas(Cozinha *l1, Cozinha *l2) {
    printf("Bem-vindo ao sistema de simulacao do BigPapao (duas lojas)!\n");
    printf("Fluxo: Escolha a loja -> adicione itens (1-7) -> 0 finaliza e envia -> repita ou -1 encerra.\n");

    int id_loja1 = 1;
    int id_loja2 = 1;

    for (;;) {
        int loja = ler_escolha_loja();
        if (loja == -1) {
            printf(">> Registro encerrado.\n");
            return;
        }

        Cozinha *c = (loja == 1) ? l1 : l2;
        int *id_pedido_atual_ptr = (loja == 1) ? &id_loja1 : &id_loja2;

        printf("\n--- Registrando Pedido #%d (Loja %d) ---\n", *id_pedido_atual_ptr, loja);

        TipoItem *itens_do_pedido = NULL;
        int qtd_itens = 0;
        int escolha = -2;

        for (;;) {
            // Exibe o cardápio completo antes de cada escolha.
            imprimir_menu_itens();

            printf("Sua escolha: ");
            // Validação robusta
            while (scanf("%d", &escolha) != 1) {
                printf("Entrada invalida. Digite (-1, 0 ou 1-7): ");
                limpar_buffer_entrada();
            }
            limpar_buffer_entrada();

            // Encerrar toda a coleta global (aceito também neste ponto)
            if (escolha == -1) {
                if (qtd_itens > 0) {
                    Pedido *novo_pedido = (Pedido *)malloc(sizeof(Pedido));
                    if (novo_pedido == NULL) {
                        perror("Falha critica de alocacao (malloc pedido)");
                        free(itens_do_pedido);
                        exit(1);
                    }
                    novo_pedido->id = *id_pedido_atual_ptr;
                    novo_pedido->status = STATUS_NA_FILA;
                    novo_pedido->itens = itens_do_pedido;
                    novo_pedido->num_itens = qtd_itens;
                    novo_pedido->tarefas_preparo_restantes = qtd_itens;
                    novo_pedido->proximo = NULL;

                    iniciar_pedido_imediato(c, novo_pedido);
                    c->total_pedidos_criados++;
                    printf(">> Pedido #%d (Loja %d) finalizado com %d itens e ENVIADO à produção.\n",
                           *id_pedido_atual_ptr, loja, qtd_itens);
                    (*id_pedido_atual_ptr)++;
                } else {
                    printf(">> Registro encerrado.\n");
                }
                return; // sai de toda a coleta
            }

            // Finalizar o pedido atual da loja
            if (escolha == 0) {
                break; // sai para finalizar/descartar conforme qtd_itens
            }

            // Adicionar item ao pedido atual
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

                // Acrescenta 'quantidade' unidades do item escolhido
                for (int k = 0; k < quantidade; k++) {
                    int novo_qtd = qtd_itens + 1;
                    void *pp = itens_do_pedido;
                    int rc = safe_realloc_array(&pp, (size_t)novo_qtd, sizeof(TipoItem));
                    if (rc != 0) {
                        if (rc == -1) fprintf(stderr, "Overflow ao alocar itens do pedido\n");
                        else perror("Falha critica de alocacao (realloc itens)");
                        free(itens_do_pedido);
                        exit(1);
                    }
                    itens_do_pedido = (TipoItem *)pp;
                    itens_do_pedido[novo_qtd - 1] = (TipoItem)(escolha - 1);
                    qtd_itens = novo_qtd;
                }

                printf(">> (%d) - '%s' adicionado(s) ao pedido (Loja %d).\n\n",
                       quantidade, NOMES_ITENS[escolha - 1], loja);
            } else {
                printf("Opcao invalida! Tente novamente.\n");
            }
        }

        // Finaliza pedido normal (0 digitado)
        if (qtd_itens > 0) {
            Pedido *novo_pedido = (Pedido *)malloc(sizeof(Pedido));
            if (novo_pedido == NULL) {
                perror("Falha critica de alocacao (malloc pedido)");
                free(itens_do_pedido);
                exit(1);
            }

            novo_pedido->id = *id_pedido_atual_ptr;
            novo_pedido->status = STATUS_NA_FILA;
            novo_pedido->itens = itens_do_pedido;
            novo_pedido->num_itens = qtd_itens;
            novo_pedido->tarefas_preparo_restantes = qtd_itens;
            novo_pedido->proximo = NULL;

            // Envio imediato à produção no tempo atual da loja
            iniciar_pedido_imediato(c, novo_pedido);

            c->total_pedidos_criados++;
            printf(">> Pedido #%d (Loja %d) finalizado com %d itens e ENVIADO à produção.\n",
                   *id_pedido_atual_ptr, loja, qtd_itens);
        } else {
            printf(">> Pedido #%d (Loja %d) cancelado por nao ter itens.\n", *id_pedido_atual_ptr, loja);
        }

        // Próximo pedido apenas para a loja escolhida
        (*id_pedido_atual_ptr)++;
    }
}

/* =============================================================================
   4. MOTOR DA SIMULAÇÃO (POR LOJA)
   ============================================================================= */

Pedido *encontrar_pedido_em_andamento(Cozinha *c, int pedido_id) {
    for (int i = 0; i < c->num_pedidos_em_andamento; i++) {
        if (c->pedidos_em_andamento[i]->id == pedido_id)
            return c->pedidos_em_andamento[i];
    }
    return NULL;
}

// O "Gerente de Chão": aloca tarefas pendentes para funcionários livres.
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

        // Regra: cada funcionário pode iniciar no máximo 1 tarefa nesta rodada.
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

            printf("[Loja %d | Tempo: %ds] Func. #%d iniciou %s do Pedido #%d (conclui em %ds).\n",
                   c->loja,
                   c->tempo_atual,
                   c->funcionarios[id_func_alocado].id, // usa ID real do funcionário
                   NOMES_ITENS[tarefa_atual->tipo_item],
                   tarefa_atual->pedido_id,
                   tarefa_atual->tempo_conclusao);

            // Remove da fila de preparo (swap-and-pop)
            c->tarefas_na_fila_preparo[i] = c->tarefas_na_fila_preparo[c->num_tarefas_na_fila_preparo - 1];
            c->num_tarefas_na_fila_preparo--;
            tarefas_alocadas_nesta_rodada++;
        }
    }
}

void executar_simulacao(Cozinha *cozinha) {
    if (cozinha->total_pedidos_criados == 0) {
        printf("\n=== Loja %d: Nenhum pedido para simular ===\n", cozinha->loja);
        return;
    }

    printf("\n=== INICIANDO SIMULACAO DA COZINHA BIGPAPAO (Loja %d) ===\n", cozinha->loja);

    while (cozinha->total_pedidos_criados > (cozinha->atendidos_no_prazo + cozinha->atendidos_com_atraso)) {
        int alguma_acao_ocorreu;
        do {
            alguma_acao_ocorreu = 0;

            // Tenta despachar tarefas pendentes neste tempo
            int num_tarefas_antes = cozinha->num_tarefas_em_execucao;
            despachar_tarefas(cozinha);
            if (cozinha->num_tarefas_em_execucao > num_tarefas_antes) {
                alguma_acao_ocorreu = 1;
            }

            // Processa tarefas que terminaram
            for (int i = cozinha->num_tarefas_em_execucao - 1; i >= 0; i--) {
                Tarefa *tarefa_concluida = &cozinha->tarefas_em_execucao[i];
                if (tarefa_concluida->tempo_conclusao <= cozinha->tempo_atual) {
                    alguma_acao_ocorreu = 1;
                    Pedido *pedido_pai = encontrar_pedido_em_andamento(cozinha, tarefa_concluida->pedido_id);
                    if (!pedido_pai) {
                        cozinha->tarefas_em_execucao[i] = cozinha->tarefas_em_execucao[cozinha->num_tarefas_em_execucao - 1];
                        cozinha->num_tarefas_em_execucao--;
                        continue;
                    }

                    printf("[Loja %d | Tempo: %ds] %s do Pedido #%d foi concluido.\n",
                           cozinha->loja, cozinha->tempo_atual, NOMES_ITENS[tarefa_concluida->tipo_item], pedido_pai->id);

                    if (tarefa_concluida->tipo_item == ITEM_MONTAGEM) {
                        int tempo_de_producao = cozinha->tempo_atual - pedido_pai->tempo_chegada;
                        if (tempo_de_producao <= TEMPO_MAX_ATENDIMENTO) {
                            cozinha->atendidos_no_prazo++;
                            pedido_pai->status = STATUS_CONCLUIDO_NO_PRAZO;
                            printf(">> [Loja %d] Pedido #%d FINALIZADO com SUCESSO. Tempo: %ds.\n", cozinha->loja, pedido_pai->id, tempo_de_producao);
                        } else {
                            cozinha->atendidos_com_atraso++;
                            pedido_pai->status = STATUS_CONCLUIDO_ATRASADO;
                            printf(">> [Loja %d] Pedido #%d FINALIZADO com ATRASO. Tempo: %ds (PREJUIZO).\n", cozinha->loja, pedido_pai->id, tempo_de_producao);
                        }
                        imprimir_composicao_bandejas(pedido_pai);

                        // Remove de em_andamento e libera memória
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
                            printf("[Loja %d | Tempo: %ds] Pedido #%d pronto para montagem.\n", cozinha->loja, cozinha->tempo_atual, pedido_pai->id);
                        }
                    }

                    // Remove tarefa concluída (swap-and-pop)
                    cozinha->tarefas_em_execucao[i] = cozinha->tarefas_em_execucao[cozinha->num_tarefas_em_execucao - 1];
                    cozinha->num_tarefas_em_execucao--;
                }
            }
        } while (alguma_acao_ocorreu);

        // Avanço de tempo: próximo término de tarefa
        int proximo_tempo_tarefa = INT_MAX;
        if (cozinha->num_tarefas_em_execucao > 0) {
            for (int i = 0; i < cozinha->num_tarefas_em_execucao; i++) {
                if (cozinha->tarefas_em_execucao[i].tempo_conclusao < proximo_tempo_tarefa) {
                    proximo_tempo_tarefa = cozinha->tarefas_em_execucao[i].tempo_conclusao;
                }
            }
        }

        if (proximo_tempo_tarefa == INT_MAX) {
            if (cozinha->total_pedidos_criados > (cozinha->atendidos_no_prazo + cozinha->atendidos_com_atraso)) {
                if (cozinha->num_tarefas_na_fila_preparo > 0) {
                    printf("AVISO [Loja %d]: Simulacao parada. Existem %d tarefas na fila, mas nenhum funcionario disponivel/habilitado (gargalo).\n",
                           cozinha->loja, cozinha->num_tarefas_na_fila_preparo);
                } else {
                    printf("AVISO [Loja %d]: Simulacao parada, mas nem todos os pedidos foram concluidos. Verifique logica.\n", cozinha->loja);
                }
            }
            break;
        }

        if (proximo_tempo_tarefa > cozinha->tempo_atual) {
            cozinha->tempo_atual = proximo_tempo_tarefa;
        } else {
            cozinha->tempo_atual++; // segurança
        }
    }

    printf("\n=== LOJA %d: SIMULACAO FINALIZADA EM %d s ===\n", cozinha->loja, cozinha->tempo_atual);
    printf("Total de Pedidos: %d\n", cozinha->total_pedidos_criados);
    printf("Atendidos no Prazo: %d\n", cozinha->atendidos_no_prazo);
    printf("Atendidos com Atraso (Prejuizo): %d\n", cozinha->atendidos_com_atraso);
}

/* =============================================================================
   5. FUNÇÃO PRINCIPAL (DUAS LOJAS)
   ============================================================================= */

int main(void) {
    Cozinha loja1, loja2;
    inicializar_cozinha(&loja1, 1);
    inicializar_cozinha(&loja2, 2);

    coletar_pedidos_duas_lojas(&loja1, &loja2);

    if (loja1.total_pedidos_criados > 0) executar_simulacao(&loja1);
    if (loja2.total_pedidos_criados > 0) executar_simulacao(&loja2);

    limpar_cozinha(&loja1);
    limpar_cozinha(&loja2);
    return 0;
}

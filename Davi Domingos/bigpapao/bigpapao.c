/*
  Simulação de cozinha (duas lojas + CSV + Tempo Global + next-event)
  - ÚNICA ENTRADA: arquivo CSV no formato tempo_global,destino,q1..q7 (ver exemplo abaixo).
  - tempo_global = segundo desde a abertura em que o pedido foi feito (timestamp de chegada).
  - destino = 1 (Loja 1) ou 2 (Loja 2).
  - q1..q7 = quantidades dos 7 itens do cardápio, na ordem de NOMES_ITENS.
  - A simulação por loja usa avanço por próximo evento: min(chegada, término de tarefa).

  Compilação sugerida:
    gcc -std=c11 -Wall -Wextra -Wpedantic -O2 bigpapao.c -o bigpapao

  Execução:
    ./bigpapao pedidos.csv
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <errno.h>

/* =============================================================================
   0. FUNÇÕES UTILITÁRIAS
   ============================================================================= */

static int safe_realloc_array(void **pp, size_t count, size_t elem_size) {
    if (count != 0 && elem_size > SIZE_MAX / count) return -1;
    void *tmp = realloc(*pp, count * elem_size);
    if (tmp == NULL && count != 0) return -2;
    *pp = tmp;
    return 0;
}

/* =============================================================================
   1. DEFINIÇÕES E ESTRUTURAS DE DADOS
   ============================================================================= */

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
    int tempo_chegada;               // Tempo Global (timestamp desde a abertura)
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
    int capacidade_por_funcionario;
    int validade_produto_min;
} Equipamento;

typedef struct Cozinha {
    int tempo_atual;

    Funcionario funcionarios[NUM_FUNCIONARIOS];
    Equipamento chapa;
    Equipamento fritadeira;
    Equipamento liquidificador;

    Pedido *pedidos_na_fila_espera;      // lista ligada ordenada por tempo_chegada asc
    Pedido **pedidos_em_andamento;
    int num_pedidos_em_andamento;
    Tarefa *tarefas_na_fila_preparo;
    int num_tarefas_na_fila_preparo;
    Tarefa *tarefas_em_execucao;
    int num_tarefas_em_execucao;

    int total_pedidos_criados;
    int atendidos_no_prazo;
    int atendidos_com_atraso;

    int loja; // 1 ou 2
} Cozinha;

static int item_e_bebida(TipoItem t) {
    return (t == ITEM_REFRIGERANTE || t == ITEM_SUCO || t == ITEM_MILK_SHAKE);
}

/* =============================================================================
   2. INICIALIZAÇÃO, FILAS E LIMPEZA
   ============================================================================= */

void inicializar_cozinha(Cozinha *cozinha, int loja) {
    memset(cozinha, 0, sizeof(Cozinha));
    cozinha->loja = loja;
    cozinha->chapa.capacidade_por_funcionario = 3;
    cozinha->fritadeira.capacidade_por_funcionario = 2;
    cozinha->fritadeira.validade_produto_min = 60;
    cozinha->liquidificador.capacidade_por_funcionario = 4;
    cozinha->liquidificador.validade_produto_min = 30;

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

static void adicionar_pedido_na_fila_espera_ordenado(Cozinha *c, Pedido *novo_pedido) {
    novo_pedido->proximo = NULL;
    if (c->pedidos_na_fila_espera == NULL || novo_pedido->tempo_chegada < c->pedidos_na_fila_espera->tempo_chegada) {
        novo_pedido->proximo = c->pedidos_na_fila_espera;
        c->pedidos_na_fila_espera = novo_pedido;
        return;
    }
    Pedido *cur = c->pedidos_na_fila_espera;
    while (cur->proximo && cur->proximo->tempo_chegada <= novo_pedido->tempo_chegada) cur = cur->proximo;
    novo_pedido->proximo = cur->proximo;
    cur->proximo = novo_pedido;
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
   3. IMPRESSÃO E BUSCAS
   ============================================================================= */

void imprimir_composicao_bandejas(Pedido *pedido) {
    printf("\n--- Composicao do Pedido #%d ---\n", pedido->id);
    TipoItem *itens_comer = (TipoItem *)malloc(sizeof(TipoItem) * (size_t)pedido->num_itens);
    TipoItem *itens_beber = (TipoItem *)malloc(sizeof(TipoItem) * (size_t)pedido->num_itens);
    if (!itens_comer || !itens_beber) {
        perror("Falha ao alocar memoria para impressao de bandejas");
        free(itens_comer); free(itens_beber);
        return;
    }
    int count_comer = 0, count_beber = 0;
    for (int i = 0; i < pedido->num_itens; i++) {
        if (item_e_bebida(pedido->itens[i])) itens_beber[count_beber++] = pedido->itens[i];
        else itens_comer[count_comer++] = pedido->itens[i];
    }
    int bandeja_num = 1, idxc = 0, idxb = 0;
    while (idxc < count_comer || idxb < count_beber) {
        printf("Bandeja %d:\n", bandeja_num++);
        for (int i = 0; i < 2 && idxc < count_comer; i++) printf("  - %s\n", NOMES_ITENS[itens_comer[idxc++]]);
        for (int i = 0; i < 2 && idxb < count_beber; i++) printf("  - %s\n", NOMES_ITENS[itens_beber[idxb++]]);
    }
    printf("---------------------------------\n");
    free(itens_comer); free(itens_beber);
}

Pedido *encontrar_pedido_em_andamento(Cozinha *c, int pedido_id) {
    for (int i = 0; i < c->num_pedidos_em_andamento; i++) {
        if (c->pedidos_em_andamento[i]->id == pedido_id) return c->pedidos_em_andamento[i];
    }
    return NULL;
}

/* =============================================================================
   4. MOTOR (DESPACHO E LOOP DE EVENTOS)
   ============================================================================= */

void despachar_tarefas(Cozinha *c) {
    Habilidade habilidades[] = {HABILIDADE_SANDUICHE, HABILIDADE_BATATA, HABILIDADE_BEBIDAS, HABILIDADE_MONTAGEM};
    for (int h = 0; h < 4; h++) {
        int funcs_livres[NUM_FUNCIONARIOS], nlivres = 0;
        for (int i = 0; i < NUM_FUNCIONARIOS; i++) {
            if ((c->funcionarios[i].habilidades & habilidades[h]) && (c->funcionarios[i].livre_a_partir_de <= c->tempo_atual)) {
                funcs_livres[nlivres++] = i;
            }
        }
        if (nlivres == 0) continue;

        int capacidade_total = nlivres;
        int alocadas = 0;

        for (int i = c->num_tarefas_na_fila_preparo - 1; i >= 0; i--) {
            if (alocadas >= capacidade_total) break;
            Tarefa *t = &c->tarefas_na_fila_preparo[i];
            int ok = 0;
            switch (habilidades[h]) {
                case HABILIDADE_SANDUICHE:
                    ok = (t->tipo_item >= ITEM_SANDUICHE_SIMPLES && t->tipo_item <= ITEM_SANDUICHE_ELABORADO); break;
                case HABILIDADE_BATATA:   ok = (t->tipo_item == ITEM_BATATA_FRITA); break;
                case HABILIDADE_BEBIDAS:  ok = item_e_bebida(t->tipo_item); break;
                case HABILIDADE_MONTAGEM: ok = (t->tipo_item == ITEM_MONTAGEM); break;
            }
            if (!ok) continue;

            int idf = funcs_livres[alocadas];
            t->tempo_conclusao = c->tempo_atual + TEMPOS_DE_PREPARO[t->tipo_item];
            c->funcionarios[idf].livre_a_partir_de = t->tempo_conclusao;

            c->num_tarefas_em_execucao++;
            void *pp = c->tarefas_em_execucao;
            int rc = safe_realloc_array(&pp, (size_t)c->num_tarefas_em_execucao, sizeof(Tarefa));
            if (rc != 0) { perror("Falha alocar tarefas_em_execucao"); exit(1); }
            c->tarefas_em_execucao = (Tarefa *)pp;
            c->tarefas_em_execucao[c->num_tarefas_em_execucao - 1] = *t;

            Pedido *p = encontrar_pedido_em_andamento(c, t->pedido_id);
            int tg = p ? p->tempo_chegada : -1;
            printf("[Loja %d | Tempo: %ds | Tempo Global do Pedido: %ds] Func. #%d iniciou %s do Pedido #%d (conclui em %ds).\n",
                   c->loja, c->tempo_atual, tg, c->funcionarios[idf].id, NOMES_ITENS[t->tipo_item],
                   t->pedido_id, t->tempo_conclusao);

            c->tarefas_na_fila_preparo[i] = c->tarefas_na_fila_preparo[c->num_tarefas_na_fila_preparo - 1];
            c->num_tarefas_na_fila_preparo--;
            alocadas++;
        }
    }
}

static void iniciar_pedido(Cozinha *c, Pedido *p) {
    p->status = STATUS_EM_PREPARO;
    c->num_pedidos_em_andamento++;
    void *pp = c->pedidos_em_andamento;
    int rc = safe_realloc_array(&pp, (size_t)c->num_pedidos_em_andamento, sizeof(Pedido *));
    if (rc != 0) { perror("Falha alocar pedidos_em_andamento"); exit(1); }
    c->pedidos_em_andamento = (Pedido **)pp;
    c->pedidos_em_andamento[c->num_pedidos_em_andamento - 1] = p;

    printf("[Loja %d | Tempo: %ds | Tempo Global do Pedido: %ds] Iniciando preparo do Pedido #%d.\n",
           c->loja, c->tempo_atual, p->tempo_chegada, p->id);

    for (int i = 0; i < p->num_itens; i++) {
        Tarefa t = (Tarefa){p->id, p->itens[i], 0};
        c->num_tarefas_na_fila_preparo++;
        void *pp2 = c->tarefas_na_fila_preparo;
        rc = safe_realloc_array(&pp2, (size_t)c->num_tarefas_na_fila_preparo, sizeof(Tarefa));
        if (rc != 0) { perror("Falha alocar tarefas_na_fila_preparo"); exit(1); }
        c->tarefas_na_fila_preparo = (Tarefa *)pp2;
        c->tarefas_na_fila_preparo[c->num_tarefas_na_fila_preparo - 1] = t;
    }
    c->total_pedidos_criados++;
}

void executar_simulacao(Cozinha *c) {
    printf("\n=== INICIANDO SIMULACAO (Loja %d) ===\n", c->loja);

    while (c->pedidos_na_fila_espera != NULL ||
           c->num_tarefas_na_fila_preparo > 0 ||
           c->num_tarefas_em_execucao > 0 ||
           c->num_pedidos_em_andamento > 0) {

        int houve_acao;
        do {
            houve_acao = 0;

            // 1) Chegadas agendadas no tempo atual (Tempo Global)
            while (c->pedidos_na_fila_espera && c->pedidos_na_fila_espera->tempo_chegada <= c->tempo_atual) {
                Pedido *p = c->pedidos_na_fila_espera;
                c->pedidos_na_fila_espera = p->proximo;
                p->proximo = NULL;
                iniciar_pedido(c, p);
                houve_acao = 1;
            }

            // 2) Despacho de tarefas
            int antes = c->num_tarefas_em_execucao;
            despachar_tarefas(c);
            if (c->num_tarefas_em_execucao > antes) houve_acao = 1;

            // 3) Conclusões de tarefas no tempo atual
            for (int i = c->num_tarefas_em_execucao - 1; i >= 0; i--) {
                Tarefa *t = &c->tarefas_em_execucao[i];
                if (t->tempo_conclusao <= c->tempo_atual) {
                    houve_acao = 1;
                    Pedido *p = encontrar_pedido_em_andamento(c, t->pedido_id);
                    if (p) {
                        printf("[Loja %d | Tempo: %ds | Tempo Global do Pedido: %ds] %s do Pedido #%d foi concluido.\n",
                               c->loja, c->tempo_atual, p->tempo_chegada, NOMES_ITENS[t->tipo_item], p->id);
                        if (t->tipo_item == ITEM_MONTAGEM) {
                            int tempo_prod = c->tempo_atual - p->tempo_chegada;
                            if (tempo_prod <= TEMPO_MAX_ATENDIMENTO) {
                                c->atendidos_no_prazo++;
                                p->status = STATUS_CONCLUIDO_NO_PRAZO;
                                printf(">> [Loja %d] Pedido #%d FINALIZADO no prazo. Tempo (Global): %ds.\n",
                                       c->loja, p->id, tempo_prod);
                            } else {
                                c->atendidos_com_atraso++;
                                p->status = STATUS_CONCLUIDO_ATRASADO;
                                printf(">> [Loja %d] Pedido #%d FINALIZADO com atraso. Tempo (Global): %ds.\n",
                                       c->loja, p->id, tempo_prod);
                            }
                            imprimir_composicao_bandejas(p);

                            // remove pedido de em_andamento e libera memória
                            int idx = -1;
                            for (int j = 0; j < c->num_pedidos_em_andamento; j++) {
                                if (c->pedidos_em_andamento[j]->id == p->id) { idx = j; break; }
                            }
                            if (idx != -1) {
                                free(p->itens);
                                free(p);
                                c->pedidos_em_andamento[idx] =
                                    c->pedidos_em_andamento[c->num_pedidos_em_andamento - 1];
                                c->num_pedidos_em_andamento--;
                            }
                        } else {
                            p->tarefas_preparo_restantes--;
                            if (p->tarefas_preparo_restantes == 0 && p->status == STATUS_EM_PREPARO) {
                                p->status = STATUS_AGUARDANDO_MONTAGEM;
                                Tarefa tm = (Tarefa){p->id, ITEM_MONTAGEM, 0};
                                c->num_tarefas_na_fila_preparo++;
                                void *pp2 = c->tarefas_na_fila_preparo;
                                int rc = safe_realloc_array(&pp2, (size_t)c->num_tarefas_na_fila_preparo, sizeof(Tarefa));
                                if (rc != 0) { perror("Falha alocar tarefas_na_fila_preparo"); exit(1); }
                                c->tarefas_na_fila_preparo = (Tarefa *)pp2;
                                c->tarefas_na_fila_preparo[c->num_tarefas_na_fila_preparo - 1] = tm;
                                printf("[Loja %d | Tempo: %ds | Tempo Global do Pedido: %ds] Pedido #%d pronto para montagem.\n",
                                       c->loja, c->tempo_atual, p->tempo_chegada, p->id);
                            }
                        }
                    }
                    // remove tarefa concluída
                    c->tarefas_em_execucao[i] = c->tarefas_em_execucao[c->num_tarefas_em_execucao - 1];
                    c->num_tarefas_em_execucao--;
                }
            }
        } while (houve_acao);

        // Próximo evento: menor entre próxima conclusão e próxima chegada
        int proximo_tempo_tarefa = INT_MAX;
        for (int i = 0; i < c->num_tarefas_em_execucao; i++) {
            if (c->tarefas_em_execucao[i].tempo_conclusao < proximo_tempo_tarefa) {
                proximo_tempo_tarefa = c->tarefas_em_execucao[i].tempo_conclusao;
            }
        }
        int proximo_tempo_chegada = INT_MAX;
        if (c->pedidos_na_fila_espera) proximo_tempo_chegada = c->pedidos_na_fila_espera->tempo_chegada;

        int proximo_evento = proximo_tempo_tarefa < proximo_tempo_chegada
                             ? proximo_tempo_tarefa
                             : proximo_tempo_chegada;

        if (proximo_evento == INT_MAX) break;
        if (proximo_evento > c->tempo_atual) c->tempo_atual = proximo_evento;
        else c->tempo_atual++;
    }

    printf("\n=== LOJA %d: SIMULACAO FINALIZADA EM %d s ===\n", c->loja, c->tempo_atual);
    printf("Total processados: %d\n", c->atendidos_no_prazo + c->atendidos_com_atraso);
    printf("No prazo: %d | Atrasados: %d\n", c->atendidos_no_prazo, c->atendidos_com_atraso);
}

/* =============================================================================
   5. LEITURA DE CSV (tempo_global,destino,q1..q7)
   ============================================================================= */

static int parse_int(const char *s, int *out) {
    char *end = NULL;
    errno = 0;
    long v = strtol(s, &end, 10);
    if (errno != 0) return -1;
    if (end == s) return -2;
    while (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n') end++;
    if (*end != '\0') return -3;
    if (v < INT_MIN || v > INT_MAX) return -4;
    *out = (int)v;
    return 0;
}

static Pedido* criar_pedido_por_quantidades(int id_pedido, int tempo_chegada, const int q[7]) {
    int total = 0;
    for (int i = 0; i < 7; i++) {
        if (q[i] < 0) return NULL;
        total += q[i];
    }
    if (total <= 0) return NULL;

    Pedido *p = (Pedido *)malloc(sizeof(Pedido));
    if (!p) return NULL;
    p->id = id_pedido;
    p->tempo_chegada = tempo_chegada;
    p->status = STATUS_NA_FILA;
    p->tarefas_preparo_restantes = total;
    p->num_itens = total;
    p->proximo = NULL;
    p->itens = (TipoItem *)malloc(sizeof(TipoItem) * (size_t)total);
    if (!p->itens) { free(p); return NULL; }

    int idx = 0;
    for (int i = 0; i < 7; i++) for (int k = 0; k < q[i]; k++) p->itens[idx++] = (TipoItem)i;
    return p;
}

static int carregar_pedidos_csv(const char *caminho, Cozinha *l1, Cozinha *l2, int *proximo_id_global) {
    FILE *f = fopen(caminho, "r");
    if (!f) { perror("Nao foi possivel abrir o CSV"); return -1; }

    char linha[1024];
    int lnum = 0, carregados = 0;

    // Detecta header textual (1ª célula não numérica)
    long pos0 = ftell(f);
    if (fgets(linha, sizeof linha, f)) {
        lnum++;
        char tmp[1024];
        strncpy(tmp, linha, sizeof tmp); tmp[sizeof tmp - 1] = '\0';
        char *tok = strtok(tmp, ",");
        int ntest;
        if (tok && parse_int(tok, &ntest) == 0) {
            fseek(f, pos0, SEEK_SET);
            lnum = 0;
        }
    } else { fclose(f); return 0; }

    while (fgets(linha, sizeof linha, f)) {
        lnum++;
        char *campos[9] = {0}; int n = 0;
        for (char *tok = strtok(linha, ","); tok && n < 9; tok = strtok(NULL, ",")) {
            while (*tok == ' ' || *tok == '\t') tok++;
            char *e = tok + strlen(tok);
            while (e > tok && (e[-1] == ' ' || e[-1] == '\t' || e[-1] == '\r' || e[-1] == '\n')) e--;
            *e = '\0';
            campos[n++] = tok;
        }
        if (n == 0) continue;
        if (n != 9) { fprintf(stderr, "Linha %d ignorada: esperado 9 colunas, obtido %d\n", lnum, n); continue; }

        int tempo_global = 0, destino = 0, q[7] = {0};
        int ok = 1;
        if (parse_int(campos[0], &tempo_global) != 0) ok = 0;
        if (parse_int(campos[1], &destino) != 0) ok = 0;
        for (int i = 0; i < 7; i++) if (parse_int(campos[2 + i], &q[i]) != 0) { ok = 0; break; }
        if (!ok) { fprintf(stderr, "Linha %d ignorada: valores invalidos\n", lnum); continue; }
        if (!(destino == 1 || destino == 2)) { fprintf(stderr, "Linha %d ignorada: destino deve ser 1 ou 2\n", lnum); continue; }

        Pedido *p = criar_pedido_por_quantidades((*proximo_id_global)++, tempo_global, q);
        if (!p) { fprintf(stderr, "Linha %d ignorada: pedido vazio ou erro de memoria\n", lnum); continue; }

        if (destino == 1) adicionar_pedido_na_fila_espera_ordenado(l1, p);
        else adicionar_pedido_na_fila_espera_ordenado(l2, p);

        carregados++;
    }
    fclose(f);
    return carregados;
}

/* =============================================================================
   6. MAIN
   ============================================================================= */

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s caminho.csv\n", argv[0]);
        return 1;
    }

    Cozinha loja1, loja2;
    inicializar_cozinha(&loja1, 1);
    inicializar_cozinha(&loja2, 2);

    int next_id = 1;
    int n = carregar_pedidos_csv(argv[1], &loja1, &loja2, &next_id);
    if (n < 0) return 1;
    printf("[Sistema] %d pedidos carregados do CSV.\n", n);

    executar_simulacao(&loja1);
    executar_simulacao(&loja2);

    limpar_cozinha(&loja1);
    limpar_cozinha(&loja2);
    return 0;
}

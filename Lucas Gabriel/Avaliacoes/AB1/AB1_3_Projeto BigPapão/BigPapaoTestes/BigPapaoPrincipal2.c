#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

// =============================================================================
// 1. DEFINIÇÕES E ESTRUTURAS DE DADOS
// =============================================================================

#define NUM_FUNCIONARIOS 13
#define TEMPO_MAX_ATENDIMENTO 300

typedef enum
{
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
    "Batata Frita", "Refrigerante", "Suco", "Milk Shake", "Montagem de Bandeja"};

typedef enum
{
    STATUS_NA_FILA,
    STATUS_EM_PREPARO,
    STATUS_AGUARDANDO_MONTAGEM,
    STATUS_EM_MONTAGEM,
    STATUS_CONCLUIDO_NO_PRAZO,
    STATUS_CONCLUIDO_ATRASADO
} StatusPedido;

// --- ALTERAÇÃO 1: 'HABILIDADE_SUCO' foi removida e as outras renumeradas ---
typedef enum
{
    HABILIDADE_SANDUICHE = 1 << 0,
    HABILIDADE_BATATA = 1 << 1,
    HABILIDADE_BEBIDAS = 1 << 2,
    HABILIDADE_MONTAGEM = 1 << 3 // Antes era 1 << 4
} Habilidade;

const int TEMPOS_DE_PREPARO[] = {58, 88, 105, 190, 5, 38, 60, 30};

typedef struct Tarefa
{
    int pedido_id;
    TipoItem tipo_item;
    int tempo_conclusao;
} Tarefa;

typedef struct Pedido
{
    int id;
    int tempo_chegada;
    StatusPedido status;
    int tarefas_preparo_restantes;
    TipoItem *itens;
    int num_itens;
    struct Pedido *proximo;
} Pedido;

typedef struct
{
    int id;
    unsigned int habilidades;
    int livre_a_partir_de;
} Funcionario;

typedef struct
{
    int capacidade_por_funcionario;
    int validade_produto_min;
} Equipamento;

typedef struct
{
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
// 2. FUNÇÕES DE GERENCIAMENTO DA COZINHA
// =============================================================================

void inicializar_cozinha(Cozinha *cozinha)
{
    memset(cozinha, 0, sizeof(Cozinha));

    printf("--- Configurando a Cozinha com as Regras da Equipe ---\n");
    cozinha->chapa.capacidade_por_funcionario = 3;
    cozinha->fritadeira.capacidade_por_funcionario = 2;
    cozinha->fritadeira.validade_produto_min = 60;
    cozinha->liquidificador.capacidade_por_funcionario = 4;
    cozinha->liquidificador.validade_produto_min = 30;
    printf("Capacidade por funcionario: %d sanduiches, %d batatas, %d milk-shakes.\n\n",
           cozinha->chapa.capacidade_por_funcionario,
           cozinha->fritadeira.capacidade_por_funcionario,
           cozinha->liquidificador.capacidade_por_funcionario);

    // Mapeamento dos 13 funcionários e suas habilidades
    // (ID, HABILIDADES, LIVRE_A_PARTIR_DE)

    cozinha->funcionarios[0] = (Funcionario){1, HABILIDADE_SANDUICHE | HABILIDADE_BATATA, 0};
    cozinha->funcionarios[1] = (Funcionario){2, HABILIDADE_SANDUICHE | HABILIDADE_BATATA, 0};
    cozinha->funcionarios[2] = (Funcionario){3, HABILIDADE_SANDUICHE | HABILIDADE_BEBIDAS, 0};
    cozinha->funcionarios[3] = (Funcionario){4, HABILIDADE_SANDUICHE, 0};
    cozinha->funcionarios[4] = (Funcionario){5, HABILIDADE_SANDUICHE, 0};
    cozinha->funcionarios[5] = (Funcionario){6, HABILIDADE_SANDUICHE | HABILIDADE_BATATA, 0};
    cozinha->funcionarios[6] = (Funcionario){7, HABILIDADE_BATATA, 0};
    cozinha->funcionarios[7] = (Funcionario){8, HABILIDADE_BEBIDAS | HABILIDADE_MONTAGEM, 0};
    cozinha->funcionarios[8] = (Funcionario){9, HABILIDADE_MONTAGEM, 0};
    cozinha->funcionarios[9] = (Funcionario){10, 0 | 0, 0};                     // Habilidade de (separação / caixa ) ---> não produtiva
    cozinha->funcionarios[10] = (Funcionario){11, 0 | HABILIDADE_SANDUICHE, 0}; // Habilidade de sanduiche / (separação) --> não produtiva
    cozinha->funcionarios[11] = (Funcionario){12, 0 | HABILIDADE_BEBIDAS, 0};   // Habilidade de bebidas / (caixa) --> não produtiva
    cozinha->funcionarios[12] = (Funcionario){13, 0, 0};                        // Habilidade de (caixa) --> não produtiva
}

void adicionar_pedido_na_fila_espera(Cozinha *c, Pedido *novo_pedido)
{
    novo_pedido->proximo = NULL;
    if (c->pedidos_na_fila_espera == NULL)
    {
        c->pedidos_na_fila_espera = novo_pedido;
    }
    else
    {
        Pedido *atual = c->pedidos_na_fila_espera;
        while (atual->proximo != NULL)
            atual = atual->proximo;
        atual->proximo = novo_pedido;
    }
}

void limpar_cozinha(Cozinha *c)
{
    free(c->tarefas_em_execucao);
    free(c->tarefas_na_fila_preparo);
    for (int i = 0; i < c->num_pedidos_em_andamento; i++)
    {
        free(c->pedidos_em_andamento[i]->itens);
        free(c->pedidos_em_andamento[i]);
    }
    free(c->pedidos_em_andamento);
    Pedido *atual = c->pedidos_na_fila_espera;
    while (atual != NULL)
    {
        Pedido *temp = atual;
        atual = atual->proximo;
        free(temp->itens);
        free(temp);
    }
}

// =============================================================================
// 3. MENU INTERATIVO PARA COLETAR PEDIDOS
// =============================================================================
// (Esta função não precisa de alterações)
void coletar_pedidos_do_usuario(Cozinha *c)
{
    int num_pedidos_iniciais;
    printf("Bem-vindo ao sistema de simulacao do BigPapao!\n");
    printf("Quantos pedidos iniciais deseja registrar? ");
    scanf("%d", &num_pedidos_iniciais);

    for (int i = 0; i < num_pedidos_iniciais; i++)
    {
        int id_pedido_atual = c->total_pedidos_criados + 1;
        printf("\n--- Registrando Pedido #%d ---\n", id_pedido_atual);

        TipoItem *itens_do_pedido = NULL;
        int qtd_itens = 0;
        int escolha = -1;

        do
        {
            printf("Escolha os itens para o pedido #%d:\n", id_pedido_atual);
            for (int j = 0; j < 7; j++)
                printf("  %d. %s\n", j + 1, NOMES_ITENS[j]);
            printf("  0. Finalizar Pedido\n");
            printf("Digite o numero do item: ");
            scanf("%d", &escolha);

            if (escolha >= 1 && escolha <= 7)
            {
                int quantidade;
                printf("Digite a quantidade do item: ");
                scanf("%d", &quantidade);
                if (quantidade < 1)
                {
                    printf("Quantidade invalida, adicionando 1 unidade.\n");
                    quantidade = 1;
                }
                for (int k = 0; k < quantidade; k++)
                {
                    qtd_itens++;
                    itens_do_pedido = (TipoItem *)realloc(itens_do_pedido, sizeof(TipoItem) * qtd_itens);
                    itens_do_pedido[qtd_itens - 1] = (TipoItem)(escolha - 1);
                }
                printf(">> (%d) - '%s' adicionado(s) ao pedido.\n\n", quantidade, NOMES_ITENS[escolha - 1]);
            }
            else if (escolha != 0)
            {
                printf("Opcao invalida! Tente novamente.\n\n");
            }

        } while (escolha != 0);

        if (qtd_itens > 0)
        {
            Pedido *novo_pedido = (Pedido *)malloc(sizeof(Pedido));
            novo_pedido->id = id_pedido_atual;
            novo_pedido->tempo_chegada = 0;
            novo_pedido->status = STATUS_NA_FILA;
            novo_pedido->itens = itens_do_pedido;
            novo_pedido->num_itens = qtd_itens;
            novo_pedido->tarefas_preparo_restantes = qtd_itens;

            adicionar_pedido_na_fila_espera(c, novo_pedido);
            c->total_pedidos_criados++;
            printf(">> Pedido #%d finalizado com %d itens e adicionado a fila.\n", id_pedido_atual, qtd_itens);
        }
        else
        {
            printf(">> Pedido #%d cancelado por nao ter itens.\n", id_pedido_atual);
        }
    }
}

// =============================================================================
// 4. O MOTOR DA SIMULAÇÃO
// =============================================================================

Pedido *encontrar_pedido_em_andamento(Cozinha *c, int pedido_id)
{
    for (int i = 0; i < c->num_pedidos_em_andamento; i++)
    {
        if (c->pedidos_em_andamento[i]->id == pedido_id)
            return c->pedidos_em_andamento[i];
    }
    return NULL;
}

// --- ALTERAÇÃO 3: Lógica de despacho agora unifica as bebidas ---
void despachar_tarefas(Cozinha *c)
{
    // Listas para guiar o despacho. Note que SUCO não está mais aqui.
    Habilidade habilidades[] = {HABILIDADE_SANDUICHE, HABILIDADE_BATATA, HABILIDADE_BEBIDAS, HABILIDADE_MONTAGEM};
    Equipamento *equipamentos[] = {&c->chapa, &c->fritadeira, &c->liquidificador, NULL};

    for (int h = 0; h < 4; h++)
    {
        int funcs_livres[NUM_FUNCIONARIOS];
        int num_funcs_livres = 0;
        for (int i = 0; i < NUM_FUNCIONARIOS; i++)
        {
            if ((c->funcionarios[i].habilidades & habilidades[h]) && (c->funcionarios[i].livre_a_partir_de <= c->tempo_atual))
            {
                funcs_livres[num_funcs_livres++] = i;
            }
        }

        if (num_funcs_livres == 0)
            continue;

        int capacidade_total = num_funcs_livres * (equipamentos[h] ? equipamentos[h]->capacidade_por_funcionario : 1);
        int tarefas_alocadas_nesta_rodada = 0;

        for (int i = c->num_tarefas_na_fila_preparo - 1; i >= 0; i--)
        {
            if (tarefas_alocadas_nesta_rodada >= capacidade_total)
                break;

            Tarefa *tarefa_atual = &c->tarefas_na_fila_preparo[i];
            int tarefa_corresponde = 0;

            // Condição unificada para todas as bebidas
            if (habilidades[h] == HABILIDADE_BEBIDAS)
            {
                if (tarefa_atual->tipo_item == ITEM_REFRIGERANTE || tarefa_atual->tipo_item == ITEM_SUCO || tarefa_atual->tipo_item == ITEM_MILK_SHAKE)
                {
                    tarefa_corresponde = 1;
                }
            }
            // Condição para Sanduíches
            else if (habilidades[h] == HABILIDADE_SANDUICHE)
            {
                if (tarefa_atual->tipo_item >= ITEM_SANDUICHE_SIMPLES && tarefa_atual->tipo_item <= ITEM_SANDUICHE_ELABORADO)
                {
                    tarefa_corresponde = 1;
                }
            }
            // Condições para outras tarefas diretas
            else
            {
                TipoItem tipo_mapeado;
                if (habilidades[h] == HABILIDADE_BATATA)
                    tipo_mapeado = ITEM_BATATA_FRITA;
                else if (habilidades[h] == HABILIDADE_MONTAGEM)
                    tipo_mapeado = ITEM_MONTAGEM;

                if (tarefa_atual->tipo_item == tipo_mapeado)
                {
                    tarefa_corresponde = 1;
                }
            }

            if (tarefa_corresponde)
            {
                int id_func_alocado = funcs_livres[tarefas_alocadas_nesta_rodada % num_funcs_livres];
                tarefa_atual->tempo_conclusao = c->tempo_atual + TEMPOS_DE_PREPARO[tarefa_atual->tipo_item];

                c->funcionarios[id_func_alocado].livre_a_partir_de = tarefa_atual->tempo_conclusao;

                c->num_tarefas_em_execucao++;
                c->tarefas_em_execucao = (Tarefa *)realloc(c->tarefas_em_execucao, sizeof(Tarefa) * c->num_tarefas_em_execucao);
                c->tarefas_em_execucao[c->num_tarefas_em_execucao - 1] = *tarefa_atual;

                printf("[Tempo: %ds] Func. #%d iniciou %s do Pedido #%d (conclui em %ds).\n", c->tempo_atual, id_func_alocado + 1, NOMES_ITENS[tarefa_atual->tipo_item], tarefa_atual->pedido_id, tarefa_atual->tempo_conclusao);

                c->tarefas_na_fila_preparo[i] = c->tarefas_na_fila_preparo[c->num_tarefas_na_fila_preparo - 1];
                c->num_tarefas_na_fila_preparo--;

                tarefas_alocadas_nesta_rodada++;
            }
        }
    }
}

void executar_simulacao(Cozinha *cozinha)
{
    printf("\n=== INICIANDO SIMULACAO DA COZINHA BIGPAPAO ===\n");

    while (cozinha->total_pedidos_criados > (cozinha->atendidos_no_prazo + cozinha->atendidos_com_atraso))
    {
        // --- NOVA LÓGICA DE LOOP INTERNO ---
        // Este loop garante que o despachante rode várias vezes no mesmo instante de tempo
        // se novas tarefas forem criadas, até que a situação se estabilize.
        int alguma_acao_ocorreu;
        do
        {
            alguma_acao_ocorreu = 0; // Reseta a flag no início de cada micro-passo

            // ETAPA 1: Iniciar preparo de novos pedidos que chegaram.
            if (cozinha->pedidos_na_fila_espera != NULL && cozinha->pedidos_na_fila_espera->tempo_chegada <= cozinha->tempo_atual)
            {
                alguma_acao_ocorreu = 1; // Uma ação ocorreu: um novo pedido começou

                Pedido *pedido_iniciado = cozinha->pedidos_na_fila_espera;
                cozinha->pedidos_na_fila_espera = cozinha->pedidos_na_fila_espera->proximo;

                pedido_iniciado->status = STATUS_EM_PREPARO;
                cozinha->num_pedidos_em_andamento++;
                cozinha->pedidos_em_andamento = (Pedido **)realloc(cozinha->pedidos_em_andamento, sizeof(Pedido *) * cozinha->num_pedidos_em_andamento);
                cozinha->pedidos_em_andamento[cozinha->num_pedidos_em_andamento - 1] = pedido_iniciado;

                printf("[Tempo: %ds] Iniciando preparo do Pedido #%d.\n", cozinha->tempo_atual, pedido_iniciado->id);
                for (int i = 0; i < pedido_iniciado->num_itens; i++)
                {
                    // A correção que já havíamos discutido, com 3 valores.
                    Tarefa t = {pedido_iniciado->id, pedido_iniciado->itens[i], 0};
                    cozinha->num_tarefas_na_fila_preparo++;
                    cozinha->tarefas_na_fila_preparo = (Tarefa *)realloc(cozinha->tarefas_na_fila_preparo, sizeof(Tarefa) * cozinha->num_tarefas_na_fila_preparo);
                    cozinha->tarefas_na_fila_preparo[cozinha->num_tarefas_na_fila_preparo - 1] = t;
                }
            }

            // ETAPA 2: Despachar tarefas para funcionários livres.
            // Guardamos o número de tarefas antes para ver se algo mudou.
            int num_tarefas_antes_despacho = cozinha->num_tarefas_em_execucao;
            despachar_tarefas(cozinha);
            if (cozinha->num_tarefas_em_execucao > num_tarefas_antes_despacho)
            {
                alguma_acao_ocorreu = 1; // Uma ação ocorreu: novas tarefas foram despachadas
            }

            // ETAPA 4 (ANTECIPADA): Processar as tarefas que terminaram EXATAMENTE AGORA.
            for (int i = cozinha->num_tarefas_em_execucao - 1; i >= 0; i--)
            {
                Tarefa *tarefa_concluida = &cozinha->tarefas_em_execucao[i];
                if (tarefa_concluida->tempo_conclusao <= cozinha->tempo_atual)
                {
                    Pedido *pedido_pai = encontrar_pedido_em_andamento(cozinha, tarefa_concluida->pedido_id);
                    if (!pedido_pai)
                        continue;

                    printf("[Tempo: %ds] %s do Pedido #%d foi concluido.\n", cozinha->tempo_atual, NOMES_ITENS[tarefa_concluida->tipo_item], pedido_pai->id);

                    if (tarefa_concluida->tipo_item == ITEM_MONTAGEM)
                    {
                        int tempo_total = cozinha->tempo_atual - pedido_pai->tempo_chegada;
                        if (tempo_total <= TEMPO_MAX_ATENDIMENTO)
                        {
                            cozinha->atendidos_no_prazo++;
                            pedido_pai->status = STATUS_CONCLUIDO_NO_PRAZO;
                            printf(">> Pedido #%d FINALIZADO com SUCESSO em %ds.\n", pedido_pai->id, tempo_total);
                        }
                        else
                        {
                            cozinha->atendidos_com_atraso++;
                            pedido_pai->status = STATUS_CONCLUIDO_ATRASADO;
                            printf(">> Pedido #%d FINALIZADO com ATRASO em %ds (PREJUIZO).\n", pedido_pai->id, tempo_total);
                        }
                    }
                    else
                    {
                        pedido_pai->tarefas_preparo_restantes--;
                        if (pedido_pai->tarefas_preparo_restantes == 0 && pedido_pai->status == STATUS_EM_PREPARO)
                        {
                            alguma_acao_ocorreu = 1; // Uma ação ocorreu: nova tarefa de montagem foi criada

                            pedido_pai->status = STATUS_AGUARDANDO_MONTAGEM;
                            Tarefa t = {pedido_pai->id, ITEM_MONTAGEM, 0};
                            cozinha->num_tarefas_na_fila_preparo++;
                            cozinha->tarefas_na_fila_preparo = (Tarefa *)realloc(cozinha->tarefas_na_fila_preparo, sizeof(Tarefa) * cozinha->num_tarefas_na_fila_preparo);
                            cozinha->tarefas_na_fila_preparo[cozinha->num_tarefas_na_fila_preparo - 1] = t;
                            printf("[Tempo: %ds] Pedido #%d pronto para montagem.\n", cozinha->tempo_atual, pedido_pai->id);
                        }
                    }
                    cozinha->tarefas_em_execucao[i] = cozinha->tarefas_em_execucao[cozinha->num_tarefas_em_execucao - 1];
                    cozinha->num_tarefas_em_execucao--;
                }
            }

        } while (alguma_acao_ocorreu); // Se algo aconteceu, repete o ciclo para garantir que tudo foi processado neste instante.

        // ETAPA 3: Avançar o relógio para o próximo evento importante.
        if (cozinha->num_tarefas_em_execucao == 0)
        {
            // Se não há mais nada em execução, mas ainda há pedidos para fazer,
            // a simulação pode ter travado por falta de recursos.
            if (cozinha->total_pedidos_criados > (cozinha->atendidos_no_prazo + cozinha->atendidos_com_atraso))
            {
                printf("AVISO: Simulacao parada por falta de acoes ou recursos. Verifique gargalos.\n");
            }
            break; // Termina a simulação.
        }

        int proximo_tempo = INT_MAX;
        for (int i = 0; i < cozinha->num_tarefas_em_execucao; i++)
        {
            if (cozinha->tarefas_em_execucao[i].tempo_conclusao < proximo_tempo)
            {
                proximo_tempo = cozinha->tarefas_em_execucao[i].tempo_conclusao;
            }
        }
        // Garante que o tempo sempre avance, mesmo que por 1s, se houver trabalho pendente.
        if (proximo_tempo <= cozinha->tempo_atual)
        {
            cozinha->tempo_atual++;
        }
        else
        {
            cozinha->tempo_atual = proximo_tempo;
        }
    }

    // RELATÓRIO FINAL
    printf("\n=== SIMULACAO FINALIZADA EM %d SEGUNDOS ===\n", cozinha->tempo_atual);
    printf("Total de Pedidos: %d\n", cozinha->total_pedidos_criados);
    printf("Atendidos no Prazo: %d\n", cozinha->atendidos_no_prazo);
    printf("Atendidos com Atraso (Prejuizo): %d\n", cozinha->atendidos_com_atraso);
}

// =============================================================================
// 5. FUNÇÃO PRINCIPAL
// =============================================================================
int main()
{
    system("cls||clear");
    Cozinha cozinha;
    inicializar_cozinha(&cozinha);
    coletar_pedidos_do_usuario(&cozinha);

    if (cozinha.total_pedidos_criados > 0)
    {
        executar_simulacao(&cozinha);
    }
    else
    {
        printf("\nNenhum pedido foi registrado. Encerrando o programa.\n");
    }

    limpar_cozinha(&cozinha);
    return 0;
}
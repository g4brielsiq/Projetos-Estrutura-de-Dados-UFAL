#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

// Bibliotecas para criação de pastas
#ifdef _WIN32
#include <direct.h> // Para _mkdir no Windows
#else
#include <sys/stat.h> // Para mkdir no Linux/macOS
#include <sys/types.h>
#endif

// =============================================================================
// 1. DEFINIÇÕES E ESTRUTURAS DE DADOS
// =============================================================================

#define NUM_FUNCIONARIOS 13
#define TEMPO_MAX_ATENDIMENTO 300

// Nomes dos arquivos e pastas de E/S
#define PASTA_INPUT "Input"
#define ARQUIVO_INPUT "Input/entrada.txt"
#define PASTA_OUTPUT "Output"
#define ARQUIVO_OUTPUT "Output/resultado.txt"

typedef enum
{
    ITEM_BATATA_FRITA, // Mantido como 0 para corresponder ao formato do arquivo
    ITEM_SANDUICHE_SIMPLES,
    ITEM_SANDUICHE_MEDIO,
    ITEM_SANDUICHE_ELABORADO,
    ITEM_REFRIGERANTE,
    ITEM_SUCO,
    ITEM_MILK_SHAKE,
    ITEM_MONTAGEM
} TipoItem;

const char *NOMES_ITENS[] = {
    "Batata Frita", "Sanduiche Simples", "Sanduiche Medio", "Sanduiche Elaborado",
    "Refrigerante", "Suco", "Milk Shake", "Montagem de Bandeja"};

typedef enum
{
    STATUS_NA_FILA,
    STATUS_EM_PREPARO,
    STATUS_AGUARDANDO_MONTAGEM,
    STATUS_EM_MONTAGEM,
    STATUS_CONCLUIDO_NO_PRAZO,
    STATUS_CONCLUIDO_ATRASADO
} StatusPedido;

typedef enum
{
    HABILIDADE_SANDUICHE = 1 << 0,
    HABILIDADE_BATATA = 1 << 1,
    HABILIDADE_BEBIDAS = 1 << 2,
    HABILIDADE_MONTAGEM = 1 << 3
} Habilidade;

// Tempos de preparo reordenados para corresponder ao enum (Batata é a 0)
const int TEMPOS_DE_PREPARO[] = {190, 58, 88, 105, 5, 38, 60, 30};

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
    int loja_id;
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
// 2. FUNÇÕES DE GERENCIAMENTO DA COZINHA (Sem alterações)
// =============================================================================

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

Pedido *encontrar_pedido_em_andamento(Cozinha *c, int pedido_id)
{
    for (int i = 0; i < c->num_pedidos_em_andamento; i++)
    {
        if (c->pedidos_em_andamento[i]->id == pedido_id)
            return c->pedidos_em_andamento[i];
    }
    return NULL;
}

void imprimir_composicao_bandejas(Pedido *pedido)
{
    printf("\n--- Composicao do Pedido #%d ---\n", pedido->id);
    TipoItem itens_comer[pedido->num_itens];
    TipoItem itens_beber[pedido->num_itens];
    int count_comer = 0, count_beber = 0;
    for (int i = 0; i < pedido->num_itens; i++)
    {
        // Batata(0), Sanduiches(1,2,3) são "de comer"
        if (pedido->itens[i] <= ITEM_SANDUICHE_ELABORADO)
        {
            itens_comer[count_comer++] = pedido->itens[i];
        }
        else
        {
            itens_beber[count_beber++] = pedido->itens[i];
        }
    }
    int bandeja_num = 1, idx_comer_atual = 0, idx_beber_atual = 0;
    while (idx_comer_atual < count_comer || idx_beber_atual < count_beber)
    {
        printf("Bandeja %d:\n", bandeja_num++);
        for (int i = 0; i < 2 && idx_comer_atual < count_comer; i++)
            printf("  - %s\n", NOMES_ITENS[itens_comer[idx_comer_atual++]]);
        for (int i = 0; i < 2 && idx_beber_atual < count_beber; i++)
            printf("  - %s\n", NOMES_ITENS[itens_beber[idx_beber_atual++]]);
    }
    printf("---------------------------------\n");
}

// =============================================================================
// 3. LÓGICA DE ENTRADA E ROTEAMENTO (MODIFICADAS)
// =============================================================================

void criar_pasta_se_nao_existe(const char *nome_pasta)
{
#ifdef _WIN32
    _mkdir(nome_pasta);
#else
    mkdir(nome_pasta, 0777);
#endif
}

void criar_arquivo_entrada_exemplo()
{
    FILE *fp = fopen(ARQUIVO_INPUT, "r");
    if (fp == NULL)
    {
        printf("Aviso: Arquivo '%s' nao encontrado. Criando um exemplo...\n", ARQUIVO_INPUT);
        fp = fopen(ARQUIVO_INPUT, "w");
        if (fp == NULL)
        {
            printf("Erro fatal: Nao foi possivel criar o arquivo de entrada de exemplo.\n");
            return;
        }
        fprintf(fp, "senha ................................. 1\n");
        fprintf(fp, "batata frita........................... 2\n");
        fprintf(fp, "sanduíche simples...................... 6\n");
        fprintf(fp, "sanduíche médio........................ 5\n");
        fprintf(fp, "sanduíche elaborado.................... 2\n");
        fprintf(fp, "refrigerante........................... 4\n");
        fprintf(fp, "suco................................... 0\n");
        fprintf(fp, "milk shake............................. 1\n");
        fprintf(fp, "distância 1............................ 0\n");
        fprintf(fp, "distância 2............................ 5\n");
        fprintf(fp, "----‐--‐-------------------------------------\n");
        fprintf(fp, "senha.................................. 0\n");
        fprintf(fp, "‐-----‐--------------------------------------\n");
        fprintf(fp, "senha ................................. 2\n");
        fprintf(fp, "batata frita........................... 5\n");
        fprintf(fp, "sanduíche simples...................... 4\n");
        fprintf(fp, "sanduíche médio........................ 5\n");
        fprintf(fp, "sanduíche elaborado.................... 9\n");
        fprintf(fp, "refrigerante........................... 2\n");
        fprintf(fp, "suco................................... 1\n");
        fprintf(fp, "milk shake............................. 1\n");
        fprintf(fp, "distância 1............................ 11\n");
        fprintf(fp, "distância 2............................ 9\n");
        fprintf(fp, "---------------------------------------------\n");
        fclose(fp);
        printf("Arquivo '%s' criado. Por favor, edite-o com os dados da simulacao e rode o programa novamente.\n", ARQUIVO_INPUT);
        exit(1);
    }
    else
    {
        fclose(fp);
    }
}

Pedido *criar_pedido_com_itens(int id_pedido, int tempo_chegada, int quantidades[])
{
    TipoItem *itens_do_pedido = NULL;
    int qtd_total_itens = 0;
    for (int i = 0; i < 7; i++)
    {
        for (int k = 0; k < quantidades[i]; k++)
        {
            qtd_total_itens++;
            itens_do_pedido = (TipoItem *)realloc(itens_do_pedido, sizeof(TipoItem) * qtd_total_itens);
            itens_do_pedido[qtd_total_itens - 1] = (TipoItem)i;
        }
    }
    if (qtd_total_itens > 0)
    {
        Pedido *novo_pedido = (Pedido *)malloc(sizeof(Pedido));
        novo_pedido->id = id_pedido;
        novo_pedido->tempo_chegada = tempo_chegada;
        novo_pedido->status = STATUS_NA_FILA;
        novo_pedido->itens = itens_do_pedido;
        novo_pedido->num_itens = qtd_total_itens;
        novo_pedido->tarefas_preparo_restantes = qtd_total_itens;
        printf(">> Pedido #%d criado com %d itens. Chegada em t=%ds.\n", id_pedido, qtd_total_itens, tempo_chegada);
        return novo_pedido;
    }
    return NULL;
}

void rotear_pedido_ifood(Pedido *pedido, Cozinha *loja1, Cozinha *loja2)
{
    if (pedido == NULL)
        return;
    int carga_loja_1 = loja1->num_tarefas_na_fila_preparo;
    int carga_loja_2 = loja2->num_tarefas_na_fila_preparo;
    printf("[Roteador iFood] Carga Loja 1: %d tarefas | Carga Loja 2: %d tarefas.\n", carga_loja_1, carga_loja_2);
    if (carga_loja_1 <= carga_loja_2)
    {
        printf("[Roteador iFood] Enviando Pedido #%d para a Loja 1 (menos ocupada).\n", pedido->id);
        adicionar_pedido_na_fila_espera(loja1, pedido);
    }
    else
    {
        printf("[Roteador iFood] Enviando Pedido #%d para a Loja 2 (menos ocupada).\n", pedido->id);
        adicionar_pedido_na_fila_espera(loja2, pedido);
    }
}

// --- FUNÇÃO DE LEITURA DE ARQUIVO CORRIGIDA ---
void carregar_eventos_do_arquivo(FILE *fp, Cozinha *loja1, Cozinha *loja2)
{
    char linha[256];
    int tempo_de_chegada_atual = 0;

    while (fgets(linha, sizeof(linha), fp) != NULL)
    {
        // Procura pela palavra "senha" no início da linha (ignorando espaços em branco)
        char *p = linha;
        while (isspace(*p))
            p++; // Pula espaços em branco no início

        if (strncmp(p, "senha", 5) == 0)
        {
            int id;

            // NOVA LÓGICA DE PARSING: Encontra o último número na linha
            char *ptr_num = p + strlen(p) - 1; // Começa do fim da linha
            while (ptr_num >= p && !isdigit(*ptr_num))
                ptr_num--; // Recua até o último dígito
            while (ptr_num >= p && isdigit(*ptr_num))
                ptr_num--; // Recua até o início do número
            ptr_num++;     // Avança para o primeiro dígito do número

            if (sscanf(ptr_num, "%d", &id) == 1)
            { // Tenta ler o número

                if (id == 0)
                {
                    tempo_de_chegada_atual++;
                    printf("[Carregador] Tick de tempo ocioso. Proxima chegada em t=%d\n", tempo_de_chegada_atual);
                }
                else
                {
                    // É um pedido válido, vamos ler os 9 blocos seguintes
                    int quantidades[7];
                    int dist1, dist2;
                    char buffer[256];

                    // Lê as 7 quantidades de itens
                    for (int i = 0; i < 7; i++)
                    {
                        if (fgets(buffer, sizeof(buffer), fp) == NULL)
                            break;
                        ptr_num = buffer + strlen(buffer) - 1;
                        while (ptr_num >= buffer && !isdigit(*ptr_num))
                            ptr_num--;
                        while (ptr_num >= buffer && isdigit(*ptr_num))
                            ptr_num--;
                        ptr_num++;
                        sscanf(ptr_num, "%d", &quantidades[i]);
                    }

                    // Lê as 2 distâncias
                    if (fgets(buffer, sizeof(buffer), fp) == NULL)
                        break;
                    ptr_num = buffer + strlen(buffer) - 1;
                    while (ptr_num >= buffer && !isdigit(*ptr_num))
                        ptr_num--;
                    while (ptr_num >= buffer && isdigit(*ptr_num))
                        ptr_num--;
                    ptr_num++;
                    sscanf(ptr_num, "%d", &dist1);

                    if (fgets(buffer, sizeof(buffer), fp) == NULL)
                        break;
                    ptr_num = buffer + strlen(buffer) - 1;
                    while (ptr_num >= buffer && !isdigit(*ptr_num))
                        ptr_num--;
                    while (ptr_num >= buffer && isdigit(*ptr_num))
                        ptr_num--;
                    ptr_num++;
                    sscanf(ptr_num, "%d", &dist2);

                    Pedido *novo_pedido = criar_pedido_com_itens(id, tempo_de_chegada_atual, quantidades);
                    if (novo_pedido == NULL)
                        continue;

                    // Roteia o pedido com base nas distâncias lidas
                    if (dist1 == 0)
                    {
                        printf("[Carregador] Pedido #%d presencial na Loja 1.\n", id);
                        adicionar_pedido_na_fila_espera(loja1, novo_pedido);
                    }
                    else if (dist2 == 0)
                    {
                        printf("[Carregador] Pedido #%d presencial na Loja 2.\n", id);
                        adicionar_pedido_na_fila_espera(loja2, novo_pedido);
                    }
                    else
                    {
                        printf("[Carregador] Pedido #%d e um pedido iFood (Dist: L1=%d, L2=%d).\n", id, dist1, dist2);
                        rotear_pedido_ifood(novo_pedido, loja1, loja2);
                    }
                    // Avança o relógio de chegada para o próximo evento
                    tempo_de_chegada_atual++;
                }
            }
        }
        // Ignora linhas que não começam com "senha" (como as linhas de "----")
    }
    printf("\n=== Carregamento do arquivo de entrada concluido. ===\n");
}

// =============================================================================
// 4. O MOTOR DA SIMULAÇÃO (Sem alterações)
// =============================================================================

void despachar_tarefas(Cozinha *c)
{
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
            switch (habilidades[h])
            {
            case HABILIDADE_SANDUICHE:
                if (tarefa_atual->tipo_item >= ITEM_SANDUICHE_SIMPLES && tarefa_atual->tipo_item <= ITEM_SANDUICHE_ELABORADO)
                    tarefa_corresponde = 1;
                break;
            case HABILIDADE_BATATA:
                if (tarefa_atual->tipo_item == ITEM_BATATA_FRITA)
                    tarefa_corresponde = 1;
                break;
            case HABILIDADE_BEBIDAS:
                if (tarefa_atual->tipo_item >= ITEM_REFRIGERANTE && tarefa_atual->tipo_item <= ITEM_MILK_SHAKE)
                    tarefa_corresponde = 1;
                break;
            case HABILIDADE_MONTAGEM:
                if (tarefa_atual->tipo_item == ITEM_MONTAGEM)
                    tarefa_corresponde = 1;
                break;
            }
            if (tarefa_corresponde)
            {
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

void atualizar_cozinha_um_passo(Cozinha *cozinha, int tempo_global_atual)
{
    cozinha->tempo_atual = tempo_global_atual;
    for (int i = cozinha->num_tarefas_em_execucao - 1; i >= 0; i--)
    {
        Tarefa *tarefa_concluida = &cozinha->tarefas_em_execucao[i];
        if (tarefa_concluida->tempo_conclusao <= cozinha->tempo_atual)
        {
            Pedido *pedido_pai = encontrar_pedido_em_andamento(cozinha, tarefa_concluida->pedido_id);
            if (!pedido_pai)
                continue;
            printf("[Loja %d | Tempo: %ds] %s do Pedido #%d foi concluido.\n",
                   cozinha->loja_id, cozinha->tempo_atual,
                   NOMES_ITENS[tarefa_concluida->tipo_item], pedido_pai->id);
            if (tarefa_concluida->tipo_item == ITEM_MONTAGEM)
            {
                int tempo_de_producao = cozinha->tempo_atual - pedido_pai->tempo_chegada;
                if (tempo_de_producao <= TEMPO_MAX_ATENDIMENTO)
                {
                    cozinha->atendidos_no_prazo++;
                    pedido_pai->status = STATUS_CONCLUIDO_NO_PRAZO;
                    printf(">> [Loja %d] Pedido #%d FINALIZADO com SUCESSO. Tempo de producao: %ds.\n",
                           cozinha->loja_id, pedido_pai->id, tempo_de_producao);
                }
                else
                {
                    cozinha->atendidos_com_atraso++;
                    pedido_pai->status = STATUS_CONCLUIDO_ATRASADO;
                    printf(">> [Loja %d] Pedido #%d FINALIZADO com ATRASO. Tempo de producao: %ds (PREJUIZO).\n",
                           cozinha->loja_id, pedido_pai->id, tempo_de_producao);
                }
                imprimir_composicao_bandejas(pedido_pai);
            }
            else
            {
                pedido_pai->tarefas_preparo_restantes--;
                if (pedido_pai->tarefas_preparo_restantes == 0 && pedido_pai->status == STATUS_EM_PREPARO)
                {
                    pedido_pai->status = STATUS_AGUARDANDO_MONTAGEM;
                    Tarefa t = {pedido_pai->id, ITEM_MONTAGEM, 0};
                    cozinha->num_tarefas_na_fila_preparo++;
                    cozinha->tarefas_na_fila_preparo = (Tarefa *)realloc(cozinha->tarefas_na_fila_preparo, sizeof(Tarefa) * cozinha->num_tarefas_na_fila_preparo);
                    cozinha->tarefas_na_fila_preparo[cozinha->num_tarefas_na_fila_preparo - 1] = t;
                    printf("[Loja %d | Tempo: %ds] Pedido #%d pronto para montagem.\n",
                           cozinha->loja_id, cozinha->tempo_atual, pedido_pai->id);
                }
            }
            cozinha->tarefas_em_execucao[i] = cozinha->tarefas_em_execucao[cozinha->num_tarefas_em_execucao - 1];
            cozinha->num_tarefas_em_execucao--;
        }
    }
    while (cozinha->pedidos_na_fila_espera != NULL && cozinha->pedidos_na_fila_espera->tempo_chegada <= cozinha->tempo_atual)
    {
        Pedido *pedido_iniciado = cozinha->pedidos_na_fila_espera;
        cozinha->pedidos_na_fila_espera = cozinha->pedidos_na_fila_espera->proximo;
        pedido_iniciado->status = STATUS_EM_PREPARO;
        cozinha->num_pedidos_em_andamento++;
        cozinha->pedidos_em_andamento = (Pedido **)realloc(cozinha->pedidos_em_andamento, sizeof(Pedido *) * cozinha->num_pedidos_em_andamento);
        cozinha->pedidos_em_andamento[cozinha->num_pedidos_em_andamento - 1] = pedido_iniciado;
        printf("[Loja %d | Tempo: %ds] Iniciando preparo do Pedido #%d.\n",
               cozinha->loja_id, cozinha->tempo_atual, pedido_iniciado->id);
        cozinha->total_pedidos_criados++;
        for (int i = 0; i < pedido_iniciado->num_itens; i++)
        {
            Tarefa t = {pedido_iniciado->id, pedido_iniciado->itens[i], 0};
            cozinha->num_tarefas_na_fila_preparo++;
            cozinha->tarefas_na_fila_preparo = (Tarefa *)realloc(cozinha->tarefas_na_fila_preparo, sizeof(Tarefa) * cozinha->num_tarefas_na_fila_preparo);
            cozinha->tarefas_na_fila_preparo[cozinha->num_tarefas_na_fila_preparo - 1] = t;
        }
    }
    despachar_tarefas(cozinha);
}

int cozinhas_estao_ocupadas(Cozinha *loja1, Cozinha *loja2)
{
    int loja1_ocupada = loja1->pedidos_na_fila_espera != NULL ||
                        loja1->num_tarefas_na_fila_preparo > 0 ||
                        loja1->num_tarefas_em_execucao > 0;
    int loja2_ocupada = loja2->pedidos_na_fila_espera != NULL ||
                        loja2->num_tarefas_na_fila_preparo > 0 ||
                        loja2->num_tarefas_em_execucao > 0;
    return loja1_ocupada || loja2_ocupada;
}

// =============================================================================
// 5. FUNÇÃO PRINCIPAL (MODIFICADA PARA LER DE ARQUIVO)
// =============================================================================
int main()
{
    system("cls||clear");

    // 1. Criação das Pastas de E/S
    printf("Verificando pastas de Input/Output...\n");
    criar_pasta_se_nao_existe(PASTA_INPUT);
    criar_pasta_se_nao_existe(PASTA_OUTPUT);

    // 2. Redirecionamento da Saída (stdout)
    if (freopen(ARQUIVO_OUTPUT, "w", stdout) == NULL)
    {
        perror("Erro ao redirecionar a saida para o arquivo");
        return 1;
    }

    // 3. Criação e Abertura do Arquivo de Entrada
    criar_arquivo_entrada_exemplo();

    FILE *arquivo_entrada = fopen(ARQUIVO_INPUT, "r");
    if (arquivo_entrada == NULL)
    {
        printf("Erro fatal: Nao foi possivel abrir o arquivo '%s' para leitura.\n", ARQUIVO_INPUT);
        return 1;
    }
    printf("Lendo eventos do arquivo '%s'...\n", ARQUIVO_INPUT);

    // 4. Inicialização das Cozinhas
    Cozinha cozinha_loja_1;
    Cozinha cozinha_loja_2;
    inicializar_cozinha(&cozinha_loja_1, 1);
    inicializar_cozinha(&cozinha_loja_2, 2);

    // 5. Carregamento dos Eventos
    carregar_eventos_do_arquivo(arquivo_entrada, &cozinha_loja_1, &cozinha_loja_2);
    fclose(arquivo_entrada);

    // 6. O Motor da Simulação (Loop de Tempo Global)
    printf("\n=== INICIANDO SIMULACAO DA COZINHA BIGPAPAO ===\n");
    int tempo_global = 0;

    while (cozinhas_estao_ocupadas(&cozinha_loja_1, &cozinha_loja_2))
    {
        atualizar_cozinha_um_passo(&cozinha_loja_1, tempo_global);
        atualizar_cozinha_um_passo(&cozinha_loja_2, tempo_global);

        tempo_global++;

        if (tempo_global > 100000)
        {
            printf("Simulacao excedeu 100.000 segundos. Interrompendo.\n");
            break;
        }
    }

    // 7. Relatório Final
    printf("\n=== SIMULACAO FINALIZADA EM %d SEGUNDOS (TEMPO TOTAL DECORRIDO) ===\n", tempo_global);

    printf("\n--- Resultados Loja 1 ---\n");
    printf("Total de Pedidos Processados: %d\n", cozinha_loja_1.total_pedidos_criados);
    printf("Atendidos no Prazo: %d\n", cozinha_loja_1.atendidos_no_prazo);
    printf("Atendidos com Atraso (Prejuizo): %d\n", cozinha_loja_1.atendidos_com_atraso);

    printf("\n--- Resultados Loja 2 ---\n");
    printf("Total de Pedidos Processados: %d\n", cozinha_loja_2.total_pedidos_criados);
    printf("Atendidos no Prazo: %d\n", cozinha_loja_2.atendidos_no_prazo);
    printf("Atendidos com Atraso (Prejuizo): %d\n", cozinha_loja_2.atendidos_com_atraso);

    // 8. Limpa a memória de ambas as cozinhas
    limpar_cozinha(&cozinha_loja_1);
    limpar_cozinha(&cozinha_loja_2);

    return 0;
}
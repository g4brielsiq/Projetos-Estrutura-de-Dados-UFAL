#include <stdio.h>    // Entrada e saída padrão (printf, fopen, etc.)
#include <stdlib.h>   // Funções de alocação dinâmica (malloc, realloc, free) e utilidades gerais
#include <string.h>   // Manipulação de strings (strtok, memset, etc.)
#include <ctype.h>    // Funções para caracteres (não é muito usada aqui, mas está incluída)
#include <limits.h>   // Limites de tipos inteiros (INT_MAX, etc.), caso seja necessário

// Bibliotecas para criação de pastas, variando por sistema operacional
#ifdef _WIN32
#include <direct.h>   // Para _mkdir no Windows
#else
#include <sys/stat.h> // Para mkdir no Linux/macOS
#include <sys/types.h> // Tipos de dados usados por sys/stat
#endif

// =============================================================================
// 1. DEFINIÇÕES E ESTRUTURAS DE DADOS
// =============================================================================

// Número fixo de funcionários em cada cozinha
#define NUM_FUNCIONARIOS 13
// Tempo máximo aceitável de atendimento de um pedido em segundos (até aqui é “no prazo”)
#define TEMPO_MAX_ATENDIMENTO 300

// Tempo que o caixa leva para digitar/registrar um pedido (tempo entre um pedido e outro)
#define TEMPO_ATENDIMENTO_CAIXA 10

// Pastas e arquivos usados para entrada (CSV) e saída (log)
#define PASTA_INPUT "Input"
#define ARQUIVO_INPUT "Input/entrada.csv"
#define PASTA_OUTPUT "Output"
#define ARQUIVO_OUTPUT "Output/resultado.txt"

// Tipos de itens que podem compor um pedido (enumerado para facilitar leitura e comparação)
typedef enum {
    ITEM_BATATA_FRITA, ITEM_SANDUICHE_SIMPLES, ITEM_SANDUICHE_MEDIO, ITEM_SANDUICHE_ELABORADO,
    ITEM_REFRIGERANTE, ITEM_SUCO, ITEM_MILK_SHAKE,
    ITEM_MONTAGEM          // Tarefa especial que representa a montagem final da bandeja
} TipoItem;

// Nomes de exibição para cada tipo de item, na mesma ordem do enum TipoItem
const char *NOMES_ITENS[] = {
    "Batata Frita", "Sanduiche Simples", "Sanduiche Medio", "Sanduiche Elaborado",
    "Refrigerante", "Suco", "Milk Shake", "Montagem de Bandeja"
};

// Estado em que um pedido pode estar ao longo do fluxo da cozinha
typedef enum {
    STATUS_NA_FILA,             // Ainda aguardando para começar a ser preparado
    STATUS_EM_PREPARO,          // Com itens sendo preparados nas estações
    STATUS_AGUARDANDO_MONTAGEM, // Todos os itens prontos, falta apenas montagem
    STATUS_EM_MONTAGEM,         // (não usado explicitamente, mas reservado)
    STATUS_CONCLUIDO_NO_PRAZO,  // Terminado dentro do tempo máximo
    STATUS_CONCLUIDO_ATRASADO   // Terminado após o tempo máximo (prejuízo)
} StatusPedido;

// Máscaras de bits representando as habilidades de cada funcionário
typedef enum {
    HABILIDADE_SANDUICHE = 1 << 0, // Pode preparar sanduíches
    HABILIDADE_BATATA    = 1 << 1, // Pode operar fritadeira (batata)
    HABILIDADE_BEBIDAS   = 1 << 2, // Pode preparar bebidas (refrigerante, suco, milk shake)
    HABILIDADE_MONTAGEM  = 1 << 3  // Pode montar bandejas finais
} Habilidade;

// Origem do pedido: se veio de um cliente presencial ou de um app (iFood)
typedef enum {
    ORIGEM_PRESENCIAL,
    ORIGEM_IFOOD
} OrigemPedido;

// Nomes amigáveis para a origem do pedido
const char *NOME_ORIGEM[] = {
    "Presencial",
    "iFood"
};

// Tempo de preparo padrão de cada tipo de item, em segundos, na mesma ordem de TipoItem
const int TEMPOS_DE_PREPARO[] = {190, 58, 88, 105, 5, 38, 60, 30};

// Representa uma tarefa individual na cozinha (preparar um item ou montar bandeja)
typedef struct Tarefa {
    int pedido_id;           // ID do pedido ao qual essa tarefa pertence
    TipoItem tipo_item;      // Tipo de item a ser produzido
    int tempo_conclusao;     // Instante de tempo em que a tarefa terminará
} Tarefa;

// Representa um pedido completo do cliente
typedef struct Pedido {
    int id;                        // Identificador único do pedido
    int tempo_chegada;             // Momento em que o pedido fica disponível para a cozinha (fim da digitação)
    StatusPedido status;           // Estado atual do pedido
    int tarefas_preparo_restantes; // Quantidade de itens ainda faltando preparar antes da montagem
    TipoItem *itens;               // Vetor dinâmico com os itens que compõem o pedido
    int num_itens;                 // Quantidade total de itens no vetor
    OrigemPedido origem;           // Origem do pedido (presencial ou iFood)
    int dist_loja1;                // Distância até a loja 1 (para roteamento iFood)
    int dist_loja2;                // Distância até a loja 2 (para roteamento iFood)
    struct Pedido *proximo;        // Ponteiro para o próximo pedido em filas encadeadas
} Pedido;

// Representa um funcionário da cozinha, com suas habilidades e disponibilidade
typedef struct {
    int id;                    // Identificador do funcionário (inclui o número da loja)
    unsigned int habilidades;  // Conjunto de habilidades (máscara de bits Habilidade)
    int livre_a_partir_de;     // Tempo em que esse funcionário estará livre para nova tarefa
} Funcionario;

// Representa um equipamento compartilhado (chapa, fritadeira, liquidificador)
typedef struct {
    int capacidade_por_funcionario; // Quantos itens um funcionário consegue produzir em paralelo nessa estação
    int validade_produto_min;       // Validade dos produtos dessa estação em minutos (para controle futuro)
} Equipamento;

// Estrutura principal que modela o estado de uma cozinha de uma loja
typedef struct {
    int loja_id;                        // Identificador da loja (1 ou 2)
    int tempo_atual;                    // Tempo atual da simulação visto por essa cozinha
    Funcionario funcionarios[NUM_FUNCIONARIOS]; // Vetor fixo de funcionários
    Equipamento chapa;                  // Estação para sanduíches
    Equipamento fritadeira;             // Estação para batatas
    Equipamento liquidificador;         // Estação para bebidas
    Pedido *pedidos_na_fila_espera;     // Lista encadeada de pedidos que já chegaram, mas ainda não começaram preparo
    Pedido **pedidos_em_andamento;      // Vetor dinâmico de ponteiros para pedidos já em preparo
    int num_pedidos_em_andamento;       // Quantos pedidos estão em andamento
    Tarefa *tarefas_na_fila_preparo;    // Vetor dinâmico de tarefas aguardando alocação a funcionários
    int num_tarefas_na_fila_preparo;    // Quantidade de tarefas aguardando
    Tarefa *tarefas_em_execucao;        // Vetor dinâmico de tarefas atualmente sendo executadas
    int num_tarefas_em_execucao;        // Quantidade de tarefas em execução
    int total_pedidos_criados;          // Contador de pedidos processados pela loja
    int atendidos_no_prazo;             // Quantos foram concluídos dentro do tempo máximo
    int atendidos_com_atraso;           // Quantos foram concluídos com atraso
} Cozinha;

// Fila global de pedidos iFood que ainda não foram roteados para nenhuma loja
Pedido *fila_ifood_pendente = NULL;

// =============================================================================
// 2. FUNÇÕES DE GERENCIAMENTO DA COZINHA
// =============================================================================

// Inicializa toda a estrutura da cozinha de uma loja, definindo equipamentos e funcionários
void inicializar_cozinha(Cozinha *cozinha, int loja_id)
{
    // Zera toda a memória da estrutura Cozinha, incluindo ponteiros e contadores
    memset(cozinha, 0, sizeof(Cozinha));
    cozinha->loja_id = loja_id;
    printf("--- Configurando a Cozinha da Loja %d ---\n", loja_id);

    // Configuração básica da capacidade de cada equipamento
    cozinha->chapa.capacidade_por_funcionario = 3;
    cozinha->fritadeira.capacidade_por_funcionario = 2;
    cozinha->fritadeira.validade_produto_min = 60;
    cozinha->liquidificador.capacidade_por_funcionario = 4;
    cozinha->liquidificador.validade_produto_min = 30;
    printf("Configuracao da Loja %d concluida.\n\n", loja_id);

    // Criação dos funcionários com IDs e combinações de habilidades diferentes
    cozinha->funcionarios[0]  = (Funcionario){(loja_id * 100) + 1,  HABILIDADE_SANDUICHE | HABILIDADE_BATATA, 0};
    cozinha->funcionarios[1]  = (Funcionario){(loja_id * 100) + 2,  HABILIDADE_SANDUICHE | HABILIDADE_BATATA, 0};
    cozinha->funcionarios[2]  = (Funcionario){(loja_id * 100) + 3,  HABILIDADE_SANDUICHE | HABILIDADE_BEBIDAS, 0};
    cozinha->funcionarios[3]  = (Funcionario){(loja_id * 100) + 4,  HABILIDADE_SANDUICHE,                    0};
    cozinha->funcionarios[4]  = (Funcionario){(loja_id * 100) + 5,  HABILIDADE_SANDUICHE,                    0};
    cozinha->funcionarios[5]  = (Funcionario){(loja_id * 100) + 6,  HABILIDADE_SANDUICHE | HABILIDADE_BATATA, 0};
    cozinha->funcionarios[6]  = (Funcionario){(loja_id * 100) + 7,  HABILIDADE_BATATA,                       0};
    cozinha->funcionarios[7]  = (Funcionario){(loja_id * 100) + 8,  HABILIDADE_BEBIDAS | HABILIDADE_MONTAGEM, 0};
    cozinha->funcionarios[8]  = (Funcionario){(loja_id * 100) + 9,  HABILIDADE_MONTAGEM,                     0};
    cozinha->funcionarios[9]  = (Funcionario){(loja_id * 100) + 10, 0,                                       0}; // Sem habilidade específica
    cozinha->funcionarios[10] = (Funcionario){(loja_id * 100) + 11, HABILIDADE_SANDUICHE,                    0};
    cozinha->funcionarios[11] = (Funcionario){(loja_id * 100) + 12, HABILIDADE_BEBIDAS,                      0};
    cozinha->funcionarios[12] = (Funcionario){(loja_id * 100) + 13, 0,                                       0}; // Reservado/flexível
}

// Insere um pedido no final da fila encadeada de espera da cozinha
void adicionar_pedido_na_fila_espera(Cozinha *c, Pedido *novo_pedido) {
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

// Adiciona um pedido iFood à fila global de pendentes, para ser roteado mais tarde
void adicionar_ifood_pendente(Pedido *p) {
    p->proximo = NULL;
    if (fila_ifood_pendente == NULL) {
        fila_ifood_pendente = p;
    } else {
        Pedido *atual = fila_ifood_pendente;
        while (atual->proximo != NULL)
            atual = atual->proximo;
        atual->proximo = p;
    }
}

// Libera memória dinâmica associada à cozinha (tarefas, pedidos em andamento e fila de espera)
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
    // fila_ifood_pendente é global; espera-se que esteja vazia ao final da simulação.
}

// Procura um pedido pelo ID dentro do vetor de pedidos em andamento
Pedido *encontrar_pedido_em_andamento(Cozinha *c, int pedido_id) {
    for (int i = 0; i < c->num_pedidos_em_andamento; i++) {
        if (c->pedidos_em_andamento[i]->id == pedido_id)
            return c->pedidos_em_andamento[i];
    }
    return NULL;
}

// Imprime a distribuição de itens de um pedido em bandejas (máx. 2 comidas e 2 bebidas por bandeja)
void imprimir_composicao_bandejas(Pedido *pedido) {
    printf("\n--- Composicao do Pedido #%d (%s) ---\n",
           pedido->id, NOME_ORIGEM[pedido->origem]);

    TipoItem itens_comer[pedido->num_itens];
    TipoItem itens_beber[pedido->num_itens];
    int count_comer = 0, count_beber = 0;

    // Separa itens de comer (batata/sanduíche) e beber (bebidas)
    for (int i = 0; i < pedido->num_itens; i++) {
        if (pedido->itens[i] <= ITEM_SANDUICHE_ELABORADO) {
            itens_comer[count_comer++] = pedido->itens[i];
        } else {
            itens_beber[count_beber++] = pedido->itens[i];
        }
    }

    int bandeja_num = 1, idx_comer_atual = 0, idx_beber_atual = 0;
    // Monta bandejas com até 2 comidas e 2 bebidas cada
    while (idx_comer_atual < count_comer || idx_beber_atual < count_beber) {
        printf("Bandeja %d:\n", bandeja_num++);
        for (int i = 0; i < 2 && idx_comer_atual < count_comer; i++)
            printf("  - %s\n", NOMES_ITENS[itens_comer[idx_comer_atual++]]);
        for (int i = 0; i < 2 && idx_beber_atual < count_beber; i++)
            printf("  - %s\n", NOMES_ITENS[itens_beber[idx_beber_atual++]]);
    }

    printf("---------------------------------\n");
}

// =============================================================================
// 3. LÓGICA DE ENTRADA E ROTEAMENTO (CSV)
// =============================================================================

// Garante que uma pasta existe; se não, tenta criá-la
void criar_pasta_se_nao_existe(const char* nome_pasta) {
#ifdef _WIN32
    _mkdir(nome_pasta);
#else
    mkdir(nome_pasta, 0777);
#endif
}

// Cria um arquivo de entrada CSV de exemplo caso não exista
void criar_arquivo_entrada_exemplo() {
    FILE* fp = fopen(ARQUIVO_INPUT, "r");
    if (fp == NULL) {
        printf("Aviso: Arquivo '%s' nao encontrado. Criando um exemplo...\n", ARQUIVO_INPUT);
        fp = fopen(ARQUIVO_INPUT, "w");
        if (fp == NULL) {
            printf("Erro fatal: Nao foi possivel criar o arquivo de entrada de exemplo.\n");
            return;
        }
        // Cabeçalho com o formato esperado pelo leitor
        fprintf(fp, "ID_Pedido,Qtd_Batata,Qtd_Simples,Qtd_Medio,Qtd_Elaborado,Qtd_Refri,Qtd_Suco,Qtd_Milkshake,Dist_Loja_1,Dist_Loja_2\n");
        // Alguns pedidos de exemplo (1, 2 presenciais e 3 iFood)
        fprintf(fp, "1,2,6,5,2,4,0,1,0,5\n");
        fprintf(fp, "2,0,0,0,0,1,0,0,5,0\n"); // Pedido 2 para Loja 2
        fprintf(fp, "0,0,0,0,0,0,0,0,0,0\n"); // Tempo ocioso (ID 0)
        fprintf(fp, "3,5,4,5,9,2,1,1,11,9\n"); // Pedido 3 (iFood)
        fclose(fp);
        printf("Arquivo '%s' criado. Por favor, edite-o com os dados da simulacao e rode o programa novamente.\n", ARQUIVO_INPUT);
        exit(1); // Sai para o usuário editar o arquivo antes da simulação
    } else {
        fclose(fp);
    }
}

// Cria um pedido a partir das quantidades de cada item, montando o vetor dinâmico de itens
Pedido* criar_pedido_com_itens(int id_pedido, int tempo_chegada, int quantidades[], OrigemPedido origem) {
    TipoItem *itens_do_pedido = NULL;
    int qtd_total_itens = 0;

    // Para cada tipo de item, adiciona 'quantidades[i]' cópias ao vetor
    for (int i = 0; i < 7; i++) {
        for (int k = 0; k < quantidades[i]; k++) {
            qtd_total_itens++;
            itens_do_pedido = (TipoItem *)realloc(itens_do_pedido, sizeof(TipoItem) * qtd_total_itens);
            itens_do_pedido[qtd_total_itens - 1] = (TipoItem)i;
        }
    }

    // Se o pedido realmente tem itens, aloca a struct Pedido e inicializa seus campos
    if (qtd_total_itens > 0) {
        Pedido *novo_pedido = (Pedido *)malloc(sizeof(Pedido));
        novo_pedido->id = id_pedido;
        novo_pedido->tempo_chegada = tempo_chegada;
        novo_pedido->status = STATUS_NA_FILA;
        novo_pedido->itens = itens_do_pedido;
        novo_pedido->num_itens = qtd_total_itens;
        novo_pedido->tarefas_preparo_restantes = qtd_total_itens;
        novo_pedido->origem = origem;
        novo_pedido->dist_loja1 = 0;
        novo_pedido->dist_loja2 = 0;
        novo_pedido->proximo = NULL;
        printf(">> Pedido #%d (%s) criado com %d itens. Chegada em t=%ds.\n",
               id_pedido, NOME_ORIGEM[origem], qtd_total_itens, tempo_chegada);
        return novo_pedido;
    }
    return NULL;
}

// Roteia um pedido iFood para a loja mais adequada, considerando carga e distância
void rotear_pedido_ifood(Pedido *pedido, Cozinha *loja1, Cozinha *loja2,
                         int dist1, int dist2)
{
    if (pedido == NULL) return;

    int carga_loja_1 = loja1->num_tarefas_na_fila_preparo;
    int carga_loja_2 = loja2->num_tarefas_na_fila_preparo;

    printf("[Roteador iFood] Carga Loja 1: %d tarefas | Carga Loja 2: %d tarefas.\n",
           carga_loja_1, carga_loja_2);

    // Caso especial: ambas as lojas sem fila, prioriza loja mais próxima
    if (carga_loja_1 == 0 && carga_loja_2 == 0) {
        if (dist1 <= dist2) {
            printf("[Roteador iFood] Ambas lojas livres. Enviando Pedido #%d (%s) "
                   "para a Loja 1 (mais proxima).\n",
                   pedido->id, NOME_ORIGEM[pedido->origem]);
            adicionar_pedido_na_fila_espera(loja1, pedido);
        } else {
            printf("[Roteador iFood] Ambas lojas livres. Enviando Pedido #%d (%s) "
                   "para a Loja 2 (mais proxima).\n",
                   pedido->id, NOME_ORIGEM[pedido->origem]);
            adicionar_pedido_na_fila_espera(loja2, pedido);
        }
        return;
    }

    // Caso geral: envia para a loja com menor carga de tarefas pendentes
    if (carga_loja_1 <= carga_loja_2) {
        printf("[Roteador iFood] Enviando Pedido #%d (%s) para a Loja 1 (menos ocupada).\n",
               pedido->id, NOME_ORIGEM[pedido->origem]);
        adicionar_pedido_na_fila_espera(loja1, pedido);
    } else {
        printf("[Roteador iFood] Enviando Pedido #%d (%s) para a Loja 2 (menos ocupada).\n",
               pedido->id, NOME_ORIGEM[pedido->origem]);
        adicionar_pedido_na_fila_espera(loja2, pedido);
    }
}

// Lê o arquivo CSV, converte cada linha em eventos e cria pedidos com tempo global e tempo de caixa
void carregar_eventos_do_arquivo(FILE* fp, Cozinha* loja1, Cozinha* loja2) {
    char linha[256];

    // Representa em que segundo o próximo pedido começa a ser digitado no caixa
    int tempo_de_chegada_atual = 0;

    // Pula a primeira linha (cabeçalho do CSV)
    fgets(linha, sizeof(linha), fp);

    // Lê linha a linha até o fim do arquivo
    while (fgets(linha, sizeof(linha), fp) != NULL) {
        int valores[10];
        int i = 0;
        char* token = strtok(linha, ",");

        // Separa os 10 campos esperados da linha
        while (token != NULL && i < 10) {
            valores[i++] = atoi(token);
            token = strtok(NULL, ",");
        }
        if (i < 10) continue; // Ignora linhas incompletas

        int id = valores[0];

        // Linha com ID 0 representa um segundo ocioso (ninguém digitando pedido)
        if (id == 0) {
            tempo_de_chegada_atual++;
            printf("[Carregador] Tick de tempo ocioso. Proximo evento em t=%d\n",
                   tempo_de_chegada_atual);
            continue;
        }

        // Monta o vetor de quantidades de itens para o pedido
        int quantidades[7];
        for (int j = 0; j < 7; j++) {
            quantidades[j] = valores[j + 1];
        }
        int dist1 = valores[8];
        int dist2 = valores[9];

        // Define origem com base nas distâncias (0 => presencial)
        OrigemPedido origem;
        if (dist1 == 0 || dist2 == 0) {
            origem = ORIGEM_PRESENCIAL;
        } else {
            origem = ORIGEM_IFOOD;
        }

        // O pedido começa a ser digitado em tempo_de_chegada_atual e termina em +TEMPO_ATENDIMENTO_CAIXA
        int tempo_inicio_digitacao = tempo_de_chegada_atual;
        int tempo_fim_digitacao = tempo_inicio_digitacao + TEMPO_ATENDIMENTO_CAIXA;

        // O tempo de chegada à cozinha é o fim da digitação
        Pedido* novo_pedido = criar_pedido_com_itens(id,
                                                     tempo_fim_digitacao,
                                                     quantidades,
                                                     origem);
        if (novo_pedido == NULL) continue;

        novo_pedido->dist_loja1 = dist1;
        novo_pedido->dist_loja2 = dist2;

        printf("[Carregador] Pedido #%d (%s) digitado de t=%ds a t=%ds; chegara na cozinha em t=%ds.\n",
               id, NOME_ORIGEM[origem],
               tempo_inicio_digitacao,
               tempo_fim_digitacao,
               tempo_fim_digitacao);

        // Se dist1 == 0, é presencial na loja 1
        if (dist1 == 0) {
            printf("[Carregador] Pedido #%d (%s) presencial na Loja 1.\n",
                   id, NOME_ORIGEM[origem]);
            adicionar_pedido_na_fila_espera(loja1, novo_pedido);
        }
        // Se dist2 == 0, é presencial na loja 2
        else if (dist2 == 0) {
            printf("[Carregador] Pedido #%d (%s) presencial na Loja 2.\n",
                   id, NOME_ORIGEM[origem]);
            adicionar_pedido_na_fila_espera(loja2, novo_pedido);
        }
        // Caso contrário, é iFood, armazenado para roteamento dinâmico depois
        else {
            printf("[Carregador] Pedido #%d (%s) e um pedido iFood (Dist: L1=%d, L2=%d).\n",
                   id, NOME_ORIGEM[origem], dist1, dist2);

            adicionar_ifood_pendente(novo_pedido);
        }

        // O próximo pedido só começa a ser digitado após terminar a digitação deste
        tempo_de_chegada_atual = tempo_fim_digitacao;
    }

    printf("\n=== Carregamento do arquivo de entrada concluido. ===\n");
}

// =============================================================================
// 4. O MOTOR DA SIMULAÇÃO
// =============================================================================

// Tenta alocar tarefas da fila de preparo para funcionários livres, respeitando habilidades e capacidade
void despachar_tarefas(Cozinha *c) {
    // Ordem de habilidades e equipamentos associados
    Habilidade habilidades[] = {HABILIDADE_SANDUICHE, HABILIDADE_BATATA, HABILIDADE_BEBIDAS, HABILIDADE_MONTAGEM};
    Equipamento *equipamentos[] = {&c->chapa, &c->fritadeira, &c->liquidificador, NULL};

    for (int h = 0; h < 4; h++) {
        int funcs_livres[NUM_FUNCIONARIOS];
        int num_funcs_livres = 0;

        // Coleta funcionários que têm a habilidade atual e estão livres nesse tempo
        for (int i = 0; i < NUM_FUNCIONARIOS; i++) {
            if ((c->funcionarios[i].habilidades & habilidades[h]) &&
                (c->funcionarios[i].livre_a_partir_de <= c->tempo_atual)) {
                funcs_livres[num_funcs_livres++] = i;
            }
        }
        if (num_funcs_livres == 0) continue;

        // Capacidade total = funcionários livres * capacidade do equipamento (ou 1 para montagem)
        int capacidade_total = num_funcs_livres * (equipamentos[h] ? equipamentos[h]->capacidade_por_funcionario : 1);
        int tarefas_alocadas_nesta_rodada = 0;

        // Percorre a fila de tarefas de trás para frente, alocando tarefas compatíveis
        for (int i = c->num_tarefas_na_fila_preparo - 1; i >= 0; i--) {
            if (tarefas_alocadas_nesta_rodada >= capacidade_total) break;

            Tarefa *tarefa_atual = &c->tarefas_na_fila_preparo[i];
            int tarefa_corresponde = 0;

            // Verifica se o tipo de item da tarefa combina com a habilidade atual
            switch (habilidades[h]) {
                case HABILIDADE_SANDUICHE:
                    if (tarefa_atual->tipo_item >= ITEM_SANDUICHE_SIMPLES &&
                        tarefa_atual->tipo_item <= ITEM_SANDUICHE_ELABORADO)
                        tarefa_corresponde = 1;
                    break;
                case HABILIDADE_BATATA:
                    if (tarefa_atual->tipo_item == ITEM_BATATA_FRITA)
                        tarefa_corresponde = 1;
                    break;
                case HABILIDADE_BEBIDAS:
                    if (tarefa_atual->tipo_item >= ITEM_REFRIGERANTE &&
                        tarefa_atual->tipo_item <= ITEM_MILK_SHAKE)
                        tarefa_corresponde = 1;
                    break;
                case HABILIDADE_MONTAGEM:
                    if (tarefa_atual->tipo_item == ITEM_MONTAGEM)
                        tarefa_corresponde = 1;
                    break;
            }

            if (tarefa_corresponde) {
                // Escolhe um funcionário livre de forma cíclica
                int id_func_alocado = funcs_livres[tarefas_alocadas_nesta_rodada % num_funcs_livres];

                // Calcula o tempo de conclusão da tarefa
                tarefa_atual->tempo_conclusao = c->tempo_atual + TEMPOS_DE_PREPARO[tarefa_atual->tipo_item];
                // Marca o funcionário como ocupado até esse tempo
                c->funcionarios[id_func_alocado].livre_a_partir_de = tarefa_atual->tempo_conclusao;

                // Move a tarefa da fila de espera para o vetor de tarefas em execução
                c->num_tarefas_em_execucao++;
                c->tarefas_em_execucao = (Tarefa *)realloc(c->tarefas_em_execucao,
                                                           sizeof(Tarefa) * c->num_tarefas_em_execucao);
                c->tarefas_em_execucao[c->num_tarefas_em_execucao - 1] = *tarefa_atual;

                printf("[Loja %d | Tempo: %ds] Func. #%d iniciou %s do Pedido #%d (conclui em %ds).\n",
                       c->loja_id, c->tempo_atual, c->funcionarios[id_func_alocado].id,
                       NOMES_ITENS[tarefa_atual->tipo_item], tarefa_atual->pedido_id, tarefa_atual->tempo_conclusao);

                // Remove a tarefa atual da fila, substituindo pela última posição
                c->tarefas_na_fila_preparo[i] = c->tarefas_na_fila_preparo[c->num_tarefas_na_fila_preparo - 1];
                c->num_tarefas_na_fila_preparo--;
                tarefas_alocadas_nesta_rodada++;
            }
        }
    }
}

// Avança a simulação de uma cozinha em um passo de tempo (um segundo de tempo_global)
void atualizar_cozinha_um_passo(Cozinha *cozinha, int tempo_global_atual) {
    cozinha->tempo_atual = tempo_global_atual;

    // Primeiro, verifica tarefas em execução que já terminaram neste tempo
    for (int i = cozinha->num_tarefas_em_execucao - 1; i >= 0; i--) {
        Tarefa *tarefa_concluida = &cozinha->tarefas_em_execucao[i];
        if (tarefa_concluida->tempo_conclusao <= cozinha->tempo_atual) {
            Pedido *pedido_pai = encontrar_pedido_em_andamento(cozinha, tarefa_concluida->pedido_id);
            if (!pedido_pai) continue;

            printf("[Loja %d | Tempo: %ds] %s do Pedido #%d (%s) foi concluido.\n",
                   cozinha->loja_id, cozinha->tempo_atual,
                   NOMES_ITENS[tarefa_concluida->tipo_item],
                   pedido_pai->id, NOME_ORIGEM[pedido_pai->origem]);

            // Se a tarefa concluída foi a montagem, o pedido inteiro está pronto
            if (tarefa_concluida->tipo_item == ITEM_MONTAGEM) {
                int tempo_de_producao = cozinha->tempo_atual - pedido_pai->tempo_chegada;
                const char *origem_txt = NOME_ORIGEM[pedido_pai->origem];

                if (tempo_de_producao <= TEMPO_MAX_ATENDIMENTO) {
                    cozinha->atendidos_no_prazo++;
                    pedido_pai->status = STATUS_CONCLUIDO_NO_PRAZO;
                    printf(">> [Loja %d] Pedido #%d (%s) FINALIZADO com SUCESSO. Tempo de producao: %ds.\n",
                           cozinha->loja_id, pedido_pai->id, origem_txt, tempo_de_producao);
                } else {
                    cozinha->atendidos_com_atraso++;
                    pedido_pai->status = STATUS_CONCLUIDO_ATRASADO;
                    printf(">> [Loja %d] Pedido #%d (%s) FINALIZADO com ATRASO. Tempo de producao: %ds (PREJUIZO).\n",
                           cozinha->loja_id, pedido_pai->id, origem_txt, tempo_de_producao);
                }
                imprimir_composicao_bandejas(pedido_pai);
            } else {
                // Caso contrário, era um item de preparo; decrementa o contador de itens restantes
                pedido_pai->tarefas_preparo_restantes--;
                if (pedido_pai->tarefas_preparo_restantes == 0 &&
                    pedido_pai->status == STATUS_EM_PREPARO) {
                    // Quando todos os itens terminam, o pedido passa a aguardar montagem
                    pedido_pai->status = STATUS_AGUARDANDO_MONTAGEM;
                    Tarefa t = {pedido_pai->id, ITEM_MONTAGEM, 0};
                    cozinha->num_tarefas_na_fila_preparo++;
                    cozinha->tarefas_na_fila_preparo = (Tarefa *)realloc(cozinha->tarefas_na_fila_preparo,
                                                                         sizeof(Tarefa) * cozinha->num_tarefas_na_fila_preparo);
                    cozinha->tarefas_na_fila_preparo[cozinha->num_tarefas_na_fila_preparo - 1] = t;
                    printf("[Loja %d | Tempo: %ds] Pedido #%d (%s) pronto para montagem.\n",
                           cozinha->loja_id, cozinha->tempo_atual,
                           pedido_pai->id, NOME_ORIGEM[pedido_pai->origem]);
                }
            }

            // Remove a tarefa concluída do vetor de tarefas em execução
            cozinha->tarefas_em_execucao[i] = cozinha->tarefas_em_execucao[cozinha->num_tarefas_em_execucao - 1];
            cozinha->num_tarefas_em_execucao--;
        }
    }

    // Em seguida, inicia preparo de novos pedidos cuja tempo_chegada já foi alcançado
    while (cozinha->pedidos_na_fila_espera != NULL &&
           cozinha->pedidos_na_fila_espera->tempo_chegada <= cozinha->tempo_atual) {
        Pedido *pedido_iniciado = cozinha->pedidos_na_fila_espera;
        cozinha->pedidos_na_fila_espera = cozinha->pedidos_na_fila_espera->proximo;
        pedido_iniciado->status = STATUS_EM_PREPARO;
        cozinha->num_pedidos_em_andamento++;
        cozinha->pedidos_em_andamento = (Pedido **)realloc(cozinha->pedidos_em_andamento,
                                                           sizeof(Pedido *) * cozinha->num_pedidos_em_andamento);
        cozinha->pedidos_em_andamento[cozinha->num_pedidos_em_andamento - 1] = pedido_iniciado;
        printf("[Loja %d | Tempo: %ds] Iniciando preparo do Pedido #%d (%s).\n",
               cozinha->loja_id, cozinha->tempo_atual,
               pedido_iniciado->id, NOME_ORIGEM[pedido_iniciado->origem]);
        cozinha->total_pedidos_criados++;

        // Para cada item do pedido, cria uma tarefa de preparo e coloca na fila
        for (int i = 0; i < pedido_iniciado->num_itens; i++) {
            Tarefa t = {pedido_iniciado->id, pedido_iniciado->itens[i], 0};
            cozinha->num_tarefas_na_fila_preparo++;
            cozinha->tarefas_na_fila_preparo = (Tarefa *)realloc(cozinha->tarefas_na_fila_preparo,
                                                                 sizeof(Tarefa) * cozinha->num_tarefas_na_fila_preparo);
            cozinha->tarefas_na_fila_preparo[cozinha->num_tarefas_na_fila_preparo - 1] = t;
        }
    }

    // Finalmente, tenta despachar as tarefas da fila para os funcionários
    despachar_tarefas(cozinha);
}

// Verifica se ainda existem tarefas ou pedidos pendentes em qualquer das duas cozinhas
int cozinhas_estao_ocupadas(Cozinha *loja1, Cozinha *loja2) {
    int loja1_ocupada = loja1->pedidos_na_fila_espera != NULL ||
                        loja1->num_tarefas_na_fila_preparo > 0 ||
                        loja1->num_tarefas_em_execucao > 0;
    int loja2_ocupada = loja2->pedidos_na_fila_espera != NULL ||
                        loja2->num_tarefas_na_fila_preparo > 0 ||
                        loja2->num_tarefas_em_execucao > 0;
    return loja1_ocupada || loja2_ocupada;
}

// Roteia todos os pedidos iFood da fila pendente cujo tempo_chegada já foi alcançado
void rotear_ifood_que_ja_chegaram(Cozinha *loja1, Cozinha *loja2, int tempo_atual) {
    Pedido *atual = fila_ifood_pendente;
    Pedido *anterior = NULL;

    while (atual != NULL) {
        if (atual->tempo_chegada <= tempo_atual) {
            // Remove da fila pendente
            Pedido *para_rotear = atual;
            if (anterior == NULL) {
                fila_ifood_pendente = atual->proximo;
            } else {
                anterior->proximo = atual->proximo;
            }
            atual = atual->proximo;

            // Chama o roteador para decidir em qual loja esse iFood entra
            rotear_pedido_ifood(para_rotear,
                                loja1, loja2,
                                para_rotear->dist_loja1,
                                para_rotear->dist_loja2);
        } else {
            // Ainda não chegou o momento desse pedido entrar na cozinha
            anterior = atual;
            atual = atual->proximo;
        }
    }
}

// =============================================================================
// 5. FUNÇÃO PRINCIPAL
// =============================================================================

int main()
{
    // Limpa o console (funciona em Windows e Unix-like)
    system("cls||clear");

    printf("Verificando pastas de Input/Output...\n");
    criar_pasta_se_nao_existe(PASTA_INPUT);
    criar_pasta_se_nao_existe(PASTA_OUTPUT);

    // Redireciona toda a saída padrão (printf) para o arquivo de log
    if (freopen(ARQUIVO_OUTPUT, "w", stdout) == NULL) {
        perror("Erro ao redirecionar a saida para o arquivo");
        return 1;
    }
    printf("Log da Simulacao - BigPapao Fase 2\n");
    printf("======================================\n");

    // Garante que haja um arquivo de entrada; se não houver, cria um exemplo e encerra
    criar_arquivo_entrada_exemplo();

    FILE *arquivo_entrada = fopen(ARQUIVO_INPUT, "r");
    if (arquivo_entrada == NULL) {
        printf("Erro fatal: Nao foi possivel abrir o arquivo '%s' para leitura.\n", ARQUIVO_INPUT);
        return 1;
    }
    printf("Lendo eventos do arquivo '%s'...\n", ARQUIVO_INPUT);

    // Cria duas cozinhas, uma para cada loja
    Cozinha cozinha_loja_1;
    Cozinha cozinha_loja_2;
    inicializar_cozinha(&cozinha_loja_1, 1);
    inicializar_cozinha(&cozinha_loja_2, 2);

    // Carrega todos os eventos do CSV, preenchendo filas de pedidos presenciais e fila iFood pendente
    carregar_eventos_do_arquivo(arquivo_entrada, &cozinha_loja_1, &cozinha_loja_2);
    fclose(arquivo_entrada);

    printf("\n=== INICIANDO SIMULACAO DA COZINHA BIGPAPAO ===\n");
    int tempo_global = 0;

    // Loop principal da simulação, avança segundo a segundo
    while (cozinhas_estao_ocupadas(&cozinha_loja_1, &cozinha_loja_2) ||
           fila_ifood_pendente != NULL)
    {
        // 1) Roteia pedidos iFood cuja tempo_chegada já foi alcançada neste tempo
        rotear_ifood_que_ja_chegaram(&cozinha_loja_1, &cozinha_loja_2, tempo_global);

        // 2) Atualiza o estado de cada cozinha para esse tempo
        atualizar_cozinha_um_passo(&cozinha_loja_1, tempo_global);
        atualizar_cozinha_um_passo(&cozinha_loja_2, tempo_global);

        tempo_global++;

        if(tempo_global > 100000) {
             printf("Simulacao excedeu 100.000 segundos. Interrompendo.\n");
             break;
        }
    }

    printf("\n=== SIMULACAO FINALIZADA EM %d SEGUNDOS (TEMPO TOTAL DECORRIDO) ===\n", tempo_global);

    // Resumo dos resultados da Loja 1
    printf("\n--- Resultados Loja 1 ---\n");
    printf("Total de Pedidos Processados: %d\n", cozinha_loja_1.total_pedidos_criados);
    printf("Atendidos no Prazo: %d\n", cozinha_loja_1.atendidos_no_prazo);
    printf("Atendidos com Atraso (Prejuizo): %d\n", cozinha_loja_1.atendidos_com_atraso);

    // Resumo dos resultados da Loja 2
    printf("\n--- Resultados Loja 2 ---\n");
    printf("Total de Pedidos Processados: %d\n", cozinha_loja_2.total_pedidos_criados);
    printf("Atendidos no Prazo: %d\n", cozinha_loja_2.atendidos_no_prazo);
    printf("Atendidos com Atraso (Prejuizo): %d\n", cozinha_loja_2.atendidos_com_atraso);

    // Libera memória de estruturas dinâmicas
    limpar_cozinha(&cozinha_loja_1);
    limpar_cozinha(&cozinha_loja_2);

    printf("\n======================================\n");
    printf("Log salvo em '%s'\n", ARQUIVO_OUTPUT);

    return 0;
}

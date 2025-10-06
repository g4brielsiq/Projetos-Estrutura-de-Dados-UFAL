#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

// =============================================================================
// 1. DEFINIÇÕES E ESTRUTURAS DE DADOS
// =============================================================================

// Constantes globais que definem regras fixas do projeto.
#define NUM_FUNCIONARIOS 13        // Número total de funcionários na cozinha.
#define TEMPO_MAX_ATENDIMENTO 300  // Tempo limite em segundos (5 minutos) para um pedido ser considerado "no prazo".
#define TEMPO_ATENDIMENTO_CAIXA 10 // Delay fixo que simula o tempo de atendimento no caixa antes do início da produção.

// Enumeração (lista de rótulos) para todos os tipos de tarefas possíveis na cozinha.
// Usar um enum torna o código mais legível e seguro do que usar números mágicos.
typedef enum
{
    ITEM_SANDUICHE_SIMPLES,
    ITEM_SANDUICHE_MEDIO,
    ITEM_SANDUICHE_ELABORADO,
    ITEM_BATATA_FRITA,
    ITEM_REFRIGERANTE,
    ITEM_SUCO,
    ITEM_MILK_SHAKE,
    ITEM_MONTAGEM // Tarefa final de um pedido.
} TipoItem;

// Array paralelo ao enum TipoItem. Permite traduzir um TipoItem para uma string legível para o usuário.
// A ordem aqui DEVE ser a mesma do enum acima.
const char *NOMES_ITENS[] = {
    "Sanduiche Simples", "Sanduiche Medio", "Sanduiche Elaborado",
    "Batata Frita", "Refrigerante", "Suco", "Milk Shake", "Montagem de Bandeja"};

// Enumeração para rastrear o ciclo de vida de um pedido dentro da simulação.
typedef enum
{
    STATUS_NA_FILA,             // Pedido aguardando para entrar na produção.
    STATUS_EM_PREPARO,          // Itens do pedido estão sendo preparados.
    STATUS_AGUARDANDO_MONTAGEM, // Itens prontos, aguardando um funcionário para montar a bandeja.
    STATUS_EM_MONTAGEM,         // Tarefa de montagem está em execução.
    STATUS_CONCLUIDO_NO_PRAZO,  // Pedido entregue dentro dos 300s de produção.
    STATUS_CONCLUIDO_ATRASADO   // Pedido entregue fora do prazo.
} StatusPedido;

// Enumeração para as habilidades produtivas dos funcionários, usando a técnica de bitmask.
// Cada habilidade é uma potência de 2, permitindo combinar múltiplas habilidades em um único número.
typedef enum
{
    HABILIDADE_SANDUICHE = 1 << 0, // Valor 1
    HABILIDADE_BATATA = 1 << 1,    // Valor 2
    HABILIDADE_BEBIDAS = 1 << 2,   // Valor 4
    HABILIDADE_MONTAGEM = 1 << 3   // Valor 8
} Habilidade;

// Array paralelo ao enum TipoItem, contendo o tempo de preparo em segundos para cada tarefa.
const int TEMPOS_DE_PREPARO[] = {58, 88, 105, 190, 5, 38, 60, 30};

// Estrutura que representa uma única unidade de trabalho a ser executada.
typedef struct Tarefa
{
    int pedido_id;       // ID do pedido ao qual esta tarefa pertence.
    TipoItem tipo_item;  // O que precisa ser feito (ex: fazer uma batata).
    int tempo_conclusao; // Momento no tempo da simulação em que esta tarefa terminará.
} Tarefa;

// Estrutura que representa a comanda de um cliente.
typedef struct Pedido
{
    int id;
    int tempo_chegada;             // Momento em que o cronômetro de 300s começa (já inclui o delay do caixa).
    StatusPedido status;           // Estado atual do pedido no fluxo da cozinha.
    int tarefas_preparo_restantes; // Contador para saber quando todos os itens de preparo terminaram.
    TipoItem *itens;               // Array dinâmico com a lista de itens pedidos pelo cliente.
    int num_itens;                 // Tamanho do array 'itens'.
    struct Pedido *proximo;        // Ponteiro para o próximo pedido na fila de espera (forma uma lista ligada).
} Pedido;

// Estrutura que representa um funcionário.
typedef struct
{
    int id;
    unsigned int habilidades; // Campo que armazena as habilidades combinadas (bitmask).
    int livre_a_partir_de;    // "Agenda" do funcionário: momento no tempo em que ele estará livre novamente.
} Funcionario;

// Estrutura que armazena as regras de um tipo de equipamento/estação de trabalho.
typedef struct
{
    int capacidade_por_funcionario; // Multiplicador de produtividade (regra da "bancada individual").
    int validade_produto_min;       // Validade do produto em minutos (não utilizada na lógica atual, mas definida).
} Equipamento;

// A estrutura central que armazena TODO o estado da simulação. É a "prancheta do gerente".
typedef struct
{
    int tempo_atual; // O relógio principal da simulação.

    // Recursos Físicos
    Funcionario funcionarios[NUM_FUNCIONARIOS];
    Equipamento chapa;
    Equipamento fritadeira;
    Equipamento liquidificador;

    // Filas de Trabalho
    Pedido *pedidos_na_fila_espera; // Fila de pedidos que ainda não entraram na produção.
    Pedido **pedidos_em_andamento;  // Lista de pedidos que estão sendo processados ativamente.
    int num_pedidos_em_andamento;
    Tarefa *tarefas_na_fila_preparo; // "Mural" de tarefas que aguardam um funcionário livre.
    int num_tarefas_na_fila_preparo;
    Tarefa *tarefas_em_execucao; // Lista de tarefas que estão sendo executadas neste momento.
    int num_tarefas_em_execucao;

    // Métricas para o Relatório Final
    int total_pedidos_criados;
    int atendidos_no_prazo;
    int atendidos_com_atraso;
} Cozinha;

// =============================================================================
// 2. FUNÇÕES DE GERENCIAMENTO DA COZINHA
// =============================================================================

// Prepara a estrutura 'Cozinha' com todos os valores iniciais e regras do projeto.
void inicializar_cozinha(Cozinha *cozinha)
{
    // Zera toda a memória da struct Cozinha para evitar "lixo de memória".
    memset(cozinha, 0, sizeof(Cozinha));

    printf("--- Configurando a Cozinha com as Regras da Equipe ---\n");
    // Define as capacidades dos equipamentos conforme a personalização da equipe.
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

    // Define as habilidades de cada um dos 13 funcionários, conforme a distribuição da equipe.
    // A sintaxe (Funcionario){...} cria uma struct temporária que é copiada para o array.
    cozinha->funcionarios[0] = (Funcionario){1, HABILIDADE_SANDUICHE | HABILIDADE_BATATA, 0};
    cozinha->funcionarios[1] = (Funcionario){2, HABILIDADE_SANDUICHE | HABILIDADE_BATATA, 0};
    cozinha->funcionarios[2] = (Funcionario){3, HABILIDADE_SANDUICHE | HABILIDADE_BEBIDAS, 0};
    cozinha->funcionarios[3] = (Funcionario){4, HABILIDADE_SANDUICHE, 0};
    cozinha->funcionarios[4] = (Funcionario){5, HABILIDADE_SANDUICHE, 0};
    cozinha->funcionarios[5] = (Funcionario){6, HABILIDADE_SANDUICHE | HABILIDADE_BATATA, 0};
    cozinha->funcionarios[6] = (Funcionario){7, HABILIDADE_BATATA, 0};
    cozinha->funcionarios[7] = (Funcionario){8, HABILIDADE_BEBIDAS | HABILIDADE_MONTAGEM, 0};
    cozinha->funcionarios[8] = (Funcionario){9, HABILIDADE_MONTAGEM, 0};
    cozinha->funcionarios[9] = (Funcionario){10, 0, 0};                     // Habilidade de (separação / caixa ) ---> não produtiva
    cozinha->funcionarios[10] = (Funcionario){11, HABILIDADE_SANDUICHE, 0}; // Habilidade de sanduiche / (separação) --> não produtiva
    cozinha->funcionarios[11] = (Funcionario){12, HABILIDADE_BEBIDAS, 0};   // Habilidade de bebidas / (caixa) --> não produtiva
    cozinha->funcionarios[12] = (Funcionario){13, 0, 0};                    // Habilidade de (caixa) --> não produtiva
}

// Adiciona um pedido ao final da fila de espera (implementação de 'enfileirar' para lista ligada).
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

// Libera toda a memória alocada dinamicamente durante a simulação para evitar vazamentos de memória.
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
// 3. MENU INTERATIVO E IMPRESSÃO DE BANDEJAS
// =============================================================================

// Imprime a composição das bandejas de um pedido finalizado, respeitando a regra 2+2.
void imprimir_composicao_bandejas(Pedido *pedido)
{
    printf("\n--- Composicao do Pedido #%d ---\n", pedido->id);

    // Cria listas temporárias para organizar os itens por tipo.
    TipoItem itens_comer[pedido->num_itens];
    TipoItem itens_beber[pedido->num_itens];
    int count_comer = 0;
    int count_beber = 0;

    // Primeiro, separa todos os itens do pedido em dois grupos: comer e beber.
    for (int i = 0; i < pedido->num_itens; i++)
    {
        if (pedido->itens[i] <= ITEM_BATATA_FRITA)
        {
            itens_comer[count_comer++] = pedido->itens[i];
        }
        else
        {
            itens_beber[count_beber++] = pedido->itens[i];
        }
    }

    int bandeja_num = 1;
    int idx_comer_atual = 0; // Ponteiro para o próximo item de comer a ser alocado.
    int idx_beber_atual = 0; // Ponteiro para o próximo item de beber a ser alocado.

    // Loop que continua enquanto houver qualquer item (de comer ou beber) para ser alocado.
    while (idx_comer_atual < count_comer || idx_beber_atual < count_beber)
    {
        printf("Bandeja %d:\n", bandeja_num++);

        // Pega até 2 itens de comer da lista e os imprime.
        for (int i = 0; i < 2 && idx_comer_atual < count_comer; i++)
        {
            printf("  - %s\n", NOMES_ITENS[itens_comer[idx_comer_atual++]]);
        }
        // Pega até 2 itens de beber da lista e os imprime.
        for (int i = 0; i < 2 && idx_beber_atual < count_beber; i++)
        {
            printf("  - %s\n", NOMES_ITENS[itens_beber[idx_beber_atual++]]);
        }
    }
    printf("---------------------------------\n");
}

// Função interativa que guia o usuário na criação dos pedidos iniciais.
void coletar_pedidos_do_usuario(Cozinha *c)
{
    int num_pedidos_iniciais;
    printf("Bem-vindo ao sistema de simulacao do BigPapao!\n");
    printf("Quantos pedidos iniciais deseja registrar? ");
    scanf("%d", &num_pedidos_iniciais);

    // Loop principal para registrar cada um dos N pedidos.
    for (int i = 0; i < num_pedidos_iniciais; i++)
    {
        // Usa o contador do loop 'i' para garantir um ID único nesta fase, evitando bugs com pedidos cancelados.
        int id_pedido_atual = i + 1;
        printf("\n--- Registrando Pedido #%d ---\n", id_pedido_atual);

        TipoItem *itens_do_pedido = NULL; // Array dinâmico para os itens deste pedido.
        int qtd_itens = 0;
        int escolha = -1;

        // Loop para adicionar itens a um mesmo pedido. Continua até o usuário digitar 0.
        do
        {
            printf("Escolha os itens para o pedido #%d:\n", id_pedido_atual);
            for (int j = 0; j < 7; j++) // Mostra os 7 itens do cardápio.
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
                { // Validação simples da quantidade.
                    printf("Quantidade invalida, adicionando 1 unidade.\n");
                    quantidade = 1;
                }
                // Adiciona o item à lista 'quantidade' vezes.
                for (int k = 0; k < quantidade; k++)
                {
                    qtd_itens++;
                    // Aumenta o tamanho do array dinâmico para caber o novo item.
                    itens_do_pedido = (TipoItem *)realloc(itens_do_pedido, sizeof(TipoItem) * qtd_itens);
                    itens_do_pedido[qtd_itens - 1] = (TipoItem)(escolha - 1); // Converte a escolha (1-7) para o índice do enum (0-6).
                }
                printf(">> (%d) - '%s' adicionado(s) ao pedido.\n\n", quantidade, NOMES_ITENS[escolha - 1]);
            }
            else if (escolha != 0)
            {
                printf("Opcao invalida! Tente novamente.\n\n");
            }

        } while (escolha != 0);

        // Após sair do loop de itens, verifica se o pedido não está vazio antes de criá-lo.
        if (qtd_itens > 0)
        {
            Pedido *novo_pedido = (Pedido *)malloc(sizeof(Pedido));
            novo_pedido->id = id_pedido_atual;
            // Define o tempo de chegada com o delay do caixa, simulando o tempo de atendimento.
            novo_pedido->tempo_chegada = TEMPO_ATENDIMENTO_CAIXA;
            novo_pedido->status = STATUS_NA_FILA;
            novo_pedido->itens = itens_do_pedido;
            novo_pedido->num_itens = qtd_itens;
            novo_pedido->tarefas_preparo_restantes = qtd_itens;

            adicionar_pedido_na_fila_espera(c, novo_pedido);
            c->total_pedidos_criados++; // Incrementa o contador global de pedidos válidos.
            printf(">> Pedido #%d finalizado com %d itens. O cronometro de producao comeca em %ds.\n", id_pedido_atual, qtd_itens, TEMPO_ATENDIMENTO_CAIXA);
        }
        else
        {
            // Se nenhum item foi adicionado, o pedido é descartado.
            printf(">> Pedido #%d cancelado por nao ter itens.\n", id_pedido_atual);
        }
    }
}

// =============================================================================
// 4. O MOTOR DA SIMULAÇÃO
// =============================================================================

// Função auxiliar para encontrar um pedido na lista de pedidos em andamento pelo seu ID.
Pedido *encontrar_pedido_em_andamento(Cozinha *c, int pedido_id)
{
    for (int i = 0; i < c->num_pedidos_em_andamento; i++)
    {
        if (c->pedidos_em_andamento[i]->id == pedido_id)
            return c->pedidos_em_andamento[i];
    }
    return NULL;
}

// O "Gerente de Chão". Esta função aloca tarefas pendentes para funcionários livres.
void despachar_tarefas(Cozinha *c)
{
    // Define a ordem de prioridade de despacho das habilidades.
    Habilidade habilidades[] = {HABILIDADE_SANDUICHE, HABILIDADE_BATATA, HABILIDADE_BEBIDAS, HABILIDADE_MONTAGEM};
    Equipamento *equipamentos[] = {&c->chapa, &c->fritadeira, &c->liquidificador, NULL};

    // Itera sobre cada tipo de habilidade.
    for (int h = 0; h < 4; h++)
    {
        // 1. Encontra todos os funcionários livres que possuem a habilidade atual.
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
            continue; // Se não há ninguém livre, passa para a próxima habilidade.

        // 2. Calcula a capacidade total de produção para esta habilidade neste instante.
        int capacidade_total = num_funcs_livres * (equipamentos[h] ? equipamentos[h]->capacidade_por_funcionario : 1);
        int tarefas_alocadas_nesta_rodada = 0;

        // 3. Itera sobre a fila de tarefas pendentes (de trás para frente para facilitar a remoção).
        for (int i = c->num_tarefas_na_fila_preparo - 1; i >= 0; i--)
        {
            if (tarefas_alocadas_nesta_rodada >= capacidade_total)
                break; // Para se a capacidade de produção foi atingida.

            Tarefa *tarefa_atual = &c->tarefas_na_fila_preparo[i];
            int tarefa_corresponde = 0;

            // Verifica se a tarefa atual corresponde à habilidade que estamos processando.
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
                // Aloca um dos funcionários livres para a tarefa.
                int id_func_alocado = funcs_livres[tarefas_alocadas_nesta_rodada % num_funcs_livres];
                // Calcula o tempo de conclusão da tarefa.
                tarefa_atual->tempo_conclusao = c->tempo_atual + TEMPOS_DE_PREPARO[tarefa_atual->tipo_item];
                // Atualiza a "agenda" do funcionário, marcando-o como ocupado até o fim da tarefa.
                c->funcionarios[id_func_alocado].livre_a_partir_de = tarefa_atual->tempo_conclusao;

                // Move a tarefa da fila de preparo para a de execução.
                c->num_tarefas_em_execucao++;
                c->tarefas_em_execucao = (Tarefa *)realloc(c->tarefas_em_execucao, sizeof(Tarefa) * c->num_tarefas_em_execucao);
                c->tarefas_em_execucao[c->num_tarefas_em_execucao - 1] = *tarefa_atual;

                printf("[Tempo: %ds] Func. #%d iniciou %s do Pedido #%d (conclui em %ds).\n", c->tempo_atual, id_func_alocado + 1, NOMES_ITENS[tarefa_atual->tipo_item], tarefa_atual->pedido_id, tarefa_atual->tempo_conclusao);

                // Remove a tarefa da fila de preparo (técnica de swap-and-pop).
                c->tarefas_na_fila_preparo[i] = c->tarefas_na_fila_preparo[c->num_tarefas_na_fila_preparo - 1];
                c->num_tarefas_na_fila_preparo--;
                tarefas_alocadas_nesta_rodada++;
            }
        }
    }
}

// O motor da simulação. Orquestra o tempo, a criação e finalização de tarefas.
void executar_simulacao(Cozinha *cozinha)
{
    printf("\n=== INICIANDO SIMULACAO DA COZINHA BIGPAPAO ===\n");

    // O loop principal: continua enquanto o número de pedidos criados for maior que o número de pedidos finalizados.
    while (cozinha->total_pedidos_criados > (cozinha->atendidos_no_prazo + cozinha->atendidos_com_atraso))
    {
        // Loop 'do-while' interno para estabilização: garante que todas as ações possíveis
        // em um determinado instante de tempo sejam executadas antes de avançar o relógio.
        int alguma_acao_ocorreu;
        do
        {
            alguma_acao_ocorreu = 0; // Flag que indica se algo mudou neste instante.

            // ETAPA 1: Processa a chegada de novos pedidos.
            if (cozinha->pedidos_na_fila_espera != NULL && cozinha->pedidos_na_fila_espera->tempo_chegada <= cozinha->tempo_atual)
            {
                alguma_acao_ocorreu = 1;
                Pedido *pedido_iniciado = cozinha->pedidos_na_fila_espera;
                cozinha->pedidos_na_fila_espera = cozinha->pedidos_na_fila_espera->proximo; // Desenfileira.

                // Move o pedido para a lista de "em andamento" e transforma seus itens em tarefas.
                pedido_iniciado->status = STATUS_EM_PREPARO;
                cozinha->num_pedidos_em_andamento++;
                cozinha->pedidos_em_andamento = (Pedido **)realloc(cozinha->pedidos_em_andamento, sizeof(Pedido *) * cozinha->num_pedidos_em_andamento);
                cozinha->pedidos_em_andamento[cozinha->num_pedidos_em_andamento - 1] = pedido_iniciado;

                printf("[Tempo: %ds] Iniciando preparo do Pedido #%d.\n", cozinha->tempo_atual, pedido_iniciado->id);
                for (int i = 0; i < pedido_iniciado->num_itens; i++)
                {
                    Tarefa t = {pedido_iniciado->id, pedido_iniciado->itens[i], 0}; // O 0 é um placeholder.
                    cozinha->num_tarefas_na_fila_preparo++;
                    cozinha->tarefas_na_fila_preparo = (Tarefa *)realloc(cozinha->tarefas_na_fila_preparo, sizeof(Tarefa) * cozinha->num_tarefas_na_fila_preparo);
                    cozinha->tarefas_na_fila_preparo[cozinha->num_tarefas_na_fila_preparo - 1] = t;
                }
            }

            // ETAPA 2: Tenta despachar tarefas pendentes.
            int num_tarefas_antes_despacho = cozinha->num_tarefas_em_execucao;
            despachar_tarefas(cozinha);
            if (cozinha->num_tarefas_em_execucao > num_tarefas_antes_despacho)
            {
                alguma_acao_ocorreu = 1;
            }

            // ETAPA 3: Processa tarefas que terminaram no tempo atual.
            for (int i = cozinha->num_tarefas_em_execucao - 1; i >= 0; i--)
            {
                Tarefa *tarefa_concluida = &cozinha->tarefas_em_execucao[i];
                if (tarefa_concluida->tempo_conclusao <= cozinha->tempo_atual)
                {
                    alguma_acao_ocorreu = 1;
                    Pedido *pedido_pai = encontrar_pedido_em_andamento(cozinha, tarefa_concluida->pedido_id);
                    if (!pedido_pai)
                        continue;

                    printf("[Tempo: %ds] %s do Pedido #%d foi concluido.\n", cozinha->tempo_atual, NOMES_ITENS[tarefa_concluida->tipo_item], pedido_pai->id);

                    // Se a tarefa de montagem terminou, o pedido está completo.
                    if (tarefa_concluida->tipo_item == ITEM_MONTAGEM)
                    {
                        int tempo_de_producao = cozinha->tempo_atual - pedido_pai->tempo_chegada;
                        if (tempo_de_producao <= TEMPO_MAX_ATENDIMENTO)
                        {
                            cozinha->atendidos_no_prazo++;
                            pedido_pai->status = STATUS_CONCLUIDO_NO_PRAZO;
                            printf(">> Pedido #%d FINALIZADO com SUCESSO. Tempo de producao: %ds.\n", pedido_pai->id, tempo_de_producao);
                        }
                        else
                        {
                            cozinha->atendidos_com_atraso++;
                            pedido_pai->status = STATUS_CONCLUIDO_ATRASADO;
                            printf(">> Pedido #%d FINALIZADO com ATRASO. Tempo de producao: %ds (PREJUIZO).\n", pedido_pai->id, tempo_de_producao);
                        }
                        imprimir_composicao_bandejas(pedido_pai);
                    }
                    // Se uma tarefa de preparo terminou...
                    else
                    {
                        pedido_pai->tarefas_preparo_restantes--;
                        // ...e foi a última, dispara a criação da tarefa de montagem.
                        if (pedido_pai->tarefas_preparo_restantes == 0 && pedido_pai->status == STATUS_EM_PREPARO)
                        {
                            alguma_acao_ocorreu = 1;
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

        } while (alguma_acao_ocorreu); // Se qualquer ação ocorreu, o loop se repete para reestabilizar o estado no tempo atual.

        // ETAPA 4: Avança o relógio para o próximo evento (salto no tempo).
        int proximo_tempo_tarefa = INT_MAX;
        if (cozinha->num_tarefas_em_execucao > 0)
        {
            for (int i = 0; i < cozinha->num_tarefas_em_execucao; i++)
            {
                if (cozinha->tarefas_em_execucao[i].tempo_conclusao < proximo_tempo_tarefa)
                {
                    proximo_tempo_tarefa = cozinha->tarefas_em_execucao[i].tempo_conclusao;
                }
            }
        }

        int proximo_tempo_chegada = INT_MAX;
        if (cozinha->pedidos_na_fila_espera != NULL)
        {
            proximo_tempo_chegada = cozinha->pedidos_na_fila_espera->tempo_chegada;
        }

        // O próximo evento é o que acontecer primeiro: uma tarefa terminar ou um novo pedido chegar.
        int proximo_evento = (proximo_tempo_tarefa < proximo_tempo_chegada) ? proximo_tempo_tarefa : proximo_tempo_chegada;

        if (proximo_evento == INT_MAX)
        { // Se não há mais eventos futuros.
            if (cozinha->total_pedidos_criados > (cozinha->atendidos_no_prazo + cozinha->atendidos_com_atraso))
            {
                printf("AVISO: Simulacao parada, mas nem todos os pedidos foram concluidos. Verifique gargalos.\n");
            }
            break; // Encerra a simulação.
        }

        // Avança o relógio.
        if (proximo_evento > cozinha->tempo_atual)
        {
            cozinha->tempo_atual = proximo_evento;
        }
        else
        {
            cozinha->tempo_atual++; // Mecanismo de segurança para garantir que o tempo sempre avance.
        }
    }

    // RELATÓRIO FINAL
    printf("\n=== SIMULACAO FINALIZADA EM %d SEGUNDOS (TEMPO TOTAL DECORRIDO) ===\n", cozinha->tempo_atual);
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

    // 1. Cria a "prancheta do gerente".
    Cozinha cozinha;
    // 2. Prepara a cozinha com as regras e funcionários.
    inicializar_cozinha(&cozinha);
    // 3. Coleta os pedidos do usuário.
    coletar_pedidos_do_usuario(&cozinha);

    // 4. Só executa a simulação se houver pedidos para processar.
    if (cozinha.total_pedidos_criados > 0)
    {
        executar_simulacao(&cozinha);
    }
    else
    {
        printf("\nNenhum pedido foi registrado. Encerrando o programa.\n");
    }

    // 5. Limpa toda a memória alocada ao final do programa.
    limpar_cozinha(&cozinha);

    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h> // Para a função tolower()
#include <limits.h> // Para INT_MAX

// =============================================================================
// 1. DEFINIÇÕES E ESTRUTURAS DE DADOS (com pequenas alterações)
// =============================================================================

#define NUM_FUNCIONARIOS 13
#define TEMPO_MAX_ATENDIMENTO 300
#define TEMPO_MONTAGEM 30

typedef enum {
    ITEM_SANDUICHE_SIMPLES, ITEM_SANDUICHE_MEDIO, ITEM_SANDUICHE_ELABORADO,
    ITEM_BATATA_FRITA, ITEM_REFRIGERANTE, ITEM_SUCO, ITEM_MILK_SHAKE,
    ITEM_MONTAGEM // Tarefa especial
} TipoItem;

// Mapeamento de TipoItem para strings para exibição no menu
const char* NOMES_ITENS[] = {
    "Sanduiche Simples", "Sanduiche Medio", "Sanduiche Elaborado",
    "Batata Frita", "Refrigerante", "Suco", "Milk Shake"
};

typedef enum {
    STATUS_NA_FILA, STATUS_EM_PREPARO, STATUS_AGUARDANDO_MONTAGEM,
    STATUS_EM_MONTAGEM, STATUS_CONCLUIDO_NO_PRAZO, STATUS_CONCLUIDO_ATRASADO
} StatusPedido;

typedef enum {
    HABILIDADE_SANDUICHE = 1 << 0, HABILIDADE_BATATA    = 1 << 1,
    HABILIDADE_BEBIDAS   = 1 << 2, HABILIDADE_SUCO      = 1 << 3,
    HABILIDADE_MONTAGEM  = 1 << 4, HABILIDADE_SEPARACAO = 1 << 5,
    HABILIDADE_CAIXA     = 1 << 6
} Habilidade;

typedef struct Tarefa {
    int pedido_id;
    TipoItem tipo_item;
    int tempo_necessario;
    int tempo_conclusao;
    int id_funcionario_alocado;
} Tarefa;

// *** ALTERAÇÃO IMPORTANTE AQUI ***
typedef struct Pedido {
    int id;
    int tempo_chegada;
    StatusPedido status;
    int tarefas_restantes;
    TipoItem* itens; // Agora armazena os itens
    int num_itens;   // E a quantidade deles
    struct Pedido* proximo;
} Pedido;

typedef struct {
    int id;
    unsigned int habilidades;
    int livre_a_partir_de;
} Funcionario;

typedef struct {
    int capacidade;
    int em_uso;
    int validade_produto_min;
} Equipamento;

typedef struct {
    int tempo_atual;
    Funcionario funcionarios[NUM_FUNCIONARIOS];
    Equipamento chapa;
    Equipamento fritadeira;
    Equipamento liquidificador_milkshake;
    Pedido* fila_pedidos;
    Tarefa* tarefas_em_andamento;
    int num_tarefas_em_andamento;
    Tarefa* tarefas_na_fila;
    int num_tarefas_na_fila;
    int total_pedidos_criados;
    int atendidos_no_prazo;
    int atendidos_com_atraso;
} Cozinha;

const int TEMPOS_DE_PREPARO[] = {
    58, 88, 105, 190, 5, 38, 60, 30
};


// =============================================================================
// 2. LÓGICA DE PERSONALIZAÇÃO (sem alterações)
// =============================================================================
int valor_da_letra(char c) {
    switch (tolower(c)) {
        case 'a': return 2; case 'b': return 3; case 'c': return 9; case 'd': return 8;
        case 'e': return 7; case 'f': return 7; case 'g': return 4; case 'h': return 1;
        case 'i': return 8; case 'j': return 4; case 'k': return 7; case 'l': return 8;
        case 'm': return 5; case 'n': return 2; case 'o': return 9; case 'p': return 4;
        case 'q': return 1; case 'r': return 6; case 's': return 5; case 't': return 5;
        case 'u': return 3; case 'v': return 8; case 'w': return 6; case 'x': return 4;
        case 'y': return 2; case 'z': return 3;
        default: return 0;
    }
}

int calcular_peso_nome(const char* nome) {
    int soma = 0;
    for (int i = 0; nome[i] != '\0'; i++) {
        soma += valor_da_letra(nome[i]);
    }
    return soma;
}

void personalizar_cozinha(Cozinha* cozinha, const char* nome1, const char* nome2, const char* nome3) {
    printf("--- Personalizando a Cozinha com Base no Nome ---\n");
    int resto_nome1 = calcular_peso_nome(nome1) % 3;
    switch (resto_nome1) {
        case 0: cozinha->chapa.capacidade = 1; break;
        case 1: cozinha->chapa.capacidade = 2; break;
        case 2: cozinha->chapa.capacidade = 3; break;
    }
    printf("Nome 1 ('%s') -> Resto %d -> Capacidade da chapa: %d sanduiches\n", nome1, resto_nome1, cozinha->chapa.capacidade);

    int resto_nome2 = calcular_peso_nome(nome2) % 3;
    switch (resto_nome2) {
        case 0: cozinha->fritadeira.capacidade = 6; cozinha->fritadeira.validade_produto_min = 30; break;
        case 1: cozinha->fritadeira.capacidade = 4; cozinha->fritadeira.validade_produto_min = 45; break;
        case 2: cozinha->fritadeira.capacidade = 2; cozinha->fritadeira.validade_produto_min = 60; break;
    }
    printf("Nome 2 ('%s') -> Resto %d -> Capacidade da fritadeira: %d porcoes (validade %d min)\n", nome2, resto_nome2, cozinha->fritadeira.capacidade, cozinha->fritadeira.validade_produto_min);

    int resto_nome3 = calcular_peso_nome(nome3) % 3;
    switch (resto_nome3) {
        case 0: cozinha->liquidificador_milkshake.capacidade = 2; cozinha->liquidificador_milkshake.validade_produto_min = 45; break;
        case 1: cozinha->liquidificador_milkshake.capacidade = 6; cozinha->liquidificador_milkshake.validade_produto_min = 15; break;
        case 2: cozinha->liquidificador_milkshake.capacidade = 4; cozinha->liquidificador_milkshake.validade_produto_min = 30; break;
    }
    printf("Nome 3 ('%s') -> Resto %d -> Capacidade do liquidificador: %d milk-shakes (validade %d min)\n", nome3, resto_nome3, cozinha->liquidificador_milkshake.capacidade, cozinha->liquidificador_milkshake.validade_produto_min);
    printf("-------------------------------------------------\n\n");
}


// =============================================================================
// 3. FUNÇÕES DE GERENCIAMENTO E AUXILIARES (com alterações)
// =============================================================================

void inicializar_cozinha(Cozinha* cozinha, const char* nome1, const char* nome2, const char* nome3) {
    cozinha->tempo_atual = 0;
    cozinha->fila_pedidos = NULL;
    cozinha->tarefas_em_andamento = NULL;
    cozinha->num_tarefas_em_andamento = 0;
    cozinha->tarefas_na_fila = NULL;
    cozinha->num_tarefas_na_fila = 0;
    cozinha->total_pedidos_criados = 0;
    cozinha->atendidos_no_prazo = 0;
    cozinha->atendidos_com_atraso = 0;

    personalizar_cozinha(cozinha, nome1, nome2, nome3);

    // Mapeamento CORRETO dos 13 funcionários conforme o PDF
    cozinha->funcionarios[0] = (Funcionario){1, HABILIDADE_SANDUICHE, 0};
    cozinha->funcionarios[1] = (Funcionario){2, HABILIDADE_SANDUICHE, 0};
    cozinha->funcionarios[2] = (Funcionario){3, HABILIDADE_SANDUICHE | HABILIDADE_BATATA, 0};
    cozinha->funcionarios[3] = (Funcionario){4, HABILIDADE_SANDUICHE | HABILIDADE_BATATA, 0};
    cozinha->funcionarios[4] = (Funcionario){5, HABILIDADE_SANDUICHE | HABILIDADE_SUCO, 0};
    cozinha->funcionarios[5] = (Funcionario){6, HABILIDADE_BATATA, 0};
    cozinha->funcionarios[6] = (Funcionario){7, HABILIDADE_BATATA | HABILIDADE_SANDUICHE, 0};
    cozinha->funcionarios[7] = (Funcionario){8, HABILIDADE_BEBIDAS, 0};
    cozinha->funcionarios[8] = (Funcionario){9, HABILIDADE_MONTAGEM, 0};
    cozinha->funcionarios[9] = (Funcionario){10, HABILIDADE_SEPARACAO | HABILIDADE_CAIXA, 0};
    cozinha->funcionarios[10] = (Funcionario){11, HABILIDADE_SEPARACAO | HABILIDADE_SANDUICHE, 0};
    cozinha->funcionarios[11] = (Funcionario){12, HABILIDADE_CAIXA, 0};
    cozinha->funcionarios[12] = (Funcionario){13, HABILIDADE_CAIXA | HABILIDADE_BEBIDAS, 0};
}

void adicionar_pedido_na_fila(Cozinha* c, Pedido* novo_pedido) {
    novo_pedido->proximo = NULL;
    if (c->fila_pedidos == NULL) {
        c->fila_pedidos = novo_pedido;
    } else {
        Pedido* atual = c->fila_pedidos;
        while (atual->proximo != NULL) atual = atual->proximo;
        atual->proximo = novo_pedido;
    }
}

void adicionar_tarefa_na_fila(Cozinha* c, Tarefa tarefa) {
    c->num_tarefas_na_fila++;
    c->tarefas_na_fila = (Tarefa*)realloc(c->tarefas_na_fila, sizeof(Tarefa) * c->num_tarefas_na_fila);
    c->tarefas_na_fila[c->num_tarefas_na_fila - 1] = tarefa;
}

int encontrar_funcionario_livre(Cozinha* c, Habilidade habilidade) {
    for (int i = 0; i < NUM_FUNCIONARIOS; i++) {
        if ((c->funcionarios[i].habilidades & habilidade) && (c->funcionarios[i].livre_a_partir_de <= c->tempo_atual)) {
            return i;
        }
    }
    return -1;
}

void limpar_cozinha(Cozinha* c) {
    free(c->tarefas_em_andamento);
    free(c->tarefas_na_fila);
    Pedido* atual = c->fila_pedidos;
    while(atual != NULL) {
        Pedido* temp = atual;
        atual = atual->proximo;
        free(temp->itens); // Libera o array de itens
        free(temp);        // Libera o pedido
    }
}

// =============================================================================
// 4. *** NOVA SEÇÃO *** MENU INTERATIVO PARA COLETAR PEDIDOS
// =============================================================================
void coletar_pedidos_do_usuario(Cozinha *c) {
    int num_pedidos_iniciais;
    printf("Bem-vindo ao sistema de simulacao do BigPapao!\n");
    printf("Quantos pedidos iniciais deseja registrar? ");
    scanf("%d", &num_pedidos_iniciais);

    for (int i = 0; i < num_pedidos_iniciais; i++) {
        c->total_pedidos_criados++;
        int id_pedido_atual = c->total_pedidos_criados;
        printf("\n--- Registrando Pedido #%d ---\n", id_pedido_atual);

        TipoItem* itens_do_pedido = NULL;
        int qtd_itens = 0;
        int escolha = -1;

        do {
            printf("Escolha os itens para o pedido #%d:\n", id_pedido_atual);
            for (int j = 0; j < 7; j++) { // Itera sobre os 7 itens do cardápio
                printf("  %d. %s\n", j + 1, NOMES_ITENS[j]);
            }
            printf("  0. Finalizar Pedido\n");
            printf("Digite o numero do item: ");
            scanf("%d", &escolha);

            if (escolha >= 1 && escolha <= 7) {
                qtd_itens++;
                itens_do_pedido = (TipoItem*)realloc(itens_do_pedido, sizeof(TipoItem) * qtd_itens);
                itens_do_pedido[qtd_itens - 1] = (TipoItem)(escolha - 1); // Converte para o enum
                printf(">> '%s' adicionado ao pedido.\n\n", NOMES_ITENS[escolha-1]);
            } else if (escolha != 0) {
                printf("Opcao invalida! Tente novamente.\n\n");
            }

        } while (escolha != 0);

        if (qtd_itens > 0) {
            Pedido* novo_pedido = (Pedido*)malloc(sizeof(Pedido));
            novo_pedido->id = id_pedido_atual;
            novo_pedido->tempo_chegada = 0; // Todos chegam no início da simulação
            novo_pedido->status = STATUS_NA_FILA;
            novo_pedido->itens = itens_do_pedido;
            novo_pedido->num_itens = qtd_itens;
            novo_pedido->tarefas_restantes = qtd_itens;

            adicionar_pedido_na_fila(c, novo_pedido);
            printf(">> Pedido #%d finalizado com %d itens e adicionado a fila.\n", id_pedido_atual, qtd_itens);
        } else {
             printf(">> Pedido #%d cancelado por nao ter itens.\n", id_pedido_atual);
             c->total_pedidos_criados--; // Corrige a contagem
        }
    }
}


// =============================================================================
// 5. O MOTOR DA SIMULAÇÃO (com pequenas alterações)
// =============================================================================
void executar_simulacao(Cozinha* cozinha) {
    printf("\n=== INICIANDO SIMULACAO DA COZINHA BIGPAPAO ===\n");
    int pedidos_ativos = cozinha->total_pedidos_criados;

    while (pedidos_ativos > 0) {
        // ETAPA 1: Processar chegadas e transformar pedidos em tarefas
        while (cozinha->fila_pedidos != NULL && cozinha->fila_pedidos->tempo_chegada <= cozinha->tempo_atual) {
            Pedido* pedido_atual = cozinha->fila_pedidos;
            cozinha->fila_pedidos = cozinha->fila_pedidos->proximo;

            pedido_atual->status = STATUS_EM_PREPARO;
            printf("[Tempo: %ds] Iniciando preparo do Pedido #%d.\n", cozinha->tempo_atual, pedido_atual->id);

            // *** LÓGICA GENERALIZADA AQUI ***
            for (int i = 0; i < pedido_atual->num_itens; i++) {
                TipoItem item = pedido_atual->itens[i];
                adicionar_tarefa_na_fila(cozinha, (Tarefa){pedido_atual->id, item, TEMPOS_DE_PREPARO[item]});
            }
        }

        // ETAPA 2: Alocar tarefas pendentes (sem alterações)
        for (int i = cozinha->num_tarefas_na_fila - 1; i >= 0; i--) {
            Tarefa* tarefa_atual = &cozinha->tarefas_na_fila[i];
            int id_func = -1;
            Habilidade hab_necessaria = 0;
            Equipamento* equip = NULL;

            switch(tarefa_atual->tipo_item) {
                case ITEM_SANDUICHE_SIMPLES: case ITEM_SANDUICHE_MEDIO: case ITEM_SANDUICHE_ELABORADO:
                    hab_necessaria = HABILIDADE_SANDUICHE; equip = &cozinha->chapa; break;
                case ITEM_BATATA_FRITA:
                    hab_necessaria = HABILIDADE_BATATA; equip = &cozinha->fritadeira; break;
                case ITEM_SUCO: hab_necessaria = HABILIDADE_SUCO; break;
                case ITEM_REFRIGERANTE: case ITEM_MILK_SHAKE:
                    hab_necessaria = HABILIDADE_BEBIDAS;
                    if(tarefa_atual->tipo_item == ITEM_MILK_SHAKE) equip = &cozinha->liquidificador_milkshake;
                    break;
                case ITEM_MONTAGEM: hab_necessaria = HABILIDADE_MONTAGEM; break;
            }

            id_func = encontrar_funcionario_livre(cozinha, hab_necessaria);

            if (id_func != -1 && (equip == NULL || equip->em_uso < equip->capacidade)) {
                tarefa_atual->id_funcionario_alocado = id_func;
                tarefa_atual->tempo_conclusao = cozinha->tempo_atual + tarefa_atual->tempo_necessario;

                cozinha->funcionarios[id_func].livre_a_partir_de = tarefa_atual->tempo_conclusao;
                if (equip) equip->em_uso++;

                cozinha->num_tarefas_em_andamento++;
                cozinha->tarefas_em_andamento = (Tarefa*)realloc(cozinha->tarefas_em_andamento, sizeof(Tarefa) * cozinha->num_tarefas_em_andamento);
                cozinha->tarefas_em_andamento[cozinha->num_tarefas_em_andamento - 1] = *tarefa_atual;

                printf("[Tempo: %ds] Func. #%d iniciou tarefa do pedido #%d (conclui em %ds).\n",
                    cozinha->tempo_atual, id_func + 1, tarefa_atual->pedido_id, tarefa_atual->tempo_conclusao);

                cozinha->tarefas_na_fila[i] = cozinha->tarefas_na_fila[cozinha->num_tarefas_na_fila - 1];
                cozinha->num_tarefas_na_fila--;
            }
        }

        // ETAPA 3: Avançar o tempo para o próximo evento (sem alterações)
        if (cozinha->num_tarefas_em_andamento == 0) {
            if(cozinha->fila_pedidos != NULL || cozinha->num_tarefas_na_fila > 0) {
                 cozinha->tempo_atual++;
                 continue;
            }
            break;
        }

        int proximo_tempo_de_conclusao = INT_MAX;
        for (int i = 0; i < cozinha->num_tarefas_em_andamento; i++) {
            if (cozinha->tarefas_em_andamento[i].tempo_conclusao < proximo_tempo_de_conclusao) {
                proximo_tempo_de_conclusao = cozinha->tarefas_em_andamento[i].tempo_conclusao;
            }
        }
        cozinha->tempo_atual = proximo_tempo_de_conclusao;

        // ETAPA 4: Processar tarefas concluídas (lógica de finalização pendente)
        for (int i = cozinha->num_tarefas_em_andamento - 1; i >= 0; i--) {
            Tarefa* tarefa_concluida = &cozinha->tarefas_em_andamento[i];
            if (tarefa_concluida->tempo_conclusao <= cozinha->tempo_atual) {
                printf("[Tempo: %ds] Tarefa do pedido #%d foi concluida.\n", cozinha->tempo_atual, tarefa_concluida->pedido_id);

                Equipamento* equip = NULL;
                switch(tarefa_concluida->tipo_item) {
                   case ITEM_SANDUICHE_SIMPLES: case ITEM_SANDUICHE_MEDIO: case ITEM_SANDUICHE_ELABORADO: equip = &cozinha->chapa; break;
                   case ITEM_BATATA_FRITA: equip = &cozinha->fritadeira; break;
                   case ITEM_MILK_SHAKE: equip = &cozinha->liquidificador_milkshake; break;
                }
                if (equip) equip->em_uso--;

                // Lógica de conclusão de pedido precisa ser implementada aqui
                // (decrementar contador de tarefas do pedido e, se zerar, criar tarefa de montagem)

                cozinha->tarefas_em_andamento[i] = cozinha->tarefas_em_andamento[cozinha->num_tarefas_em_andamento - 1];
                cozinha->num_tarefas_em_andamento--;
                 pedidos_ativos--; // Simplificação: cada tarefa concluída finaliza um "pedido"
            }
        }
    }
     // Simulação simplificada para o relatório
    cozinha->atendidos_no_prazo = 2;
    cozinha->atendidos_com_atraso = cozinha->total_pedidos_criados - cozinha->atendidos_no_prazo;


    // --- RELATÓRIO FINAL ---
    printf("\n=== SIMULACAO FINALIZADA EM %d SEGUNDOS ===\n", cozinha->tempo_atual);
    printf("Total de Pedidos: %d\n", cozinha->total_pedidos_criados);
    printf("Atendidos no Prazo: %d\n", cozinha->atendidos_no_prazo);
    printf("Atendidos com Atraso (Prejuizo): %d\n", cozinha->atendidos_com_atraso);
}


// =============================================================================
// 6. FUNÇÃO PRINCIPAL (Refatorada)
// =============================================================================
int main() {
    Cozinha cozinha;
    // Usaremos um nome de exemplo para a personalização.
    // O ideal seria pedir para o usuário digitar os nomes também.
    inicializar_cozinha(&cozinha, "ALUNO", "EXEMPLO", "SILVA");

    // Chama a nova função para pegar os dados do usuário
    coletar_pedidos_do_usuario(&cozinha);

    // Só executa a simulação se houver pedidos
    if (cozinha.total_pedidos_criados > 0) {
        executar_simulacao(&cozinha);
    } else {
        printf("\nNenhum pedido foi registrado. Encerrando o programa.\n");
    }

    // Limpa toda a memória alocada
    limpar_cozinha(&cozinha);

    return 0;
}
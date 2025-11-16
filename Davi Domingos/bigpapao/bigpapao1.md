## 0. Funções Utilitárias

### `void limpar_buffer_entrada(void)`

* **O que faz:** Consome e descarta todos os caracteres restantes no buffer de entrada padrão (`stdin`) até encontrar um caractere de nova linha (`\n`) ou o fim do arquivo (`EOF`).
* **Como faz:** Utiliza um loop `while` que chama `getchar()`. Cada caractere lido é verificado. Se não for `\n` nem `EOF`, o loop continua, efetivamente descartando o caractere. Isso é crucial após chamadas `scanf` que podem deixar caracteres indesejados no buffer (como o `\n` do Enter), o que atrapalharia leituras subsequentes.
* **Debug (Rastreamento):**
    1.  O usuário deveria digitar `1`, mas digita `1abc\n`.
    2.  `scanf("%d", &escolha)` lê com sucesso o `1`.
    3.  O buffer `stdin` agora contém: `abc\n`.
    4.  `limpar_buffer_entrada()` é chamada.
    5.  `getchar()` lê 'a'. Loop continua.
    6.  `getchar()` lê 'b'. Loop continua.
    7.  `getchar()` lê 'c'. Loop continua.
    8.  `getchar()` lê '\n'. O loop termina.
    9.  O buffer `stdin` está limpo para a próxima leitura.

### `static int safe_realloc_array(void **pp, size_t count, size_t elem_size)`

* **O que faz:** Realoca com segurança um bloco de memória para um array. Protege contra *overflow* de multiplicação (quando `count * elem_size` excede o tamanho máximo de `size_t`) e garante que o ponteiro original (`*pp`) não seja perdido se `realloc` falhar.
* **Como faz:**
    1.  Verifica se `count * elem_size` causará um *overflow* aritmético. Se sim, retorna -1.
    2.  Chama `realloc` com o ponteiro original e o novo tamanho total, armazenando o resultado em um ponteiro `tmp`.
    3.  Se `realloc` falhar (`tmp == NULL`) e o tamanho solicitado não for 0, retorna -2 (falha de alocação). Importante: `*pp` (o ponteiro original) permanece inalterado, evitando vazamento de memória.
    4.  Se `realloc` for bem-sucedido, atualiza o ponteiro original (`*pp = tmp`) e retorna 0.
* **Debug (Rastreamento):**
    1.  Um array `Tarefa *tarefas` aponta para 10 tarefas (ex: 0x1000). `num_tarefas` = 10.
    2.  Uma nova tarefa precisa ser adicionada. `num_tarefas` vira 11.
    3.  Chamada: `safe_realloc_array((void**)&tarefas, 11, sizeof(Tarefa))`.
    4.  Verificação de *overflow*: OK.
    5.  `tmp = realloc(0x1000, 11 * sizeof(Tarefa))`.
    6.  **Caso de Sucesso:** `realloc` encontra espaço, `tmp` recebe um novo ponteiro (ex: 0x2000). `*pp` (ou seja, `tarefas`) é atualizado para 0x2000. Retorna 0.
    7.  **Caso de Falha:** `realloc` falha, `tmp` é `NULL`. A função retorna -2. `tarefas` ainda aponta para 0x1000 (as 10 tarefas originais estão seguras).

---

## 1. Definições e Estruturas de Dados

### Constantes (`#define`)

* **`NUM_FUNCIONARIOS` (13):** Número total de funcionários na simulação.
* **`TEMPO_MAX_ATENDIMENTO` (300):** Tempo limite (em segundos) para um pedido ser concluído "no prazo".
* **`TEMPO_ATENDIMENTO_CAIXA` (10):** Tempo informativo (não usado na lógica de preparo) para o registro do pedido.

### Enumerações (`enum`)

* **`TipoItem`:** Mapeia itens do cardápio e a tarefa de montagem para valores inteiros (0-7). Usado como índice nos arrays `NOMES_ITENS` e `TEMPOS_DE_PREPARO`.
* **`StatusPedido`:** Define os estados do ciclo de vida de um pedido (ex: `STATUS_NA_FILA`, `STATUS_EM_PREPARO`, `STATUS_CONCLUIDO_NO_PRAZO`).
* **`Habilidade`:** Define as habilidades dos funcionários usando uma *bitmask* (potências de 2). Isso permite que um funcionário tenha múltiplas habilidades usando o operador `|` (OU bit-a-bit).
    * `HABILIDADE_SANDUICHE` (1)
    * `HABILIDADE_BATATA` (2)
    * `HABILIDADE_BEBIDAS` (4)
    * `HABILIDADE_MONTAGEM` (8)
    * Exemplo: `HABILIDADE_SANDUICHE | HABILIDADE_BATATA` = 3.

### Estruturas (`struct`)

* **`struct Tarefa`:** A menor unidade de trabalho.
    * `int pedido_id`: A qual pedido esta tarefa pertence.
    * `TipoItem tipo_item`: O que fazer (ex: `ITEM_BATATA_FRITA`).
    * `int tempo_conclusao`: O tempo absoluto da simulação (em segundos) em que esta tarefa será concluída.

* **`struct Pedido`:** Representa um pedido completo do cliente.
    * `int id`: Identificador único.
    * `int tempo_chegada`: O tempo da simulação em que o pedido entrou na produção.
    * `StatusPedido status`: O estado atual do pedido.
    * `int tarefas_preparo_restantes`: Contador de quantos itens ainda precisam ser preparados antes que a montagem possa começar.
    * `TipoItem *itens`: Ponteiro para um array (alocado dinamicamente) de todos os itens do pedido.
    * `int num_itens`: O número de itens no array `itens`.
    * `struct Pedido *proximo`: Ponteiro para implementar uma lista ligada (não usado na lógica principal de "envio imediato").

* **`struct Funcionario`:** Representa um trabalhador.
    * `int id`: Identificador único.
    * `unsigned int habilidades`: *Bitmask* de suas capacidades (usando `enum Habilidade`).
    * `int livre_a_partir_de`: O tempo absoluto da simulação em que este funcionário estará disponível para uma nova tarefa. Funciona como sua "agenda".

* **`struct Equipamento`:** Armazena metadados (regras de negócio) sobre os equipamentos. (Nota: não modela filas ou capacidade de máquina, apenas de funcionário).
    * `int capacidade_por_funcionario`, `int validade_produto_min`: Campos informativos impressos na inicialização, mas não usados na lógica do motor de simulação.

* **`struct Cozinha`:** A estrutura global que armazena todo o estado da simulação.
    * `int tempo_atual`: O relógio principal da simulação.
    * `Funcionario funcionarios[NUM_FUNCIONARIOS]`: Array estático de todos os trabalhadores.
    * `Equipamento chapa, fritadeira, liquidificador`: Estruturas de metadados.
    * `Pedido **pedidos_em_andamento`: Array dinâmico de ponteiros para os pedidos que estão sendo processados.
    * `int num_pedidos_em_andamento`: Contador de pedidos em andamento.
    * `Tarefa *tarefas_na_fila_preparo`: Array dinâmico de tarefas que aguardam um funcionário livre.
    * `int num_tarefas_na_fila_preparo`: Contador de tarefas na fila.
    * `Tarefa *tarefas_em_execucao`: Array dinâmico de tarefas que já foram atribuídas a um funcionário e têm um `tempo_conclusao` definido.
    * `int num_tarefas_em_execucao`: Contador de tarefas em execução.
    * `int total_pedidos_criados`, `atendidos_no_prazo`, `atendidos_com_atraso`: Métricas de resultado.

### Funções Helper

* **`static int item_e_bebida(TipoItem t)`:**
    * **O que faz:** Verifica se um `TipoItem` é classificado como bebida.
    * **Como faz:** Retorna 1 (verdadeiro) se `t` for `ITEM_REFRIGERANTE`, `ITEM_SUCO` ou `ITEM_MILK_SHAKE`. Caso contrário, retorna 0 (falso).

---

## 2. Gerenciamento da Cozinha

### `void inicializar_cozinha(Cozinha *cozinha)`

* **O que faz:** Configura o estado inicial da estrutura `Cozinha`.
* **Como faz:**
    1.  Usa `memset` para zerar toda a memória da estrutura `cozinha` (contadores vão para 0, ponteiros para `NULL`, `tempo_atual` para 0).
    2.  Imprime mensagens informativas sobre as regras da cozinha (capacidades de equipamento).
    3.  Preenche o array `cozinha->funcionarios` com os 13 funcionários, definindo seus IDs, suas `habilidades` (via *bitmask*) e seu tempo inicial `livre_a_partir_de` (0).
* **Debug (Rastreamento):**
    1.  `Cozinha cozinha;` é declarada na `main` (contém lixo de memória).
    2.  `inicializar_cozinha(&cozinha)` é chamada.
    3.  `memset` zera `cozinha`. `cozinha.tempo_atual` agora é 0. `cozinha.tarefas_na_fila_preparo` é `NULL`.
    4.  `cozinha.funcionarios[0]` é definido como `{1, HABILIDADE_SANDUICHE | HABILIDADE_BATATA, 0}`.
    5.  `cozinha.funcionarios[7]` é definido como `{8, HABILIDADE_BEBIDAS | HABILIDADE_MONTAGEM, 0}`.
    6.  A função retorna, e a `cozinha` está pronta para uso.

### `void adicionar_pedido_na_fila_espera(Cozinha *c, Pedido *novo_pedido)`

* **O que faz:** Adiciona um pedido ao final de uma lista ligada (`pedidos_na_fila_espera`). (Nota: esta função não é utilizada pela lógica principal de "envio imediato" do programa).
* **Como faz:** Percorre a lista ligada `c->pedidos_na_fila_espera` até encontrar o último nó e anexa o `novo_pedido`.

### `void limpar_cozinha(Cozinha *c)`

* **O que faz:** Libera toda a memória alocada dinamicamente (com `malloc` ou `realloc`) durante a simulação para evitar vazamentos de memória.
* **Como faz:**
    1.  Libera (`free`) os arrays dinâmicos de tarefas (`tarefas_em_execucao`, `tarefas_na_fila_preparo`).
    2.  Itera por todos os `pedidos_em_andamento` (se houver algum restante) e libera (`free`) o array `itens` de cada um, e depois o próprio `Pedido`.
    3.  Libera (`free`) o array de ponteiros `pedidos_em_andamento`.
    4.  Faz o mesmo para a lista ligada `pedidos_na_fila_espera` (embora não seja usada).
* **Debug (Rastreamento):**
    1.  A simulação termina. `c->tarefas_na_fila_preparo` aponta para um bloco de memória (ex: 0x3000) e `c->num_pedidos_em_andamento` é 0 (pois todos foram concluídos e liberados durante a simulação).
    2.  `limpar_cozinha(c)` é chamada.
    3.  `free(c->tarefas_em_execucao)` (libera `NULL` ou memória).
    4.  `free(c->tarefas_na_fila_preparo)` (libera a memória em 0x3000).
    5.  O loop `for` de `pedidos_em_andamento` não executa (0 < 0 é falso).
    6.  `free(c->pedidos_em_andamento)` (libera `NULL` ou memória).
    7.  O programa encerra sem vazamentos.

---

## 3. Menu, Impressão e Envio

### `static void imprimir_menu_itens(void)`

* **O que faz:** Exibe o cardápio de itens (1-7) e as opções de controle (0 e -1) no console.
* **Como faz:** Usa uma série de `printf`, buscando os nomes dos itens no array `NOMES_ITENS` usando os valores de `TipoItem` como índice.

### `void imprimir_composicao_bandejas(Pedido *pedido)`

* **O que faz:** Imprime a composição final do pedido, organizando os itens em bandejas segundo a regra "2 comidas + 2 bebidas" por bandeja.
* **Como faz:**
    1.  Aloca dois arrays temporários, `itens_comer` e `itens_beber`.
    2.  Itera pelos `pedido->itens` e usa `item_e_bebida()` para separar os itens nos dois arrays temporários.
    3.  Inicia um loop `while` que continua enquanto houver itens não distribuídos.
    4.  Dentro do loop, imprime "Bandeja X:" e, em seguida, usa dois loops `for` (limitados a 2 iterações cada) para retirar e imprimir até 2 itens de `itens_comer` e até 2 itens de `itens_beber`.
    5.  O loop `while` repete, criando a "Bandeja X+1", até que ambos os arrays temporários tenham sido totalmente processados.
    6.  Libera (`free`) os arrays temporários.
* **Debug (Rastreamento):**
    1.  Pedido #1: 3 Sanduíches (Comer), 1 Suco (Beber).
    2.  `itens_comer` = {Sand, Sand, Sand}, `itens_beber` = {Suco}.
    3.  **Bandeja 1:**
        * Imprime "Bandeja 1:".
        * Loop `for` (comer): Imprime "Sanduiche". Imprime "Sanduiche".
        * Loop `for` (beber): Imprime "Suco".
    4.  **Bandeja 2:**
        * Imprime "Bandeja 2:".
        * Loop `for` (comer): Imprime "Sanduiche".
        * Loop `for` (beber): Não imprime (vazio).
    5.  `while` termina. `free()` é chamado.

### `static void iniciar_pedido_imediato(Cozinha *c, Pedido *p)`

* **O que faz:** Envia um pedido recém-criado (pelo usuário) para a produção imediatamente no tempo atual da simulação.
* **Como faz:**
    1.  Define `p->tempo_chegada = c->tempo_atual` e `p->status = STATUS_EM_PREPARO`.
    2.  Aumenta o array dinâmico `c->pedidos_em_andamento` (usando `safe_realloc_array`) e adiciona o ponteiro `p` a ele.
    3.  Itera por todos os `p->itens`. Para cada item, cria uma `Tarefa` correspondente.
    4.  Adiciona cada nova `Tarefa` ao array dinâmico `c->tarefas_na_fila_preparo` (usando `safe_realloc_array`).
    5.  Chama `despachar_tarefas(c)` para tentar alocar imediatamente essas novas tarefas a funcionários livres.
* **Debug (Rastreamento):**
    1.  `c->tempo_atual` = 0. Usuário finaliza Pedido #1 com 2 itens (Sanduíche, Batata).
    2.  `iniciar_pedido_imediato(c, Pedido#1)` é chamada.
    3.  `Pedido#1->tempo_chegada` = 0.
    4.  `c->pedidos_em_andamento` é realocado e `[0]` agora aponta para Pedido #1.
    5.  Loop de itens:
        * Cria `Tarefa` {ID 1, Sanduíche}. Adiciona a `c->tarefas_na_fila_preparo`.
        * Cria `Tarefa` {ID 1, Batata}. Adiciona a `c->tarefas_na_fila_preparo`.
    6.  `c->tarefas_na_fila_preparo` agora contém 2 tarefas.
    7.  `despachar_tarefas(c)` é chamada. (Ver debug de `despachar_tarefas`).

### `void coletar_pedidos_do_usuario(Cozinha *c)`

* **O que faz:** É o loop de interface principal para o usuário. Permite ao usuário montar pedidos (itens 1-7), finalizar e enviar um pedido (0), ou encerrar totalmente a coleta (-1).
* **Como faz:**
    1.  Inicia um loop `for (;;)` para múltiplos pedidos (`id_pedido_atual`).
    2.  Dentro dele, inicia um loop `for (;;)` para coletar itens de um único pedido (armazenados em `itens_do_pedido`).
    3.  Chama `imprimir_menu_itens()` a cada iteração.
    4.  Lê a `escolha` do usuário com validação (`scanf` + `limpar_buffer_entrada`).
    5.  **Se `escolha == -1` (Encerrar):**
        * Se `qtd_itens > 0` (pedido parcial em andamento), ele primeiro aloca (`malloc`) o `Pedido`, preenche-o e chama `iniciar_pedido_imediato(c)` para enviá-lo.
        * Em seguida, `return` (sai da função `coletar_pedidos_do_usuario`).
    6.  **Se `escolha == 0` (Finalizar pedido):**
        * `break` (sai do loop *interno* de coleta de itens).
    7.  **Se `escolha == 1-7` (Adicionar item):**
        * Pede a `quantidade`.
        * Em um loop `for` (de 0 a `quantidade`), usa `safe_realloc_array` para aumentar o vetor `itens_do_pedido` e adiciona o item (ex: `(TipoItem)(escolha - 1)`).
    8.  **(Após o `break` da escolha 0):**
        * Se `qtd_itens > 0` (pedido válido), aloca (`malloc`) a estrutura `Pedido`, preenche seus campos e chama `iniciar_pedido_imediato(c)` para enviá-lo à produção.
        * Se `qtd_itens == 0`, o pedido é descartado.
    9.  Incrementa `id_pedido_atual` e o loop externo recomeça.
* **Debug (Rastreamento):**
    1.  Início (Pedido #1). `id_pedido_atual` = 1. `itens_do_pedido` = `NULL`. `qtd_itens` = 0.
    2.  Loop interno: `imprimir_menu_itens()`.
    3.  Usuário digita 1 (Sanduíche).
    4.  Pede quantidade. Usuário digita 1.
    5.  `safe_realloc_array` é chamado. `itens_do_pedido` agora tem 1 item. `qtd_itens` = 1.
    6.  Loop interno: `imprimir_menu_itens()`.
    7.  Usuário digita 0 (Finalizar).
    8.  `break` do loop interno.
    9.  Verifica `qtd_itens > 0` (Verdadeiro, 1 > 0).
    10. `malloc(sizeof(Pedido))` -> `novo_pedido`.
    11. `novo_pedido->id = 1`, `novo_pedido->itens = itens_do_pedido`, `novo_pedido->num_itens = 1`.
    12. `iniciar_pedido_imediato(c, novo_pedido)` é chamada. O Pedido #1 é enviado.
    13. `id_pedido_atual` vira 2. Loop externo recomeça (Pedido #2).
    14. Loop interno: `imprimir_menu_itens()`.
    15. Usuário digita -1 (Encerrar).
    16. Verifica `qtd_itens > 0` (Falso, 0 > 0).
    17. Imprime "Registro encerrado."
    18. `return` da função.

---

## 4. Motor da Simulação

### `Pedido *encontrar_pedido_em_andamento(Cozinha *c, int pedido_id)`

* **O que faz:** Busca um ponteiro `Pedido` no array `c->pedidos_em_andamento` usando o `pedido_id`.
* **Como faz:** Itera (loop `for`) pelo array `c->pedidos_em_andamento`. Se encontrar um pedido cujo `id` corresponda, retorna o ponteiro. Se o loop terminar sem encontrar, retorna `NULL`.

### `void despachar_tarefas(Cozinha *c)`

* **O que faz:** O "gerente" da simulação. Aloca tarefas da fila de espera (`tarefas_na_fila_preparo`) para funcionários que estão livres e habilitados.
* **Como faz:**
    1.  Itera pelas 4 Habilidades (Sanduíche, Batata, Bebidas, Montagem).
    2.  **Para cada habilidade:**
        a.  Primeiro, coleta os índices de todos os funcionários que estão livres (`livre_a_partir_de <= c->tempo_atual`) E possuem a habilidade (`& habilidades[h]`).
        b.  Se nenhum funcionário estiver livre/habilitado, `continue` para a próxima habilidade.
        c.  Itera pela `tarefas_na_fila_preparo` (de trás para frente, para permitir remoção segura).
        d.  Verifica se a tarefa corresponde à habilidade sendo processada (ex: `item_e_bebida()` se for `HABILIDADE_BEBIDAS`).
        e.  Se corresponder, aloca a tarefa ao próximo funcionário livre da lista coletada.
        f.  **Alocação:**
            i.  Calcula o `tempo_conclusao` da tarefa (`c->tempo_atual + TEMPOS_DE_PREPARO[...]`).
            ii. Atualiza o funcionário: `c->funcionarios[id_func_alocado].livre_a_partir_de = tarefa_atual->tempo_conclusao`.
            iii. Move a tarefa da fila `preparo` para a fila `execucao` (usando `safe_realloc_array` e *swap-and-pop*).
            iv. Para de alocar tarefas para esta habilidade assim que todos os funcionários coletados (no passo 2a) estiverem ocupados (Regra: 1 tarefa por funcionário por rodada `despachar_tarefas`).
* **Debug (Rastreamento):**
    1.  `c->tempo_atual` = 100.
    2.  Func. #1 (Sanduíche) está livre (Livre em 90).
    3.  Func. #4 (Sanduíche) está ocupado (Livre em 120).
    4.  Fila de Preparo: {Sanduíche(ID 1)}, {Sanduíche(ID 2)}.
    5.  Loop Habilidades (h=0, `HABILIDADE_SANDUICHE`):
    6.  Coleta livres: `funcs_livres` = {0 (índice do Func. #1)}. `num_funcs_livres` = 1.
    7.  `capacidade_total` = 1.
    8.  Itera Fila de Preparo (i=1, Tarefa Sanduíche ID 2):
    9.  Corresponde. `id_func_alocado` = 0.
    10. `tempo_conclusao` = 100 + 58 (Simples) = 158.
    11. `c->funcionarios[0].livre_a_partir_de` = 158.
    12. Tarefa (ID 2) movida para `tarefas_em_execucao`.
    13. `tarefas_alocadas_nesta_rodada` = 1. `break` (capacidade 1 atingida).
    14. Loop Habilidades (h=1, Batata): Nenhuma tarefa/funcionário.
    15. Função termina. A Tarefa (ID 1) permanece na fila de preparo.

### `void executar_simulacao(Cozinha *cozinha)`

* **O que faz:** É o motor principal da simulação, baseado em eventos discretos. Avança o tempo e processa eventos (conclusão de tarefas).
* **Como faz:**
    1.  Inicia um *loop* `while` principal que roda enquanto `total_pedidos_criados > pedidos_concluidos`.
    2.  Inicia um *loop* `do-while (alguma_acao_ocorreu)`. Este *loop* processa todos os eventos que ocorrem *exatamente* no `tempo_atual` (avanço de tempo zero).
    3.  **Dentro do `do-while`:**
        a.  Chama `despachar_tarefas(cozinha)`. Se alguma tarefa foi iniciada, `alguma_acao_ocorreu = 1`.
        b.  Itera (de trás para frente) por `tarefas_em_execucao`.
        c.  Verifica se `tarefa_concluida->tempo_conclusao <= cozinha->tempo_atual`.
        d.  **Se a tarefa terminou:**
            i.  Define `alguma_acao_ocorreu = 1`.
            ii. Encontra o `pedido_pai`.
            iii. **Se for `ITEM_MONTAGEM`:** O pedido está pronto. Calcula o tempo total, atualiza métricas (NO_PRAZO ou ATRASADO), chama `imprimir_composicao_bandejas`, e libera (`free`) a memória do pedido (itens e estrutura), removendo-o de `pedidos_em_andamento`.
            iv. **Se for item de preparo:** Decrementa `pedido_pai->tarefas_preparo_restantes`. Se chegar a 0, cria uma nova `Tarefa` de `ITEM_MONTAGEM` e a adiciona na `tarefas_na_fila_preparo`.
            v. Remove a tarefa concluída de `tarefas_em_execucao` (*swap-and-pop*).
    4.  O `do-while` repete se `alguma_acao_ocorreu` for 1 (pois uma tarefa concluída pode liberar um funcionário que pode pegar uma nova tarefa no *mesmo segundo*).
    5.  **(Fim do `do-while`):** Não há mais ações no `tempo_atual`. É hora de avançar o relógio.
    6.  Encontra o `proximo_tempo_tarefa` (o menor `tempo_conclusao` em `tarefas_em_execucao`).
    7.  Se `proximo_tempo_tarefa` for `INT_MAX` (não há tarefas em execução, mas o `while` principal não terminou), significa um *gargalo*. Imprime um aviso e dá `break`.
    8.  **Avanço de Tempo:** `cozinha->tempo_atual = proximo_tempo_tarefa`.
    9.  O *loop* `while` principal repete, agora no novo `tempo_atual`.
    10. (Após o fim do `while` principal): Imprime o relatório final.
* **Debug (Rastreamento):**
    1.  `tempo_atual` = 0. Fila Preparo: {Sand(ID 1), Bat(ID 1)}. Concluídos = 0.
    2.  `do-while`:
        a.  `despachar_tarefas()`: Aloca Sand(ID 1) (conclui em 58) e Bat(ID 1) (conclui em 190). `alguma_acao_ocorreu` = 1.
        b.  Processar Conclusões (Tempo 0): Nenhuma.
    3.  `do-while` (Repete):
        a.  `despachar_tarefas()`: Nenhuma.
        b.  Processar Conclusões: Nenhuma. `alguma_acao_ocorreu` = 0.
    4.  Avançar Tempo: `proximo_tempo_tarefa` = 58 (o Sanduíche).
    5.  `cozinha->tempo_atual` = 58.
    6.  `do-while`:
        a.  `despachar_tarefas()`: Nenhuma.
        b.  Processar Conclusões (Tempo 58): Sand(ID 1) terminou.
            i.  `alguma_acao_ocorreu` = 1.
            ii. `tarefas_preparo_restantes` do Pedido #1 vira 1.
            iii. Tarefa Sand(ID 1) removida de `tarefas_em_execucao`.
    7.  `do-while` (Repete):
        a.  `despachar_tarefas()`: (Func. do Sanduíche está livre). Nenhuma tarefa na fila.
        b.  Processar Conclusões: Nenhuma. `alguma_acao_ocorreu` = 0.
    8.  Avançar Tempo: `proximo_tempo_tarefa` = 190 (a Batata).
    9.  `cozinha->tempo_atual` = 190.
    10. `do-while`:
        a.  `despachar_tarefas()`: Nenhuma.
        b.  Processar Conclusões (Tempo 190): Bat(ID 1) terminou.
            i.  `alguma_acao_ocorreu` = 1.
            ii. `tarefas_preparo_restantes` do Pedido #1 vira 0.
            iii. Nova tarefa {ID 1, ITEM_MONTAGEM} adicionada a `tarefas_na_fila_preparo`.
            iv. Tarefa Bat(ID 1) removida.
    11. `do-while` (Repete):
        a.  `despachar_tarefas()`: Aloca Montagem(ID 1) (ex: conclui em 190 + 30 = 220). `alguma_acao_ocorreu` = 1.
        b.  Processar Conclusões: Nenhuma.
    12. `do-while` (Repete): Nenhuma ação.
    13. Avançar Tempo: `proximo_tempo_tarefa` = 220.
    14. `cozinha->tempo_atual` = 220.
    15. `do-while`:
        a.  Processar Conclusões: Montagem(ID 1) terminou.
        b.  `alguma_acao_ocorreu` = 1.
        c.  É `ITEM_MONTAGEM`. Tempo = 220 - 0 = 220. (220 <= 300).
        d.  `atendidos_no_prazo` = 1. `imprimir_composicao_bandejas`.
        e.  `free(Pedido 1)`. `num_pedidos_em_andamento` = 0.
    16. `do-while` (Repete): Nenhuma ação.
    17. Avançar Tempo: `proximo_tempo_tarefa` = `INT_MAX`.
    18. `while` principal: `total_pedidos_criados` (1) > `atendidos_no_prazo` (1) é Falso. Loop `while` termina.
    19. Imprime relatório final.

---

## 5. Função Principal

### `int main(void)`

* **O que faz:** Ponto de entrada do programa. Orquestra as três fases: Inicialização, Coleta de Pedidos e Simulação.
* **Como faz:**
    1.  Declara a `Cozinha` na *stack*.
    2.  Chama `inicializar_cozinha(&cozinha)`.
    3.  Chama `coletar_pedidos_do_usuario(&cozinha)`. O programa fica nesta função até o usuário digitar -1.
    4.  Verifica se `cozinha.total_pedidos_criados > 0`.
    5.  Se sim, chama `executar_simulacao(&cozinha)` para processar os pedidos.
    6.  Se não, informa que nenhum pedido foi feito.
    7.  Chama `limpar_cozinha(&cozinha)` para liberar toda a memória alocada.
    8.  Retorna 0.

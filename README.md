# 🏪 Cozinha Big Papão — Simulador de Produção

> Sistema que simula a operação de uma cozinha de fast-food (BigPapão). Objetivo: atender pedidos em até **5 minutos (300s)** para evitar reembolsos.

---

## 🔎 Visão Geral do Sistema

O simulador gerencia pedidos, itens, funcionários com habilidades específicas e equipamentos com capacidade limitada. Ele processa tarefas em paralelo, faz montagem de bandejas e calcula métricas de desempenho (tempo por pedido, taxa de reembolso, utilização de equipamentos, etc.).

---

## 🏗️ Arquitetura do Sistema

### Estruturas de dados principais (resumo)

* **Item**

  * `nome` — nome do produto (ex.: "Sanduíche Médio")
  * `tempo_preparo` — tempo necessário (em segundos)
  * `tipo` — comer / beber
  * `status` — (pendente / em_preparo / pronto)
  * `tempo_restante` — contador enquanto estiver em preparo

* **Pedido**

  * `numero` — id do pedido
  * `itens[]` — vetor/lista de `Item`
  * `status` — (recebido / em_preparo / montagem / pronto)
  * `itens_prontos` — contador
  * `tempo_chegada` / `tempo_entrega`

* **Funcionario**

  * `id` / `nome`
  * `habilidades[]` — ex: {"Sanduiche", "Batata", "Bebida", "Montagem"}
  * `ocupado` — boolean
  * `tarefa_atual` — referência para pedido/item
  * `tempo_restante` — tempo até liberar

* **Fila**

  * Filas separadas por tipo de tarefa: `fila_sanduiches`, `fila_batatas`, `fila_bebidas`, `fila_montagem`
  * Cada fila preserva a ordem de chegada (FIFO)

---

## 🔄 Fluxo de Funcionamento

1. **Entrada de pedidos** — pedidos chegam com lista de itens.

2. **Distribuição para filas especializadas**:

   * Sanduíches → `fila_sanduiches`
   * Batatas → `fila_batatas`
   * Bebidas → `fila_bebidas`

3. **Alocação de funcionários** — procura funcionários LIVRES com a habilidade requerida:

```c
// Exemplo de alocação (pseudo-C)
if (!funcionarios[f].ocupado && tem_habilidade(&funcionarios[f], "Sanduiche")) {
    // Atribui tarefa ao funcionário
}
```

4. **Verificação de limites de equipamentos** — o sistema checa antes de iniciar cada tarefa:

* **Chapa**: máximo **3** sanduíches simultâneos
* **Peneira**: máximo **2** porções de batata simultâneas

5. **Processamento em paralelo** — sistema faz ticks de tempo e decrementa tempos restantes dos funcionários:

```text
TEMPO 0s:
- Funcionário 1: Fazendo Sanduíche Médio (Pedido 1) - 88s restantes
- Funcionário 5: Fritando Batatas (Pedido 1) - 190s restantes
- Funcionário 7: Preparando Suco (Pedido 1) - 38s restantes

TEMPO 40s:
- Funcionário 7: Termina Suco ✅
- Funcionário 7: Começa Refrigerante (Pedido 2) - 5s restantes
```

6. **Montagem da bandeja** — quando *todos* os itens de um pedido estiverem prontos, o pedido vai para montagem (ou é atribuído a um montador):

```c
if (pedido_pronto(&pedidos[p])) {
    // Coloca na fila de montagem ou atribui direto a montador
}
```

* **Tempo de montagem**: 30 segundos por bandeja
* **Regra de bandeja**: máximo **2 itens de comer + 2 itens de beber** por bandeja

7. **Finalização do pedido** — ao final da montagem calcula-se o tempo total (chegada → entrega) e valida-se se ultrapassou 300s (reembolso).

---

## 👥 Sistema de Funcionários (configuração sugerida)

* 5 funcionários sanduíches (2 também fazem batatas, 1 também faz sucos)
* 2 funcionários batatas (1 também faz sanduíches)
* 1 funcionário bebidas (também monta bandeja)
* 1 funcionário só montagem
* 2 funcionários separação (1 também caixa, 1 também sanduíches)
* 2 funcionários caixa (1 também bebidas)

### Alocação inteligente (estratégia)

* Priorizar **pedidos mais antigos** primeiro
* Alocar multi-habilitados para equilibrar filas (ex.: sanduicheiros ajudam nas batatas quando necessário)

Exemplo de lógica de prioridade:

```c
// Percorre pedidos do mais antigo ao mais novo
for (int p = 0; p < num_pedidos; p++) {
    if (pedidos[p].status == 1) { // Pedido em preparo
        for (int i = 0; i < pedidos[p].num_itens; i++) {
            if (!pedidos[p].itens[i].pronto && !pedidos[p].itens[i].em_preparo) {
                // Tenta alocar funcionário para este item
            }
        }
    }
}
```

---

## ⏱️ Simulação de Tempo (loop principal)

* Loop por `tick` (segundos), por exemplo até `400s` ou até todas as filas vazias.
* A cada tick:

  * Decrementa `tempo_restante` de funcionários ocupados
  * Libera funcionários que terminaram suas tarefas
  * Atribui novas tarefas a funcionários livres (respeitando habilidades + limites de equipamento)
  * Verifica pedidos prontos e enfilera montagem
  * Processa fila de montagem

Exemplo do loop:

```c
while (tempo_atual < 400) { // Simula até 400 segundos
    tempo_atual++;
    processar_tarefas(); // Atualiza todo o sistema

    if (tempo_atual % 30 == 0) {
        mostrar_estado_cozinha(); // Mostra painel a cada 30s
    }
}
```

---

## 🎯 Estratégias para minimizar perdas

1. **Priorização por pedido** — foco em completar pedidos inteiros antes de abrir muitos novos
2. **Balanceamento de carga** — usar funcionários multi-habilitados onde houver mais fila
3. **Respeitar limites de capacidade** — evita gargalos e re-trabalhos
4. **Monitoramento contínuo** — painel em tempo real para identificar e mitigar problemas

---

## 📊 Métricas de Performance calculadas

* Tempo total por pedido (chegada → entrega)
* Taxa de reembolso (pedidos > 300s)
* Eficiência dos funcionários (tempo ocupado / tempo total)
* Utilização dos equipamentos (chapa, peneira, etc.)

---

## 🖥️ Interface do Usuário (Painel sugerido)

```text
╔══════════════════════════════════════════════════╗
║                 🏪 COZINHA BIG PAPÃO 🏪         ║
╠══════════════════════════════════════════════════╣
║ Tempo: 150 segundos                              ║
║ 📊 ESTATÍSTICAS:                                 ║
║   Pedidos ativos: 3   Em montagem: 1             ║
║ 👨‍🍳 FUNCIONÁRIOS:                                ║
║  1. Fazendo Sanduiche    Ped 1 - 45s             ║
║  2. Livre               Sanduiche,Batata         ║
║ 📦 PEDIDOS:                                      ║
║  Pedido  1: 👨‍🍳 2/3 itens     | Chegada: 0s      ║
║  Pedido  2: 📦 Em montagem    | Chegada: 60s     ║
╚══════════════════════════════════════════════════╝
```

---

## 🎪 Funcionalidades Especiais

* **Pedidos predefinidos**: 5 cenários de teste com combinações variadas
* **Modo interativo**: criar pedidos personalizados via menu
* **Simulação passo a passo**: avançar de 10 em 10 segundos
* **Simulação completa**: executar até conclusão automática

---

## ✅ Cenários de Teste sugeridos

1. Pedido simples (1 sanduíche) — objetivo: < 120s
2. Pedido misto pequeno (sanduíche + bebida) — objetivo: < 180s
3. Pedido grande (2 sanduíches + batata + bebida) — testar limite de chapa/peneira
4. Picos (vários pedidos simultâneos) — testar balanceamento e reembolsos
5. Caso extremo (equipamentos com menor capacidade) — avaliar fila de montagem

---

## 📂 Como usar (rápido)

1. Copie este conteúdo para um arquivo `README.md` no seu repositório.
2. Use o simulador em modo `dev` para ajustar tempos e número de funcionários.
3. Teste os 5 cenários predefinidos e ajuste parâmetros (capacidades, tempos, número de funcionários).

---

## ✍️ Observações finais

Este README descreve a lógica do simulador e fornece trechos de código/psuedo-c como referência para implementação. Se quiser, posso gerar também um `README.md` pronto para download ou criar o esqueleto do código C completo com as estruturas e funções base.

---

© BigPapão — Simulador de Cozinha

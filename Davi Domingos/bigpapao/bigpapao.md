### Visão geral
- O sistema simula duas cozinhas operando em paralelo lógico, processando pedidos a partir de um único CSV com carimbo de chegada em um relógio de simulação a eventos discretos que avança por saltos para o próximo evento agendado.[2]
- O relógio da simulação usa o mecanismo de “próximo evento”, em que o tempo não incrementa em passos constantes, mas salta do instante do evento atual para o instante do próximo evento ordenado por tempo na lista de eventos.[3]
- O ritmo de execução é desacelerado para visualização, dormindo delta_sim/10 segundos reais antes de cada salto de tempo lógico, mantendo uma taxa de 10 s simulados para 1 s de relógio de parede por meio de nanosleep no POSIX ou Sleep no Windows.[4]

### Modelo de eventos e tempo
- Em DES, cada evento representa uma mudança instantânea de estado, e a simulação mantém uma lista ordenada de eventos futuros, avançando o relógio diretamente para o menor tempo de evento disponível, evitando “ticks” fixos e períodos ociosos.[2]
- A coluna tempo_global no CSV é o timestamp de chegada do pedido desde a abertura e agenda a entrada desse pedido na loja correta naquele instante do relógio lógico, garantindo causalidade e métricas de tempo no sistema consistentes.[2]
- O tempo total de produção por pedido é calculado como tempo_atual_termino − tempo_global_chegada do pedido, o que alinha a métrica ao paradigma de filas e eventos discretos tradicional.[2]

### Formato do CSV
- O arquivo segue CSV simples com header textual e campos separados por vírgula, compatível com RFC 4180 quanto a cabeçalho opcional, número fixo de campos por registro e uso de vírgulas como delimitadores, o que facilita a ingestão com fgets e tokenização.[1]
- Estrutura de colunas: tempo_global, destino, sanduiche_simples, sanduiche_medio, sanduiche_elaborado, batata_frita, refrigerante, suco, milk_shake, mantendo todos os campos de itens como inteiros não negativos por linha.[1]
- Valores que contenham separadores especiais ou quebras de linha devem ser entre aspas conforme a gramática de RFC 4180 se presentes, mas o projeto limita-se a inteiros simples sem necessidade de escape, reduzindo a complexidade de parsing.[1]

### Exemplo de CSV
- Exemplo mínimo para validação funcional:  
```
tempo_global,destino,sanduiche_simples,sanduiche_medio,sanduiche_elaborado,batata_frita,refrigerante,suco,milk_shake
0,1,2,0,1,0,0,0,0
2,2,0,1,0,1,1,0,0
5,1,1,0,0,0,2,0,1
7,2,0,0,0,2,0,1,0
```

- Exemplo expandido para testes mais robustos com chegadas simultâneas e estresse por tipo de recurso, exercitando escalonamento e filas sob cargas variadas em ambas as lojas:  
```
tempo_global,destino,sanduiche_simples,sanduiche_medio,sanduiche_elaborado,batata_frita,refrigerante,suco,milk_shake
0,1,2,0,1,0,0,0,0
0,2,0,1,0,1,0,1,0
1,1,0,0,0,3,0,0,0
2,2,0,0,0,0,3,0,0
3,1,0,2,0,0,0,0,0
3,2,1,0,0,0,0,0,2
5,1,0,0,0,0,0,4,0
7,2,0,0,1,2,0,0,0
10,1,5,0,0,0,0,0,0
12,2,0,0,0,0,0,0,5
15,1,0,0,0,0,2,2,0
20,2,1,1,1,1,1,1,1
25,1,0,0,0,0,0,0,10
25,2,10,0,0,0,0,0,0
30,1,0,0,0,10,0,0,0
35,2,0,0,5,0,0,0,0
40,1,0,0,0,0,0,0,0
45,2,3,2,1,3,2,1,0
50,1,0,1,0,0,5,0,0
55,2,2,2,2,2,2,2,2
```


### Compilação
- Comando recomendado de produção: gcc -std=c11 -Wall -Wextra -Wpedantic -O2 -o bigpapao bigpapao.c, que aplica o padrão C11, habilita avisos amplos e otimiza com segurança sem agressividade excessiva.[5]
- Para depuração, use: gcc -std=c11 -Wall -Wextra -Wpedantic -O0 -g -o bigpapao bigpapao.c, desativando otimizações e incluindo símbolos para uso em depuradores como gdb.[6]
- Em Windows com MinGW, os mesmos parâmetros se aplicam, gerando um executável .exe e mantendo a compatibilidade com a API Sleep exposta por <windows.h> no código.[6]

### Execução
- Execute sempre informando o caminho do CSV como primeiro argumento: ./bigpapao pedidos.csv, o que aciona a leitura exclusiva do arquivo e o agendamento de todas as chegadas definidas por tempo_global no relógio de simulação.[1]
- Caminhos relativos são resolvidos a partir do diretório de trabalho corrente do processo, e caminhos absolutos funcionam independentemente da localização do binário, desde que haja permissão de leitura.[1]

### Logs
- Todas as mensagens são prefixadas com “[Loja X]” e mostram o Tempo da simulação no instante do evento e o Tempo Global do pedido para rastreabilidade, facilitando a auditoria do fluxo de chegada, início de preparo, despacho de tarefas e conclusão com prazo ou atraso.[2]
- O tempo de produção impresso ao finalizar um pedido é a diferença entre o tempo do evento de término e o tempo_global de chegada, que é a métrica clássica de tempo no sistema em DES e modelagem de filas.[2]

### Ritmo 10:1 (simulação vs. tempo real)
- Antes de cada avanço do relógio para o próximo evento, o programa calcula delta_sim e chama nanosleep no POSIX ou Sleep no Windows por delta_sim/10 segundos reais, realizando a desaceleração desejada sem abandonar o avanço por próximo evento.[4]
- nanosleep provê resolução de nanossegundos e não interage com sinais de forma problemática conforme POSIX, sendo preferível a sleep/usleep em portabilidade moderna para temporizações proporcionais a deltas de simulação.[4]

### Estrutura do código
- Núcleos principais: inicialização de cozinhas, leitura do CSV com criação dos pedidos por loja, loop de simulação conjunta por próximo evento, despacho de tarefas por habilidades e remoção/fechamento de pedidos ao concluir montagem, com atualização de métricas.[2]
- As estruturas incluem Pedido, Tarefa e Cozinha, além de arrays de Funcionarios parametrizados por habilidades, e filas internas para pedidos na espera, em andamento, tarefas a despachar e tarefas em execução.[2]

### CSV e robustez de parsing
- A leitura adota header textual opcional e tokenização por vírgulas com validação de inteiros, seguindo a expectativa de número fixo de campos por linha e sem necessidade de escapes ao operar com dados numéricos simples.[1]
- Adoção de cabeçalhos explícitos torna o CSV autodescritivo e alinhado com padrões de publicação de dados tabulares recomendando RFC 4180 para interoperabilidade e legibilidade por ferramentas genéricas.[7]

### Portabilidade
- Em sistemas POSIX, o sono proporcional usa nanosleep declarado em <time.h>, que oferece maior precisão e comportamento padronizado para suspender a thread pelo intervalo relativo desejado entre eventos.[4]
- Em Windows, a função Sleep opera em milissegundos e integra-se facilmente a binários gerados por MinGW, oferecendo uma aproximação adequada para ritmos modestos como 10:1 sem requisitos adicionais de bibliotecas.[8]

### Tratamento de erros
- O parser ignora linhas com contagem incorreta de colunas ou valores não numéricos e reporta avisos, preservando a execução com registros válidos restantes conforme orientação de robustez para CSV com header claro e campos numéricos.[1]
- Alocações dinâmicas usam um helper de realloc seguro com checagem de overflow em count*size, retornando erro e abortando a execução com mensagens quando não há memória suficiente, reduzindo riscos de corrupção.[2]

### Limitações e extensões
- O modelo atual assume capacidades e tempos fixos por tipo de item e não modela interrupções, reprocessos ou prioridades, que podem ser incorporados com novas regras de escalonamento e estados adicionais na lista de eventos.[3]
- Extensões como relatórios agregados, exportação de métricas em CSV e visualização temporal podem ser adicionadas sem alterar o núcleo do motor de próximo evento que já estrutura o tempo e a causalidade corretamente.[2]

Referências
- RFC 4180: formato comum e MIME type text/csv, com cabeçalho opcional e gramática de campos.[1]
- nanosleep: suspensão com precisão de nanossegundos para ritmos proporcionais ao delta de simulação.[4]
- Relógio de simulação a eventos discretos e avanço por próximo evento.[2]


[20](https://sites.uclouvain.be/SystInfo/manpages/man3/clock_nanosleep.3posix.html)
[21](https://www.haskoning.com/en/twinn/blogs/2024/what-is-discrete-event-simulation-and-how-does-it-work)
[22](https://help.gooddata.com/classic/en/data-integration/data-preparation-and-distribution/data-preparation-and-distribution-pipeline/data-pipeline-reference/data-warehouse-reference/how-to-set-up-a-connection-to-data-warehouse/connecting-to-data-warehouse-from-cloudconnect/loading-data-through-cloudconnect-to-data-warehouse/use-rfc-4180-compliant-csv-files-for-upload/)

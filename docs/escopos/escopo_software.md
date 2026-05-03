# Escopo – Subgrupo de Software

## Frentes de Trabalho

### Software Embarcado

- Leitura e interpretação dos sensores de distância (detectar paredes nas células adjacentes);
- Monitoramento da posição atual no labirinto (odometria e controle de orientação);
- Construção incremental do mapa do labirinto em tempo de execução;
- Tomada de decisão de rota com algoritmo de exploração (Flood Fill ou similar);
- Detecção da chegada na área objetivo (sala central do labirinto);
- Transmissão dos dados de telemetria para o sistema web via comunicação sem fio.

### Sistema Web de Telemetria

- Receber dados em tempo real via WebSocket ou protocolo MQTT;
- Exibir em tempo real os seguintes dados de telemetria:

    - Tipo do labirinto (4×4, 8×8 ou 16×16);
    - Trajeto percorrido, visualizado sobre o mapa do labirinto;
    - Consumo de bateria;
    - Velocidade média;
    - Tempo de conclusão;
    - Desafio cumprido (Sim / Não).

- Persistir os dados finais em banco de dados ao término de cada corrida;
- Permitir consultas históricas por labirinto individual ou por todos os labirintos;
- Exibir os dados armazenados em tela de histórico com filtros.

### Entregáveis por Marco

| Entregável | Descrição | Marco |
|---|---|---|
| Algoritmo de exploração (embarcado) | Código funcional de navegação com Flood Fill | PC2 |
| Módulo de sensoriamento | Leitura e interpretação dos sensores de distância | PC2 |
| Módulo de odometria | Monitoramento de posição e orientação do robô | PC2 |
| Transmissor de telemetria | Envio dos dados do embarcado para o servidor web | PC2 |
| Backend da aplicação web | API que recebe, processa e persiste os dados | PC2 |
| Frontend de telemetria | Interface web com dados ao vivo durante a corrida | IPP |
| Banco de dados e histórico | Persistência e consulta dos dados finais | IPP |
| Sistema integrado e testado | Todos os módulos integrados nos três labirintos | APT |
| Documentação técnica (UML) | Diagramas de casos de uso, classes, sequência e arquitetura | Por entrega |

### Fora do Escopo

Os itens abaixo não são responsabilidade do subgrupo de Software:

- Projeto e montagem da estrutura física do micromouse (Subgrupo de Estrutura);
- Seleção, aquisição e conexão dos componentes eletrônicos (Subgrupo de Hardware);
- Cálculo e gerenciamento do consumo energético e da bateria (Subgrupo de Energia);
- Construção física do labirinto de testes 4×4 (responsabilidade coletiva do grupo).

### Premissas e Dependências

- O microcontrolador escolhido pelo Subgrupo de Hardware suporta comunicação sem fio e permite programação em Python;
- O Subgrupo de Hardware fornecerá interface de acesso aos sensores e encoders com documentação mínima;
- A apresentação final ocorrerá em ambiente com acesso a rede local (Wi-Fi);
- O grupo terá acesso a pelo menos um labirinto 4×4 para testes de integração antes do PC2.

### Restrições

- Nenhuma solução de software pronta pode substituir o desenvolvimento próprio;
- Ferramentas de inteligência artificial generativa não podem ser usadas nas atividades avaliativas;
- O código-fonte e a memória do labirinto não podem ser alterados durante a resolução de um trajeto;
- Cada issue no GitHub deve ter no máximo dois responsáveis.

### Critérios de Aceitação

| Critério | Como será verificado |
|---|---|
| Micromouse navega autonomamente no labirinto 4×4 | Demonstração na Inspeção Prévia (IPP) |
| Micromouse navega nos labirintos 8×8 e 16×16 | Demonstração na Apresentação Final (APT) |
| Dados de telemetria aparecem em tempo real no site | Observação direta durante a demonstração |
| Dados são armazenados no banco após cada corrida | Consulta na interface de histórico |
| Interface permite filtrar por labirinto ou por todos | Teste funcional na tela de histórico |
| Diagramas UML coerentes com o sistema implementado | Revisão no relatório técnico por Ponto de Controle |
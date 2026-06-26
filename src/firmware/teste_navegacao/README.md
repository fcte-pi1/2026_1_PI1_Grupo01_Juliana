# Teste de Navegação para Robô Micromouse com ESP32

Projeto desenvolvido em **ESP-IDF** para validar a integração dos principais módulos de um robô Micromouse, incluindo controle dos motores, odometria, encoders, sensores infravermelhos e interface de usuário por botão e buzzer.

---

## Objetivo

Este projeto tem como finalidade testar a integração entre os módulos responsáveis pela navegação do robô antes da implementação do algoritmo completo de exploração do labirinto.

Atualmente o firmware realiza testes de:

* Inicialização do hardware;
* Controle dos motores;
* Leitura dos encoders;
* Atualização da odometria;
* Inicialização dos sensores infravermelhos;
* Seleção do tipo de labirinto através de botão externo;
* Reprodução de sinais sonoros utilizando buzzer;
* Comunicação entre interrupções e tarefas do FreeRTOS utilizando Task Notifications.

O suporte ao monitoramento de potência utilizando o INA226 já está parcialmente implementado, porém permanece comentado durante esta etapa de desenvolvimento.

---

# Hardware utilizado

* ESP32
* Driver de motores DRV8833
* 2 motores DC com encoder óptico
* 4 sensores infravermelhos
* Buzzer passivo
* Botão de seleção
* (Opcional) INA226 para monitoramento de corrente e tensão

---

# Estrutura do projeto

Os principais módulos utilizados são:

```
main/
│
├── main.c
├── m_driver
├── movimentacao
├── encoder
├── odometria
├── infrared
└── power_module (em desenvolvimento)
```

### Descrição dos módulos

| Módulo         | Função                                                 |
| -------------- | ------------------------------------------------------ |
| `m_driver`     | Controle dos motores via MCPWM                         |
| `encoder`      | Leitura dos encoders utilizando PCNT                   |
| `movimentacao` | Movimentos básicos do robô                             |
| `odometria`    | Atualização da posição estimada                        |
| `infrared`     | Leitura dos sensores IR e tratamento de interrupções   |
| `power_module` | Interface para o INA226 (temporariamente desabilitada) |

---

# Funcionamento

Durante a inicialização o firmware executa:

1. Configuração do botão de seleção;
2. Configuração do buzzer;
3. Inicialização dos encoders;
4. Inicialização do driver dos motores;
5. Inicialização dos sensores infravermelhos;
6. Inicialização da odometria;
7. Criação da tarefa responsável pelo tratamento do botão.

Após inicializado, o firmware realiza testes periódicos de movimentação enquanto permanece apto a responder ao botão externo.

## Comportamento Esperado
O micromouse deverá realizar os bips de inicialização e, após alguns segundos, começar a avançar o equivalente a uma célula do labirinto (aprox. 18cm - 2,4cm). Ele deverá repetir esse comportamento até que os sensores frontais acusem algum obstáculo. Nesse momento, ele realizará a rotina de interrupção que deverá alterar a trajetória do robô de maneira adequada.

---

# Seleção do labirinto

O botão conectado ao GPIO configurado como entrada com interrupção permite alternar entre os modos de operação.

Tipos disponíveis:

* 4×4
* 8×8

Cada seleção é indicada por uma sequência sonora diferente reproduzida pelo buzzer.

---

# Interrupções

O projeto utiliza interrupções de GPIO para:

* Botão de seleção
* Sensores infravermelhos

As ISRs executam apenas o código mínimo necessário e sinalizam tarefas do FreeRTOS através de **Task Notifications**, reduzindo o tempo gasto em contexto de interrupção.

---

# Recursos utilizados da ESP-IDF

* FreeRTOS
* GPIO
* LEDC
* MCPWM
* PCNT
* Task Notifications
* ISR Service

---

# Funcionalidades implementadas

* [x] Controle dos motores
* [x] Leitura dos encoders
* [x] Odometria básica
* [x] Sensores infravermelhos
* [x] Buzzer
* [x] Botão com interrupção
* [x] Comunicação ISR → Task
* [ ] Integração completa do INA226
* [ ] Algoritmo de exploração do labirinto
* [ ] Planejamento de trajetória
* [ ] Resolução automática do labirinto

---

# Compilação

Projeto desenvolvido utilizando **ESP-IDF**.

```
idf.py build
idf.py flash
idf.py monitor
```

---

# Observações

Este repositório representa uma etapa intermediária do desenvolvimento do robô, focada principalmente na integração entre hardware e software. A estrutura do código está sendo organizada em módulos independentes para facilitar manutenção, reutilização e expansão das funcionalidades.

O código continuará evoluindo com a implementação do algoritmo de navegação e da estratégia completa para competições de Micromouse.


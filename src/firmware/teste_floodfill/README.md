# teste_floodfill

Teste embarcado do algoritmo Flood Fill com exploração incremental (issue #13).

O robô não conhece o labirinto: começa com o mapa de paredes vazio, lê os
sensores IR a cada célula, recalcula o Flood Fill sobre o mapa conhecido e anda
para o vizinho acessível de menor distância, até parar na área central 2x2.

Usa as primitivas já validadas no carrinho (`movimentacao_move_cell`,
`movimentacao_turn_clws`, `movimentacao_turn_ctclws`).

## Como rodar

```bash
cd src/firmware/teste_floodfill
idf.py build
idf.py flash monitor
```

## Antes de gravar, conferir em `main/teste_floodfill.c` (bloco CONFIGURACAO)

- `LADO` — tamanho do labirinto (4 para o 4x4 de teste, 8 para o 8x8).
- `LINHA_INICIAL` / `COLUNA_INICIAL` / `ORIENT_INICIAL` — célula e orientação
  onde o robô é posicionado (padrão: canto (0,0) virado para o NORTE, que aqui
  aponta para dentro do labirinto).
- `IR_NIVEL_PAREDE` — nível digital do sensor quando há parede. Padrão `0`
  (saída vai para LOW ao detectar). **Se as paredes vierem invertidas, troque
  para `1`.**
- `SENSOR_FRENTE` / `SENSOR_ESQUERDA` / `SENSOR_DIREITA` — quais pinos IR usar.

## Calibração em campo

A cada célula o monitor imprime a leitura crua dos 5 sensores
(`FRONT/FL/FR/L/R`), a interpretação (parede frente/esq/dir) e a decisão
tomada. Use isso para ajustar `IR_NIVEL_PAREDE` e conferir se os sensores
laterais estão lendo as paredes corretamente antes de confiar na navegação.

## Limitações conhecidas (desta versão de teste)

- Só faz a ida até o centro (não faz corrida de volta nem speed run).
- Front é lido só pelo `IR_FRONT`; se ele não estiver instalado, ajustar
  `SENSOR_FRENTE` para combinar `IR_FL`/`IR_FR`.
- O tamanho do labirinto é fixo por `#define`. A integração com o seletor de
  modo (botão + buzzer da branch `feature-hw-selecao-labirinto`) entra depois,
  trocando o `#define LADO` por `seletor_modo_lado()`.

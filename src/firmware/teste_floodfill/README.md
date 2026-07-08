# teste_floodfill

Teste embarcado do algoritmo **Flood Fill** com exploração incremental (issue #13),
portado para rodar sobre a **base de navegação atual** (o mesmo `movimentacao`
usado pelo `teste_navegacao`, com o PD da reta e a curva de 90° já ajustados).

O robô não conhece o labirinto: começa com o mapa de paredes vazio, lê os
sensores IR a cada célula, recalcula o Flood Fill sobre o mapa conhecido e anda
para o vizinho acessível de menor distância, até parar na área central 2x2.

## Como rodar

```bash
cd src/firmware/teste_floodfill
idf.py build
idf.py flash monitor
```

## Antes de gravar, conferir no bloco CONFIGURAÇÃO de `main/teste_floodfill.c`

- `LADO` — tamanho do labirinto (4 para o 4x4 de teste, 8 para o 8x8).
- `LINHA_INICIAL` / `COLUNA_INICIAL` / `ORIENT_INICIAL` — célula e orientação
  onde o robô é posicionado (padrão: canto (0,0) virado para o NORTE, que aqui
  aponta para dentro do labirinto).
- `IR_NIVEL_PAREDE` — nível digital do sensor quando há parede. Padrão `0`
  (saída vai para LOW ao detectar). **Se as paredes vierem invertidas, troque
  para `1`.**
- `USA_SENSOR_FRONTAL` — padrão `0`. Como o sensor frontal dedicado (GPIO 34)
  está comentado na `infrared.h` da base (provavelmente não instalado), a parede
  da frente é detectada pelos dois diagonais **FL e FR juntos**. Se você instalar
  e confiar no frontal, ponha `1`.

## Diferenças em relação à versão da branch `feature/13-floodfill-embarcado`

- Usa a `movimentacao` **da base atual**: `movimentacao_move_cell` agora devolve
  `bool` + `status`. Se o robô **não** conseguir avançar a célula (parede que o
  sensor não pegou, roda presa), o código **marca a parede à frente** e o Flood
  Fill recalcula — a posição `(linha, coluna)` nunca dessincroniza do mapa.
- Não usa o componente `navigation` (task reativa por interrupção): a leitura
  dos sensores é **síncrona** (sente → decide → anda), que é o fluxo natural do
  Flood Fill.
- Detecção de frente por padrão via diagonais (ver `USA_SENSOR_FRONTAL`).

## Calibração em campo

A cada célula o monitor imprime a leitura crua dos 5 sensores
(`FRONT/FL/FR/L/R`), a interpretação (parede frente/esq/dir) e a decisão
tomada. Use isso para ajustar `IR_NIVEL_PAREDE` e conferir se os sensores
laterais estão lendo as paredes corretamente antes de confiar na navegação.

O ponto mais sensível é a **precisão do giro de 90°**: o encoder tem resolução
grossa (~5,6°/pulso), então erros de giro acumulam ao longo de vários passos.
Se o robô for perdendo o alinhamento, ajuste `THETA_CURVA_MARGIN` e os limites
`PWM_CURVA_MIN/MAX` em `components/movimentacao/movimentacao.c`.

## Limitações conhecidas (desta versão de teste)

- Só faz a ida até o centro (não faz corrida de volta nem speed run).
- O tamanho do labirinto é fixo por `#define LADO`. A integração com o seletor
  de modo (botão + buzzer) pode trocar o `#define` por uma leitura em runtime
  depois.

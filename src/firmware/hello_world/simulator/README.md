# Flood Fill Simulator

Simulador em C do algoritmo Flood Fill utilizado no projeto Micromouse.  
O objetivo do programa é validar a lógica de navegação autônoma antes da integração com o firmware embarcado no ESP32.

O simulador gera labirintos aleatórios e executa uma simulação visual do robô se deslocando até a área objetivo central utilizando Flood Fill.

---

# Objetivos do Simulador

- Validar o algoritmo Flood Fill
- Testar navegação autônoma
- Simular labirintos de diferentes tamanhos
- Servir como base para futura implementação embarcada
- Auxiliar no desenvolvimento do software do Micromouse

---

# Funcionalidades

- Geração aleatória de labirintos utilizando DFS (Depth-First Search)
- Suporte para labirintos:
  - 4x4
  - 8x8
  - 16x16
- Área objetivo central 2x2
- Navegação automática até o centro
- Visualização ASCII em tempo real no terminal

---

# Como Rodar o Simulador

#### 1. Entrar na pasta do simulador

```bash
cd src/firmware/hello_world/simulator
```

#### 2. Compilar código

```bash
gcc flood_fill.c -o flood_fill
```

#### 3. Executar o código

```bash
./flood_fill
```

---

# Representação Visual

Durante a execução:

- `R` representa o robô
- `X` representa a área objetivo central

Exemplo:

```text
+---+---+---+---+
|           |   |
+---+---+   +   +
|     X   R     |
+   +   +   +   +
|   | X   X |   |
+   +---+   +   +
|       |       |
+---+---+---+---+

# Documentação do Frontend - Sistema de Telemetria

O frontend do Micromouse foi projetado para ser uma Single Page Application (SPA) responsiva, permitindo que o operador monitore o robô em tempo real e consulte o desempenho histórico das corridas.

## Objetivos do Frontend
- **Visualização em Tempo Real (HU03):** Dashboard com telemetria viva (posição, velocidade, bateria).
- **Mapeamento Incremental (HU02):** Representação visual do labirinto conforme o robô explora.
- **Consulta Histórica (HU04):** Interface para filtrar e analisar resultados de corridas passadas.

---

# Tecnologias Utilizadas

| Camada | Tecnologia |
|---|---|
| Framework | React (Vite) |
| Roteamento | React Router DOM |
| Estilização | CSS / Inline Styles |

---

# Arquitetura de Rotas

O roteamento é gerenciado pelo `react-router-dom`, utilizando um layout base para navegação persistente.

| Rota | Componente | Descrição |
|---|---|---|
| `/` | `Telemetry.jsx` | Dashboard de telemetria ao vivo e mapa. |
| `/historico` | `History.jsx` | Tabela de resultados e filtros por labirinto. |

---

# Estrutura de Componentes

### 1. `Layout` (Base)
Componente pai que contém o **Header** e a **Navegação**. Ele utiliza o `<Outlet />` para renderizar as páginas específicas.

### 2. `Telemetry` (Página)
Consome dados em tempo real. Exibe:
- Status da bateria (HU05).
- Cronômetro de corrida.
- Velocidade e trajetória.
- Componente `MazeMap`.

### 3. `History` (Página)
Interface de consulta ao banco de dados (SQLite via FastAPI).
- Filtro por tamanho de labirinto (4x4, 8x8, 16x16).
- Indicador de "Desafio Cumprido" (HU06).

### 4. `MazeMap` (Componente Reutilizável)
Responsável por desenhar a grade do labirinto e a posição atual (X, Y) do Micromouse. No momento, funciona como um placeholder para a lógica de grid.

---

# Estrutura de Arquivos

```text
src/frontend/
├── public/              # Arquivos estáticos (ícones, logos)
├── src/
│   ├── components/      # Componentes menores e reutilizáveis
│   │   ├── Layout.jsx
│   │   └── MazeMap.jsx
│   ├── pages/           # Páginas principais (rotas)
│   │   ├── Telemetry.jsx
│   │   └── History.jsx
│   ├── App.jsx          # Configuração de rotas e provedores
│   └── main.jsx         # Ponto de entrada do React
├── index.html           # Template HTML base
├── package.json         # Dependências (React, Router, Vite)
└── vite.config.js       # Configurações de build
```

---

# Como Rodar

Para executar o painel de telemetria localmente, certifique-se de ter o **Node.js** instalado na sua máquina.

## 1. Navegar até a pasta do frontend
Abra o terminal na raiz do projeto e entre na pasta correspondente:
```bash
cd src/frontend
```

## 2. Instalar as dependências
Este comando baixa todos os pacotes necessários (React, Vite, React Router, etc) listados no `package.json`:

```bash
npm install
```

## 3. Executar em modo de desenvolvimento
Inicie o servidor local para visualizar as alterações em tempo real:

```bash
npm run dev
```

## 4. Acessar a aplicação
Após o servidor iniciar, aparecerá a URL [http://localhost:5173](http://localhost:5173). Acesse o link apertando crtl+click nessa URL.
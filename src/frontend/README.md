# React + Vite

This template provides a minimal setup to get React working in Vite with HMR and some ESLint rules.

Currently, two official plugins are available:

- [@vitejs/plugin-react](https://github.com/vitejs/vite-plugin-react/blob/main/packages/plugin-react) uses [Oxc](https://oxc.rs)
- [@vitejs/plugin-react-swc](https://github.com/vitejs/vite-plugin-react/blob/main/packages/plugin-react-swc) uses [SWC](https://swc.rs/)

## React Compiler

The React Compiler is not enabled on this template because of its impact on dev & build performances. To add it, see [this documentation](https://react.dev/learn/react-compiler/installation).

## Expanding the ESLint configuration

If you are developing a production application, we recommend using TypeScript with type-aware lint rules enabled. Check out the [TS template](https://github.com/vitejs/vite/tree/main/packages/create-vite/template-react-ts) for information on how to integrate TypeScript and [`typescript-eslint`](https://typescript-eslint.io) in your project.

## ✅ Testes automatizados (Vitest + Testing Library)

> 🤖 **Precisa do robô / ESP32 / backend rodando para os testes? NÃO.**
> Os testes do frontend rodam **100% sem hardware e sem backend**. Eles usam o
> **jsdom** (um DOM em memória, sem navegador) e, quando uma página precisar de
> dados da API ou de WebSocket, isso é **mockado** dentro do próprio teste. Você
> só precisa do **Node + as dependências do `package.json`**.

### Como rodar

```bash
# dentro de src/frontend
npm install            # instala as dependências (inclui Vitest e Testing Library)
npm test               # roda todos os testes uma vez
npm run test:watch     # modo interativo (re-roda ao salvar)
npm run test:coverage  # roda com relatório de cobertura
```

O relatório HTML de cobertura é gerado em `coverage/index.html`. A meta da
entrega é **cobertura ≥ 70%**.

### Onde ficam os testes

- Os testes ficam **ao lado do arquivo testado**, com sufixo `.test.jsx`
  (ex.: `src/App.test.jsx`, futuramente `src/pages/History.test.jsx`).
- O setup global está em `src/test/setup.js` (matchers do jest-dom + limpeza do
  DOM entre testes). `describe/it/expect` são **globais** — não precisa importar.

### Padrão para novos testes

Use `render` + `screen` do Testing Library e consulte por papel/texto (como o
usuário enxerga). Veja o molde em `src/App.test.jsx`:

```jsx
import { render, screen } from '@testing-library/react'
import History from './pages/History'

it('exemplo', () => {
  render(<History />)
  expect(screen.getByText('Histórico de Corridas')).toBeInTheDocument()
})
```

> O smoke test atual cobre o `App` (navegação + página inicial). As suítes
> completas das páginas e componentes são as Tarefas T10–T12.

## 🎭 Testes E2E / sistema (Playwright)

Diferente dos testes acima (que rodam em `jsdom`), os testes **E2E** abrem a
aplicação em um **navegador real** (Chromium headless) e validam os fluxos do
ponto de vista do usuário — são os testes de sistema/funcional exigidos na
seção 7.5 do relatório.

### Como rodar

```bash
# dentro de src/frontend
npm install                  # instala dependências (inclui @playwright/test)
npx playwright install chromium   # baixa o navegador (só na primeira vez)
npm run e2e                  # roda os testes E2E (headless)
npm run e2e:ui               # modo interativo (UI do Playwright)
npm run e2e:report           # abre o último relatório HTML
```

Não precisa subir o frontend manualmente: o `playwright.config.js` tem um
`webServer` que executa `npm run dev` automaticamente antes dos testes.

### Onde ficam os testes

- Os specs E2E ficam em **`e2e/`** com sufixo `.spec.js` (ex.: `e2e/smoke.spec.js`).
  Eles **não** são executados pelo Vitest (o Vitest está restrito a `src/`).
- Configuração em `playwright.config.js`.

### ⚠️ E2E que dependem do backend (Tarefas T13–T15)

O `webServer` padrão sobe **apenas o frontend** — suficiente para o smoke test e
fluxos de navegação. Os E2E que precisam de **dados reais da API** (telemetria,
histórico) devem subir também o backend. Para isso, transforme o `webServer` em
uma lista no `playwright.config.js`:

```js
webServer: [
  { command: 'npm run dev', url: 'http://localhost:5173', reuseExistingServer: !process.env.CI },
  {
    // backend: ajuste o caminho do uvicorn ao seu ambiente (venv)
    command: 'cd ../backend && venv/bin/python -m uvicorn app.main:app --port 8000',
    url: 'http://localhost:8000/health',
    reuseExistingServer: !process.env.CI,
  },
],
```

> Os fluxos E2E devem seguir o **roteiro de testes funcionais** (Tarefa T2).
> O molde inicial está em `e2e/smoke.spec.js`.

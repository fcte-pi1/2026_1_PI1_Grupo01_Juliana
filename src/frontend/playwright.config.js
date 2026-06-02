import { defineConfig, devices } from '@playwright/test'

// Configuração dos testes E2E (sistema/funcional) da aplicação web.
// Os testes ficam em ./e2e e rodam contra o frontend servido pelo Vite.
//
// O `webServer` abaixo sobe apenas o FRONTEND, o suficiente para o smoke test
// e para os fluxos de UI. Os testes E2E que dependem de dados reais do backend
// (Tarefas T13–T15) devem subir também a API — veja a nota no README
// (seção "Testes E2E") sobre como adicionar o backend ao `webServer`.
export default defineConfig({
  testDir: './e2e',
  fullyParallel: true,
  forbidOnly: !!process.env.CI,
  retries: process.env.CI ? 2 : 0,
  reporter: 'html',
  use: {
    baseURL: 'http://localhost:5173',
    trace: 'on-first-retry',
  },
  projects: [
    { name: 'chromium', use: { ...devices['Desktop Chrome'] } },
  ],
  webServer: {
    command: 'npm run dev',
    url: 'http://localhost:5173',
    reuseExistingServer: !process.env.CI,
    timeout: 120 * 1000,
  },
})

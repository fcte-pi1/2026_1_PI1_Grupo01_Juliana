import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'

// https://vite.dev/config/
export default defineConfig({
  plugins: [react()],
  test: {
    // Ambiente de DOM para testar componentes React sem navegador real.
    environment: 'jsdom',
    // Expõe describe/it/expect globalmente, sem precisar importar em cada teste.
    globals: true,
    // Carrega matchers do jest-dom (toBeInTheDocument etc.) antes dos testes.
    setupFiles: './src/test/setup.js',
    // Restringe o Vitest aos testes em src/; os specs E2E (Playwright) ficam em e2e/.
    include: ['src/**/*.{test,spec}.{js,jsx}'],
    css: true,
    coverage: {
      provider: 'v8',
      reporter: ['text', 'html'],
      // Mede cobertura do código de aplicação, ignorando bootstrap e os testes.
      include: ['src/**/*.{js,jsx}'],
      exclude: ['src/main.jsx', 'src/test/**', '**/*.test.{js,jsx}'],
    },
  },
})

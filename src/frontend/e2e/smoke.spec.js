import { test, expect } from '@playwright/test'

// Teste-fumaça (smoke test) E2E: garante que a aplicação carrega no navegador
// e que a navegação principal funciona. Serve de molde para as Tarefas
// T13–T15 (fluxos de telemetria, histórico e reconexão).

test('a aplicação carrega e exibe a navegação principal', async ({ page }) => {
  await page.goto('/')

  await expect(page.getByRole('heading', { name: 'Micromouse Telemetria' })).toBeVisible()
  await expect(page.getByRole('link', { name: 'Telemetria ao Vivo' })).toBeVisible()
  await expect(page.getByRole('link', { name: 'Histórico' })).toBeVisible()
})

test('navega da telemetria para o histórico pela barra de navegação', async ({ page }) => {
  await page.goto('/')

  await expect(page.getByRole('heading', { name: 'Painel de Controle' })).toBeVisible()

  await page.getByRole('link', { name: 'Histórico' }).click()

  await expect(page).toHaveURL(/\/historico$/)
  await expect(page.getByRole('heading', { name: 'Histórico de Corridas' })).toBeVisible()
})

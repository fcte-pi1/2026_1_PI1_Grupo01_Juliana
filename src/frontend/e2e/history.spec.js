import { test, expect } from '@playwright/test'

// E2E (Tarefa T14, issue #131): consulta de histórico de corridas e aplicação
// de filtros por tamanho de labirinto.
//
// Em vez de subir o backend, interceptamos as chamadas a GET /corridas via
// page.route() e devolvemos dados controlados. O mock replica o comportamento
// real da API: filtra pelo query param `tamanho` quando presente e devolve
// todas as corridas quando ausente — exatamente como useHistory.js consome.

const CORRIDAS = [
  { id: 1, tamanho: '4x4',   duracao: 45,  resultado: 'concluida'    },
  { id: 2, tamanho: '8x8',   duracao: 120, resultado: 'falha'        },
  { id: 3, tamanho: '4x4',   duracao: 33,  resultado: 'concluida'    },
  { id: 4, tamanho: '16x16', duracao: 310, resultado: 'concluida' },
  { id: 5, tamanho: '8x8',   duracao: 98,  resultado: 'em_andamento'    },
]

test.beforeEach(async ({ page }) => {
  // Semeia os dados de corridas interceptando o endpoint antes de navegar.
  await page.route('**/corridas**', (route) => {
    const url    = new URL(route.request().url())
    const filtro = url.searchParams.get('tamanho')
    const dados  = filtro
      ? CORRIDAS.filter((c) => c.tamanho === filtro)
      : CORRIDAS

    route.fulfill({
      status:      200,
      contentType: 'application/json',
      body:        JSON.stringify(dados),
    })
  })

  await page.goto('/historico')

  // Aguarda a página terminar de carregar (tabela ou mensagem de vazio visível)
  await expect(
    page.getByRole('table').or(page.locator('p').filter({ hasText: /nenhuma corrida/i }))
  ).toBeVisible({ timeout: 8_000 })
})

test('exibe todas as corridas semeadas ao abrir a página de histórico', async ({ page }) => {
  await expect(page.getByRole('heading', { name: 'Histórico de Corridas' })).toBeVisible()

  // Filtro deve iniciar em "todos"
  await expect(page.locator('#filtro-tamanho')).toHaveValue('todos')

  // Todas as 5 corridas devem aparecer na tabela (linhas de dados, sem cabeçalho)
  const linhas = page.getByRole('row').filter({ hasNot: page.getByRole('columnheader') })
  await expect(linhas).toHaveCount(5)

  // Contador de rodapé deve refletir o total
  await expect(page.locator('p').filter({ hasText: /corrida.*encontrada/i }))
    .toContainText('5 corridas encontradas')
})

test('filtro por labirinto exibe apenas corridas do tamanho selecionado', async ({ page }) => {
  await page.locator('#filtro-tamanho').selectOption('4x4')

  // Apenas as corridas de ID 1 e 3 devem aparecer
  const linhas = page.getByRole('row').filter({ hasNot: page.getByRole('columnheader') })
  await expect(linhas).toHaveCount(2)

  // Nenhuma célula de outro tamanho pode estar visível
  await expect(page.getByRole('cell', { name: /^(8x8|16x16)$/ })).toHaveCount(0)

  // Contador indica o filtro ativo
  await expect(page.locator('p').filter({ hasText: /corrida.*encontrada/i }))
    .toContainText('2 corridas encontradas para 4x4')
})

test('selecionar "todos" após filtrar restaura o histórico completo', async ({ page }) => {
  // Aplica um filtro para reduzir a lista
  await page.locator('#filtro-tamanho').selectOption('8x8')
  const linhas = page.getByRole('row').filter({ hasNot: page.getByRole('columnheader') })
  await expect(linhas).toHaveCount(2)

  // Volta para "todos"
  await page.locator('#filtro-tamanho').selectOption('todos')

  // Todas as 5 corridas voltam a aparecer
  await expect(linhas).toHaveCount(5)
  await expect(page.locator('#filtro-tamanho')).toHaveValue('todos')
  await expect(page.locator('p').filter({ hasText: /corrida.*encontrada/i }))
    .toContainText('5 corridas encontradas')
})

test('corrida selecionada exibe ID, tamanho, duração e resultado corretos', async ({ page }) => {
  // Localiza a linha pela célula de ID exata (evita falso match com IDs maiores)
  const linhaCorrida1 = page
    .getByRole('row')
    .filter({ has: page.getByRole('cell', { name: '1', exact: true }) })
    .first()

  await expect(linhaCorrida1.getByRole('cell').nth(0)).toHaveText('1')
  await expect(linhaCorrida1.getByRole('cell').nth(1)).toHaveText('4x4')
  await expect(linhaCorrida1.getByRole('cell').nth(2)).toHaveText('00:00:45')
  await expect(linhaCorrida1.getByRole('cell').nth(3)).toHaveText('✅ Concluída')
})
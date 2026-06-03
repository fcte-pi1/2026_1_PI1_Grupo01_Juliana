import { test, expect } from '@playwright/test'

// E2E (Tarefa T13, issue #130): fluxo de telemetria ao vivo.
// Automatiza o roteiro funcional da Tarefa T2.
//
// Mesma estratégia do reconexao.spec.js: injetamos um WebSocket falso via
// addInitScript ANTES da aplicação carregar, evitando dependência do backend.
// Controlamos abertura, envio de mensagens e queda a partir do teste.
//
// Observação: a app roda com <StrictMode>, que no modo dev monta o efeito duas
// vezes — operamos sempre sobre a ÚLTIMA instância criada (a ativa).

// ---------------------------------------------------------------------------
// Fixtures de telemetria
// ---------------------------------------------------------------------------

// Tamanho padrão do labirinto é 4×4 → chegada no bloco central 2×2.
const T0 = '2026-05-10T12:00:00Z'
const T1 = '2026-05-10T12:00:01Z'
const T2 = '2026-05-10T12:00:05Z' // 5 s depois de T0 → tempo "00:00:05"

const EV_INICIAL = {
  posicao_x: 0, posicao_y: 0,
  velocidade: 1.0, nivel_bateria: 100,
  timestamp: T0,
}
const EV_INTERMEDIARIO = {
  posicao_x: 1, posicao_y: 2,
  velocidade: 3.0, nivel_bateria: 73,
  timestamp: T1,
}
// Velocidades 1.0 e 3.0 → média = 2.00 m/s
// Timestamps T0 e T2 → delta = 5 s → "00:00:05"

const EV_CHEGADA = {
  posicao_x: 2, posicao_y: 2,
  velocidade: 0.5, nivel_bateria: 90,
  timestamp: T2,
}

// ---------------------------------------------------------------------------
// Setup: injeta o WebSocket falso antes de cada teste
// ---------------------------------------------------------------------------

test.beforeEach(async ({ page }) => {
  await page.addInitScript(() => {
    class FakeWebSocket {
      constructor(url) {
        this.url = url
        this._fechada = false
        window.__wsInstances.push(this)
      }
      send() {}
      close() {
        this._fechada = true
        if (this.onclose) this.onclose({})
      }
      // Chamados pelo teste para simular o servidor:
      abrir() {
        if (!this._fechada && this.onopen) this.onopen({})
      }
      enviar(dado) {
        if (!this._fechada && this.onmessage)
          this.onmessage({ data: JSON.stringify(dado) })
      }
      derrubar() {
        this._fechada = true
        if (this.onclose) this.onclose({})
      }
    }
    window.__wsInstances = []
    window.WebSocket = FakeWebSocket
  })
})

// ---------------------------------------------------------------------------
// Helpers (mesma convenção do reconexao.spec.js)
// ---------------------------------------------------------------------------

const abrirUltima  = (page)       => page.evaluate(() => window.__wsInstances.at(-1).abrir())
const enviarUltima = (page, dado) => page.evaluate((d) => window.__wsInstances.at(-1).enviar(d), dado)

async function abrirTelemetria(page) {
  await page.goto('/')
  await page.waitForFunction(() => window.__wsInstances && window.__wsInstances.length >= 1)
  // A página já abre na tela de telemetria conforme o smoke test
}

// ---------------------------------------------------------------------------
// CT-WS: Status de conexão
// ---------------------------------------------------------------------------

test('exibe "Conectado" após o WebSocket abrir', async ({ page }) => {
  await abrirTelemetria(page)
  await abrirUltima(page)
  await expect(page.getByText(/Conectado/)).toBeVisible()
})

// ---------------------------------------------------------------------------
// CT-RT: Atualização de métricas em tempo real
// ---------------------------------------------------------------------------

test('exibe placeholders antes de receber qualquer evento', async ({ page }) => {
  await abrirTelemetria(page)
  await abrirUltima(page)

  await expect(page.getByText(/-- m\/s/)).toBeVisible()
  await expect(page.getByText(/--%/)).toBeVisible()
})

test('atualiza posição atual ao receber evento de telemetria', async ({ page }) => {
  await abrirTelemetria(page)
  await abrirUltima(page)

  await enviarUltima(page, EV_INTERMEDIARIO)

  await expect(page.getByText('(1, 2)')).toBeVisible()
})

test('calcula e exibe velocidade média corretamente', async ({ page }) => {
  await abrirTelemetria(page)
  await abrirUltima(page)

  // Velocidades 1.0 e 3.0 → média esperada = 2.00 m/s
  await enviarUltima(page, EV_INICIAL)
  await enviarUltima(page, EV_INTERMEDIARIO)

  await expect(page.getByText('2.00 m/s')).toBeVisible()
})

test('exibe nível de bateria do último evento', async ({ page }) => {
  await abrirTelemetria(page)
  await abrirUltima(page)

  await enviarUltima(page, EV_INTERMEDIARIO) // nivel_bateria: 73

  await expect(page.getByText(/73%/)).toBeVisible()
})

test('exibe tempo decorrido calculado a partir dos timestamps', async ({ page }) => {
  await abrirTelemetria(page)
  await abrirUltima(page)

  // T0 → T2: delta = 5 s → "00:00:05"
  await enviarUltima(page, EV_INICIAL)
  await enviarUltima(page, EV_CHEGADA)

  await expect(page.getByText('00:00:05')).toBeVisible()
})

// ---------------------------------------------------------------------------
// CT-MZ: Grid do labirinto
// ---------------------------------------------------------------------------

test('renderiza o grid do labirinto ao entrar na tela', async ({ page }) => {
  await abrirTelemetria(page)
  await abrirUltima(page)

  // MazeMap renderiza role="grid" com aria-label="Labirinto 4x4" (tamanho padrão)
  await expect(page.getByRole('grid', { name: /Labirinto 4x4/i })).toBeVisible()
})

test('redesenha o grid ao mudar o tamanho do labirinto', async ({ page }) => {
  await abrirTelemetria(page)
  await abrirUltima(page)

  const select = page.getByRole('combobox', { name: /tamanho do labirinto/i })
  await select.selectOption('8')
  await expect(select).toHaveValue('8')

  // aria-label muda para "Labirinto 8x8" após a troca
  await expect(page.getByRole('grid', { name: /Labirinto 8x8/i })).toBeVisible()
})

// ---------------------------------------------------------------------------
// CT-DC: Desafio cumprido
// ---------------------------------------------------------------------------

test('exibe "Não" para desafio cumprido antes de atingir a chegada', async ({ page }) => {
  await abrirTelemetria(page)
  await abrirUltima(page)

  await enviarUltima(page, EV_INICIAL) // (0,0) — não é chegada

  // O label "Desafio Cumprido:" fica visível e o valor ao lado é "Não"
  await expect(page.getByText(/Desafio Cumprido/i)).toBeVisible()
  await expect(page.getByText('Não')).toBeVisible()
})

test('exibe "Sim" para desafio cumprido ao atingir o bloco central do labirinto 4×4', async ({ page }) => {
  await abrirTelemetria(page)
  await abrirUltima(page)

  await enviarUltima(page, EV_INICIAL)
  await enviarUltima(page, EV_CHEGADA)

  await expect(page.getByText('Sim')).toBeVisible()
})

// ---------------------------------------------------------------------------
// CT-RB: Robustez — mensagens inválidas não quebram a UI
// ---------------------------------------------------------------------------

test('ignora mensagem não-JSON sem travar a interface', async ({ page }) => {
  await page.addInitScript(() => {
    // Sobrescreve enviar para permitir string bruta (não serializa com JSON.stringify)
    window.__enviarRaw = (texto) => {
      const ws = window.__wsInstances.at(-1)
      if (ws && ws.onmessage) ws.onmessage({ data: texto })
    }
  })

  await abrirTelemetria(page)
  await abrirUltima(page)

  // Envia lixo — o hook deve ignorar silenciosamente
  await page.evaluate(() => window.__enviarRaw('isso não é JSON'))

  // Depois envia evento válido — a UI deve continuar funcionando
  await enviarUltima(page, EV_INICIAL)
  await expect(page.getByText('(0, 0)')).toBeVisible()
})

test('ignora mensagem de erro do backend sem exibir dados incorretos', async ({ page }) => {
  await abrirTelemetria(page)
  await abrirUltima(page)

  // Objeto sem posicao_x/posicao_y numéricos — deve ser descartado pelo hook
  await enviarUltima(page, { erro: 'sensor offline', timestamp: T0 })

  // Métricas continuam com placeholder
  await expect(page.getByText(/-- m\/s/)).toBeVisible()
})
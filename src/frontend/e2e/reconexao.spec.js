import { test, expect } from '@playwright/test'

// E2E (Tarefa T15, issue #132): robustez de conexão/reconexão do WebSocket de
// telemetria. Automatiza o roteiro manual descrito na issue #24.
//
// Em vez de subir o backend, injetamos um WebSocket falso no navegador ANTES da
// aplicação carregar e controlamos abertura, mensagens e queda a partir do
// teste. Assim o cenário fica determinístico e não depende de rede real.
//
// Observação: a app roda com <StrictMode>, que no modo dev monta o efeito duas
// vezes — por isso operamos sempre sobre a ÚLTIMA conexão criada (a ativa) e o
// socket falso ignora chamadas após ser fechado.

const TELEMETRIA_INICIAL = {
  id: 1, corrida_id: 1, posicao_x: 0, posicao_y: 0,
  nivel_bateria: 100, velocidade: 0.4, timestamp: '2026-05-10T12:00:00Z',
}
const TELEMETRIA_POS_RECONEXAO = {
  id: 2, corrida_id: 1, posicao_x: 1, posicao_y: 1,
  nivel_bateria: 80, velocidade: 0.6, timestamp: '2026-05-10T12:00:05Z',
}

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
      // Disparados pelo teste para simular o servidor:
      abrir() {
        if (!this._fechada && this.onopen) this.onopen({})
      }
      enviar(dado) {
        if (!this._fechada && this.onmessage) this.onmessage({ data: JSON.stringify(dado) })
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

const abrirUltima = (page) => page.evaluate(() => window.__wsInstances.at(-1).abrir())
const enviarUltima = (page, dado) =>
  page.evaluate((d) => window.__wsInstances.at(-1).enviar(d), dado)
const derrubarUltima = (page) => page.evaluate(() => window.__wsInstances.at(-1).derrubar())

test('perda de conexão é sinalizada, reconecta e os dados voltam a atualizar', async ({ page }) => {
  await page.goto('/')
  await page.waitForFunction(() => window.__wsInstances && window.__wsInstances.length >= 1)

  // 1. Conexão inicial estabelecida + telemetria atualizando a GUI.
  await abrirUltima(page)
  await expect(page.getByText(/Conectado/)).toBeVisible()
  await enviarUltima(page, TELEMETRIA_INICIAL)
  await expect(page.getByText(/100%/)).toBeVisible()

  const conexoesAntes = await page.evaluate(() => window.__wsInstances.length)

  // 2. Queda de conexão durante a telemetria -> indicador de conexão perdida.
  await derrubarUltima(page)
  await expect(page.getByText(/Desconectado/)).toBeVisible()

  // 3. Reconexão automática: o hook abre uma nova conexão sozinho.
  await page.waitForFunction(
    (n) => window.__wsInstances.length > n,
    conexoesAntes,
    { timeout: 7000 },
  )

  // 4. Conexão restaurada.
  await abrirUltima(page)
  await expect(page.getByText(/Conectado/)).toBeVisible()

  // 5. Os dados voltam a atualizar após a reconexão.
  await enviarUltima(page, TELEMETRIA_POS_RECONEXAO)
  await expect(page.getByText(/80%/)).toBeVisible()
  await expect(page.getByText(/\(1, 1\)/)).toBeVisible()
})

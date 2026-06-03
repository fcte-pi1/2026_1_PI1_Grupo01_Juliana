// Testes unitários da página de Telemetria — Tarefa T10 (#127).
//
// Complementam os testes do núcleo entregue na issue #19 (Telemetry.test.jsx),
// cobrindo de ponta a ponta o comportamento da página:
//   1. Renderização inicial da tela;
//   2. Atualização da UI ao receber dados via WebSocket (mockado);
//   3. Exibição do grid do labirinto conforme a telemetria;
//   4. Estado de "sem dados" / conexão não estabelecida;
//   5. Independência de backend real (nenhuma conexão de rede é aberta).
//
// O WebSocket é sempre controlado pelos testes: ou por um dublê (FakeWebSocket),
// ou removido do ambiente. Assim os testes são determinísticos e nunca tocam o
// backend nem a rede — inclusive em ambientes (Node/undici) que já trazem um
// WebSocket global.
import { render, screen, act, fireEvent } from '@testing-library/react'
import { describe, it, expect, beforeEach, afterEach, vi } from 'vitest'
import Telemetry from './Telemetry'

// --- Dublê de WebSocket -----------------------------------------------------
// Registra cada instância criada e expõe helpers para disparar manualmente os
// eventos que o backend emitiria (open / message / close), reproduzindo o
// formato TelemetriaOut transmitido no broadcast.
class FakeWebSocket {
  constructor(url) {
    this.url = url
    this.closed = false
    FakeWebSocket.instances.push(this)
  }
  close() {
    this.closed = true
  }
  // Helpers de teste
  emitOpen() {
    act(() => this.onopen?.())
  }
  emitTelemetria(evento) {
    act(() => this.onmessage?.({ data: JSON.stringify(evento) }))
  }
  emitRaw(data) {
    act(() => this.onmessage?.({ data }))
  }
  emitClose() {
    act(() => this.onclose?.())
  }
}

function instalarWebSocketFake() {
  FakeWebSocket.instances = []
  vi.stubGlobal('WebSocket', FakeWebSocket)
}

// Simula um ambiente sem suporte a WebSocket (o hook entra em "indisponível").
function removerWebSocket() {
  vi.stubGlobal('WebSocket', undefined)
}

// Monta um evento de telemetria válido (TelemetriaOut) com defaults sensatos.
function telemetria(overrides = {}) {
  return {
    id: 1,
    corrida_id: 1,
    posicao_x: 0,
    posicao_y: 0,
    nivel_bateria: 100,
    velocidade: 0.4,
    timestamp: '2026-05-10T12:00:00Z',
    ...overrides,
  }
}

const COR_POSICAO = '#1565c0'

// ===========================================================================
// 1. Renderização inicial
// ===========================================================================

describe('Telemetry (T10) — renderização inicial', () => {
  beforeEach(() => instalarWebSocketFake())
  afterEach(() => vi.unstubAllGlobals())

  it('renderiza título, indicador de status e seletor de tamanho', () => {
    render(<Telemetry />)

    expect(screen.getByRole('heading', { name: 'Painel de Controle' })).toBeInTheDocument()
    expect(screen.getByText('Status da Corrida')).toBeInTheDocument()
    expect(screen.getByLabelText(/Tamanho do labirinto/i)).toHaveValue('4')
  })

  it('exibe placeholders das métricas enquanto não há dados', () => {
    render(<Telemetry />)

    expect(screen.getByText('-- m/s')).toBeInTheDocument()
    expect(screen.getByText(/--%/)).toBeInTheDocument() // valor compartilha o nó com o emoji 🔋
    expect(screen.getByText('00:00:00')).toBeInTheDocument()
    expect(screen.getByText('--')).toBeInTheDocument() // posição
    expect(screen.getByText('Não')).toBeInTheDocument() // desafio não cumprido
  })

  it('mostra o mapa 4x4 (16 células) por padrão', () => {
    render(<Telemetry />)

    expect(screen.getByRole('heading', { name: /Mapa do Labirinto \(4x4\)/ })).toBeInTheDocument()
    expect(screen.getByRole('grid').children).toHaveLength(16)
  })
})

// ===========================================================================
// 2. Estado de "sem dados" / conexão não estabelecida
// ===========================================================================

describe('Telemetry (T10) — sem dados / conexão não estabelecida', () => {
  afterEach(() => vi.unstubAllGlobals())

  it('sem WebSocket no ambiente, marca status indisponível e ainda renderiza', () => {
    removerWebSocket()

    render(<Telemetry />)

    expect(screen.getByText(/WebSocket indispon/i)).toBeInTheDocument()
    expect(screen.getByText(/Aguardando dados de telemetria/i)).toBeInTheDocument()
  })

  it('marca status "Desconectado" quando a conexão cai', () => {
    instalarWebSocketFake()

    render(<Telemetry />)
    const ws = FakeWebSocket.instances[0]
    ws.emitOpen()
    expect(screen.getByText(/Conectado/)).toBeInTheDocument()

    ws.emitClose()
    expect(screen.getByText(/Desconectado/)).toBeInTheDocument()
  })
})

// ===========================================================================
// 3. Atualização da UI via WebSocket (mockado)
// ===========================================================================

describe('Telemetry (T10) — atualização via WebSocket', () => {
  beforeEach(() => instalarWebSocketFake())
  afterEach(() => vi.unstubAllGlobals())

  it('abre a conexão no endpoint /ws/telemetria e marca "Conectado"', () => {
    render(<Telemetry />)
    const ws = FakeWebSocket.instances[0]

    expect(ws).toBeTruthy()
    expect(ws.url).toMatch(/\/ws\/telemetria$/)

    ws.emitOpen()
    expect(screen.getByText(/Conectado/)).toBeInTheDocument()
  })

  it('atualiza velocidade média, bateria, posição e tempo ao receber eventos', () => {
    render(<Telemetry />)
    const ws = FakeWebSocket.instances[0]
    ws.emitOpen()

    ws.emitTelemetria(telemetria({ id: 1, posicao_x: 0, posicao_y: 0, nivel_bateria: 100, velocidade: 0.4, timestamp: '2026-05-10T12:00:00Z' }))
    ws.emitTelemetria(telemetria({ id: 2, posicao_x: 2, posicao_y: 3, nivel_bateria: 85.4, velocidade: 0.6, timestamp: '2026-05-10T12:00:05Z' }))

    expect(screen.getByText('0.50 m/s')).toBeInTheDocument() // média de 0.4 e 0.6
    expect(screen.getByText(/85%/)).toBeInTheDocument() // arredonda 85.4 -> 85 (nó compartilhado com 🔋)
    expect(screen.getByText('(2, 3)')).toBeInTheDocument() // posição do último evento
    expect(screen.getByText('00:00:05')).toBeInTheDocument() // 5s entre o primeiro e o último
  })

  it('ignora mensagens de erro / não-telemetria sem quebrar a tela', () => {
    render(<Telemetry />)
    const ws = FakeWebSocket.instances[0]
    ws.emitOpen()

    ws.emitTelemetria({ erro: 'corrida inexistente' }) // sem posicao_x/y
    ws.emitRaw('isto não é JSON')

    // Continua no estado "sem dados".
    expect(screen.getByText('-- m/s')).toBeInTheDocument()
    expect(screen.getByText(/Aguardando dados de telemetria/i)).toBeInTheDocument()
  })
})

// ===========================================================================
// 4. Grid do labirinto conforme a telemetria
// ===========================================================================

describe('Telemetry (T10) — grid do labirinto', () => {
  beforeEach(() => instalarWebSocketFake())
  afterEach(() => vi.unstubAllGlobals())

  it('pinta a célula da posição atual e remove o aviso de aguardando', () => {
    render(<Telemetry />)
    const ws = FakeWebSocket.instances[0]
    ws.emitOpen()
    ws.emitTelemetria(telemetria({ posicao_x: 0, posicao_y: 0 }))

    expect(screen.queryByText(/Aguardando dados de telemetria/i)).not.toBeInTheDocument()
    // No 4x4, a célula (0,0) é o índice 0 do grid e recebe a cor de posição atual.
    const primeiraCelula = screen.getByRole('grid').children[0]
    expect(primeiraCelula).toHaveStyle({ backgroundColor: COR_POSICAO })
  })

  it('atualiza o grid ao trocar o tamanho do labirinto no seletor', () => {
    render(<Telemetry />)

    const seletor = screen.getByLabelText(/Tamanho do labirinto/i)
    fireEvent.change(seletor, { target: { value: '16' } })

    expect(screen.getByRole('heading', { name: /Mapa do Labirinto \(16x16\)/ })).toBeInTheDocument()
    expect(screen.getByRole('grid').children).toHaveLength(256)
  })

  it('recalcula "Desafio Cumprido" conforme o tamanho selecionado', () => {
    render(<Telemetry />)
    const ws = FakeWebSocket.instances[0]
    ws.emitOpen()

    // (1,1) é célula central no 4x4 -> desafio cumprido.
    ws.emitTelemetria(telemetria({ posicao_x: 1, posicao_y: 1 }))
    expect(screen.getByText('Sim')).toBeInTheDocument()

    // No 16x16 a mesma posição não é o centro -> desafio deixa de estar cumprido.
    fireEvent.change(screen.getByLabelText(/Tamanho do labirinto/i), { target: { value: '16' } })
    expect(screen.getByText('Não')).toBeInTheDocument()
  })
})

// ===========================================================================
// 5. Independência de backend real
// ===========================================================================

describe('Telemetry (T10) — independência de backend', () => {
  afterEach(() => vi.unstubAllGlobals())

  it('não abre nenhuma conexão quando o WebSocket não existe', () => {
    removerWebSocket()

    render(<Telemetry />)

    expect(screen.getByRole('heading', { name: 'Painel de Controle' })).toBeInTheDocument()
    expect(screen.getByText(/WebSocket indispon/i)).toBeInTheDocument()
  })

  it('usa apenas o WebSocket dublê, sem tocar serviços reais', () => {
    instalarWebSocketFake()

    render(<Telemetry />)
    // Exatamente uma conexão foi criada, e por meio do nosso dublê.
    expect(FakeWebSocket.instances).toHaveLength(1)
    expect(FakeWebSocket.instances[0]).toBeInstanceOf(FakeWebSocket)
  })
})

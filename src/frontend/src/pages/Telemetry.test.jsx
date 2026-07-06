// Testes da tela de telemetria ao vivo (issue #19).
//
// Em jsdom não existe WebSocket nativo. No primeiro teste deixamos esse estado
// como está (o hook entra em "indisponível" e a tela ainda monta). No segundo,
// injetamos um WebSocket falso para simular exatamente o broadcast do backend
// (formato TelemetriaOut) e conferir que as métricas, o trajeto e o indicador
// de desafio reagem aos dados. Os testes de unidade mais amplos da página são a
// Tarefa T10 (#127); aqui validamos o núcleo entregue por esta issue.
import { render, screen, act } from '@testing-library/react'
import Telemetry from './Telemetry'

describe('Telemetry', () => {
  it('renderiza o painel de métricas e o seletor de tamanho', () => {
    render(<Telemetry />)

    expect(screen.getByRole('heading', { name: 'Painel de Controle' })).toBeInTheDocument()
    expect(screen.getByText('Velocidade Média:')).toBeInTheDocument()
    expect(screen.getByText('Tempo:')).toBeInTheDocument()
    expect(screen.getByText('Bateria:')).toBeInTheDocument()
    expect(screen.getByText('Desafio Cumprido:')).toBeInTheDocument()
    expect(screen.getByLabelText(/Tamanho do labirinto/i)).toBeInTheDocument()
  })

  it('mostra o mapa 4x4 por padrão e sem dados de trajeto', () => {
    render(<Telemetry />)

    expect(screen.getByRole('heading', { name: /Mapa do Labirinto \(4x4\)/ })).toBeInTheDocument()
    expect(screen.getByText(/Aguardando dados de telemetria/i)).toBeInTheDocument()
    expect(screen.getByText('Não')).toBeInTheDocument()
  })

  it('atualiza métricas e marca o desafio ao receber telemetria via WebSocket', async () => {
    // WebSocket falso: registra a instância criada e expõe os handlers que o
    // hook atribui (onopen/onmessage), para dispararmos eventos manualmente.
    class FakeWebSocket {
      constructor(url) {
        this.url = url
        FakeWebSocket.instances.push(this)
      }
      close() {}
    }
    FakeWebSocket.instances = []
    vi.stubGlobal('WebSocket', FakeWebSocket)

    try {
      render(<Telemetry />)
      const ws = FakeWebSocket.instances[0]
      expect(ws).toBeTruthy()
      expect(ws.url).toMatch(/\/ws\/telemetria$/)

      await act(async () => {
        ws.onopen()
      })
      expect(screen.getByText(/Conectado/)).toBeInTheDocument()

      // Mesmo formato (TelemetriaOut) que o backend transmite no broadcast.
      await act(async () => {
        ws.onmessage({
          data: JSON.stringify({
            id: 1, corrida_id: 1, posicao_x: 0, posicao_y: 0,
            nivel_bateria: 100, velocidade: 0.4, timestamp: '2026-05-10T12:00:00Z',
          }),
        })
        // (1,1) é célula central do 4x4 -> desafio cumprido.
        ws.onmessage({
          data: JSON.stringify({
            id: 2, corrida_id: 1, posicao_x: 1, posicao_y: 1,
            nivel_bateria: 90, velocidade: 0.6, timestamp: '2026-05-10T12:00:02Z',
            orientacao: 'LESTE', mensagem: 'Celula avancada com sucesso',
          }),
        })
      })

      expect(screen.getByText(/0\.50 m\/s/)).toBeInTheDocument() // média de 0.4 e 0.6
      expect(screen.getByText(/90%/)).toBeInTheDocument() // bateria do último evento
      expect(screen.getByText(/^Posição:/)).toBeInTheDocument()
      expect(screen.getByText('LESTE')).toBeInTheDocument()
      expect(screen.getByText('Celula avancada com sucesso')).toBeInTheDocument()
      expect(screen.getByText('Sim')).toBeInTheDocument() // desafio cumprido
    } finally {
      vi.unstubAllGlobals()
    }
  })
})

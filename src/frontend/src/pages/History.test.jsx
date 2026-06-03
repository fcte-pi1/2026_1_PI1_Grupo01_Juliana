// Testes unitários da página de Histórico e seus filtros — Tarefa T11 (#128).
//
// Cobre os critérios de conclusão:
//   1. Renderização da lista de corridas com dados mockados;
//   2. Filtro por labirinto e a opção "todos";
//   3. Estado de carregamento e de lista vazia;
//   4. Tratamento de erro de requisição (fetch mockado);
//   5. A seleção do filtro exibe os dados da corrida correta.
//
// O `fetch` é sempre mockado (vi.stubGlobal), então os testes não dependem do
// backend nem da rede.
import { render, screen, fireEvent, waitFor, within } from '@testing-library/react'
import { describe, it, expect, afterEach, vi } from 'vitest'
import History from './History'

// Resposta fake no formato esperado por useHistory (objeto tipo Response).
function resposta(dados, { ok = true, status = 200, statusText = 'OK' } = {}) {
  return Promise.resolve({
    ok,
    status,
    statusText,
    json: () => Promise.resolve(dados),
  })
}

const CORRIDAS = [
  { id: 1, tamanho: '4x4', duracao: 65, resultado: 'concluida' },
  { id: 2, tamanho: '8x8', duracao: 130, resultado: 'falha' },
]

function filtro() {
  return screen.getByLabelText(/Filtrar por tamanho/i)
}

afterEach(() => vi.unstubAllGlobals())

// ===========================================================================
// 1. Renderização da lista com dados mockados
// ===========================================================================

describe('History (T11) — lista de corridas', () => {
  it('renderiza as corridas retornadas pela API', async () => {
    vi.stubGlobal('fetch', vi.fn(() => resposta(CORRIDAS)))
    render(<History />)

    // Espera a tabela aparecer (fetch resolvido).
    const tabela = await screen.findByRole('table')
    const linhas = within(tabela)

    expect(linhas.getByText('1')).toBeInTheDocument()
    expect(linhas.getByText('4x4')).toBeInTheDocument()
    expect(linhas.getByText('00:01:05')).toBeInTheDocument() // 65s formatado
    expect(linhas.getByText('✅ Concluída')).toBeInTheDocument()

    expect(linhas.getByText('2')).toBeInTheDocument()
    expect(linhas.getByText('8x8')).toBeInTheDocument()
    expect(linhas.getByText('00:02:10')).toBeInTheDocument() // 130s formatado
    expect(linhas.getByText('❌ Falha')).toBeInTheDocument()
  })
})

// ===========================================================================
// 2. Filtro por labirinto e opção "todos"
// ===========================================================================

describe('History (T11) — filtro por tamanho', () => {
  it('inclui o tamanho na requisição e volta a "todos" sem filtro', async () => {
    const fetchMock = vi.fn(() => resposta(CORRIDAS))
    vi.stubGlobal('fetch', fetchMock)
    render(<History />)
    await screen.findByRole('table')

    // 1ª chamada: "todos" -> sem parâmetro tamanho
    expect(fetchMock.mock.calls[0][0]).not.toMatch(/tamanho=/)

    // Seleciona 4x4 -> nova requisição com tamanho=4x4
    fireEvent.change(filtro(), { target: { value: '4x4' } })
    await waitFor(() => expect(fetchMock).toHaveBeenCalledTimes(2))
    expect(fetchMock.mock.calls[1][0]).toMatch(/tamanho=4x4/)

    // Volta para "todos" -> requisição sem tamanho
    fireEvent.change(filtro(), { target: { value: 'todos' } })
    await waitFor(() => expect(fetchMock).toHaveBeenCalledTimes(3))
    expect(fetchMock.mock.calls[2][0]).not.toMatch(/tamanho=/)
  })
})

// ===========================================================================
// 3. Estado de carregamento e de lista vazia
// ===========================================================================

describe('History (T11) — carregamento e lista vazia', () => {
  it('mostra o estado de carregamento enquanto a busca não resolve', () => {
    vi.stubGlobal('fetch', vi.fn(() => new Promise(() => {}))) // nunca resolve
    render(<History />)

    expect(screen.getByText(/Carregando corridas/i)).toBeInTheDocument()
  })

  it('mostra mensagem de lista vazia quando não há corridas', async () => {
    vi.stubGlobal('fetch', vi.fn(() => resposta([])))
    render(<History />)

    expect(await screen.findByText(/Nenhuma corrida registrada ainda/i)).toBeInTheDocument()
    expect(screen.queryByRole('table')).not.toBeInTheDocument()
  })

  it('mostra mensagem de vazio específica do filtro quando há tamanho selecionado', async () => {
    vi.stubGlobal('fetch', vi.fn(() => resposta([])))
    render(<History />)
    await screen.findByText(/Nenhuma corrida registrada ainda/i)

    fireEvent.change(filtro(), { target: { value: '8x8' } })
    expect(
      await screen.findByText(/Nenhuma corrida encontrada para labirintos 8x8/i)
    ).toBeInTheDocument()
  })
})

// ===========================================================================
// 4. Tratamento de erro de requisição
// ===========================================================================

describe('History (T11) — tratamento de erro', () => {
  it('exibe o erro quando o fetch é rejeitado e permite tentar novamente', async () => {
    const fetchMock = vi
      .fn()
      .mockImplementationOnce(() => Promise.reject(new Error('Falha de rede')))
      .mockImplementationOnce(() => resposta(CORRIDAS))
    vi.stubGlobal('fetch', fetchMock)
    render(<History />)

    expect(await screen.findByText(/Falha de rede/i)).toBeInTheDocument()

    // Botão "Tentar novamente" refaz a busca (agora bem-sucedida).
    fireEvent.click(screen.getByRole('button', { name: /Tentar novamente/i }))
    expect(await screen.findByRole('table')).toBeInTheDocument()
  })

  it('exibe o erro quando a resposta HTTP não é ok (status de erro)', async () => {
    vi.stubGlobal(
      'fetch',
      vi.fn(() => resposta(null, { ok: false, status: 500, statusText: 'Internal Server Error' }))
    )
    render(<History />)

    expect(await screen.findByText(/Erro 500/i)).toBeInTheDocument()
  })
})

// ===========================================================================
// 5. A seleção do filtro exibe os dados da corrida correta
// ===========================================================================

describe('History (T11) — seleção exibe dados corretos', () => {
  it('ao filtrar, mostra apenas as corridas do tamanho escolhido', async () => {
    const apenas4x4 = [{ id: 7, tamanho: '4x4', duracao: 42, resultado: 'concluida' }]
    const fetchMock = vi
      .fn()
      .mockImplementationOnce(() => resposta(CORRIDAS)) // todos
      .mockImplementationOnce(() => resposta(apenas4x4)) // 4x4
    vi.stubGlobal('fetch', fetchMock)
    render(<History />)
    await screen.findByRole('table')

    fireEvent.change(filtro(), { target: { value: '4x4' } })

    // A corrida 4x4 (id 7) aparece; a 8x8 (❌ Falha) some.
    expect(await screen.findByText('7')).toBeInTheDocument()
    const tabela = screen.getByRole('table')
    expect(within(tabela).getByText('00:00:42')).toBeInTheDocument()
    expect(within(tabela).queryByText('❌ Falha')).not.toBeInTheDocument()
  })
})


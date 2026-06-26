import { render, screen } from '@testing-library/react'
import { describe, it, expect } from 'vitest'
import MazeMap from '../MazeMap'

describe('MazeMap', () => {
  it('renderiza com tamanho padrao 4x4', () => {
    render(<MazeMap />)
    expect(screen.getByText('Mapa do Labirinto (4x4)')).toBeInTheDocument()
  })

  it('renderiza 16 celulas para grid 4x4', () => {
    render(<MazeMap size={4} />)
    expect(screen.getByRole('grid').children).toHaveLength(16)
  })

  it('renderiza 64 celulas para grid 8x8', () => {
    render(<MazeMap size={8} />)
    expect(screen.getByText('Mapa do Labirinto (8x8)')).toBeInTheDocument()
    expect(screen.getByRole('grid').children).toHaveLength(64)
  })

  it('grid tem colunas corretas para o tamanho informado', () => {
    render(<MazeMap size={8} />)
    expect(screen.getByRole('grid')).toHaveStyle('grid-template-columns: repeat(8, 1fr)')
  })

  it('exibe mensagem de aguardando dados', () => {
    render(<MazeMap />)
    expect(screen.getByText(/aguardando dados de telemetria/i)).toBeInTheDocument()
  })
})

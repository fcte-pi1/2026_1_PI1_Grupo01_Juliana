import { render, screen } from '@testing-library/react'
import userEvent from '@testing-library/user-event'
import { MemoryRouter, Route, Routes } from 'react-router-dom'
import { describe, it, expect } from 'vitest'
import Layout from '../Layout'

function renderLayout(initialRoute = '/') {
  return render(
    <MemoryRouter initialEntries={[initialRoute]}>
      <Routes>
        <Route path="/" element={<Layout />}>
          <Route index element={<h1>Painel de Controle</h1>} />
          <Route path="historico" element={<h1>Histórico de Corridas</h1>} />
        </Route>
      </Routes>
    </MemoryRouter>
  )
}

describe('Layout', () => {
  it('renderiza o titulo no header', () => {
    renderLayout()
    expect(screen.getByText('Micromouse Telemetria')).toBeInTheDocument()
  })

  it('renderiza os dois links de navegacao', () => {
    renderLayout()
    expect(screen.getByText('Telemetria ao Vivo')).toBeInTheDocument()
    expect(screen.getByText('Histórico')).toBeInTheDocument()
  })

  it('link Telemetria ao Vivo esta ativo na rota /', () => {
    renderLayout('/')
    const linkTelemetria = screen.getByText('Telemetria ao Vivo')
    const linkHistorico = screen.getByText('Histórico')

    expect(linkTelemetria).toHaveStyle('color: #61dafb')
    expect(linkHistorico).toHaveStyle('color: #ccc')
  })

  it('link Historico esta ativo na rota /historico', () => {
    renderLayout('/historico')
    const linkTelemetria = screen.getByText('Telemetria ao Vivo')
    const linkHistorico = screen.getByText('Histórico')

    expect(linkHistorico).toHaveStyle('color: #61dafb')
    expect(linkTelemetria).toHaveStyle('color: #ccc')
  })

  it('navega de Telemetria para Historico e volta', async () => {
    const user = userEvent.setup()
    renderLayout('/')

    expect(screen.getByText('Painel de Controle')).toBeInTheDocument()

    await user.click(screen.getByText('Histórico'))
    expect(screen.getByText('Histórico de Corridas')).toBeInTheDocument()

    await user.click(screen.getByText('Telemetria ao Vivo'))
    expect(screen.getByText('Painel de Controle')).toBeInTheDocument()
  })
})

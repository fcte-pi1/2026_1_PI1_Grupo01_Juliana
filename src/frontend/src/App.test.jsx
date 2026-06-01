// Teste-fumaça (smoke test): garante que a aplicação monta sem erros e que a
// navegação principal e a página inicial (Telemetria) são renderizadas.
// Serve de molde para as Tarefas T10–T12 (testes das páginas e componentes).
import { render, screen } from '@testing-library/react'
import App from './App'

describe('App', () => {
  it('renderiza o cabeçalho de navegação', () => {
    render(<App />)
    expect(screen.getByText('Micromouse Telemetria')).toBeInTheDocument()
  })

  it('renderiza a página de Telemetria na rota inicial', () => {
    render(<App />)
    expect(screen.getByRole('heading', { name: 'Painel de Controle' })).toBeInTheDocument()
  })
})

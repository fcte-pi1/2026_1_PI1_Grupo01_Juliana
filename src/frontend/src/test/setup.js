// Setup global dos testes: adiciona os matchers do jest-dom (ex.: toBeInTheDocument)
// e limpa o DOM renderizado após cada teste para isolar um teste do outro.
import '@testing-library/jest-dom/vitest'
import { cleanup } from '@testing-library/react'
import { afterEach } from 'vitest'

afterEach(() => {
  cleanup()
})

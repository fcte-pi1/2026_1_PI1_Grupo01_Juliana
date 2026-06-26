import { useState } from 'react';
import useHistory from '../hooks/useHistory';

const TAMANHOS = ['todos', '4x4', '8x8'];

function formatarTempo(segundos) {
  if (segundos == null || isNaN(segundos)) return '—';
  const s = Math.max(0, Math.floor(segundos));
  const hh = String(Math.floor(s / 3600)).padStart(2, '0');
  const mm = String(Math.floor((s % 3600) / 60)).padStart(2, '0');
  const ss = String(s % 60).padStart(2, '0');
  return `${hh}:${mm}:${ss}`;
}

function formatarResultado(resultado) {
  const mapa = {
    concluida: '✅ Concluída',
    falha: '❌ Falha',
    em_andamento: '⏳ Em andamento',
  };
  return mapa[resultado] ?? resultado;
}

export default function History() {
  const [filtro, setFiltro] = useState('todos');
  const { corridas, carregando, erro, recarregar } = useHistory(filtro);

  return (
    <div>
      <h1>Histórico de Corridas</h1>

      <div className="filter-bar">
        <label htmlFor="filtro-tamanho">Filtrar por tamanho: </label>
        <select
          id="filtro-tamanho"
          value={filtro}
          onChange={(e) => setFiltro(e.target.value)}
          disabled={carregando}
        >
          {TAMANHOS.map((t) => (
            <option key={t} value={t}>
              {t === 'todos' ? 'Todos' : t}
            </option>
          ))}
        </select>
        <button 
          onClick={recarregar} 
          disabled={carregando} 
          title="Atualizar"
          aria-label="Atualizar histórico"
        >
          🔄
        </button>
      </div>

      {/* Feedbacks Visuais */}
      <div aria-live="polite">
        {erro && (
          <div className="feedback-container" style={{ color: 'var(--danger)' }}>
            <p style={{ marginBottom: '1rem', fontWeight: 'bold' }}>⚠️ {erro}</p>
            <button onClick={recarregar}>Tentar novamente</button>
          </div>
        )}

        {carregando && !erro && (
          <div className="feedback-container">
            <div className="spinner"></div>
            <p style={{ marginTop: '1rem', color: 'var(--text-muted)' }}>Carregando corridas...</p>
          </div>
        )}

        {!carregando && !erro && corridas.length === 0 && (
          <div className="feedback-container">
            <p style={{ fontSize: '2rem', margin: 0 }}>🏁</p>
            <p style={{ color: 'var(--text-muted)', marginTop: '0.5rem' }}>
              {filtro === 'todos'
                ? 'Nenhuma corrida registrada ainda.'
                : `Nenhuma corrida encontrada para labirintos ${filtro}.`}
            </p>
          </div>
        )}

        {!carregando && !erro && corridas.length > 0 && (
          <>
            <div className="table-responsive">
              <table className="history-table">
                <thead>
                  <tr>
                    <th>ID da Corrida</th>
                    <th>Tamanho</th>
                    <th>Duração</th>
                    <th>Resultado</th>
                  </tr>
                </thead>
                <tbody>
                  {corridas.map((corrida, idx) => (
                    <tr key={corrida.id ?? idx}>
                      <td>{corrida.id ?? '—'}</td>
                      <td>{corrida.tamanho ?? '—'}</td>
                      <td>{formatarTempo(corrida.duracao)}</td>
                      <td>{formatarResultado(corrida.resultado)}</td>
                    </tr>
                  ))}
                </tbody>
              </table>
            </div>
            <p style={{ marginTop: '1rem', fontSize: '0.85rem', color: 'var(--text-muted)', textAlign: 'right' }}>
              {corridas.length} corrida{corridas.length !== 1 ? 's' : ''} encontrada{corridas.length !== 1 ? 's' : ''}
              {filtro !== 'todos' ? ` para ${filtro}` : ''}.
            </p>
          </>
        )}
      </div>
    </div>
  );
}
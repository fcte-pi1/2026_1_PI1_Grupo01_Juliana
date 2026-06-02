import { useState } from 'react';
import useHistory from '../hooks/useHistory';

const TAMANHOS = ['todos', '4x4', '8x8', '16x16'];

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

      {/* Filtro */}
      <div style={{ marginBottom: '1rem', display: 'flex', alignItems: 'center', gap: '0.5rem' }}>
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
        <button onClick={recarregar} disabled={carregando} title="Atualizar">
          {carregando ? '⏳' : '🔄'}
        </button>
      </div>

      {/* Estado: erro */}
      {erro && (
        <div style={{ textAlign: 'center', padding: '2rem', color: '#f87171' }}>
          <p>⚠️ {erro}</p>
          <button onClick={recarregar}>Tentar novamente</button>
        </div>
      )}

      {/* Estado: carregando */}
      {carregando && !erro && (
        <p style={{ textAlign: 'center', padding: '1rem' }}>Carregando corridas...</p>
      )}

      {/* Estado: vazio */}
      {!carregando && !erro && corridas.length === 0 && (
        <p style={{ textAlign: 'center', padding: '2rem', color: '#94a3b8' }}>
          🏁{' '}
          {filtro === 'todos'
            ? 'Nenhuma corrida registrada ainda.'
            : `Nenhuma corrida encontrada para labirintos ${filtro}.`}
        </p>
      )}

      {/* Estado: dados */}
      {!carregando && !erro && corridas.length > 0 && (
        <>
          <table style={styles.table}>
            <thead>
              <tr style={styles.headerRow}>
                <th>ID da Corrida</th>
                <th>Tamanho</th>
                <th>Duração</th>
                <th>Resultado</th>
              </tr>
            </thead>
            <tbody>
              {corridas.map((corrida, idx) => (
                <tr key={corrida.id ?? idx}>
                  <td style={styles.td}>{corrida.id ?? '—'}</td>
                  <td style={styles.td}>{corrida.tamanho ?? '—'}</td>
                  <td style={styles.td}>{formatarTempo(corrida.duracao)}</td>
                  <td style={styles.td}>{formatarResultado(corrida.resultado)}</td>
                </tr>
              ))}
            </tbody>
          </table>
          <p style={{ marginTop: '0.5rem', fontSize: '0.8rem', color: '#64748b', textAlign: 'right' }}>
            {corridas.length} corrida{corridas.length !== 1 ? 's' : ''} encontrada
            {corridas.length !== 1 ? 's' : ''}
            {filtro !== 'todos' ? ` para ${filtro}` : ''}.
          </p>
        </>
      )}
    </div>
  );
}

const styles = {
  table: {
    width: '100%',
    borderCollapse: 'collapse',
    marginTop: '1rem',
  },
  headerRow: {
    backgroundColor: '#282c34',
    color: 'white',
    textAlign: 'left',
  },
  td: {
    padding: '0.7rem 1rem',
    borderBottom: '1px solid #2d3748',
  },
};

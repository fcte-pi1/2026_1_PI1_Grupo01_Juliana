import { useState } from 'react';
import MazeMap from '../components/MazeMap';
import useTelemetry, { STATUS } from '../hooks/useTelemetry';

const TAMANHOS = [4, 8];

const STATUS_INFO = {
  [STATUS.conectando]: { texto: 'Conectando…', cor: 'var(--warning)' },
  [STATUS.conectado]: { texto: 'Conectado', cor: 'var(--success)' },
  [STATUS.desconectado]: { texto: 'Desconectado', cor: 'var(--danger)' },
  [STATUS.indisponivel]: { texto: 'WebSocket indisponível', cor: 'var(--neutral)' },
};

export default function Telemetry() {
  const [size, setSize] = useState(4);
  const t = useTelemetry(size);
  const statusInfo = STATUS_INFO[t.status] ?? STATUS_INFO[STATUS.desconectado];

  return (
    <div>
      <div className="telemetry-header">
        <h1 style={{ margin: 0 }}>Painel de Controle</h1>
        <span 
          className="status-pill" 
          style={{ backgroundColor: statusInfo.cor }}
          aria-live="polite"
        >
          <span aria-hidden="true">●</span> {statusInfo.texto}
        </span>
      </div>

      <div className="filter-bar">
        <label htmlFor="tamanho-labirinto">Tamanho do labirinto: </label>
        <select
          id="tamanho-labirinto"
          value={size}
          onChange={(e) => setSize(Number(e.target.value))}
        >
          {TAMANHOS.map((n) => (
            <option key={n} value={n}>
              {n}x{n}
            </option>
          ))}
        </select>
      </div>

      <div className="dashboard-grid">
        <div className="data-panel">
          <h3>Status da Corrida</h3>
          <div className="stat-box">
            <strong>Velocidade Média:</strong>
            <span>{t.velocidadeMedia != null ? `${t.velocidadeMedia.toFixed(2)} m/s` : '-- m/s'}</span>
          </div>
          <div className="stat-box">
            <strong>Tempo:</strong> 
            <span>{t.tempo}</span>
          </div>
          <div className="stat-box">
            <strong>Bateria:</strong>
            <span>{t.bateria != null ? `${Math.round(t.bateria)}%` : '--%'} 🔋</span>
          </div>
          <div className="stat-box">
            <strong>Desafio Cumprido:</strong>
            <span style={{ color: t.desafioCumprido ? 'var(--success)' : 'var(--danger)', fontWeight: 'bold' }}>
              {t.desafioCumprido ? 'Sim' : 'Não'}
            </span>
          </div>
          <div className="stat-box">
            <strong>Posição:</strong>
            <span>{t.posicaoAtual ? `(${t.posicaoAtual.x}, ${t.posicaoAtual.y})` : '--'}</span>
          </div>
          <div style={styles.statBox}>
            <strong>Orientação:</strong> {t.orientacao ?? '--'}
          </div>
          <div style={styles.statBox}>
            <strong>Eventos recebidos:</strong> {t.totalEventos}
          </div>
        </div>

        <div className="map-panel">
          <MazeMap size={size} trajeto={t.trajeto} posicaoAtual={t.posicaoAtual} />
        </div>
      </div>

      <div style={styles.logPanel}>
        <h3>Log do robô</h3>
        {t.logs.length === 0 ? (
          <p style={styles.logEmpty}>Aguardando eventos do carrinho…</p>
        ) : (
          <ul style={styles.logList}>
            {[...t.logs].reverse().map((item) => (
              <li key={item.id} style={styles.logItem}>
                <span style={styles.logTime}>{item.horario}</span>
                <span style={styles.logMessage}>{item.mensagem}</span>
                <span style={styles.logMeta}>
                  ({item.posicao.x}, {item.posicao.y})
                  {item.orientacao ? ` · ${item.orientacao}` : ''}
                </span>
              </li>
            ))}
          </ul>
        )}
      </div>
    </div>
  );
}

const styles = {
  titleRow: {
    display: 'flex',
    alignItems: 'center',
    gap: '1rem',
    flexWrap: 'wrap',
  },
  statusPill: {
    color: 'white',
    fontSize: '13px',
    fontWeight: 'bold',
    padding: '4px 10px',
    borderRadius: '12px',
  },
  controls: {
    marginTop: '0.5rem',
  },
  dashboard: {
    display: 'flex',
    gap: '2rem',
    marginTop: '1rem',
    flexWrap: 'wrap',
  },
  dataPanel: {
    flex: 1,
    minWidth: '240px',
    padding: '1rem',
    backgroundColor: '#f4f4f4',
    borderRadius: '8px',
  },
  mapPanel: {
    flex: 2,
    minWidth: '280px',
  },
  statBox: {
    padding: '10px',
    borderBottom: '1px solid #ddd',
    fontSize: '16px',
  },
  logPanel: {
    marginTop: '1.5rem',
    padding: '1rem',
    backgroundColor: '#111',
    color: '#eee',
    borderRadius: '8px',
    maxHeight: '320px',
    overflowY: 'auto',
  },
  logEmpty: {
    color: '#aaa',
    fontStyle: 'italic',
    margin: 0,
  },
  logList: {
    listStyle: 'none',
    margin: 0,
    padding: 0,
  },
  logItem: {
    display: 'grid',
    gridTemplateColumns: '72px 1fr',
    gap: '0.25rem 0.75rem',
    padding: '8px 0',
    borderBottom: '1px solid #333',
    fontSize: '14px',
  },
  logTime: {
    color: '#8bc34a',
    fontFamily: 'monospace',
  },
  logMessage: {
    gridColumn: '2 / 3',
    fontWeight: 600,
  },
  logMeta: {
    gridColumn: '2 / 3',
    color: '#aaa',
    fontSize: '12px',
  },
};

import { useState } from 'react';
import MazeMap from '../components/MazeMap';
import useTelemetry, { STATUS } from '../hooks/useTelemetry';

const TAMANHOS = [4, 8, 16];

const STATUS_INFO = {
  [STATUS.conectando]: { texto: 'Conectando…', cor: '#b58900' },
  [STATUS.conectado]: { texto: 'Conectado', cor: '#2e7d32' },
  [STATUS.desconectado]: { texto: 'Desconectado', cor: '#c62828' },
  [STATUS.indisponivel]: { texto: 'WebSocket indisponível', cor: '#888' },
};

export default function Telemetry() {
  const [size, setSize] = useState(4);
  const t = useTelemetry(size);
  const statusInfo = STATUS_INFO[t.status] ?? STATUS_INFO[STATUS.desconectado];

  return (
    <div>
      <div style={styles.titleRow}>
        <h1>Painel de Controle</h1>
        <span style={{ ...styles.statusPill, backgroundColor: statusInfo.cor }}>
          ● {statusInfo.texto}
        </span>
      </div>

      <div style={styles.controls}>
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

      <div style={styles.dashboard}>
        {/* Painel de métricas */}
        <div style={styles.dataPanel}>
          <h3>Status da Corrida</h3>
          <div style={styles.statBox}>
            <strong>Velocidade Média:</strong>{' '}
            {t.velocidadeMedia != null ? `${t.velocidadeMedia.toFixed(2)} m/s` : '-- m/s'}
          </div>
          <div style={styles.statBox}>
            <strong>Tempo:</strong> {t.tempo}
          </div>
          <div style={styles.statBox}>
            <strong>Bateria:</strong> {t.bateria != null ? `${Math.round(t.bateria)}%` : '--%'} 🔋
          </div>
          <div style={styles.statBox}>
            <strong>Desafio Cumprido:</strong>{' '}
            <span style={{ color: t.desafioCumprido ? '#2e7d32' : '#c62828', fontWeight: 'bold' }}>
              {t.desafioCumprido ? 'Sim' : 'Não'}
            </span>
          </div>
          <div style={styles.statBox}>
            <strong>Posição:</strong>{' '}
            {t.posicaoAtual ? `(${t.posicaoAtual.x}, ${t.posicaoAtual.y})` : '--'}
          </div>
        </div>

        {/* Mapa do labirinto com o trajeto em tempo real */}
        <div style={styles.mapPanel}>
          <MazeMap size={size} trajeto={t.trajeto} posicaoAtual={t.posicaoAtual} />
        </div>
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
};

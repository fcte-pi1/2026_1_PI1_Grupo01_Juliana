import { useState } from 'react';
import MazeMap from '../components/MazeMap';
import useTelemetry, { STATUS } from '../hooks/useTelemetry';

const TAMANHOS = [4, 8, 16];

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
        </div>

        <div className="map-panel">
          <MazeMap size={size} trajeto={t.trajeto} posicaoAtual={t.posicaoAtual} />
        </div>
      </div>
    </div>
  );
}
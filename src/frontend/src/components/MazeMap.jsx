import { ehChegada } from '../utils/maze';

const CORES = {
  posicao: '#2563eb', // var(--primary)
  chegada: '#10b981', // var(--success)
  inicio: '#f59e0b',  // var(--warning)
  trajeto: '#bfdbfe', 
  vazia: 'white',
};

// Desenha a grade NxN do labirinto e sobrepõe o trajeto percorrido, a posição
// atual do micromouse e a célula de chegada. A célula i (ordem row-major) mapeia
// para x = coluna (i % size) e y = linha (i / size), com a origem no topo-esquerda.
export default function MazeMap({ size = 4, trajeto = [], posicaoAtual = null }) {
  const totalCells = size * size;
  const visitadas = new Set(trajeto.map((p) => `${p.x},${p.y}`));
  const inicio = trajeto[0] ?? null;

  function corDaCelula(x, y) {
    if (posicaoAtual && posicaoAtual.x === x && posicaoAtual.y === y) return CORES.posicao;
    if (ehChegada(x, y, size)) return CORES.chegada;
    if (inicio && inicio.x === x && inicio.y === y) return CORES.inicio;
    if (visitadas.has(`${x},${y}`)) return CORES.trajeto;
    return CORES.vazia;
  }

  return (
    <div className="data-panel" style={{ padding: '1rem' }}>
      <h3 style={{ margin: '0 0 0.5rem', color: 'var(--header-bg)' }}>Mapa do Labirinto ({size}x{size})</h3>
      {trajeto.length === 0 && (
        <p style={{ fontSize: '14px', color: 'var(--text-muted)', marginBottom: '1rem' }}>
          Aguardando dados de telemetria...
        </p>
      )}

      <div
        role="grid"
        aria-label={`Labirinto ${size}x${size}`}
        style={{ ...styles.grid, gridTemplateColumns: `repeat(${size}, 1fr)` }}
      >
        {Array.from({ length: totalCells }).map((_, i) => {
          const x = i % size;
          const y = Math.floor(i / size);
          return <div key={i} style={{ ...styles.cell, backgroundColor: corDaCelula(x, y) }} />;
        })}
      </div>

      <div style={styles.legenda}>
        <ItemLegenda cor={CORES.inicio} texto="Início" />
        <ItemLegenda cor={CORES.trajeto} texto="Trajeto" />
        <ItemLegenda cor={CORES.posicao} texto="Posição atual" />
        <ItemLegenda cor={CORES.chegada} texto="Chegada" />
      </div>
    </div>
  );
}

function ItemLegenda({ cor, texto }) {
  return (
    <span style={styles.legendaItem}>
      <span style={{ ...styles.legendaCor, backgroundColor: cor }} />
      {texto}
    </span>
  );
}

const styles = {
  grid: {
    display: 'grid',
    gap: '2px',
    width: '100%',
    maxWidth: '400px',
    aspectRatio: '1 / 1',
    margin: '0 auto 1rem',
    backgroundColor: 'var(--border)',
    border: '2px solid var(--border)',
    borderRadius: '4px',
    overflow: 'hidden'
  },
  cell: {
    backgroundColor: 'white',
  },
  legenda: {
    display: 'flex',
    flexWrap: 'wrap',
    gap: '12px',
    fontSize: '0.85rem',
    color: 'var(--text-muted)',
    justifyContent: 'center'
  },
  legendaItem: { display: 'inline-flex', alignItems: 'center', gap: '6px' },
  legendaCor: {
    width: '12px',
    height: '12px',
    borderRadius: '2px',
    display: 'inline-block',
  },
};
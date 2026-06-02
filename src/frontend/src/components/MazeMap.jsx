import { ehChegada } from '../utils/maze';

const CORES = {
  posicao: '#1565c0', // posição atual do robô
  chegada: '#c8e6c9', // bloco 2x2 central (objetivo)
  inicio: '#ffe0b2', // primeira célula do trajeto
  trajeto: '#bbdefb', // células já visitadas
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
    <div style={styles.container}>
      <h3 style={styles.titulo}>Mapa do Labirinto ({size}x{size})</h3>
      {trajeto.length === 0 && (
        <p style={styles.aviso}>Aguardando dados de telemetria do ESP32…</p>
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
  container: {
    width: '100%',
    padding: '1rem',
    border: '1px solid #ddd',
    borderRadius: '8px',
    backgroundColor: '#fafafa',
  },
  titulo: { margin: '0 0 0.25rem', color: '#444' },
  aviso: { fontSize: '14px', color: '#999', marginTop: 0 },
  grid: {
    display: 'grid',
    gap: '2px',
    width: 'min(360px, 100%)',
    aspectRatio: '1 / 1',
    margin: '0.5rem 0',
    backgroundColor: '#999',
    border: '2px solid #999',
  },
  cell: {
    border: '1px solid #e0e0e0',
  },
  legenda: {
    display: 'flex',
    flexWrap: 'wrap',
    gap: '12px',
    fontSize: '13px',
    color: '#555',
  },
  legendaItem: { display: 'inline-flex', alignItems: 'center', gap: '4px' },
  legendaCor: {
    width: '14px',
    height: '14px',
    borderRadius: '3px',
    border: '1px solid #ccc',
    display: 'inline-block',
  },
};

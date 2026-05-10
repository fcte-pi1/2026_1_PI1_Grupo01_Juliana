export default function MazeMap({ size = 4 }) {
  const totalCells = size * size;

  return (
    <div style={styles.container}>
      <h3 style={{ margin: 0, color: '#666' }}>Mapa do Labirinto ({size}x{size})</h3>
      <p style={{ fontSize: '14px', color: '#999' }}>Aguardando dados de mapeamento do ESP32...</p>
      
      {/* Grid dinâmico baseado na prop 'size' */}
      <div style={{
        ...styles.grid,
        gridTemplateColumns: `repeat(${size}, 1fr)`
      }}>
        {Array.from({ length: totalCells }).map((_, i) => (
          <div key={i} style={styles.cell}></div>
        ))}
      </div>
    </div>
  );
}

const styles = {
  container: {
    width: '100%',
    height: '400px',
    border: '2px dashed #ccc',
    display: 'flex',
    flexDirection: 'column',
    alignItems: 'center',
    justifyContent: 'center',
    backgroundColor: '#f9f9f9',
    borderRadius: '8px'
  },
  grid: {
    display: 'grid',
    gridTemplateColumns: 'repeat(4, 1fr)',
    gap: '2px',
    marginTop: '20px',
    width: '250px',
    height: '150px',
    backgroundColor: '#ddd',
    border: '2px solid #999'
  },
  cell: {
    backgroundColor: 'white'
  }
};
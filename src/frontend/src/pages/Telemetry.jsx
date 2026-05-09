import MazeMap from '../components/MazeMap';

export default function Telemetry() {
  return (
    <div>
      <h1>Painel de Controle</h1>
      <div style={styles.dashboard}>
        
        {/* Painel de Dados */}
        <div style={styles.dataPanel}>
          <h3>Status da Corrida</h3>
          <div style={styles.statBox}><strong>Velocidade Média:</strong> -- m/s</div>
          <div style={styles.statBox}><strong>Tempo:</strong> 00:00:00</div>
          <div style={styles.statBox}><strong>Bateria (HU05):</strong> 100% 🔋</div>
          <div style={styles.statBox}><strong>Status:</strong> Explorando...</div>
        </div>

        {/* Componente do Mapa */}
        <div style={styles.mapPanel}>
          <MazeMap />
        </div>

      </div>
    </div>
  );
}

const styles = {
  dashboard: {
    display: 'flex',
    gap: '2rem',
    marginTop: '1rem'
  },
  dataPanel: {
    flex: 1,
    padding: '1rem',
    backgroundColor: '#f4f4f4',
    borderRadius: '8px'
  },
  mapPanel: {
    flex: 2
  },
  statBox: {
    padding: '10px',
    borderBottom: '1px solid #ddd',
    fontSize: '16px'
  }
};
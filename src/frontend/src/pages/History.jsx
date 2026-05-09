export default function History() {
  return (
    <div>
      <h1>Histórico de Corridas</h1>
      <div style={{ marginBottom: '1rem' }}>
        <label>Filtrar por tamanho: </label>
        <select>
          <option value="todos">Todos</option>
          <option value="4x4">4x4</option>
          <option value="8x8">8x8</option>
          <option value="16x16">16x16</option>
        </select>
      </div>

      <table style={styles.table}>
        <thead>
          <tr style={styles.headerRow}>
            <th>ID da Corrida</th>
            <th>Tamanho</th>
            <th>Tempo Total</th>
            <th>Desafio Cumprido? (HU06)</th>
            <th>Ações</th>
          </tr>
        </thead>
        <tbody>
          <tr>
            <td colSpan="5" style={{ textAlign: 'center', padding: '1rem' }}>
              Nenhum dado recebido do banco (SQLite) ainda.
            </td>
          </tr>
        </tbody>
      </table>
    </div>
  );
}

const styles = {
  table: {
    width: '100%',
    borderCollapse: 'collapse',
    marginTop: '1rem'
  },
  headerRow: {
    backgroundColor: '#282c34',
    color: 'white',
    textAlign: 'left',
  }
};
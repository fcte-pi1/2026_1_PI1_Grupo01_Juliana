import { Outlet, Link, useLocation } from 'react-router-dom';

export default function Layout() {
  const location = useLocation();

  return (
    <div>
      <header style={styles.header}>
        <h2>Micromouse Telemetria</h2>
        <nav style={styles.nav}>
          <Link 
            to="/" 
            style={location.pathname === '/' ? styles.activeLink : styles.link}
          >
            Telemetria ao Vivo
          </Link>
          <Link 
            to="/historico" 
            style={location.pathname === '/historico' ? styles.activeLink : styles.link}
          >
            Histórico
          </Link>
        </nav>
      </header>
      
      <main style={styles.main}>
        {/* O Outlet renderiza a página atual baseada na rota */}
        <Outlet /> 
      </main>
    </div>
  );
}

const styles = {
  header: {
    display: 'flex',
    justifyContent: 'space-between',
    alignItems: 'center',
    padding: '1rem 2rem',
    backgroundColor: '#282c34',
    color: 'white'
  },
  nav: {
    display: 'flex',
    gap: '20px'
  },
  link: {
    color: '#ccc',
    textDecoration: 'none',
    fontWeight: 'bold'
  },
  activeLink: {
    color: '#61dafb',
    textDecoration: 'none',
    fontWeight: 'bold',
    borderBottom: '2px solid #61dafb'
  },
  main: {
    padding: '2rem'
  }
};
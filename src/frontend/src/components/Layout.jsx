import { Outlet, Link, useLocation } from 'react-router-dom';

export default function Layout() {
  const location = useLocation();

  return (
    <div>
      <header className="layout-header">
        <h2>Micromouse Telemetria</h2>
        <nav className="nav-links">
          <Link 
            to="/" 
            className={`nav-link ${location.pathname === '/' ? 'active' : ''}`}
          >
            Telemetria ao Vivo
          </Link>
          <Link 
            to="/historico" 
            className={`nav-link ${location.pathname === '/historico' ? 'active' : ''}`}
          >
            Histórico
          </Link>
        </nav>
      </header>
      
      <main className="main-content">
        <Outlet /> 
      </main>
    </div>
  );
}
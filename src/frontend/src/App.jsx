import { BrowserRouter, Routes, Route } from 'react-router-dom';
import Layout from './components/Layout';
import Telemetry from './pages/Telemetry';
import History from './pages/History';

function App() {
  return (
    <BrowserRouter>
      <Routes>
        <Route path="/" element={<Layout />}>
          {/* A rota index renderiza a Telemetria por padrão */}
          <Route index element={<Telemetry />} />
          <Route path="historico" element={<History />} />
        </Route>
      </Routes>
    </BrowserRouter>
  );
}

export default App;
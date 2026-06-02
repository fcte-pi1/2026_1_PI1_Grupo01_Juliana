import { useEffect, useState, useCallback } from 'react';

// URL base da API REST. Configure VITE_API_URL no .env para apontar ao backend.
// Ex.: VITE_API_URL=http://localhost:8000
const API_URL =
  import.meta.env?.VITE_API_URL ??
  `http://${typeof window !== 'undefined' ? window.location.hostname : 'localhost'}:8000`;

// Busca o histórico de corridas no endpoint GET /corridas.
// `tamanho` pode ser '4x4', '8x8', '16x16' ou 'todos' (sem filtro).
export default function useHistory(tamanho) {
  const [corridas, setCorridas] = useState([]);
  const [carregando, setCarregando] = useState(true);
  const [erro, setErro] = useState(null);

  const buscar = useCallback(async () => {
    setCarregando(true);
    setErro(null);

    try {
      const params = new URLSearchParams();
      if (tamanho !== 'todos') params.set('tamanho', tamanho);

      const resposta = await fetch(`${API_URL}/corridas?${params.toString()}`);

      if (!resposta.ok) {
        throw new Error(`Erro ${resposta.status}: ${resposta.statusText}`);
      }

      const dados = await resposta.json();
      setCorridas(dados);
    } catch (err) {
      setErro(err.message ?? 'Erro desconhecido ao carregar histórico.');
      setCorridas([]);
    } finally {
      setCarregando(false);
    }
  }, [tamanho]);

  useEffect(() => {
    buscar();
  }, [buscar]);

  return { corridas, carregando, erro, recarregar: buscar };
}
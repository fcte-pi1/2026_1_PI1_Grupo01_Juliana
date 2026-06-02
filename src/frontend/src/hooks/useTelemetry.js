import { useEffect, useMemo, useRef, useState } from 'react';
import { ehChegada } from '../utils/maze';

// URL do WebSocket de telemetria. Em integração/produção, aponte para a API
// FastAPI definindo VITE_WS_URL (ex.: ws://localhost:8000/ws/telemetria).
const WS_URL =
  import.meta.env?.VITE_WS_URL ??
  `ws://${typeof window !== 'undefined' ? window.location.hostname : 'localhost'}:8000/ws/telemetria`;

const RECONEXAO_MS = 2000;

export const STATUS = {
  conectando: 'conectando',
  conectado: 'conectado',
  desconectado: 'desconectado',
  indisponivel: 'indisponivel', // ambiente sem WebSocket (ex.: testes em jsdom)
};

function formatarTempo(segundos) {
  const s = Math.max(0, Math.floor(segundos));
  const hh = String(Math.floor(s / 3600)).padStart(2, '0');
  const mm = String(Math.floor((s % 3600) / 60)).padStart(2, '0');
  const ss = String(s % 60).padStart(2, '0');
  return `${hh}:${mm}:${ss}`;
}

// Consome o WebSocket /ws/telemetria como observador passivo: apenas recebe os
// eventos transmitidos pelo backend e deriva as métricas exibidas no painel.
// `size` é o tamanho do labirinto escolhido na UI e afeta apenas o cálculo de
// "desafio cumprido" (não reabre a conexão).
export default function useTelemetry(size) {
  // O estado inicial já reflete a ausência de WebSocket (ex.: jsdom nos testes),
  // evitando um setState síncrono dentro do efeito.
  const [status, setStatus] = useState(() =>
    typeof WebSocket === 'undefined' ? STATUS.indisponivel : STATUS.conectando
  );
  const [eventos, setEventos] = useState([]);

  const wsRef = useRef(null);
  const reconexaoRef = useRef(null);
  const montadoRef = useRef(true);

  useEffect(() => {
    montadoRef.current = true;

    if (typeof WebSocket === 'undefined') {
      return () => {
        montadoRef.current = false;
      };
    }

    function agendarReconexao() {
      if (!montadoRef.current) return;
      clearTimeout(reconexaoRef.current);
      reconexaoRef.current = setTimeout(() => {
        setStatus(STATUS.conectando);
        conectar();
      }, RECONEXAO_MS);
    }

    function conectar() {
      if (!montadoRef.current) return;

      let ws;
      try {
        ws = new WebSocket(WS_URL);
      } catch {
        agendarReconexao();
        return;
      }
      wsRef.current = ws;

      ws.onopen = () => setStatus(STATUS.conectado);

      ws.onmessage = (ev) => {
        try {
          const dado = JSON.parse(ev.data);
          // Ignora mensagens de erro do backend ({ erro, ... }); só aceita eventos
          // de telemetria com posição válida.
          if (typeof dado.posicao_x === 'number' && typeof dado.posicao_y === 'number') {
            setEventos((prev) => [...prev, dado]);
          }
        } catch {
          // mensagem não-JSON: ignora
        }
      };

      ws.onerror = () => ws.close();

      ws.onclose = () => {
        if (!montadoRef.current) return;
        setStatus(STATUS.desconectado);
        agendarReconexao();
      };
    }

    conectar();

    return () => {
      montadoRef.current = false;
      clearTimeout(reconexaoRef.current);
      const ws = wsRef.current;
      if (ws) {
        ws.onclose = null; // evita disparar reconexão durante o unmount
        ws.onerror = null;
        ws.close();
      }
    };
  }, []);

  const derivado = useMemo(() => {
    const trajeto = eventos.map((e) => ({ x: e.posicao_x, y: e.posicao_y }));
    const ultimo = eventos[eventos.length - 1] ?? null;

    const velocidades = eventos
      .map((e) => e.velocidade)
      .filter((v) => typeof v === 'number');
    const velocidadeMedia = velocidades.length
      ? velocidades.reduce((a, b) => a + b, 0) / velocidades.length
      : null;

    let segundos = 0;
    if (eventos.length >= 2) {
      const inicio = Date.parse(eventos[0].timestamp);
      const fim = Date.parse(ultimo.timestamp);
      if (!Number.isNaN(inicio) && !Number.isNaN(fim)) {
        segundos = (fim - inicio) / 1000;
      }
    }

    return {
      trajeto,
      posicaoAtual: ultimo ? { x: ultimo.posicao_x, y: ultimo.posicao_y } : null,
      bateria: ultimo ? ultimo.nivel_bateria : null,
      velocidadeMedia,
      tempo: formatarTempo(segundos),
      desafioCumprido: trajeto.some((p) => ehChegada(p.x, p.y, size)),
      totalEventos: eventos.length,
    };
  }, [eventos, size]);

  return { status, ...derivado };
}

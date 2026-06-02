// Geometria do labirinto do micromouse.
// O objetivo do desafio é alcançar a "chegada", que é o bloco 2x2 no centro
// do labirinto NxN (convenção clássica do Micromouse). Funciona para 4x4,
// 8x8 e 16x16 porque o centro de um N par são as células [N/2-1, N/2].
export function ehChegada(x, y, size) {
  const inferior = size / 2 - 1;
  const superior = size / 2;
  return x >= inferior && x <= superior && y >= inferior && y <= superior;
}

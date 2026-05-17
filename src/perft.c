#include <stdio.h>
#include "chess.h"

/* --------------------------------------------------------- */
/* Recursive perft                                           */
/* --------------------------------------------------------- */
uint64_t perft(const int depth){
  if (depth == 0)
    return 1ULL;

  uint64_t nodes = 0;

  /*
   * Save move buffer position.
   * Critical for recursive safety.
   */
  int* saved_lmp = lmp;

  /*
   * Generate legal moves.
   */
  if (mantom == 0)
    gen_white_legal();
  else
    gen_black_legal();

  const int* p = saved_lmp;

  while (p < lmp - 2){
    p++; /* skip score */

    const int move = *p++;

    /*
     * Make move
     */
    if (mantom == 0)
      white_move(move);
    else
      black_move(move);

    mantom ^= 1;

    nodes += perft(depth - 1);

    mantom ^= 1;

    /*
     * Undo move
     */
    if (mantom == 0)
      white_undo();
    else
      black_undo();
  }

  /*
   * Restore move buffer.
   */
  lmp = saved_lmp;

  return nodes;
}

/* --------------------------------------------------------- */
/* Public entry                                              */
/* --------------------------------------------------------- */

uint64_t divide(const int depth){
  const long start_time =
    get_current_time_ms();

  uint64_t total = 0;

  int* saved_lmp = lmp;

  if (mantom == 0)
    gen_white_legal();
  else
    gen_black_legal();

  const int* p = saved_lmp;

  while (p < lmp - 2){
    p++; /* skip score */

    const int move = *p++;

    if (mantom == 0)
      white_move(move);
    else
      black_move(move);

    mantom ^= 1;

    const uint64_t nodes =
      perft(depth - 1);

    mantom ^= 1;

    if (mantom == 0)
      white_undo();
    else
      black_undo();

    char buf[8];

    move_to_uci(move,buf);

    printf("%s: %llu\n",buf,nodes);

    total += nodes;
  }

  lmp = saved_lmp;

  return total;
}

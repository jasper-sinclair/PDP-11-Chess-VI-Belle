/*
 * white_play / white_play1 / white_quiesce
 */
#include <stdio.h>
#include <string.h>
#include "chess.h"
/* External function for periodic updates */
/* External declarations for WinBoard integration */
/* ------------------------------------------------------------------ */
/* Internal: apply killer/history/PV scores to an already-generated   */
/* move list in lmbuf[p1..lmp-2].  Overwrites the score slot.         */
/* ------------------------------------------------------------------ */
static void rescore_moves(int* p1, const int cur_ply){
  int* p = p1;
  while (p < lmp - 2){
    const int move = *(p + 1);
    if ((unsigned)move == 0xfffffffc) break;
    *p = score_move_internal(move,cur_ply);
    p += 2;
  }
  /* Re-sort with updated scores (ascending = worst first for White) */
  const int n = (int)((lmp - 2 - p1) / 2);
  if (n > 1)
    qsort(p1,n,sizeof(int) * 2,compare_desc);
}

/* ------------------------------------------------------------------ */
/* white_play — root search (White is minimizer)                       */
/* Implements aspiration windows.                                      */
/* ------------------------------------------------------------------ */
int white_play(void){
  int v1, v2, *p2;

  if (value < ivalue) ivalue = value;

  int ab = 0;
  ply = 0;

  if (MAX_PV_LENGTH > 0) pv_length[0] = 0;
  search_pos_hash[0] = pos_fingerprint(); /* root position hash */

  int* p1 = static_eval(); /* generates + sorts moves */

  if (lmp == p1){
    abmove = 0;
    lmp = p1;
    return ivalue;
  }

  /* Rescore with PV / killer / history on top of static ordering */
  rescore_moves(p1,0);

  /* Aspiration: try window [prev-50 .. prev+50]; widen on failure.
   * On the first iteration (depth=1) use a full window.             */

  /* --- try tight window first ------------------------------------ */
  {
    const int prev_score = ivalue;
    int asp_lo = prev_score - 50;
    int asp_hi = prev_score + 50;
    int full = 0;

    /* For depth=1 or very first call always use full window */
    if (depth <= 1){
      asp_lo = -3000;
      asp_hi = 3000;
      full = 1;
    }

  retry:
    v1 = asp_hi; /* White minimizes: start at upper bound */
    ab = 0;
    {
      /* Track best non-repeating move to avoid draws in winning positions */
      int _norep_ab = 0, _norep_v = asp_hi;
      int _rep_ab = 0, _rep_v = asp_hi;

      p2 = p1; /* BUG-FIX: always reset move pointer on retry */
      while (p2 < lmp){
        const int move = *(p2 + 1);
        if ((unsigned)move == 0xfffffffc || move == -4) break;
        if (time_control_enabled && should_stop_search()) break;

        white_move(move);
        v2 = black_play1(asp_lo,v1);

        if (v2 < v1){
          ab = move;
          v1 = v2;
          /* Update root PV */
          if (MAX_PV_LENGTH > 0){
            int clen = (1 < MAX_PV_LENGTH)?pv_length[1]:0;
            if (clen > MAX_PV_LENGTH - 1) clen = MAX_PV_LENGTH - 1;
            pv_table[0][0] = move;
            if (clen > 0)
              memcpy(&pv_table[0][1],&pv_table[1][0],
                clen * sizeof(int));
            pv_length[0] = 1 + clen;
          }
          /* Track rep/norep — BUG-FIX: flip mantom so repetition()
           * calls white_undo() (not black_undo()) on the white move. */
          mantom = 1; /* Black's turn after white_move */
          {
            const int _r = (repetition() >= 1);
            if (_r){
              if (v2 < _rep_v){
                _rep_ab = move;
                _rep_v = v2;
              }
            } else{
              if (v2 < _norep_v){
                _norep_ab = move;
                _norep_v = v2;
              }
            }
          }
          mantom = 0; /* restore */
        }
        white_undo();
        p2 += 2;
        fflush(stdout);
      }

      /* Prefer non-repeating when winning (within 150 cp).
       * Prevents engine from shuffling into a draw in a won position. */
      if (_norep_ab && ab == _rep_ab && _norep_v <= v1 + 150 && v1 < -100){
        ab = _norep_ab;
        v1 = _norep_v;
      }
    } /* end norep tracking block */

    /* Aspiration failure — widen and retry */
    if (! full){
      if (v1 <= asp_lo || v1 >= asp_hi){
        if (asp_hi - asp_lo < 400){
          asp_lo = prev_score - 200;
          asp_hi = prev_score + 200;
        } else{
          asp_lo = -3000;
          asp_hi = 3000;
          full = 1;
        }
        pv_length[0] = 0;
        goto retry;
      }
    }
  }

  abmove = ab;
  lmp = p1;
  return v1;
}

/* ------------------------------------------------------------------ */
/* white_play1 — internal White node                                  */
/* ------------------------------------------------------------------ */
int white_play1(const int alpha, const int beta){
  int v2, *p2;
  int extension = 0;

  wb_count_node();

  if (time_control_enabled && should_stop_search()) return 3000;
  if (ply >= 20) return 3000;

  /* Check for search extensions - ONLY if we're not at root */
  if (ply > 0 && ply < 19 && amp[-1] != -1){
    /* Get the move that brought us here - careful with bounds */
    if (amp >= ambuf + 4){
      const int last_move = (amp[-4] << 8) | (amp[-3] & 0xFF);

      /* Check extension: extend when giving check */
      if (move_gives_check(last_move)){
        extension = 1;
      }
      /* Capture extension: extend captures of valuable pieces */
      else if (capture_worth_extending(last_move,depth - ply)){
        extension = 1;
      }
    }
  }

  /* Apply extension - if extended beyond depth, use quiescence */
  const int effective_depth = depth - ply - extension;
  if (effective_depth <= 0){
    return white_quiesce(alpha,beta);
  }

  const int saved_mantom = mantom;
  mantom = 0;
  ply++;

  if (ply < MAX_PV_LENGTH) pv_length[ply] = 0;

  /* ── SEARCH PATH REPETITION DETECTION ────────────────────────── */
  if (ply < MAX_SEARCH_PLY){
    const unsigned int _h = pos_fingerprint();
    search_pos_hash[ply] = _h;
    {
      for (int _k = ply - 2; _k >= 0; _k -= 2){
        if (search_pos_hash[_k] == _h){
          ply--;
          mantom = saved_mantom;
          return 0;
        }
      }
    }
  }

  /* ── NULL MOVE PRUNING ────────────────────────────────────────── */
  if (! in_null_move && effective_depth >= 3 && ! in_check()){
    int _hasp = 0;
    {
      for (int _s = 0; _s < 64; _s++){
        const int _p = board[_s];
        if (_p < -1 && _p > -6){
          _hasp = 1;
          break;
        }
      }
    }
    if (_hasp){
      const int r = (effective_depth >= 6)?3:2;
      const int ep = eppos;
      eppos = 64;
      in_null_move = 1;
      ply += r - 1;
      {
        const int _ns = black_play1(alpha,alpha + 1);
        ply -= r - 1;
        in_null_move = 0;
        eppos = ep;
        if (_ns <= alpha){
          ply--;
          mantom = saved_mantom;
          return alpha;
        }
      }
    }
  }

  int* p1 = p2 = lmp;
  gen_white_moves();
  const int* sentinel_pos = lmp - 2;

  /* Rescore with PV / killer / history */
  rescore_moves(p1,ply);
  sentinel_pos = lmp - 2;

  int v1 = 3000;

  while (p2 < sentinel_pos){
    const int move = *(p2 + 1);
    if ((unsigned)move == 0xfffffffc) break;

    white_move(move);
    if (! black_attacks(wkpos)){
      v2 = black_play1(alpha,v1);

      if (v2 < v1){
        v1 = v2;

        /* Update PV */
        if (ply < MAX_PV_LENGTH){
          const int nxt = ply + 1;
          int clen = (nxt < MAX_PV_LENGTH)?pv_length[nxt]:0;
          if (clen > MAX_PV_LENGTH - ply - 1)
            clen = MAX_PV_LENGTH - ply - 1;
          pv_table[ply][0] = move;
          if (clen > 0)
            memcpy(&pv_table[ply][1],&pv_table[nxt][0],
              clen * sizeof(int));
          pv_length[ply] = 1 + clen;
        }

        if (v1 <= alpha){
          const int was_quiet = (amp[-2] == 0);
          if (was_quiet){
            store_killer(ply,move);
            const int from2 = (move >> 8) & 0xFF;
            const int to2 = move & 0xFF;
            if (from2 >= 0 && from2 < 64 && to2 >= 0 && to2 < 64){
              history[from2][to2] += depth - ply;
              if (history[from2][to2] > 500000)
                history[from2][to2] /= 2;
            }
          }
          white_undo();
          goto out;
        }
      }
    }
    white_undo();
    p2 += 2;
  }

out:
  ply--;
  lmp = p1;
  if (v1 == 3000){
    if (in_check()) v1 = 2990 + ply;
    else v1 = 0;
  }
  mantom = saved_mantom;
  return v1;
}

/* ------------------------------------------------------------------ */
/* white_quiesce                                                      */
/* ------------------------------------------------------------------ */
int white_quiesce(const int alpha, int beta){
  int *p2, *p3, v2;

  if (time_control_enabled && should_stop_search()) return value;
  if (ply >= qdepth) return value;

  if (ply < MAX_PV_LENGTH) pv_length[ply] = 0;

  /* Stand-pat */
  int v1 = value + positional_adjustment();
  if (v1 <= alpha) return v1;
  if (v1 < beta) beta = v1;

  const int saved_mantom = mantom;
  mantom = 0;

  int* p1 = p2 = p3 = lmp;
  gen_white_moves();

  /* Filter & MVV-LVA sort captures */
  while (p2 < lmp - 2){
    const int move = *(p2 + 1);
    const int target_sq = move & 0xFF;
    const int captured = board[target_sq];
    if (captured != 0){
      /* Delta pruning: skip if even a free capture can't raise stand-pat */
      const int gain = abs_val(ipval[6 + captured]);
      if (v1 - gain - 20 > beta){
        p2 += 2;
        continue;
      }
      /* SEE pruning: skip clearly losing captures (threshold -100 cp) */
      const int from_sq = (move >> 8) & 0xFF;
      if (see(from_sq,target_sq) < -100){
        p2 += 2;
        continue;
      }
      *p3++ = -(*p2);
      *p3++ = move;
    }
    p2 += 2;
  }

  if (p3 == p1){
    lmp = p1;
    mantom = saved_mantom;
    return v1;
  }

  const int nc = (p3 - p1) / 2;
  if (nc > 1) qsort(p1,nc,sizeof(int) * 2,compare_int);

  ply++;
  if (ply < MAX_PV_LENGTH) pv_length[ply] = 0;
  lmp = p3;
  p2 = p1;

  while (p2 < lmp){
    const int move = *(p2 + 1);
    white_move(move);
    if (! black_attacks(wkpos)){
      v2 = black_quiesce(alpha,beta);
      if (v2 < v1){
        v1 = v2;
        if (v1 < beta) beta = v1;
        if (v1 <= alpha){
          white_undo();
          goto q_out;
        }
      }
    }
    white_undo();
    p2 += 2;
  }

q_out:
  ply--;
  lmp = p1;
  mantom = saved_mantom;
  return v1;
}

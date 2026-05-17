/*
 * black_play / black_play1 / black_quiesce
 */
#include <stdio.h>
#include <string.h>
#include "chess.h"
//#define NULL_MOVE             // use null move heuristic

/* ------------------------------------------------------------------ */
/* Internal: rescore a generated move list for the current ply         */
/* ------------------------------------------------------------------ */
static void rescore_moves_b(int* p1, const int cur_ply){
  int* p = p1;
  while (p < lmp - 2){
    const int move = *(p + 1);
    if ((unsigned)move == 0xfffffffc) break;
    *p = score_move_internal(move,cur_ply);
    p += 2;
  }
  const int n = (int)((lmp - 2 - p1) / 2);
  if (n > 1)
    qsort(p1,n,sizeof(int) * 2,compare_desc);
}

/* ------------------------------------------------------------------ */
/* black_play — root search (Black is maximizer)                       */
/* ------------------------------------------------------------------ */
int black_play(void){
  int v1, v2, *p2;

  if (value > ivalue) ivalue = value;

  int ab = 0;
  ply = 0;

  if (MAX_PV_LENGTH > 0) pv_length[0] = 0;
  search_pos_hash[0] = pos_fingerprint(); /* root position hash */

  int* p1 = static_eval();

  if (lmp == p1){
    abmove = 0;
    lmp = p1;
    return ivalue;
  }

  rescore_moves_b(p1,0);

  {
    const int prev_score = ivalue;
    int asp_lo = prev_score - 50;
    int asp_hi = prev_score + 50;
    int full = 0;

    if (depth <= 1){
      asp_lo = -3000;
      asp_hi = 3000;
      full = 1;
    }

  retry:
    v1 = asp_lo; /* Black maximizes: start at lower bound */
    ab = 0;
    {
      int _norep_ab = 0, _norep_v = asp_lo;
      int _rep_ab = 0, _rep_v = asp_lo;

      p2 = p1; /* always reset move pointer on retry */
      while (p2 < lmp){
        const int move = *(p2 + 1);
        if ((unsigned)move == 0xfffffffc || move == -4) break;
        if (time_control_enabled && should_stop_search()) break;

        black_move(move);
        v2 = white_play1(v1,asp_hi);

        if (v2 > v1){
          ab = move;
          v1 = v2;
          if (MAX_PV_LENGTH > 0){
            int clen = (1 < MAX_PV_LENGTH)?pv_length[1]:0;
            if (clen > MAX_PV_LENGTH - 1) clen = MAX_PV_LENGTH - 1;
            pv_table[0][0] = move;
            if (clen > 0)
              memcpy(&pv_table[0][1],&pv_table[1][0],
                clen * sizeof(int));
            pv_length[0] = 1 + clen;
          }
          /* Track rep/norep — : after black_move it is White's
           * turn, so mantom must be 0 when repetition() walks backward. */
          mantom = 0;
          {
            const int _r = (repetition() >= 1);
            if (_r){
              if (v2 > _rep_v){
                _rep_ab = move;
                _rep_v = v2;
              }
            } else{
              if (v2 > _norep_v){
                _norep_ab = move;
                _norep_v = v2;
              }
            }
          }
          mantom = 1; /* restore */
        }
        black_undo();
        p2 += 2;
        fflush(stdout);
      }

      /* Prefer non-repeating move when winning (within 150 cp margin) */
      if (_norep_ab && ab == _rep_ab && _norep_v >= v1 - 150 && v1 > 100){
        ab = _norep_ab;
        v1 = _norep_v;
      }
    } /* end norep tracking block */

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
/* black_play1 — internal Black node                                   */
/* ------------------------------------------------------------------ */
int black_play1(const int alpha, const int beta){
  int v1, v2, *p1, *p2;
  int* sentinel_pos;
  int extension = 0;

  wb_count_node();

  if (time_control_enabled && should_stop_search()) return -3000;
  if (ply >= 20) return -3000;

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
    return black_quiesce(alpha,beta);
  }

  const int saved_mantom = mantom;
  mantom = 1;
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

  #ifdef NULL_MOVE       // need fixing. weaker play when activated
  /* ── NULL MOVE PRUNING ────────────────────────────────────────── */
  if (! in_null_move && effective_depth >= 3 && ! in_check()){
    int _hasp = 0;
    {
      int _s;
      for (_s = 0; _s < 64; _s++){
        int _p = board[_s];
        if (_p > 1 && _p < 6){
          _hasp = 1;
          break;
        }
      }
    }
    if (_hasp){
      int _R = (effective_depth >= 6)?3:2;
      int _ep = eppos;
      eppos = 64;
      in_null_move = 1;
      ply += _R - 1;
      {
        int _ns = white_play1(beta - 1,beta);
        ply -= _R - 1;
        in_null_move = 0;
        eppos = _ep;
        if (_ns >= beta){
          ply--;
          mantom = saved_mantom;
          return beta;
        }
      }
    }
  }
  #endif

  p1 = p2 = lmp;
  gen_black_moves();
  sentinel_pos = lmp - 2;

  /* Rescore with PV / killer / history */
  rescore_moves_b(p1,ply);
  sentinel_pos = lmp - 2;

  v1 = -3000;

  while (p2 < sentinel_pos){
    const int move = *(p2 + 1);
    if ((unsigned)move == 0xfffffffc) break;

    black_move(move);
    if (! white_attacks(bkpos)){
      v2 = white_play1(v1,beta);

      if (v2 > v1){
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

        if (v1 >= beta){
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
          black_undo();
          goto out;
        }
      }
    }
    black_undo();
    p2 += 2;
  }

out:
  ply--;
  lmp = p1;
  if (v1 == -3000){
    if (in_check()) v1 = -2990 - ply;
    else v1 = 0;
  }
  mantom = saved_mantom;
  return v1;
}

/* ------------------------------------------------------------------ */
/* black_quiesce                                                        */
/* ------------------------------------------------------------------ */
int black_quiesce(int alpha, const int beta){
  int *p2, *p3, v2;

  if (time_control_enabled && should_stop_search()) return value;
  if (ply >= qdepth) return value;

  if (ply < MAX_PV_LENGTH) pv_length[ply] = 0;

  int v1 = value + positional_adjustment();
  if (v1 >= beta) return v1;
  if (v1 > alpha) alpha = v1;

  const int saved_mantom = mantom;
  mantom = 1;

  int* p1 = p2 = p3 = lmp;
  gen_black_moves();

  while (p2 < lmp - 2){
    const int move = *(p2 + 1);
    const int target_sq = move & 0xFF;
    const int captured = board[target_sq];
    if (captured != 0){
      const int gain = abs_val(ipval[6 + captured]);
      if (v1 + gain + 20 < alpha){
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
    black_move(move);
    if (! white_attacks(bkpos)){
      v2 = white_quiesce(alpha,beta);
      if (v2 > v1){
        v1 = v2;
        if (v1 > alpha) alpha = v1;
        if (v1 >= beta){
          black_undo();
          goto q_out;
        }
      }
    }
    black_undo();
    p2 += 2;
  }

q_out:
  ply--;
  lmp = p1;
  mantom = saved_mantom;
  return v1;
}

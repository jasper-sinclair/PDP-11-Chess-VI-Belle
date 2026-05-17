#include <stdio.h>
#include <stdlib.h>
#include "chess.h"
/*
 * gen.c — move generation
 */
const int knight_offsets[8] = {-15,-6,10,17,15,6,-10,-17};
const int king_offsets[8] = {-9,-7,7,9,-8,-1,1,8};
const int bishop_offsets[4] = {-9,-7,7,9};
const int rook_offsets[4] = {-8,-1,1,8};
const int queen_offsets[8] = {-9,-7,7,9,-8,-1,1,8};
const int knight_masks[8] = {u2r1, u1r2, d1r2, d2r1, d2l1, d1l2, u1l2, u2l1};
const int dir_masks[8] = {uleft, uright, dleft, dright, up, left, right, down};
const int dir_offsets[8] = {-9,-7,7,9,-8,-1,1,8};
/* ------------------------------------------------------------------ */
/*  add_move                                                            */
/* ------------------------------------------------------------------ */
void add_move(const int from, const int to, const int score){
  if (lmp < lmbuf + 3998){
    *lmp++ = score;
    *lmp++ = (from << 8) | to;
  }
}

/* ------------------------------------------------------------------ */
/*  gen_white_moves — pseudo-legal moves for white                     */
/* ------------------------------------------------------------------ */
void gen_white_moves(void){
  int to, i;

  for (int pos = 0; pos < 64; pos++){
    const int piece = board[pos];
    if (piece >= 0) continue; /* white pieces are negative */

    const int piece_type = -piece;

    switch (piece_type){
    case 1: /* White Pawn */
      to = pos - 8;
      if (to >= 0 && board[to] == 0){
        add_move(pos,to,value);
        if ((dir[pos] & rank2) && board[pos - 16] == 0)
          add_move(pos,pos - 16,value);
      }
      if ((dir[pos] & uleft) == 0){
        to = pos - 9;
        if (to >= 0){
          if (board[to] > 0)
            add_move(pos,to,value - ipval[6 + board[to]]);
          else if (to == eppos)
            add_move(pos,to,value);
        }
      }
      if ((dir[pos] & uright) == 0){
        to = pos - 7;
        if (to >= 0){
          if (board[to] > 0)
            add_move(pos,to,value - ipval[6 + board[to]]);
          else if (to == eppos)
            add_move(pos,to,value);
        }
      }
      break;

    case 2: /* Knight */
      for (i = 0; i < 8; i++){
        to = pos + knight_offsets[i];
        if (to >= 0 && to < 64 && (dir[pos] & knight_masks[i]) == 0){
          if (board[to] >= 0){
            const int score = (board[to] > 0)
              ?value - ipval[6 + board[to]]:value;
            add_move(pos,to,score);
          }
        }
      }
      break;

    case 3: /* Bishop */
    case 4: /* Rook  */
    case 5: /* Queen */
      {
        const int start_dir = (piece_type == 4)?4:0;
        const int end_dir = (piece_type == 3)?4:8;
        for (int d = start_dir; d < end_dir; d++){
          to = pos;
          while (1){
            if (dir[to] & dir_masks[d]) break;
            to += dir_offsets[d];
            if (to < 0 || to >= 64) break;
            if (board[to] < 0) break;
            if (board[to] > 0){
              add_move(pos,to,value - ipval[6 + board[to]]);
              break;
            }
            add_move(pos,to,value);
          }
        }
      }
      break;

    case 6: /* King */
      for (i = 0; i < 8; i++){
        to = pos + king_offsets[i];
        if (to >= 0 && to < 64 && (dir[pos] & dir_masks[i]) == 0){
          if (board[to] >= 0){
            const int score = (board[to] > 0)
              ?value - ipval[6 + board[to]]:value;
            add_move(pos,to,score);
          }
        }
      }
      break;
    default: ;
    }
  }

  /* Castling */
  if (wkpos == 60){
    if ((flag & WKCASTLE) && board[61] == 0 && board[62] == 0 && board[63] == -4){
      if (! black_attacks(60) && ! black_attacks(61) && ! black_attacks(62))
        add_move(60,62,value);
    }
    if ((flag & WQCASTLE) && board[59] == 0 && board[58] == 0 && board[57] == 0
      && board[56] == -4){
      if (! black_attacks(60) && ! black_attacks(59) && ! black_attacks(58))
        add_move(60,58,value);
    }
  }

  if (lmp < lmbuf + 3998){
    *lmp++ = 0;
    *lmp++ = 0xfffffffc;
  }
}

/* ------------------------------------------------------------------ */
/*  gen_black_moves — pseudo-legal moves for black                    */
/* ------------------------------------------------------------------ */
void gen_black_moves(void){
  int to, i;

  for (int pos = 0; pos < 64; pos++){
    const int piece = board[pos];
    if (piece <= 0) continue; /* black pieces are positive */

    switch (piece){
    case 1: /* Black Pawn */
      to = pos + 8;
      if (to < 64 && board[to] == 0){
        add_move(pos,to,-value);
        if ((dir[pos] & rank7) && board[pos + 16] == 0)
          add_move(pos,pos + 16,-value);
      }
      if ((dir[pos] & dleft) == 0){
        to = pos + 7;
        if (to < 64){
          if (board[to] < 0)
            add_move(pos,to,-value - ipval[6 + board[to]]);
          else if (to == eppos)
            add_move(pos,to,-value);
        }
      }
      if ((dir[pos] & dright) == 0){
        to = pos + 9;
        if (to < 64){
          if (board[to] < 0)
            add_move(pos,to,-value - ipval[6 + board[to]]);
          else if (to == eppos)
            add_move(pos,to,-value);
        }
      }
      break;

    case 2: /* Knight */
      for (i = 0; i < 8; i++){
        to = pos + knight_offsets[i];
        if (to >= 0 && to < 64 && (dir[pos] & knight_masks[i]) == 0){
          if (board[to] <= 0){
            const int score = (board[to] < 0)
              ?-value - ipval[6 + board[to]]:-value;
            add_move(pos,to,score);
          }
        }
      }
      break;

    case 3: /* Bishop */
    case 4: /* Rook  */
    case 5: /* Queen */
      {
        const int start_dir = (piece == 4)?4:0;
        const int end_dir = (piece == 3)?4:8;
        for (int d = start_dir; d < end_dir; d++){
          to = pos;
          while (1){
            if (dir[to] & dir_masks[d]) break;
            to += dir_offsets[d];
            if (to < 0 || to >= 64) break;
            if (board[to] > 0) break;
            if (board[to] < 0){
              add_move(pos,to,-value - ipval[6 + board[to]]);
              break;
            }
            add_move(pos,to,-value);
          }
        }
      }
      break;

    case 6: /* King */
      for (i = 0; i < 8; i++){
        to = pos + king_offsets[i];
        if (to >= 0 && to < 64 && (dir[pos] & dir_masks[i]) == 0){
          if (board[to] <= 0){
            const int score = (board[to] < 0)
              ?-value - ipval[6 + board[to]]:-value;
            add_move(pos,to,score);
          }
        }
      }
      break;
    default: ;
    }
  }

  /* Castling */
  if (bkpos == 4){
    if ((flag & BKCASTLE) && board[5] == 0 && board[6] == 0 && board[7] == 4){
      if (! white_attacks(4) && ! white_attacks(5) && ! white_attacks(6))
        add_move(4,6,-value);
    }
    if ((flag & BQCASTLE) && board[3] == 0 && board[2] == 0 && board[1] == 0
      && board[0] == 4){
      if (! white_attacks(4) && ! white_attacks(3) && ! white_attacks(2))
        add_move(4,2,-value);
    }
  }

  if (lmp < lmbuf + 3998){ /* FIX: was lmbuf + 1000 - 2 */
    *lmp++ = 0;
    *lmp++ = 0xfffffffc;
  }
}

/* ------------------------------------------------------------------ */
/*  Legal-move filters: make the move, verify king safety, undo        */
/* ------------------------------------------------------------------ */
void gen_white_legal(void){
  const int *p2 = lmp;
  int*start = lmp;
  gen_white_moves();
  const int* end = lmp - 2;
  lmp = start;

  while (p2 < end){
    const int v = *p2++;
    const int m = *p2++;
    const int from = (m >> 8) & 0x3F;
    const int to = m & 0x3F;
    const int cap = board[to];
    const int pce = board[from];
    const int owk = wkpos;

    board[to] = pce;
    board[from] = 0;
    if (pce == -6) wkpos = to;

    if (! black_attacks(wkpos))
      add_move(from,to,v);

    board[from] = pce;
    board[to] = cap;
    wkpos = owk;
  }
  *lmp++ = 0;
  *lmp++ = 0xfffffffc;
}

void gen_black_legal(void){
  const int *p2 = lmp;
  int*start = lmp;
  gen_black_moves();
  const int* end = lmp - 2;
  lmp = start;

  while (p2 < end){
    const int v = *p2++;
    const int m = *p2++;
    const int from = (m >> 8) & 0x3F;
    const int to = m & 0x3F;
    const int cap = board[to];
    const int pce = board[from];
    const int obk = bkpos;

    board[to] = pce;
    board[from] = 0;
    if (pce == 6) bkpos = to;

    if (! white_attacks(bkpos))
      add_move(from,to,v);

    board[from] = pce;
    board[to] = cap;
    bkpos = obk;
  }
  *lmp++ = 0;
  *lmp++ = 0xfffffffc;
}

/* ------------------------------------------------------------------ */
/*  Utility                                                             */
/* ------------------------------------------------------------------ */
int compare_int(const void* a, const void* b){
  const int s1 = *(int*)a, s2 = *(int*)b;
  if (mantom == 0) return (s1 > s2)?-1:(s1 < s2); // white: descending 
  return (s1 < s2)?-1:(s1 > s2); // black: ascending  
}

/* ------------------------------------------------------------------ */
/*  Killer / history utilities                                          */
/* ------------------------------------------------------------------ */
void store_killer(const int ply_n, const int move){
  if (ply_n < 0 || ply_n >= MAX_KILLER_PLY) return;
  /* Don't store captures as killers */
  const int to = move & 0xFF;
  if (board[to] != 0) return;
  if (killer_moves[ply_n][0] == move) return;
  killer_moves[ply_n][1] = killer_moves[ply_n][0];
  killer_moves[ply_n][0] = move;
}

void clear_search_tables(void){
  int i;
  for (i = 0; i < MAX_KILLER_PLY; i++)
    killer_moves[i][0] = killer_moves[i][1] = 0;
  for (i = 0; i < 64; i++)
    for (int j = 0; j < 64; j++)
      history[i][j] = 0;
  for (i = 0; i < MAX_PV_LENGTH; i++)
    pv_length[i] = 0;
}

/* ------------------------------------------------------------------ */
/*  score_move_internal — score a move for internal node ordering      */
/*  Returns a score: higher = try first.  Used by wplay1/bplay1.      */
/*                                                                      */
/*  Priority order:                                                     */
/*   1. PV move from last iteration   (+10000000)                      */
/*   2. Captures by MVV-LVA           (+1000000 + victim*100 - aggr)   */
/*   3. Killer moves                  (+900000 / +800000)               */
/*   4. History-scored quiet moves    (history[from][to])               */
/* ------------------------------------------------------------------ */
int score_move_internal(const int move, const int cur_ply){
  const int from = (move >> 8) & 0xFF;
  const int to = move & 0xFF;
  const int captured = board[to];

  /* PV move */
  if (cur_ply < MAX_PV_LENGTH && pv_length[cur_ply] > 0 &&
    pv_table[cur_ply][0] == move)
    return 10000000;

  /* Capture: score by SEE
   *   Winning/even capture (SEE >= 0): try before quiet moves       (+1000000 + see)
   *   Losing capture (SEE < 0):        try after quiet moves (killers) (-100000 + see)
   * This correctly orders e.g. BxR before NxP before RxP(defended). */
  if (captured != 0){
    const int sv = see(from,to);
    if (sv >= 0)
      return 1000000 + sv; /* winning / even: try early  */
    return -100000 + sv;
    /* losing: try after killers  */
  }

  /* Killers */
  if (cur_ply >= 0 && cur_ply < MAX_KILLER_PLY){
    if (killer_moves[cur_ply][0] == move) return 900000;
    if (killer_moves[cur_ply][1] == move) return 800000;
  }

  /* History */
  if (from >= 0 && from < 64 && to >= 0 && to < 64)
    return history[from][to];

  return 0;
}

/* Always-descending comparator: best (highest) score first.
 * Used by rescore_moves() after injecting PV/killer/history scores,
 * because those are all positive and we always want them tried first
 * regardless of which side is to move.                               */
int compare_desc(const void* a, const void* b){
  const int s1 = *(const int*)a;
  const int s2 = *(const int*)b;
  return (s1 < s2)?1:(s1 > s2)?-1:0;
}

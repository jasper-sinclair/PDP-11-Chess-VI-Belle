/*
 * see.c — Static Exchange Evaluator
 *
 * see(from, to) simulates the full capture/recapture sequence on square
 * 'to', each side using its least-valuable attacker in turn.  Returns the
 * expected net material change for the FIRST capturer:
 *
 *   > 0  winning exchange  (e.g. pawn takes undefended rook)
 *   = 0  even exchange     (e.g. rook × rook with equal defence)
 *   < 0  losing exchange   (e.g. rook takes pawn, pawn recaptures)
 *
 * Algorithm
 * ---------
 * We build pieces_taken[d] = value of what the d-th capturer takes, then
 * fold backward with the "can-stop" property: each recapturer (d >= 1)
 * only captures when profitable.  This directly mirrors the recursion:
 *
 *   see_rec(sq, side) = max(0, val_on_sq - see_rec(sq, opponent))
 *
 * (The first capture at d=0 is the one being evaluated and is always
 * committed, so no max(0,...) is applied there.)
 *
 * X-ray attacks are revealed by marking each consumed piece in skip[]
 * and re-scanning for unblocked sliders that were hidden behind it.
 *
 * Piece codes: negative = White, positive = Black.
 * Fixed SEE values (game-phase independent): P=100 N=300 B=325 R=500 Q=975
 */
#include <string.h>
#include "chess.h"
static const int SEE_VAL[7] = {0,100,300,325,500,975,20000};
/* ------------------------------------------------------------------ */
/* slide_path_clear: 1 if every square strictly between from and to   */
/* (on same rank/file/diagonal) is empty or in skip[].               */
/* ------------------------------------------------------------------ */
static int slide_path_clear(const int from, const int to, const int skip[64]){
  const int dr = (to >> 3) - (from >> 3);
  const int df = (to & 7) - (from & 7);
  const int sr = (dr > 0) - (dr < 0);
  const int sf = (df > 0) - (df < 0);
  int r = (from >> 3) + sr;
  int f = (from & 7) + sf;
  const int tr = to >> 3, tf = to & 7;

  while (r != tr || f != tf){
    if ((unsigned)r > 7u || (unsigned)f > 7u) return 0;
    const int sq = (r << 3) | f;
    if (! skip[sq] && board[sq] != 0) return 0;
    r += sr;
    f += sf;
  }
  return 1;
}

/* ------------------------------------------------------------------ */
/* find_lva: least-valuable attacker of 'sq' for the requested side.  */
/* is_white=1 -> find White (negative) piece; 0 -> Black (positive).  */
/* Returns square of attacker, or -1.                                  */
/* ------------------------------------------------------------------ */
static int find_lva(const int sq, const int is_white, const int skip[64]){
  int best_sq = -1;
  int best_val = 99999;
  const int r_to = sq >> 3, f_to = sq & 7;

  for (int pos = 0; pos < 64; pos++){
    if (skip[pos]) continue;
    const int piece = board[pos];
    if (piece == 0) continue;
    if (is_white && piece > 0) continue; /* skip Black pieces */
    if (! is_white && piece < 0) continue; /* skip White pieces */

    const int pt = piece < 0?-piece:piece;
    const int dr = r_to - (pos >> 3);
    const int df = f_to - (pos & 7);
    const int adr = dr < 0?-dr:dr;
    const int adf = df < 0?-df:df;
    int attacks = 0;

    switch (pt){
    case 1: /* Pawn */
      /* White pawns (is_white=1) capture toward smaller row idx (dr=-1).
       * Black pawns (is_white=0) capture toward larger  row idx (dr=+1). */
      attacks = is_white?(dr == -1 && adf == 1)
        :(dr == 1 && adf == 1);
      break;
    case 2: /* Knight */
      attacks = ((adr == 1 && adf == 2) || (adr == 2 && adf == 1));
      break;
    case 3: /* Bishop */
      attacks = (adr == adf && adr > 0) && slide_path_clear(pos,sq,skip);
      break;
    case 4: /* Rook */
      attacks = (dr == 0 || df == 0) && slide_path_clear(pos,sq,skip);
      break;
    case 5: /* Queen */
      attacks = ((adr == adf && adr > 0) || dr == 0 || df == 0)
        && slide_path_clear(pos,sq,skip);
      break;
    case 6: /* King */
      attacks = (adr <= 1 && adf <= 1 && (dr || df));
      break;
    default: ;
    }

    if (attacks){
      const int v = SEE_VAL[pt];
      if (v < best_val){
        best_val = v;
        best_sq = pos;
      }
    }
  }
  return best_sq;
}

/* ------------------------------------------------------------------ */
/* see                                                                  */
/* ------------------------------------------------------------------ */
int see(const int from, const int to){
  int pieces_taken[33]; /* pieces_taken[d] = value of what d-th capturer takes */
  int skip[64];
  int d = 0;

  memset(skip,0,sizeof skip);

  const int target = board[to];
  const int mover = board[from];
  if (target == 0 || mover == 0) return 0;

  /* d=0: first capturer (mover) takes target */
  pieces_taken[0] = SEE_VAL[target < 0?-target:target];

  /* After first capture, mover's piece sits on 'to' */
  int piece_on_sq = mover;
  skip[from] = 1;
  skip[to] = 1; /* target is captured — not a recapturer */

  /* Opponent of first capturer goes next:
   * mover is White (< 0) -> opponent is Black -> is_white_next = 0
   * mover is Black (> 0) -> opponent is White -> is_white_next = 1 */
  int is_white_next = (mover < 0)?0:1;

  while (d < 31){
    d++;
    const int att = find_lva(to,is_white_next,skip);
    if (att == -1){
      d--;
      break;
    }

    /* d-th capturer takes the piece currently on 'to' */
    pieces_taken[d] = SEE_VAL[piece_on_sq < 0?-piece_on_sq:piece_on_sq];

    /* Attacker's piece moves to 'to'; mark attacker as used */
    piece_on_sq = board[att];
    skip[att] = 1;
    is_white_next ^= 1;
  }

  /*
   * Backward pass: each recapturer (d >= 1) can choose NOT to capture.
   *
   * running = "how much will the opponent gain if they recapture from here?"
   *
   * At the last step the (last) capturer gains pieces_taken[last] for free
   * (no further response), so they always recapture: running = pieces_taken[last].
   *
   * Working upward: for step i, the i-th recapturer gains
   *   pieces_taken[i] - running  (what they take minus opponent's follow-up)
   * They recapture only if this is positive.
   * running = max(0, pieces_taken[i] - running)
   *
   * The FIRST capture (d=0) is already committed; its net gain is:
   *   pieces_taken[0] - running
   * (no clamping — may be negative if the exchange is losing).
   */
  int running = 0;
  for (int i = d; i >= 1; i--){
    const int gain = pieces_taken[i] - running;
    running = gain > 0?gain:0;
  }

  return pieces_taken[0] - running;
}

/* ------------------------------------------------------------------ */
/* see_ge: fast threshold test — 1 if see(from,to) >= threshold.      */
/* ------------------------------------------------------------------ */
int see_ge(const int from, const int to, const int threshold){
  return see(from,to) >= threshold;
}

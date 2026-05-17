/*
 * endgame.c — Endgame pattern recognition and evaluation
 *
 * Provides endgame_eval() which returns a positional adjustment (same
 * sign convention as positional_adjustment() in stat.c):
 *
 *   return > 0  →  bad for White (White is the minimizer)
 *   return < 0  →  bad for Black
 *
 * Patterns covered
 * ────────────────
 * 2-man  KK                     draw (insufficient material)
 * 3-man  KQK, KRK               mating: push lone king to corner
 *        KBK, KNK               draw  (flag engine)
 * 4-man  KQQK, KQRK, KRRK      mating: corner + closeness
 *        KQBK, KQNK, KRBK,
 *        KRNK, KBBK             mating: corner + closeness
 *        KBNK                   mating: corner matching bishop color
 *        KQKR, KQKB, KQKN      winning for the queen side
 *        KRKN, KRKB             slight edge for rook side
 *        KNNK                   draw
 *        KBPK                   pawn ending bonuses
 * General
 *        KPK (all pawns, no pieces)    king + passed-pawn bonuses
 *        Rook endgames (KR*KR*)        rook on 7th bonus
 *        Any endgame                   king centralisation/activation
 */
#include "chess.h"
/* ------------------------------------------------------------------ */
/* Helpers                                                              */
/* ------------------------------------------------------------------ */
/* Chebyshev (king-walk) distance */
static int cheb(const int a, const int b){
  int dr = (a >> 3) - (b >> 3);
  if (dr < 0) dr = -dr;
  int df = (a & 7) - (b & 7);
  if (df < 0) df = -df;
  return dr > df?dr:df;
}

/* Manhattan distance */
static int manh(const int a, const int b){
  int dr = (a >> 3) - (b >> 3);
  if (dr < 0) dr = -dr;
  int df = (a & 7) - (b & 7);
  if (df < 0) df = -df;
  return dr + df;
}

/* Distance from the nearest corner (0 = on a corner, 6 = dead centre) */
static int corner_dist(const int sq){
  const int r = sq >> 3, f = sq & 7;
  const int dr = r < 4?r:7 - r;
  const int df = f < 4?f:7 - f;
  return dr + df;
}

/*
 * Minimum Manhattan distance to a corner that matches 'bishop_color'.
 * bishop_color = (rank + file) & 1
 *   0 → light squares  → light corners: a8(0), h1(63)
 *   1 → dark  squares  → dark  corners: a1(56), h8(7)
 */
static int corner_dist_by_color(const int sq, const int bishop_color){
  static const int lc[2] = {0,63}; /* light corners */
  static const int dc[2] = {56,7}; /* dark  corners */
  const int* c = bishop_color?dc:lc;
  const int d0 = manh(sq,c[0]);
  const int d1 = manh(sq,c[1]);
  return d0 < d1?d0:d1;
}

/* Absolute value (avoid macro name clash) */
static int iabs(const int x){
  return x < 0?-x:x;
}

/* ------------------------------------------------------------------ */
/* Material counting                                                    */
/* ------------------------------------------------------------------ */
typedef struct{
  int wP, wN, wB, wR, wQ;
  int bP, bN, bB, bR, bQ;
  int wB_sq; /* square of first white bishop (-1 if none) */
  int bB_sq; /* square of first black bishop (-1 if none) */
  /* Piece totals in "pawns" (approximate, for threshold tests) */
  int w_pts; /* Q=9 R=5 B=3 N=3 P=1 */
  int b_pts;
} MC;

static MC count_mc(void){
  MC m;
  m.wP = m.wN = m.wB = m.wR = m.wQ = 0;
  m.bP = m.bN = m.bB = m.bR = m.bQ = 0;
  m.wB_sq = m.bB_sq = -1;

  for (int i = 0; i < 64; i++){
    const int p = board[i];
    switch (p){
    case -1: m.wP++;
      break;
    case -2: m.wN++;
      break;
    case -3: m.wB++;
      if (m.wB_sq < 0) m.wB_sq = i;
      break;
    case -4: m.wR++;
      break;
    case -5: m.wQ++;
      break;
    case 1: m.bP++;
      break;
    case 2: m.bN++;
      break;
    case 3: m.bB++;
      if (m.bB_sq < 0) m.bB_sq = i;
      break;
    case 4: m.bR++;
      break;
    case 5: m.bQ++;
      break;
    default: ;
    }
  }
  m.w_pts = m.wQ * 9 + m.wR * 5 + m.wB * 3 + m.wN * 3 + m.wP;
  m.b_pts = m.bQ * 9 + m.bR * 5 + m.bB * 3 + m.bN * 3 + m.bP;
  return m;
}

/* ------------------------------------------------------------------ */
/* Pattern evaluators                                                   */
/* Each returns an adjustment with the standard sign convention.       */
/* ------------------------------------------------------------------ */
/*
 * drive_to_corner
 * The strong side (stm) has a lone-king target and wants to drive it
 * into a corner while keeping their own king close.
 *
 * lone_king  = square of the king being mated
 * own_king   = square of the mating king
 * sign       = +1 if bonus should be positive (bad for White)
 *             = -1 if bonus should be negative (bad for Black)
 */
static int drive_to_corner(const int lone_king, const int own_king, const int sign){
  const int cd = corner_dist(lone_king); /* 0 = on corner, 6 = centre  */
  const int kd = cheb(own_king,lone_king); /* 1 = kings adjacent         */

  /* Push lone king to a corner (+) and keep own king close (+) */
  const int bonus = (6 - cd) * 15 /* 0..90  — corner bonus      */
    + (7 - kd) * 8; /* 0..48  — proximity bonus   */

  return sign * bonus;
}

/*
 * drive_to_bishop_corner
 * For KBNK: the lone king must be driven to a corner matching the
 * bishop's square color.
 */
static int drive_to_bishop_corner(
  const int lone_king, const int own_king,
  const int bishop_sq, const int sign){
  const int bcolor = ((bishop_sq >> 3) + (bishop_sq & 7)) & 1;
  const int cd = corner_dist_by_color(lone_king,bcolor); /* 0 = right corner */
  const int kd = cheb(own_king,lone_king);

  const int bonus = (12 - cd) * 12 /* 0..144 — right-corner bonus */
    + (7 - kd) * 8; /* proximity bonus             */

  return sign * bonus;
}

/* ------------------------------------------------------------------ */
/* endgame_eval                                                         */
/* ------------------------------------------------------------------ */
int endgame_eval(void){
  const MC m = count_mc();
  const int wk = wkpos, bk = bkpos;
  int adj = 0;

  /* ── 2-MAN: bare kings — draw signal only (no bonus needed) ─── */
  if (m.w_pts == 0 && m.b_pts == 0)
    return 0;

  /* ─────────────────────────────────────────────────────────────
   * WHITE has mating material, BLACK is bare king
   * ───────────────────────────────────────────────────────────── */
  if (m.b_pts == 0){
    /* KBK or KNK: drawn — neutralise the material lead so the
     * engine stops "trying to mate" and accepts the draw.        */
    if (m.wQ == 0 && m.wR == 0 && m.wP == 0 &&
      m.wB + m.wN <= 1)
      return 0; /* exact draw, override everything */

    /* KNNK: theoretical draw but in practice hard — small bonus */
    if (m.wQ == 0 && m.wR == 0 && m.wP == 0 &&
      m.wN == 2 && m.wB == 0)
      return drive_to_corner(bk,wk,-1) / 4;

    /* KBNK: bishop + knight — must push to the right corner     */
    if (m.wQ == 0 && m.wR == 0 && m.wP == 0 &&
      m.wB == 1 && m.wN == 1 && m.wB_sq >= 0)
      return drive_to_bishop_corner(bk,wk,m.wB_sq,-1);

    /* All other winning cases (KQK, KRK, KBBK, KQRK, …):
     * drive black king to any corner.                            */
    return drive_to_corner(bk,wk,-1);
  }

  /* ─────────────────────────────────────────────────────────────
   * BLACK has mating material, WHITE is bare king
   * ───────────────────────────────────────────────────────────── */
  if (m.w_pts == 0){
    if (m.bQ == 0 && m.bR == 0 && m.bP == 0 &&
      m.bB + m.bN <= 1)
      return 0; /* drawn */

    if (m.bQ == 0 && m.bR == 0 && m.bP == 0 &&
      m.bN == 2 && m.bB == 0)
      return drive_to_corner(wk,bk,+1) / 4;

    if (m.bQ == 0 && m.bR == 0 && m.bP == 0 &&
      m.bB == 1 && m.bN == 1 && m.bB_sq >= 0)
      return drive_to_bishop_corner(wk,bk,m.bB_sq,+1);

    return drive_to_corner(wk,bk,+1);
  }

  /* ─────────────────────────────────────────────────────────────
   * 4-MAN: one side has a significant material advantage with
   * a small amount of enemy material remaining.
   * ───────────────────────────────────────────────────────────── */
  if (m.w_pts >= 5 && m.b_pts <= 3 && m.b_pts > 0){
    /* White is winning; push black king toward a corner.
     * Scale the urgency by the material imbalance.              */
    const int cd = corner_dist(bk);
    const int kd = cheb(wk,bk);
    adj -= (6 - cd) * 8 + (7 - kd) * 4;
  }
  if (m.b_pts >= 5 && m.w_pts <= 3 && m.w_pts > 0){
    const int cd = corner_dist(wk);
    const int kd = cheb(bk,wk);
    adj += (6 - cd) * 8 + (7 - kd) * 4;
  }

  /* ─────────────────────────────────────────────────────────────
   * PURE PAWN ENDINGS (no pieces except kings)
   * ───────────────────────────────────────────────────────────── */
  if (m.wQ == 0 && m.wR == 0 && m.wB == 0 && m.wN == 0 &&
    m.bQ == 0 && m.bR == 0 && m.bB == 0 && m.bN == 0){
    /* King centralisation: use squared Manhattan to centre (d4=35, e5=28) */
    int wkc = manh(wk,35);
    if (wkc > manh(wk,28)) wkc = manh(wk,28);
    int bkc = manh(bk,35);
    if (bkc > manh(bk,28)) bkc = manh(bk,28);
    adj -= (6 - wkc) * 5; /* white king near centre = better for White */
    adj += (6 - bkc) * 5;

    /* Passed pawn bonuses */
    for (int fl = 0; fl < 8; fl++){
      /* -- White passed pawn --------------------------------- */
      for (int rk = 6; rk >= 1; rk--){ /* ranks 1-6 (no promotion) */
        const int sq = rk * 8 + fl;
        if (board[sq] != -1) continue;
        int passed = 1;
        for (int af = fl - 1; af <= fl + 1; af++){
          if (af < 0 || af > 7) continue;
          for (int ar = 0; ar < rk; ar++){ /* ranks above pawn */
            if (board[ar * 8 + af] == 1){
              passed = 0;
              break;
            }
          }
          if (! passed) break;
        }
        if (! passed) continue;
        const int adv = 7 - rk; /* 1 (rank 6) to 6 (rank 1) */
        adj -= adv * 12; /* closer to promotion = better for White */
        /* King support: white king in front of pawn */
        if (wkpos / 8 < rk && iabs(wkpos % 8 - fl) <= 1)
          adj -= 15;
        break; /* only one white pawn per file relevant */
      }

      /* -- Black passed pawn --------------------------------- */
      for (int rk = 1; rk <= 6; rk++){
        const int sq = rk * 8 + fl;
        if (board[sq] != 1) continue;
        int passed = 1;
        for (int af = fl - 1; af <= fl + 1; af++){
          if (af < 0 || af > 7) continue;
          for (int ar = rk + 1; ar <= 7; ar++){ /* ranks below pawn */
            if (board[ar * 8 + af] == -1){
              passed = 0;
              break;
            }
          }
          if (! passed) break;
        }
        if (! passed) continue;
        const int adv = rk; /* 1 (rank 1) to 6 (rank 6) */
        adj += adv * 12;
        if (bkpos / 8 > rk && iabs(bkpos % 8 - fl) <= 1)
          adj += 15;
        break;
      }
    }
    return adj;
  }

  /* ─────────────────────────────────────────────────────────────
   * ROOK ENDGAMES: reward 7th-rank rook and active king
   * ───────────────────────────────────────────────────────────── */
  if (m.wQ == 0 && m.bQ == 0 && (m.wR > 0 || m.bR > 0)){
    /* White rook on 7th rank (row index 1, squares 8-15) */
    for (int i = 8; i < 16; i++)
      if (board[i] == -4) adj -= 25;
    /* Black rook on 7th rank (row index 6, squares 48-55) */
    for (int i = 48; i < 56; i++)
      if (board[i] == 4) adj += 25;

    /* Active king in rook endings */
    const int wkc = manh(wk,35);
    const int bkc = manh(bk,28);
    if (wkc < 5) adj -= (5 - wkc) * 3;
    if (bkc < 5) adj += (5 - bkc) * 3;
  }

  /* ─────────────────────────────────────────────────────────────
   * GENERAL: When total material is low, activate the kings.
   * (This complements the king PSQTs in stat.c, which switch to
   *  king_eg_psqt when game >= 3; this kicks in even slightly
   *  earlier.)
   * ───────────────────────────────────────────────────────────── */
  if (m.w_pts + m.b_pts <= 12){
    /* Penalise kings huddling in corners relative to the centre */
    const int wkr = wk >> 3, wkf = wk & 7;
    const int bkr = bk >> 3, bkf = bk & 7;
    const int wcent = iabs(wkr - 3) + iabs(wkr - 4) + iabs(wkf - 3) + iabs(wkf - 4);
    const int bcent = iabs(bkr - 3) + iabs(bkr - 4) + iabs(bkf - 3) + iabs(bkf - 4);
    /* Lower wcent = more central = better for White (adj goes negative) */
    adj -= (8 - wcent) * 3;
    adj += (8 - bcent) * 3;
  }

  return adj;
}

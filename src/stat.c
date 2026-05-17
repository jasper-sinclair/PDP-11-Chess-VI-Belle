#include <stdio.h>
#include "chess.h"
/* ------------------------------------------------------------------ */
/*  static_eval: score every legal move, sort, return ordered list    */
/* ------------------------------------------------------------------ */
int* static_eval(void){
  int *p2;
  int score;

  int* p1 = p2 = lmp;
  position_setup();

  if (mantom) gen_black_legal();
  else gen_white_legal();

  if (lmp == p1) return p1;

  int* p3 = p2 = p1;

  while (p2 < lmp){
    const int move = *(p2 + 1);
    if (move == 0xfffffffc || move == -4) break;

    if (mantom){
      black_move(move);
      score = black_static(0);
      black_undo();
    } else{
      white_move(move);
      score = white_static(0);
      white_undo();
    }

    *p3++ = score;
    *p3++ = move;
    p2 += 2;
  }

  lmp = p3;
  const int num_moves = (lmp - p1) / 2;
  if (num_moves > 0)
    qsort(p1,num_moves,sizeof(int) * 2,compare_int);

  return p1;
}

/* ------------------------------------------------------------------ */
/*  white_static                                                      */
/* ------------------------------------------------------------------ */
int white_static(const int f){
  int i = 0, h;

  for (h = 0; h < 64; h++){
    const int piece = board[h];
    if (piece == 0) continue;
    if (piece < 0){
      /* White pieces - negative values are good for White (minimizer) */
      const int material_val = abs_val(pval[6 + piece]);
      i += material_val;
      const int penalty = check_heuristic(h);
      if (penalty > 0){
        /* Apply full check_heuristic penalty (not halved).
         * Halving underestimated danger: a bishop (300) attacked by a
         * pawn (100) and undefended gave penalty=300 but only i-=150,
         * which the PSQT bonus could easily overcome, encouraging the
         * bishop-for-pawn trade.  Full penalty correctly deters it. */
        if (piece == -1) i -= 80;
        else i -= penalty;
      }
    } else{
      /* Black pieces - positive values are bad for White */
      i -= pval[6 + piece];
    }
  }

  h = 0;
  while (white_heur[h] != NULL){
    int (*p)(void) = white_heur[h++];
    const int j = (*p)();
    if (f) printf("%4d ",j);
    i += j;
  }

  if (f) printf("=%4d ",i);
  return i;
}

/* ------------------------------------------------------------------ */
/*  black_static                                                      */
/* ------------------------------------------------------------------ */
int black_static(const int f){
  int i = 0, h;

  for (h = 0; h < 64; h++){
    const int piece = board[h];
    if (piece == 0) continue;
    if (piece > 0){
      /* Black pieces - positive values are good for Black (maximizer) */
      const int material_val = pval[6 + piece];
      i += material_val;
      const int penalty = check_heuristic(h);
      if (penalty > 0){
        /* Full penalty — mirrors white_static fix above */
        if (piece == 1) i -= 80;
        else i -= penalty;
      }
    } else{
      /* White pieces - negative values are bad for Black */
      i += pval[6 + piece];
    }
  }

  h = 0;
  while (black_heur[h] != NULL){
    int (*p)(void) = black_heur[h++];
    const int j = (*p)();
    if (f) printf("%4d ",j);
    i += j;
  }

  if (f) printf("=%4d ",i);
  return -i; /* Negate because black_static returns from White's perspective */
}

int check_heuristic(const int ploc){
  int* save_lmp = lmp;
  const int pie = board[ploc];
  if (pie == 0) return 0;

  int atk = 0, def = 0;
  int min_atk_val = 9999;
  const int piece_val = abs_val(pval[6 + pie]); /* Use pval, not ipval, for scaled values */

  /* Count enemy attackers; record the cheapest one. */
  if (pie < 0) gen_black_moves();
  else gen_white_moves();
  const int* p2 = save_lmp;
  while (p2 < lmp){
    if ((*(p2 + 1) & 0xFF) == ploc){
      atk++;
      const int asq = (*(p2 + 1) >> 8) & 0x3F;
      const int av = abs_val(pval[6 + board[asq]]);
      if (av < min_atk_val) min_atk_val = av;
    }
    p2 += 2;
  }
  lmp = save_lmp;

  if (atk == 0) return 0;

  /* Count friendly defenders */
  board[ploc] = 0;
  if (pie < 0) gen_white_moves();
  else gen_black_moves();
  p2 = save_lmp;
  while (p2 < lmp){
    if ((*(p2 + 1) & 0xFF) == ploc) def++;
    p2 += 2;
  }
  lmp = save_lmp;
  board[ploc] = pie;

  /* Completely undefended: full material loss. */
  if (def == 0) return piece_val;

  /* VALUE-AWARE CHECK: cheapest attacker is worth less than piece attacked */
  if (min_atk_val < piece_val){
    const int net = piece_val - min_atk_val;
    return (net > piece_val / 2)?net:piece_val / 2;
  }

  /* More attackers than defenders of equal value */
  if (atk > def) return piece_val / 2;

  return 0;
}

/* ================================================================== */
/*  positional_adjustment                                             */
/*                                                                    */
/*  Called as stand-pat from white_quiesce and black_quiesce:         */
/*    v1 = value + positional_adjustment()                            */
/*                                                                    */
/*  This is the ONLY positional signal that reaches the actual leaf   */
/*  evaluator used by the alpha-beta search.                          */
/*                                                                    */
/*  Sign convention:                                                  */
/*    adj > 0  →  bad for White  (White is minimizer, high = worse)   */
/*    adj < 0  →  bad for Black                                       */
/* ================================================================== */
int positional_adjustment(void){
  int adj = 0;
  int h, piece, f, r;
  int wpawns[8], bpawns[8];
  int wbishops, bbishops;
  int w_minors_developed, b_minors_developed;

  /* Piece-Square Tables for Opening/Middlegame */
  static const int knight_psqt[64] = {
    -50,-40,-30,-25,-25,-30,-40,-50,
    -40,-20,0,5,5,0,-20,-40,
    -30,5,15,20,20,15,5,-30,
    -25,5,20,25,25,20,5,-25,
    -25,5,20,25,25,20,5,-25,
    -30,5,15,20,20,15,5,-30,
    -40,-20,0,5,5,0,-20,-40,
    -50,-40,-30,-25,-25,-30,-40,-50
  };

  /* Bishops strongly prefer active diagonals; passive e2/d2 squares
   * score 0 while active c4/d5/e4 etc. score 20+.  This 20 cp gap
   * is large enough to show up against check_heuristic noise.         */
  static const int bishop_psqt[64] = {
    -20,-10,-10,-10,-10,-10,-10,-20,
    -10,5,0,0,0,0,5,-10,
    -10,10,15,15,15,15,10,-10,
    -10,5,15,20,20,15,5,-10,
    -10,5,15,20,20,15,5,-10,
    -10,10,15,15,15,15,10,-10,
    -10,5,0,0,0,0,5,-10,
    -20,-10,-10,-10,-10,-10,-10,-20
  };

  static const int rook_psqt[64] = {
    0,0,0,5,5,0,0,0,
    -5,0,0,0,0,0,0,-5,
    -5,0,0,0,0,0,0,-5,
    -5,0,0,0,0,0,0,-5,
    -5,0,0,0,0,0,0,-5,
    -5,0,0,0,0,0,0,-5,
    5,10,10,10,10,10,10,5,
    0,0,0,0,0,0,0,0
  };

  static const int queen_psqt[64] = {
    -20,-10,-10,-5,-5,-10,-10,-20,
    -10,0,0,0,0,0,0,-10,
    -10,0,5,5,5,5,0,-10,
    -5,0,5,5,5,5,0,-5,
    0,0,5,5,5,5,0,-5,
    -10,0,5,5,5,5,0,-10,
    -10,0,0,0,0,0,0,-10,
    -20,-10,-10,-5,-5,-10,-10,-20
  };

  static const int king_mg_psqt[64] = {
    20,30,10,0,0,10,30,20,
    20,20,0,0,0,0,20,20,
    -10,-20,-20,-20,-20,-20,-20,-10,
    -20,-30,-30,-40,-40,-30,-30,-20,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30,
    -30,-40,-40,-50,-50,-40,-40,-30
  };

  static const int king_eg_psqt[64] = {
    -50,-30,-30,-30,-30,-30,-30,-50,
    -30,-20,0,0,0,0,-20,-30,
    -30,0,10,20,20,10,0,-30,
    -30,0,20,30,30,20,0,-30,
    -30,0,20,30,30,20,0,-30,
    -30,0,10,20,20,10,0,-30,
    -30,-20,0,0,0,0,-20,-30,
    -50,-30,-30,-30,-30,-30,-30,-50
  };

  /* ── 0. SINGLE BOARD SCAN ──────────────────────────────────────
   * Collect pawn file counts, bishop counts, and development counts.
   */
  wbishops = bbishops = 0;
  w_minors_developed = b_minors_developed = 0;
  for (f = 0; f < 8; f++)
    wpawns[f] = bpawns[f] = 0;

  for (h = 0; h < 64; h++){
    piece = board[h];
    f = h & 7;

    if (piece == -1){
      wpawns[f]++;
    } else if (piece == 1){
      bpawns[f]++;
    } else if (piece == -3){
      wbishops++;
    } else if (piece == 3){
      bbishops++;
    }

    /* Count developed minor pieces (off back rank) */
    if (game < 3){
      if (piece == -2 || piece == -3){
        /* White minor piece - developed if not on starting squares */
        if (h != 57 && h != 62 && h != 58 && h != 61)
          w_minors_developed++;
      } else if (piece == 2 || piece == 3){
        /* Black minor piece - developed if not on starting squares */
        if (h != 1 && h != 6 && h != 2 && h != 5)
          b_minors_developed++;
      }
    }
  }

  /* ── 1. PAWN STRUCTURE ─────────────────────────────────────────
   * Doubled pawns: penalty for multiple pawns on same file.
   * Isolated pawns: penalty for pawns with no friendly pawns on adjacent files.
   * Reduced values from original to avoid overwhelming material considerations.
   */
  for (f = 0; f < 8; f++){
    /* Doubled pawns */
    if (wpawns[f] > 1) adj += 12 * (wpawns[f] - 1); /* bad White */
    if (bpawns[f] > 1) adj -= 12 * (bpawns[f] - 1); /* bad Black */

    /* Isolated pawns */
    {
      int wl = (f > 0)?wpawns[f - 1]:0;
      int wr = (f < 7)?wpawns[f + 1]:0;
      if (wpawns[f] && ! wl && ! wr) adj += 10; /* was 8 – made symmetric */
    }
    {
      int bl = (f > 0)?bpawns[f - 1]:0;
      int br = (f < 7)?bpawns[f + 1]:0;
      if (bpawns[f] && ! bl && ! br) adj -= 10;
    }
    /* Connected pawn bonus (adjacent file friendly pawn) */
    if (wpawns[f] && ((f > 0 && wpawns[f - 1]) || (f < 7 && wpawns[f + 1]))) adj -= 6 * wpawns[f];
    if (bpawns[f] && ((f > 0 && bpawns[f - 1]) || (f < 7 && bpawns[f + 1]))) adj += 6 * bpawns[f];
  }

  /* ── 1b. BACKWARD PAWNS + KNIGHT OUTPOSTS ─────────────────────
   * Backward: pawn blocked by enemy pawn control with no friendly
   * support from behind.  Outpost: knight on safe advanced square.
   */
  {
    static const int op[8][8] = {
      {0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},
      {0,5,10,15,15,10,5,0},{0,5,15,20,20,15,5,0},
      {0,5,15,20,20,15,5,0},{0,5,10,15,15,10,5,0},
      {0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0}
    };
    int hh2;
    for (hh2 = 8; hh2 < 56; hh2++){
      int pp = board[hh2], pf2 = hh2 & 7, pr2 = hh2 >> 3;
      if (pp == -1){ /* backward white pawn */
        int sup = (pf2 > 0 && pr2 < 7 && board[(pr2 + 1) * 8 + (pf2 - 1)] == -1) ||
          (pf2 < 7 && pr2 < 7 && board[(pr2 + 1) * 8 + (pf2 + 1)] == -1);
        if (! sup){
          int fr = hh2 - 8;
          if ((pf2 > 0 && fr > 0 && board[fr - 1] == 1) || (pf2 < 7 && fr + 1 < 64 && board[fr + 1] == 1))
            adj += 8;
        }
      }
      if (pp == 1){ /* backward black pawn */
        int sup = (pf2 > 0 && pr2 > 0 && board[(pr2 - 1) * 8 + (pf2 - 1)] == 1) ||
          (pf2 < 7 && pr2 > 0 && board[(pr2 - 1) * 8 + (pf2 + 1)] == 1);
        if (! sup){
          int fr = hh2 + 8;
          if ((pf2 > 0 && fr > 0 && board[fr - 1] == -1) || (pf2 < 7 && fr + 1 < 64 && board[fr + 1] == -1))
            adj -= 8;
        }
      }
      if (pp == -2 && pr2 >= 2 && pr2 <= 5){ /* white knight outpost */
        int safe = ! ((pf2 > 0 && pr2 > 0 && board[(pr2 - 1) * 8 + (pf2 - 1)] == 1) ||
          (pf2 < 7 && pr2 > 0 && board[(pr2 - 1) * 8 + (pf2 + 1)] == 1));
        if (safe){
          int bon = op[pr2][pf2];
          if ((pf2 > 0 && pr2 < 7 && board[(pr2 + 1) * 8 + (pf2 - 1)] == -1) ||
            (pf2 < 7 && pr2 < 7 && board[(pr2 + 1) * 8 + (pf2 + 1)] == -1))
            bon *= 2;
          adj -= bon;
        }
      }
      if (pp == 2 && pr2 >= 2 && pr2 <= 5){ /* black knight outpost */
        int safe = ! ((pf2 > 0 && pr2 < 7 && board[(pr2 + 1) * 8 + (pf2 - 1)] == -1) ||
          (pf2 < 7 && pr2 < 7 && board[(pr2 + 1) * 8 + (pf2 + 1)] == -1));
        if (safe){
          int bon = op[7 - pr2][pf2];
          if ((pf2 > 0 && pr2 > 0 && board[(pr2 - 1) * 8 + (pf2 - 1)] == 1) ||
            (pf2 < 7 && pr2 > 0 && board[(pr2 - 1) * 8 + (pf2 + 1)] == 1))
            bon *= 2;
          adj += bon;
        }
      }
    }
  }

  /* ── 2. PASSED PAWNS ───────────────────────────────────────────
   * Reduced bonus from original (was 15-115, now 8-40).
   * A passed pawn is valuable but shouldn't outweigh a piece.
   */
  for (h = 0; h < 64; h++){
    piece = board[h];

    if (piece == -1){ /* White pawn */
      f = h & 7;
      r = h >> 3;
      {
        int passed = 1;
        int ff0 = (f > 0)?f - 1:0;
        int ff1 = (f < 7)?f + 1:7;
        int rr;
        for (rr = r - 1; rr >= 0 && passed; rr--){
          int ff;
          for (ff = ff0; ff <= ff1; ff++){
            if (board[rr * 8 + ff] == 1){
              passed = 0;
              break;
            }
          }
        }
        if (passed && r >= 1 && r <= 6){
          /* exponential bonus: 12/25/45/65/95/130 cp */
          static const int wpb[6] = {12,25,45,65,95,130};
          adj -= wpb[6 - r]; /* r=6→wpb[0]=12 ... r=1→wpb[5]=130 */
        }
      }
    } else if (piece == 1){ /* Black pawn */
      f = h & 7;
      r = h >> 3;
      {
        int passed = 1;
        int ff0 = (f > 0)?f - 1:0;
        int ff1 = (f < 7)?f + 1:7;
        int rr;
        for (rr = r + 1; rr <= 7 && passed; rr++){
          int ff;
          for (ff = ff0; ff <= ff1; ff++){
            if (board[rr * 8 + ff] == -1){
              passed = 0;
              break;
            }
          }
        }
        if (passed && r >= 1 && r <= 6){
          static const int bpb[6] = {12,25,45,65,95,130};
          adj += bpb[r - 1]; /* r=1→bpb[0]=12 ... r=6→bpb[5]=130 */
        }
      }
    }
  }

  /* ── 2b. RULE OF THE SQUARE + KING TROPISM (endgame) ──────────
   * Detect unstoppable passed pawns; reward king proximity.
   * Uses portable Chebyshev distance (no GCC statement expressions).
   */
  if (game >= 2){
    int wkr = wkpos >> 3, wkf2 = wkpos & 7, bkr = bkpos >> 3, bkf2 = bkpos & 7;
    int hh2;
    for (hh2 = 0; hh2 < 64; hh2++){
      int pp = board[hh2];
      int pf2, pr2, wd, bd, dr, df;
      if (! pp) continue;
      pf2 = hh2 & 7;
      pr2 = hh2 >> 3;
      if (pp == -1 && pr2 >= 1 && pr2 <= 6){
        dr = abs_val(wkr - pr2);
        df = abs_val(wkf2 - pf2);
        wd = dr > df?dr:df;
        dr = abs_val(bkr - pr2);
        df = abs_val(bkf2 - pf2);
        bd = dr > df?dr:df;
        adj -= (7 - wd) * 3; /* White king near own passer */
        if (bd > pr2 + 1) adj -= 55; /* Rule of the square */
      }
      if (pp == 1 && pr2 >= 1 && pr2 <= 6){
        dr = abs_val(bkr - pr2);
        df = abs_val(bkf2 - pf2);
        bd = dr > df?dr:df;
        dr = abs_val(wkr - pr2);
        df = abs_val(wkf2 - pf2);
        wd = dr > df?dr:df;
        adj += (7 - bd) * 3; /* Black king near own passer */
        if (wd > (7 - pr2) + 1) adj += 55; /* Rule of the square */
      }
    }
  }

  /* ── 3. BISHOP PAIR ────────────────────────────────────────────
   * Reduced from 30 to 20 - important but not decisive.
   */
  if (wbishops >= 2) adj -= 20; /* good for White */
  if (bbishops >= 2) adj += 20; /* good for Black */

  /* ── 4. ROOKS ON OPEN / SEMI-OPEN FILES ────────────────────────
   * Reduced bonuses from original (15/8 to 10/5).
   */
  for (h = 0; h < 64; h++){
    piece = board[h];
    f = h & 7;

    if (piece == -4 || piece == -5){ /* White rook or queen */
      if (! wpawns[f])
        adj -= bpawns[f]?5:10;
    } else if (piece == 4 || piece == 5){ /* Black rook or queen */
      if (! bpawns[f])
        adj += wpawns[f]?5:10;
    }
  }

  /* ── 5. ROOK ON 7TH RANK ────────────────────────────────────────
   * Reduced from 20 to 12.
   */
  for (h = 8; h < 16; h++)
    if (board[h] == -4) adj -= 12;
  for (h = 48; h < 56; h++)
    if (board[h] == 4) adj += 12;

  /* ── 6. PIECE-SQUARE TABLES (Opening/Middlegame) ───────────────
   * Encourage development and good piece placement.
   */
  if (game < 3){
    for (h = 0; h < 64; h++){
      piece = board[h];
      if (piece == -2){ /* White knight: mirror (63-h) */
        adj -= knight_psqt[63 - h];
      } else if (piece == 2){ /* Black knight: direct (h) */
        adj += knight_psqt[h];
      } else if (piece == -3){ /* White bishop: mirror */
        adj -= bishop_psqt[63 - h];
      } else if (piece == 3){ /* Black bishop: direct */
        adj += bishop_psqt[h];
      } else if (piece == -4){ /* White rook: mirror */
        adj -= rook_psqt[63 - h];
      } else if (piece == 4){ /* Black rook: direct */
        adj += rook_psqt[h];
      } else if (piece == -5){ /* White queen: mirror */
        adj -= queen_psqt[63 - h];
      } else if (piece == 5){ /* Black queen: direct */
        adj += queen_psqt[h];
      } else if (piece == -6){ /* White king: mirror */
        adj -= king_mg_psqt[63 - h];
      } else if (piece == 6){ /* Black king: direct */
        adj += king_mg_psqt[h];
      }
    }
  } else{
    /* Endgame - kings become more active */
    for (h = 0; h < 64; h++){
      piece = board[h];
      if (piece == -6){
        adj -= king_eg_psqt[63 - h]; /* mirror */
      } else if (piece == 6){
        adj += king_eg_psqt[h]; /* direct */
      }
    }
  }

  /* ── 7. DEVELOPMENT BONUS ────────────────────────────────────────
   * Encourage getting pieces off the back rank.
   * Reduced from 8 to 5 per piece.
   */
  if (game < 2){
    adj -= w_minors_developed * 5; /* good for White */
    adj += b_minors_developed * 5; /* bad for White */
  }

  /* ── 8. QUEEN DEVELOPMENT PENALTY ───────────────────────────────
   * Queen out too early with undeveloped pieces.
   */
  if (game < 2){
    /* White queen moved from d1 while minors undeveloped */
    if (board[59] != -5 && w_minors_developed < 2){
      adj += 30;
    }
    /* Black queen moved from d8 while minors undeveloped */
    if (board[3] != 5 && b_minors_developed < 2){
      adj -= 30;
    }
  }

  /* ── 9. KING SAFETY ──────────────────────────────────────────────
   * Penalise an uncastled king even when castling rights remain —
   * the quiescence stand-pat otherwise sees no urgency to castle.
   * Swing: ~60 cp from un-castled to castled (was only 25 when rights
   * existed, now 25+20=45 bonus for castling + 20 penalty for staying).
   */
  if (game < 3){
    /* White king safety */
    if (wkpos == 60){
      /* King still on e1 — always bad in the middlegame */
      adj += 20;
      if (! (flag & (WKCASTLE | WQCASTLE)))
        adj += 25; /* lost castling rights on top */
    } else if (wkpos == 62 || wkpos == 58){
      adj -= 30; /* properly castled: reward */
    } else if (wkpos >= 56){
      adj += 15; /* shuffled but still back rank */
    } else{
      adj += 60; /* king wandered to centre */
    }

    /* Black king safety (mirrored) */
    if (bkpos == 4){
      adj -= 20;
      if (! (flag & (BKCASTLE | BQCASTLE)))
        adj -= 25;
    } else if (bkpos == 6 || bkpos == 2){
      adj += 30;
    } else if (bkpos <= 7){
      adj -= 15;
    } else{
      adj -= 60;
    }
  }

  /* ── 10. PAWN SHELTER AROUND THE CASTLED KING ───────────────────
   * Protect pawns in front of castled king.
   */
  if (game < 3){
    /* White kingside castle (g1) */
    if (wkpos == 62){
      /* Check f2, g2, h2 pawns */
      if (board[53] != -1) adj += 15; /* f2 pawn missing */
      if (board[54] != -1) adj += 15; /* g2 pawn missing */
      if (board[55] != -1) adj += 10; /* h2 pawn missing */
    }
    /* White queenside castle (c1) */
    else if (wkpos == 58){
      /* Check a2, b2, c2 pawns */
      if (board[48] != -1) adj += 10; /* a2 pawn missing */
      if (board[49] != -1) adj += 15; /* b2 pawn missing */
      if (board[50] != -1) adj += 15; /* c2 pawn missing */
    }

    /* Black kingside castle (g8) */
    if (bkpos == 6){
      if (board[13] != 1) adj -= 15; /* f7 pawn missing */
      if (board[14] != 1) adj -= 15; /* g7 pawn missing */
      if (board[15] != 1) adj -= 10; /* h7 pawn missing */
    }
    /* Black queenside castle (c8) */
    else if (bkpos == 2){
      if (board[8] != 1) adj -= 10; /* a7 pawn missing */
      if (board[9] != 1) adj -= 15; /* b7 pawn missing */
      if (board[10] != 1) adj -= 15; /* c7 pawn missing */
    }
  }

  /* ── 10b. PAWN STORM ────────────────────────────────────────────
   * Opposite-wing castling: reward advancing pawns toward enemy king.
   */
  if (game < 3){
    int wkf2 = wkpos & 7, bkf2 = bkpos & 7;
    if ((wkf2 <= 3 && bkf2 >= 4) || (wkf2 >= 4 && bkf2 <= 3)){
      int hh2;
      for (hh2 = 8; hh2 < 48; hh2++)
        if (board[hh2] == -1){
          int pf2 = hh2 & 7, pr2 = hh2 >> 3;
          if ((bkf2 >= 4 && pf2 >= 4) || (bkf2 <= 3 && pf2 <= 3)){
            int adv = 5 - pr2;
            if (adv > 0) adj -= adv * 5;
          }
        }
      for (hh2 = 16; hh2 < 56; hh2++)
        if (board[hh2] == 1){
          int pf2 = hh2 & 7, pr2 = hh2 >> 3;
          if ((wkf2 >= 4 && pf2 >= 4) || (wkf2 <= 3 && pf2 <= 3)){
            int adv = pr2 - 2;
            if (adv > 0) adj += adv * 5;
          }
        }
    }
  }
  /* Small tempo incentive */
  if (game < 3){
    if (value < -30) adj += 8;
    if (value > 30) adj -= 8;
  }

  /* ── 11. PAWN-ATTACK PIECE SAFETY ──────────────────────────────
   * The quiescence stand-pat (value + positional_adjustment) had no
   * awareness of pieces standing on pawn-attacked squares.  This
   * allowed the engine to move a bishop to a pawn-attacked square
   * and receive an optimistic stand-pat that ignored the imminent
   * pawn capture (the classic "bishop for pawn" horizon effect).
   *
   * For every non-pawn piece attacked by an enemy pawn but NOT
   * defended by a friendly pawn, apply a penalty equal to half the
   * expected material loss (piece_value - pawn_value) / 2.  The
   * full loss is already handled by the alpha-beta captures in
   * quiescence; this partial penalty ensures the stand-pat evaluation
   * steers the engine away from such squares in move ordering.
   *
   * Uses only direct board array reads — no move generation needed.
   */
  {
    int hh2;
    for (hh2 = 0; hh2 < 64; hh2++){
      int pp = board[hh2];
      /* Only care about non-pawn, non-king pieces */
      if (pp == 0 || pp == -1 || pp == 1 || pp == -6 || pp == 6)
        continue;

      int f2 = hh2 & 7, r2 = hh2 >> 3;
      int pp_val = abs_val(ipval[6 + pp]);

      if (pp < 0){
        /* White piece: attacked by black pawn from the rank above? */
        int attacked = (r2 > 0) &&
        ((f2 > 0 && board[(r2 - 1) * 8 + (f2 - 1)] == 1) ||
          (f2 < 7 && board[(r2 - 1) * 8 + (f2 + 1)] == 1));
        if (attacked){
          /* Defended by a white pawn from the rank below? */
          int defended = (r2 < 7) &&
          ((f2 > 0 && board[(r2 + 1) * 8 + (f2 - 1)] == -1) ||
            (f2 < 7 && board[(r2 + 1) * 8 + (f2 + 1)] == -1));
          if (! defended)
            adj += (pp_val - 100) / 2; /* bad for White */
        }
      } else{
        /* Black piece: attacked by white pawn from the rank below? */
        int attacked = (r2 < 7) &&
        ((f2 > 0 && board[(r2 + 1) * 8 + (f2 - 1)] == -1) ||
          (f2 < 7 && board[(r2 + 1) * 8 + (f2 + 1)] == -1));
        if (attacked){
          /* Defended by a black pawn from the rank above? */
          int defended = (r2 > 0) &&
          ((f2 > 0 && board[(r2 - 1) * 8 + (f2 - 1)] == 1) ||
            (f2 < 7 && board[(r2 - 1) * 8 + (f2 + 1)] == 1));
          if (! defended)
            adj -= (pp_val - 100) / 2; /* bad for Black */
        }
      }
    }
  }

  /* ── 12. ENDGAME PATTERN KNOWLEDGE ─────────────────────────────
   * Corner-driving for KQK/KRK/KBNK/KBBK, passed-pawn bonuses,
   * rook-on-7th, king activation, etc.  Only fired in the endgame
   * (game >= 2) to avoid disturbing middlegame evaluations.
   */
  if (game >= 2)
    adj += endgame_eval();

  return adj;
}

/* ------------------------------------------------------------------ */
/*  surround_king: mark squares around a king in the control table     */
/* ------------------------------------------------------------------ */
void surround_king(const int p){
  const int masks[] = {uleft, uright, dleft, dright, up, left, right, down};

  for (int k = 0; k < 8; k++){
    const int offsets[] = {-9,-7,7,9,-8,-1,1,8};
    const int to = p + offsets[k];
    if (to >= 0 && to < 64 && (dir[p] & masks[k]) == 0)
      control[to] += 10;
  }
}

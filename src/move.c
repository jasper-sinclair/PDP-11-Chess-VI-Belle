/*
 * move.c — move execution and undo
 */
#include <stdio.h>
#include <stdlib.h>
#include "chess.h"
/* ------------------------------------------------------------------ */
/* white_move                                                           */
/* ------------------------------------------------------------------ */
void white_move(const int m){
  const int from = (m >> 8) & 0xFF;
  const int to = m & 0xFF;
  /* save before we clear it */

  if (m == 0xfffffffc || m == -4){
    fprintf(stderr,"FATAL: attempt to make sentinel move\n");
    exit(1);
  }
  if (from < 0 || from > 63 || to < 0 || to > 63){
    fprintf(stderr,"FATAL: invalid move from=%d to=%d\n",from,to);
    exit(1);
  }

  const int piece = board[from];
  if (piece == 0){
    fprintf(stderr,"ERROR: no piece at square %d (move 0x%x)\n",from,m);
    exit(1);
  }

  /* Save state */
  *amp++ = value;
  *amp++ = flag;
  *amp++ = eppos;
  *amp++ = from;
  *amp++ = to;

  const int captured = board[to];
  *amp++ = captured;

  if (captured != 0)
    value -= abs_val(pval[6 + captured]);

  /* save eppos before clearing it */
  const int old_eppos = eppos;

  board[to] = piece;
  board[from] = 0;
  eppos = -1;

  switch (piece){
  case -1: /* White pawn */
    if (to - from == -16){
      /* Double push: set en-passant square */
      eppos = from - 8;
      *amp++ = MOVE_NORMAL;
    } else if (to < 8){
      /* promotion checked BEFORE diagonal capture test.
       * A capture-promotion (e.g. bxa8=Q) has to-from==-7 or -9
       * AND to<8 simultaneously; the old order fell into the
       * diagonal branch first and never promoted. */
      value -= (abs_val(pval[1]) - abs_val(pval[5]));
      board[to] = -5;
      *amp++ = MOVE_PROMO;
    } else if (to - from == -7 || to - from == -9){
      /* Diagonal non-promotion move (en-passant or normal capture) */
      if (captured == 0 && to == old_eppos){
        const int ep_sq = to + 8;
        value -= abs_val(pval[6 + board[ep_sq]]);
        board[ep_sq] = 0;
        *amp++ = MOVE_EP;
      } else{
        *amp++ = MOVE_NORMAL;
      }
    } else{
      *amp++ = MOVE_NORMAL;
    }
    break;

  case -2: /* White knight */
    *amp++ = MOVE_NORMAL;
    break;

  case -3: /* White bishop */
    *amp++ = MOVE_NORMAL;
    break;

  case -4: /* White rook */
    if (from == 63) flag &= ~WKCASTLE;
    else if (from == 56) flag &= ~WQCASTLE;
    *amp++ = MOVE_NORMAL;
    break;

  case -5: /* White queen */
    *amp++ = MOVE_NORMAL;
    break;

  case -6: /* White king */
    wkpos = to;
    flag &= ~(WKCASTLE | WQCASTLE);
    if (from == 60 && to == 62){
      board[61] = -4;
      board[63] = 0;
      *amp++ = MOVE_KCASTLE;
    } else if (from == 60 && to == 58){
      board[59] = -4;
      board[56] = 0;
      *amp++ = MOVE_QCASTLE;
    } else{
      *amp++ = MOVE_NORMAL;
    }
    break;

  default:
    *amp++ = MOVE_NORMAL;
    break;
  }
}

/* ------------------------------------------------------------------ */
/* black_move                                                           */
/* ------------------------------------------------------------------ */
void black_move(const int m){
  const int from = (m >> 8) & 0xFF;
  const int to = m & 0xFF;

  if (m == 0xfffffffc || m == -4){
    fprintf(stderr,"FATAL: attempt to make sentinel move\n");
    exit(1);
  }

  const int piece = board[from];

  /* Save state */
  *amp++ = value;
  *amp++ = flag;
  *amp++ = eppos;
  *amp++ = from;
  *amp++ = to;

  const int captured = board[to];
  *amp++ = captured;

  if (captured != 0)
    value += abs_val(pval[6 + captured]);

  /* save eppos before clearing it */
  const int old_eppos = eppos;

  board[to] = piece;
  board[from] = 0;
  eppos = -1;

  switch (piece){
  case 1: /* Black pawn */
    if (to - from == 16){
      eppos = from + 8;
      *amp++ = MOVE_NORMAL;
    } else if (to > 55){
      /* promotion checked BEFORE diagonal capture test.
       * A capture-promotion (e.g. cxb1=Q) has to-from==7 or 9
       * AND to>55 simultaneously; the old order fell into the
       * diagonal branch first and stored MOVE_NORMAL — the pawn
       * stayed on the promotion square instead of becoming a queen,
       * hiding the check on the white king. */
      value += (abs_val(pval[11]) - abs_val(pval[7]));
      board[to] = 5;
      *amp++ = MOVE_PROMO;
    } else if (to - from == 7 || to - from == 9){
      /* Diagonal non-promotion move (en-passant or normal capture) */
      if (captured == 0 && to == old_eppos){
        const int ep_sq = to - 8;
        value += abs_val(pval[6 + board[ep_sq]]);
        board[ep_sq] = 0;
        *amp++ = MOVE_EP;
      } else{
        *amp++ = MOVE_NORMAL;
      }
    } else{
      *amp++ = MOVE_NORMAL;
    }
    break;

  case 2: /* Black knight */
    *amp++ = MOVE_NORMAL;
    break;

  case 3: /* Black bishop */
    *amp++ = MOVE_NORMAL;
    break;

  case 4: /* Black rook */
    if (from == 7) flag &= ~BKCASTLE;
    else if (from == 0) flag &= ~BQCASTLE;
    *amp++ = MOVE_NORMAL;
    break;

  case 5: /* Black queen */
    *amp++ = MOVE_NORMAL;
    break;

  case 6: /* Black king */
    bkpos = to;
    flag &= ~(BKCASTLE | BQCASTLE);
    if (from == 4 && to == 6){
      board[5] = 4;
      board[7] = 0;
      *amp++ = MOVE_KCASTLE;
    } else if (from == 4 && to == 2){
      board[3] = 4;
      board[0] = 0;
      *amp++ = MOVE_QCASTLE;
    } else{
      *amp++ = MOVE_NORMAL;
    }
    break;

  default:
    *amp++ = MOVE_NORMAL;
    break;
  }
}

/* ------------------------------------------------------------------ */
/* white_undo                                                           */
/* ------------------------------------------------------------------ */
void white_undo(void){
  const int move_type = *--amp;
  const int captured = *--amp;
  const int to = *--amp;
  const int from = *--amp;

  eppos = *--amp;
  flag = *--amp;
  value = *--amp;

  board[from] = board[to];
  board[to] = captured;

  switch (move_type){
  case MOVE_KCASTLE:
    board[63] = -4;
    board[61] = 0;
    wkpos = 60;
    break;
  case MOVE_QCASTLE:
    board[56] = -4;
    board[59] = 0;
    wkpos = 60;
    break;
  case MOVE_EP:
    board[to + 8] = 1; /* restore the captured black pawn */
    break;
  case MOVE_PROMO:
    board[from] = -1; /* restore the white pawn */
    break;
  default: ;
  }

  if (board[from] == -6) wkpos = from;
}

/* ------------------------------------------------------------------ */
/* black_undo                                                           */
/* ------------------------------------------------------------------ */
void black_undo(void){
  const int move_type = *--amp;
  const int captured = *--amp;
  const int to = *--amp;
  const int from = *--amp;

  eppos = *--amp;
  flag = *--amp;
  value = *--amp;

  board[from] = board[to];
  board[to] = captured;

  switch (move_type){
  case MOVE_KCASTLE:
    board[7] = 4;
    board[5] = 0;
    bkpos = 4;
    break;
  case MOVE_QCASTLE:
    board[0] = 4;
    board[3] = 0;
    bkpos = 4;
    break;
  case MOVE_EP:
    board[to - 8] = -1; /* restore the captured white pawn */
    break;
  case MOVE_PROMO:
    board[from] = 1; /* restore the black pawn */
    break;
  default: ;
  }

  if (board[from] == 6) bkpos = from;
}

/*
 * extensions.c — Search extensions for check and capture
 * 
 * Improves tactical depth by extending search in critical positions:
 * - Check extension: extend by 1 ply when giving check
 * - Capture extension: extend by 1 ply when capturing high-value pieces
 */
#include "chess.h"
/* move_gives_check - determine if a move delivers check */
int move_gives_check(const int move){
  const int from = (move >> 8) & 0xFF;
  const int to = move & 0xFF;
  const int piece = board[from];
  const int captured = board[to];
  const int original_eppos = eppos;
  int result = 0;

  if (piece == 0) return 0;

  /* Temporarily make the move */
  board[to] = piece;
  board[from] = 0;

  /* Handle special moves */
  if (piece == -1 && to < 8){
    /* Promotion - treat as queen for check detection */
    board[to] = -5;
  } else if (piece == 1 && to > 55){
    board[to] = 5;
  } else if (piece == -6 && from == 60 && to == 62){
    /* White kingside castle */
    if (board[61] == 0 && board[62] == 0 && board[63] == -4){
      board[61] = -4;
      board[63] = 0;
    }
  } else if (piece == -6 && from == 60 && to == 58){
    /* White queenside castle */
    if (board[59] == 0 && board[58] == 0 && board[57] == 0 && board[56] == -4){
      board[59] = -4;
      board[56] = 0;
    }
  } else if (piece == 6 && from == 4 && to == 6){
    /* Black kingside castle */
    if (board[5] == 0 && board[6] == 0 && board[7] == 4){
      board[5] = 4;
      board[7] = 0;
    }
  } else if (piece == 6 && from == 4 && to == 2){
    /* Black queenside castle */
    if (board[3] == 0 && board[2] == 0 && board[1] == 0 && board[0] == 4){
      board[3] = 4;
      board[0] = 0;
    }
  } else if (piece == -1 && to == original_eppos){
    /* White en passant */
    board[to + 8] = 0;
  } else if (piece == 1 && to == original_eppos){
    /* Black en passant */
    board[to - 8] = 0;
  }

  /* Check if the move gives check */
  if (piece < 0){
    /* White piece moving - check if black king is attacked */
    result = white_attacks(bkpos);
  } else{
    /* Black piece moving - check if white king is attacked */
    result = black_attacks(wkpos);
  }

  /* Undo changes */
  board[from] = piece;
  board[to] = captured;

  /* Restore special move state */
  if (piece == -6 && from == 60 && to == 62){
    if (board[61] == -4 && board[63] == 0){
      board[61] = 0;
      board[63] = -4;
    }
  } else if (piece == -6 && from == 60 && to == 58){
    if (board[59] == -4 && board[56] == 0){
      board[59] = 0;
      board[56] = -4;
    }
  } else if (piece == 6 && from == 4 && to == 6){
    if (board[5] == 4 && board[7] == 0){
      board[5] = 0;
      board[7] = 4;
    }
  } else if (piece == 6 && from == 4 && to == 2){
    if (board[3] == 4 && board[0] == 0){
      board[3] = 0;
      board[0] = 4;
    }
  } else if (piece == -1 && to == original_eppos){
    if (board[to + 8] == 0){
      board[to + 8] = 1;
    }
  } else if (piece == 1 && to == original_eppos){
    if (board[to - 8] == 0){
      board[to - 8] = -1;
    }
  }

  return result;
}

/* capture_worth_extending - determine if a capture should extend search */
int capture_worth_extending(const int move, const int depth_remaining){
  const int to = move & 0xFF;
  const int captured = board[to];

  /* No capture */
  if (captured == 0) return 0;

  /* Don't extend at very shallow depths to avoid explosion */
  if (depth_remaining < 2) return 0;

  /* Make sure we don't extend too aggressively */
  if (depth_remaining > 6) return 0; /* Limit maximum extension depth */

  const int captured_val = abs_val(ipval[6 + captured]);

  /* Only extend Queen captures (most tactical) */
  if (captured_val >= 900) return 1; /* Queen only */

  return 0;
}

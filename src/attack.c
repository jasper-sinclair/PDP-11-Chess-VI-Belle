#include "chess.h"
/*
 * black_attacks - check if black attacks a square
 * Based on original att.s from PDP-11 chess
 */
int black_attacks(const int pos){
  int piece;
  int to;

  /* Pawn attacks - black pawns attack upward (to lower ranks) */
  if ((dir[pos] & uleft) == 0){
    to = pos - 9;
    if (to >= 0 && to < 64 && board[to] == 1)
      return 1;
  }
  if ((dir[pos] & uright) == 0){
    to = pos - 7;
    if (to >= 0 && to < 64 && board[to] == 1)
      return 1;
  }

  /* Knight attacks */
  for (int i = 0; i < 8; i++){
    to = pos + knight_offsets[i];
    if (to >= 0 && to < 64 && (dir[pos] & knight_masks[i]) == 0){
      if (board[to] == 2)
        return 1;
    }
  }

  /* King attacks - adjacent squares */
  for (int i = 0; i < 8; i++){
    to = pos + king_offsets[i];
    if (to >= 0 && to < 64 && (dir[pos] & dir_masks[i]) == 0){
      if (board[to] == 6)
        return 1;
    }
  }

  /* Diagonal attacks (bishop/queen) */
  for (int d = 0; d < 4; d++){
    to = pos;
    while (1){
      to += dir_offsets[d];
      if (to < 0 || to >= 64)
        break;

      const int from_sq = to - dir_offsets[d];
      if (from_sq < 0 || from_sq >= 64) break;
      if ((dir[from_sq] & dir_masks[d]) != 0)
        break;

      piece = board[to];
      if (piece != 0){
        if (piece == 3 || piece == 5) /* Bishop or Queen */
          return 1;
        break;
      }
    }
  }

  /* Orthogonal attacks (rook/queen) */
  for (int d = 4; d < 8; d++){
    to = pos;
    while (1){
      to += dir_offsets[d];
      if (to < 0 || to >= 64)
        break;

      const int from_sq = to - dir_offsets[d];
      if (from_sq < 0 || from_sq >= 64) break;
      if ((dir[from_sq] & dir_masks[d]) != 0)
        break;

      piece = board[to];
      if (piece != 0){
        if (piece == 4 || piece == 5) /* Rook or Queen */
          return 1;
        break;
      }
    }
  }

  return 0;
}

/*
 * white_attacks - check if white attacks a square
 * Based on original att.s from PDP-11 chess
 */
int white_attacks(const int pos){
  int piece;
  int to;

  /* Pawn attacks - white pawns attack downward (to higher ranks) */
  if ((dir[pos] & dleft) == 0){
    to = pos + 7;
    if (to >= 0 && to < 64 && board[to] == -1)
      return 1;
  }
  if ((dir[pos] & dright) == 0){
    to = pos + 9;
    if (to >= 0 && to < 64 && board[to] == -1)
      return 1;
  }

  /* Knight attacks */
  for (int i = 0; i < 8; i++){
    to = pos + knight_offsets[i];
    if (to >= 0 && to < 64 && (dir[pos] & knight_masks[i]) == 0){
      if (board[to] == -2)
        return 1;
    }
  }

  /* King attacks */
  for (int i = 0; i < 8; i++){
    to = pos + king_offsets[i];
    if (to >= 0 && to < 64 && (dir[pos] & dir_masks[i]) == 0){
      if (board[to] == -6)
        return 1;
    }
  }

  /* Diagonal attacks (bishop/queen) */
  for (int d = 0; d < 4; d++){
    to = pos;
    while (1){
      to += dir_offsets[d];
      if (to < 0 || to >= 64)
        break;

      const int from_sq = to - dir_offsets[d];
      if (from_sq < 0 || from_sq >= 64) break;
      if ((dir[from_sq] & dir_masks[d]) != 0)
        break;

      piece = board[to];
      if (piece != 0){
        if (piece == -3 || piece == -5) /* Bishop or Queen */
          return 1;
        break;
      }
    }
  }

  /* Orthogonal attacks (rook/queen) */
  for (int d = 4; d < 8; d++){
    to = pos;
    while (1){
      to += dir_offsets[d];
      if (to < 0 || to >= 64)
        break;

      const int from_sq = to - dir_offsets[d];
      if (from_sq < 0 || from_sq >= 64) break;
      if ((dir[from_sq] & dir_masks[d]) != 0)
        break;

      piece = board[to];
      if (piece != 0){
        if (piece == -4 || piece == -5) /* Rook or Queen */
          return 1;
        break;
      }
    }
  }

  return 0;
}

/*
 * attackers - fill attacv array with pieces attacking a square
 */
void attackers(const int pos){
  int* ap = attacv;
  int piece;
  int to;

  for (int i = 0; i < 64; i++)
    attacv[i] = 0;

  /* Pawn attacks - white and black */
  if ((dir[pos] & uleft) == 0){
    to = pos - 9;
    if (to >= 0 && to < 64 && board[to] == -1)
      *ap++ = -1;
  }
  if ((dir[pos] & uright) == 0){
    to = pos - 7;
    if (to >= 0 && to < 64 && board[to] == -1)
      *ap++ = -1;
  }
  if ((dir[pos] & dleft) == 0){
    to = pos + 7;
    if (to < 64 && to >= 0 && board[to] == 1)
      *ap++ = 1;
  }
  if ((dir[pos] & dright) == 0){
    to = pos + 9;
    if (to < 64 && to >= 0 && board[to] == 1)
      *ap++ = 1;
  }

  /* Knight attacks */
  for (int i = 0; i < 8; i++){
    to = pos + knight_offsets[i];
    if (to >= 0 && to < 64 && (dir[pos] & knight_masks[i]) == 0){
      piece = board[to];
      if (piece == 2 || piece == -2)
        *ap++ = piece;
    }
  }

  /* Sliding piece attacks - diagonal directions */
  for (int d = 0; d < 4; d++){
    to = pos;
    while (1){
      to += dir_offsets[d];
      if (to < 0 || to >= 64)
        break;

      const int from_sq = to - dir_offsets[d];
      if (from_sq < 0 || from_sq >= 64) break;
      if ((dir[from_sq] & dir_masks[d]) != 0)
        break;

      piece = board[to];
      if (piece != 0){
        if (piece == 3 || piece == -3 || piece == 5 || piece == -5)
          *ap++ = piece;
        break;
      }
    }
  }

  /* Sliding piece attacks - orthogonal directions */
  for (int d = 4; d < 8; d++){
    to = pos;
    while (1){
      to += dir_offsets[d];
      if (to < 0 || to >= 64)
        break;

      const int from_sq = to - dir_offsets[d];
      if (from_sq < 0 || from_sq >= 64) break;
      if ((dir[from_sq] & dir_masks[d]) != 0)
        break;

      piece = board[to];
      if (piece != 0){
        if (piece == 4 || piece == -4 || piece == 5 || piece == -5)
          *ap++ = piece;
        break;
      }
    }
  }

  /* King attacks */
  for (int i = 0; i < 8; i++){
    to = pos + king_offsets[i];
    if (to >= 0 && to < 64 && (dir[pos] & dir_masks[i]) == 0){
      piece = board[to];
      if (piece == 6 || piece == -6)
        *ap++ = piece;
    }
  }
}

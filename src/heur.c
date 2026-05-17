#include "chess.h"
/*
 * White heuristics - based on original wheur.c
 */
int white_heur1(void){
  int i;
  int pto = 0;

  if (amp[-2]){
    i = amp[-3];
    pto = board[i];
    board[i] = 0;
  }

  int* p1 = lmp;
  gen_white_moves();
  const int* p2 = lmp;
  lmp = p1;
  gen_black_moves();
  const int* p3 = lmp;
  lmp = p1;

  i = (p2 - p3) / 2;

  if (amp[-2])
    board[amp[-3]] = pto;

  return i;
}

int white_heur2(void){
  int i = 0;

  if (game > 2) return i;

  /* Minor pieces out */
  i += 9 * ((board[57] != -2) + (board[62] != -2));
  i += 8 * ((board[58] != -3) + (board[61] != -3));

  /* Blocked central pawns */
  if (board[51] == -1 && board[43] != 0) i -= 10;
  if (board[52] == -1 && board[44] != 0) i -= 10;

  return i;
}

int white_heur3(void){
  int i = 0;

  if (flag & WKCASTLE) i += 30;
  if (flag & WQCASTLE) i += 30;

  if ((flag & WQCASTLE) && board[48] == -1 && board[49] == -1 && board[50] == -1)
    i += 20;

  if ((flag & WKCASTLE) && board[53] == -1 && board[54] == -1 && board[55] == -1)
    i += 20;

  if (wkpos == 58 || wkpos == 62){
    i += 60;

    if (wkpos == 58){
      if (board[50] == -1) i += 10;
      if (board[49] == -1) i += 5;
      if (board[48] == -1) i += 5;
    }
    if (wkpos == 62){
      if (board[53] == -1) i += 10;
      if (board[54] == -1) i += 5;
      if (board[55] == -1) i += 5;
    }
  }

  return i;
}

int white_heur4(void){
  if (amp[-1] != 1) return 0;

  const int ploc = amp[-3];
  if (board[ploc] == -1) return 0;
  if (check_heuristic(ploc)) return 0;

  int* p1 = lmp;
  const int* p2 = p1;
  gen_black_legal();
  int i = 0;

  while (p2 < lmp - 2){
    p2++;
    black_move(*p2++);
    i = check_heuristic(ploc);
    black_undo();
    if (i)
      break;
  }

  lmp = p1;
  return i;
}

int white_heur5(void){
  int i, k;
  int pto = 0;

  if (amp[-2]){
    i = amp[-3];
    pto = board[i];
    board[i] = 0;
  }

  for (i = 0; i < 64; i++)
    control[i] = 0;

  if (game < 2){
    for (i = 0; i < 64; i++)
      control[i] += center[i];
  }

  if (mantom){
    if ((flag & (WKCASTLE | WQCASTLE)) == 0 || wkpos != 60){
      surround_king(wkpos);
    }
  } else{
    if ((flag & (BKCASTLE | BQCASTLE)) == 0 || bkpos != 4){
      surround_king(bkpos);
    }
  }

  int s = 0;
  for (i = 0; i < 64; i++){
    const int n = control[i] * 100;
    attackers(i);
    int j = 0;
    while ((k = attacv[j++]) != 0){
      const int d = pval[6 + k];
      if (d < 0)
        s -= n / (-d);
      else
        s += n / d;
    }
  }

  if (amp[-2])
    board[amp[-3]] = pto;

  return -s;
}

int white_heur6(void){
  int i = 0;

  *amp++ = -1;
  if (white_attacks(bkpos))
    if (check_mate(2,0))
      i += 15;
  amp--;

  return i;
}

/*
 * Black heuristics - based on original bheur.c
 */
int black_heur1(void){
  return -white_heur1();
}

int black_heur2(void){
  int i = 0;

  if (game > 2) return i;

  /* Minor pieces out (Knights at 1, 6; Bishops at 2, 5) */
  i += 9 * ((board[1] != 2) + (board[6] != 2));
  i += 8 * ((board[2] != 3) + (board[5] != 3));

  /* Blocked central pawns (Black pawns at 11, 12; blocked at 19, 20) */
  if (board[11] == 1 && board[19] != 0) i -= 10;
  if (board[12] == 1 && board[20] != 0) i -= 10;

  return i;
}

int black_heur3(void){
  int i = 0;

  if (flag & BKCASTLE) i += 30;
  if (flag & BQCASTLE) i += 30;

  if ((flag & BQCASTLE) && board[8] == 1 && board[9] == 1 && board[10] == 1)
    i += 20;

  if ((flag & BKCASTLE) && board[13] == 1 && board[14] == 1 && board[15] == 1)
    i += 20;

  if (bkpos == 2 || bkpos == 6){
    i += 60;

    if (bkpos == 2){
      if (board[10] == 1) i += 10;
      if (board[9] == 1) i += 5;
      if (board[8] == 1) i += 5;
    }
    if (bkpos == 6){
      if (board[13] == 1) i += 10;
      if (board[14] == 1) i += 5;
      if (board[15] == 1) i += 5;
    }
  }

  return i;
}

int black_heur4(void){
  if (amp[-1] != 1) return 0;

  const int ploc = amp[-3];
  if (board[ploc] == 1) return 0;
  if (check_heuristic(ploc)) return 0;

  int* p1 = lmp;
  const int* p2 = p1;
  gen_white_legal();
  int i = 0;

  while (p2 < lmp - 2){
    p2++;
    white_move(*p2++);
    i = check_heuristic(ploc);
    white_undo();
    if (i)
      break;
  }

  lmp = p1;
  return -i;
}

int black_heur5(void){
  return -white_heur5();
}

int black_heur6(void){
  int i = 0;

  *amp++ = -1;
  if (black_attacks(wkpos))
    if (check_mate(2,0))
      i += 15;
  amp--;

  return i;
}

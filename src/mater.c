#include "chess.h"

int check_mate(const int n, const int f){
  int a, b;
  const int original_mantom = mantom;

  mantom = ! mantom;

  if (f == 0){
    b = mate_search(n);
    mantom = original_mantom;
    return b;
  }

  b = 0;
  if (matflg){
    a = 1;
    while (a <= n){
      if (! mate_search(a)){
        if (a >= n){
          matflg = 0;
          mantom = original_mantom;
          return 0;
        }
        a++;
        continue;
      }
      b = abmove;
      break;
    }
    goto out;
  }

  a = n;
  while (a >= 0 && mate_search(a)){
    if (a == mdepth){
      matflg++;
    }
    b = abmove;
    if (a == 0)
      break;
    a--;
  }

out:
  mantom = original_mantom;
  if (b){
    int* saved_lmp = lmp;
    if (original_mantom){
      gen_white_legal();
    } else{
      gen_black_legal();
    }

    const int* p = saved_lmp;
    int move_found = 0;
    while (p < lmp){
      if (*(p + 1) == b){
        move_found = 1;
        break;
      }
      p += 2;
    }

    lmp = saved_lmp;

    if (move_found){
      abmove = b;
      return 1;
    }
    return 0;
  }
  return 0;
}

int mate_search(int ns){
  const int original_mantom = ! mantom;

  if (intrp || --ns < 0){
    return 0;
  }

  int* p1 = lmp;
  int* saved_lmp = lmp;
  const int* p2 = p1;
  int* p3 = p1;

  if (original_mantom)
    gen_white_moves();
  else
    gen_black_moves();

  while (p2 < lmp - 2){
    if (p2 + 1 >= lmp - 2) break;

    const int move = *(p2 + 1);
    if (move == 0xfffffffc || move == -4) break;

    if (original_mantom)
      white_move(move);
    else
      black_move(move);

    if (original_mantom){
      if (white_attacks(bkpos)){
        *p3 = move;
        p3++;
      }
    } else{
      if (black_attacks(wkpos)){
        *p3 = move;
        p3++;
      }
    }

    if (original_mantom)
      white_undo();
    else
      black_undo();

    p2 += 2;
  }

  lmp = p3;
  p2 = p1;

  while (p2 < lmp){
    const int move = *p2;
    if (move == 0xfffffffc || move == -4) break;

    if (original_mantom)
      white_move(move);
    else
      black_move(move);

    const int f = mate_recursive(ns);

    if (original_mantom)
      white_undo();
    else
      black_undo();

    if (f){
      abmove = move;
      lmp = saved_lmp;
      return 1;
    }
    p2++;
  }

  lmp = saved_lmp;
  return 0;
}

int mate_recursive(int ns){
  const int current_mantom = mantom;

  const int* p1 = lmp;
  int* saved_lmp = lmp;
  const int* p2 = p1;

  if (current_mantom)
    gen_black_legal();
  else
    gen_white_legal();

  if (p2 + 2 == lmp && repetition() == 0)
    ns++;

  while (p2 < lmp - 2){
    p2++;
    const int move = *p2;

    if (current_mantom)
      black_move(move);
    else
      white_move(move);

    const int f = mate_search(ns);

    if (current_mantom)
      black_undo();
    else
      white_undo();

    if (! f){
      lmp = saved_lmp;
      return 0;
    }
    p2++;
  }

  lmp = saved_lmp;
  return 1;
}

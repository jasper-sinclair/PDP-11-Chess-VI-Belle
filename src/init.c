#include <stdio.h>
#include "chess.h"

void print_clock(char* s, const int t){
  printf("%s: %d:%d%d\n",s,t / 60,(t / 10) % 6,t % 10);
}

int in_check(void){
  if (mantom){
    return white_attacks(bkpos);
  }
  return black_attacks(wkpos);
}

void inc_move(void){
  clktim[mantom] += get_clock_ms();
  if (mantom){
    moveno++;
  }
  mantom = ! mantom;
}

void dec_move(void){
  mantom = ! mantom;
  if (mantom){
    moveno--;
  }
}

void position_setup(void){
  int i, a;

  qdepth = depth + 8;

  for (i = 0; i < 13; i++)
    pval[i] = ipval[i];

  value = 0;
  for (i = 0; i < 64; i++){
    a = board[i];
    value += pval[6 + a];
  }

  if (value > 150)
    gval = 1;
  else if (value < -150)
    gval = -1;
  else
    gval = 0;

  for (i = -6; i <= 6; i++){
    a = pval[6 + i];
    if (a < 0)
      a -= 50;
    else
      a += 50;

    if (a < 0)
      a = -((-a) / 100);
    else
      a /= 100;

    if (i != 0)
      pval[6 + i] = a * 100 - gval;
  }

  a = 13800;
  for (i = 0; i < 64; i++)
    a -= abs_val(pval[6 + board[i]]);

  if (a > 4000)
    game = 3;
  else if (a > 2000)
    game = 2;
  else if (moveno > 5)
    game = 1;
  else
    game = 0;
}

void position_traverse(void (*f)(int, intptr_t), const int* p, const intptr_t a){
  const int* current_amp = amp;

  while (amp != p){
    const int m = (amp[3] << 8) | (amp[4] & 0377);
    (*f)(m,a);

    if (mantom){
      black_move(m);
      moveno++;
      mantom = 0;
    } else{
      white_move(m);
      moveno++;
      mantom = 1;
    }
  }

  while (amp != current_amp){
    if (mantom){
      mantom = 0;
      white_undo();
      moveno--;
    } else{
      mantom = 1;
      black_undo();
      moveno--;
    }
  }
}

void repetition_check(const int m, const intptr_t a){
  int* array = (int*)a;

  (void)m;

  if (mantom != array[64])
    return;

  for (int i = 0; i < 64; i++)
    if (board[i] != array[i])
      return;

  array[65]++;
}

int repetition(void){
  int a[66], i;
  int saved_board[64];
  int* saved_amp = amp;
  const int saved_mantom = mantom;
  const int saved_value = value;
  const int saved_flag = flag;
  const int saved_eppos = eppos;
  const int saved_wkpos = wkpos;
  const int saved_bkpos = bkpos;
  const int saved_moveno = moveno;

  for (i = 0; i < 64; i++){
    a[i] = board[i];
    saved_board[i] = board[i];
  }

  a[64] = mantom;
  a[65] = 0;

  i = amp - saved_amp;
  const int* p = amp;

  while (amp[-1] != -1){
    if (amp[-2])
      break;

    i = board[amp[-3]];
    if (i == 1 || i == -1)
      break;

    if (mantom) white_undo();
    else black_undo();
    dec_move();
  }

  const intptr_t array_ptr = (intptr_t)a;
  position_traverse(repetition_check,p,array_ptr);

  amp = saved_amp;
  mantom = saved_mantom;
  value = saved_value;
  flag = saved_flag;
  eppos = saved_eppos;
  wkpos = saved_wkpos;
  bkpos = saved_bkpos;
  moveno = saved_moveno;
  for (i = 0; i < 64; i++)
    board[i] = saved_board[i];

  return a[65];
}

int abs_val(const int x){
  return x < 0?-x:x;
}

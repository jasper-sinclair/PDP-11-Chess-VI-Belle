#include <stdio.h>
#include "chess.h"
#include "winboard.h"
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__DragonFly__)
#undef getchar
#endif

void print_score_wrapper(const int m, const intptr_t a){
  (void)a;
  print_score1(m);
}

void read_line(void){
  int c;

  char* p1 = sbuf;
  while ((c = getchar()) != '\n' && c != EOF){
    *p1++ = c;
  }
  *p1++ = '\0';
}

int get_char(void){
  char c;

  if (read(0,&c,1) != 1)
    return EOF;

  return c;
}

void print_board(void){
  if (winboard_mode != WB_MODE_UNDEFINED)
    return; /* Never print board in WinBoard mode */

  int i = 0;

  for (int x = 7; x >= 0; x--){
    if (! mantom || mfmt)
      putchar_c('1' + x);
    else
      putchar_c('8' - x);
    putchar_c(' ');

    for (int y = 0; y < 8; y++){
      putchar_c(' ');
      const int p = board[i++];
      if (p != 0){
        const char piece = "kqrbnp.PNBRQK"[p + 6];
        putchar_c(piece);
      } else{
        if (((i - 1) & 1) != 0)
          putchar_c('*');
        else
          putchar_c('-');
      }
    }
    putchar_c('\n');

    if (intrp)
      return;
  }

  if (mfmt)
    printf("\n   a b c d e f g h   [Algebraic format: ON]");
  else
    printf("\n   q q q q k k k k\n   r n b     b n r   [Algebraic format: OFF]");
  printf("\n");
}

void print_move1(const int m){
  printf("%d. ",moveno);
  if (mantom)
    printf("... ");
  print_move(m);
  putchar_c('\n');
}

void print_move(const int m){
  const int from = m >> 8;
  const int to = m & 0377;
  int epf = 0, pmf = 0;

  if (mfmt){
    print_algebraic(from);
    print_algebraic(to);
    return;
  }

  if (mantom) black_move(m);
  else white_move(m);

  switch (amp[-1]){
  case MOVE_NORMAL:
    print_piece(board[to]);
    putchar_c('/');
    print_square(from);
    if (amp[-2]){
      putchar_c('x');
      print_piece(amp[-2]);
      putchar_c('/');
    } else{
      putchar_c('-');
    }
    print_square(to);
    break;

  case MOVE_KCASTLE:
  case MOVE_QCASTLE:
    putchar_c('o');
    putchar_c('-');
    putchar_c('o');
    break;

  case MOVE_EP:
    epf = 1;
    print_piece(board[to]);
    putchar_c('/');
    print_square(from);
    if (amp[-2]){
      putchar_c('x');
      print_piece(amp[-2]);
      putchar_c('/');
    } else{
      putchar_c('-');
    }
    print_square(to);
    break;

  case MOVE_PROMO:
    pmf = 1;
    print_piece(board[to]);
    putchar_c('/');
    print_square(from);
    if (amp[-2]){
      putchar_c('x');
      print_piece(amp[-2]);
      putchar_c('/');
    } else{
      putchar_c('-');
    }
    print_square(to);
    break;
  default: ;
  }

  if (pmf){
    putchar_c('(');
    putchar_c('q');
    putchar_c(')');
  }
  if (epf){
    putchar_c('e');
    putchar_c('p');
  }
  if (in_check())
    putchar_c('+');

  if (mantom) black_undo();
  else white_undo();
}

void print_piece(int p){
  if (p < 0)
    p = -p;
  p = "ppnbrqk"[p];
  putchar_c(p);
}

void print_square(const int b){
  const int r = b / 8;
  int f = b % 8;

  if (f < 4){
    putchar_c('q');
  } else{
    putchar_c('k');
    f = 7 - f;
  }

  const char piece = "rnb\0"[f];
  if (piece)
    putchar_c(piece);

  putchar_c(mantom?r + '1':'8' - r);
}

void print_algebraic(const int p){
  putchar_c('a' + (p % 8));
  putchar_c('8' - (p / 8));
}

void putchar_c(const int c){
  const char ch = c;
  _write(1,&ch,1);
}

void print_time(const int a, const int b){
  printf("time = %d/%d\n",a,b);
}

void print_score1(const int m){
  if (intrp)
    return;

  if (! mantom){
    if (moveno < 10)
      putchar_c(' ');
    else
      putchar_c(moveno / 10 + '0');
    putchar_c(moveno % 10 + '0');
    putchar_c('.');
    putchar_c(' ');
  } else{
    while (column < 20)
      putchar_c(' ');
  }

  print_move(m);

  if (mantom)
    putchar_c('\n');
}

void print_score(void){
  putchar_c('\n');
  const intptr_t p = (intptr_t)amp;

  while (amp[-1] != -1){
    if (mantom) white_undo();
    else black_undo();
    dec_move();
  }

  position_traverse(print_score_wrapper,(int*)p,0);
  putchar_c('\n');
}


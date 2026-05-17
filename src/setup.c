// setup.c - Fixed version
#include <stdio.h>
#include <stdlib.h>
#include "chess.h"
#include "winboard.h"

void setup_board(void){
  /* In WinBoard mode, just reset to standard starting position */
  if (winboard_mode != WB_MODE_UNDEFINED){
    /* Reset board to standard starting position */
    int i;

    /* Clear board */
    for (i = 0; i < 64; i++){
      board[i] = 0;
    }

    /* Place black pieces (positive numbers) */
    board[0] = 4; /* rook */
    board[1] = 2; /* knight */
    board[2] = 3; /* bishop */
    board[3] = 5; /* queen */
    board[4] = 6; /* king */
    board[5] = 3; /* bishop */
    board[6] = 2; /* knight */
    board[7] = 4; /* rook */

    /* Black pawns */
    for (i = 8; i < 16; i++){
      board[i] = 1;
    }

    /* White pieces (negative numbers) */
    board[56] = -4; /* rook */
    board[57] = -2; /* knight */
    board[58] = -3; /* bishop */
    board[59] = -5; /* queen */
    board[60] = -6; /* king */
    board[61] = -3; /* bishop */
    board[62] = -2; /* knight */
    board[63] = -4; /* rook */

    /* White pawns */
    for (i = 48; i < 56; i++){
      board[i] = -1;
    }

    /* Reset game state */
    amp = ambuf + 1;
    lmp = lmbuf + 1;
    eppos = 64;
    bookp = 0;
    mantom = 0;
    moveno = 1;
    wkpos = 60;
    bkpos = 4;
    flag = 033; /* All castling rights */
    value = 0;
    ivalue = 0;

    /* Update value based on material */
    for (i = 0; i < 64; i++){
      if (board[i] != 0){
        value += pval[6 + board[i]];
      }
    }

    return;
  }

  /* Original interactive setup code for console mode */
  char bd[64];
  char *p, *ip;
  int i, c;
  int wkp = 0, bkp = 0;

  for (p = bd; p < bd + 64;)
    *p++ = 0;

  int err = 0;
  int nkng = 101;
  p = bd;

  for (i = 0; i < 8; i++){
    ip = p + 8;

  loop:
    c = getchar();

    switch (c){
    case 'K':
      nkng -= 100;
      c = 6;
      bkp = p - bd;
      break;
    case 'k':
      nkng--;
      c = -6;
      wkp = p - bd;
      break;
    case 'P':
      c = 1;
      break;
    case 'p':
      c = -1;
      break;
    case 'N':
      c = 2;
      break;
    case 'n':
      c = -2;
      break;
    case 'B':
      c = 3;
      break;
    case 'b':
      c = -3;
      break;
    case 'R':
      c = 4;
      break;
    case 'r':
      c = -4;
      break;
    case 'Q':
      c = 5;
      break;
    case 'q':
      c = -5;
      break;
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
      p += c - '0';
      goto loop;
    case ' ':
      p++;
      goto loop;
    case '\n':
      if (p > ip)
        err++;
      p = ip;
      continue;
    default:
      err++;
      if (c <= 0)
        handle_hup(0);
      goto loop;
    }

    if (p < ip)
      *p++ = c;
    goto loop;
  }

  if (nkng)
    err++;

  if (err){
    printf("Illegal setup\n");
    return;
  }

  for (i = 0; i < 64; i++)
    board[i] = bd[i];

  amp = ambuf + 1;
  lmp = lmbuf + 1;
  eppos = 64;
  bookp = 0;
  mantom = 0;
  moveno = 1;
  wkpos = wkp;
  bkpos = bkp;
  flag = 0;

  if (wkpos == 60){
    if (board[56] == -4)
      flag |= 2;
    if (board[63] == -4)
      flag |= 1;
  }

  if (bkpos == 4){
    if (board[0] == 4)
      flag |= 020;
    if (board[7] == 4)
      flag |= 010;
  }

  printf("Setup successful\n");
}

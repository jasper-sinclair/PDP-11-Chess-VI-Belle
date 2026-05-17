#include "uci.h"
#include <stdio.h>
#include <string.h>
#include "chess.h"
#include "create_book/create_book.h"
#include "winboard.h"

int uci_mode = 0;

int load_fen(const char* fen){
  int sq = 0;
  int i;

  reset_board();

  /* clear board first */
  for (i = 0; i < 64; i++)
    board[i] = 0;

  /* piece placement */
  while (*fen && *fen != ' '){
    const char c = *fen++;

    if (c == '/'){
      continue;
    }

    if (c >= '1' && c <= '8'){
      sq += c - '0';
      continue;
    }

    int piece = 0;

    switch (c){
    case 'P': piece = -1;
      break;
    case 'N': piece = -2;
      break;
    case 'B': piece = -3;
      break;
    case 'R': piece = -4;
      break;
    case 'Q': piece = -5;
      break;
    case 'K': piece = -6;
      break;

    case 'p': piece = 1;
      break;
    case 'n': piece = 2;
      break;
    case 'b': piece = 3;
      break;
    case 'r': piece = 4;
      break;
    case 'q': piece = 5;
      break;
    case 'k': piece = 6;
      break;

    default:
      return 0;
    }

    if (sq >= 64)
      return 0;

    board[sq] = piece;

    if (piece == -6)
      wkpos = sq;

    if (piece == 6)
      bkpos = sq;

    sq++;
  }

  if (*fen == ' ')
    fen++;

  /* side to move */
  mantom = (*fen == 'b');

  while (*fen && *fen != ' ')
    fen++;

  if (*fen == ' ')
    fen++;

  /* castling rights */
  flag = 0;

  if (*fen == '-'){
    fen++;
  } else{
    while (*fen && *fen != ' '){
      switch (*fen){
      case 'K': flag |= 1;
        break;
      case 'Q': flag |= 2;
        break;
      case 'k': flag |= 4;
        break;
      case 'q': flag |= 8;
        break;
      default: ;
      }

      fen++;
    }
  }

  if (*fen == ' ')
    fen++;

  /* en passant */
  eppos = 64;

  if (*fen != '-'){
    const int file = fen[0] - 'a';
    const int rank = fen[1] - '1';

    eppos = (7 - rank) * 8 + file;
  }

  /* recompute material value */
  value = 0;

  for (i = 0; i < 64; i++)
    value += ipval[6 + board[i]];

  return 1;
}

void uci_send_info(
  const int depth,
  const int score,
  const long time_ms,
  const long nodes){
  printf("info depth %d score cp %d time %ld nodes %ld pv",
    depth,
    score,
    time_ms,
    nodes);

  for (int i = 0; i < pv_length[0] && i < MAX_PV_LENGTH; i++){
    const int m = pv_table[0][i];

    const int from = (m >> 8) & 0xFF;
    const int to = m & 0xFF;

    printf(" %c%c%c%c",
      'a' + (from % 8),
      '1' + (7 - from / 8),
      'a' + (to % 8),
      '1' + (7 - to / 8));
  }

  printf("\n");
  fflush(stdout);
}

void uci_send_bestmove(const int move){
  const int from = (move >> 8) & 0xFF;
  const int to = move & 0xFF;

  printf("bestmove %c%c%c%c",
    'a' + (from % 8),
    '1' + (7 - from / 8),
    'a' + (to % 8),
    '1' + (7 - to / 8));

  /* promotion support */
  const int piece = board[from];

  if (piece == -1 && to < 8)
    printf("q");
  else if (piece == 1 && to > 55)
    printf("q");

  printf("\n");
  fflush(stdout);
}

void parse_position(const char* line){
  char* p;

  if (strstr(line,"startpos")){
    reset_board();

    p = strstr(line,"moves");
  } else if ((p = strstr(line,"fen"))){
    char fen[256];
    int i = 0;

    p += 4;

    while (*p &&
      strncmp(p," moves",6) != 0 &&
      i < 255){
      fen[i++] = *p++;
    }

    fen[i] = '\0';

    load_fen(fen);

    p = strstr(line,"moves");
  } else{
    return;
  }

  if (! p)
    return;

  p += 5;

  while (*p){
    while (*p == ' ')
      p++;

    if (strlen(p) < 4)
      break;

    const int from =
      (p[0] - 'a') +
      (7 - (p[1] - '1')) * 8;

    const int to =
      (p[2] - 'a') +
      (7 - (p[3] - '1')) * 8;

    const int move =
      (from << 8) | to;

    winboard_force_move(move);

    while (*p && *p != ' ')
      p++;
  }
}

void uci_loop(void){
  char line[1024];

  while (fgets(line,sizeof(line), stdin)){
    if (!strcmp(line, "uci\n") || !strcmp(line, "uci")){
      printf("id name Belle PDP-11 (chess.6)\n");
      printf("id author Original: Thompson/Condon, modern port: Jim Ablett & Jasper\n");
      printf("uciok\n");
    } else if (!strncmp(line, "book", 4)) {
      create_book();
    } else if (! strncmp(line,"isready",7)){
      printf("readyok\n");
    } else if (! strncmp(line,"ucinewgame",10)){
      reset_board();
    } else if (! strncmp(line,"position",8)){
      parse_position(line);
    } else if (! strncmp(line,"go",2)){
      winboard_search_time = 0;

      const char* p = strstr(line,"wtime");
      if (p)
        winboard_wtime = atoi(p + 6);

      p = strstr(line,"btime");
      if (p)
        winboard_btime = atoi(p + 6);

      p = strstr(line,"winc");
      if (p)
        winboard_winc = atoi(p + 5);

      p = strstr(line,"binc");
      if (p)
        winboard_binc = atoi(p + 5);

      p = strstr(line,"movetime");
      if (p)
        winboard_search_time = atoi(p + 9);

      p = strstr(line,"depth");
      if (p)
        winboard_search_depth = atoi(p + 6);

      /* Determine engine side */
      engine_side = mantom;

      engine_search();

      uci_send_bestmove(abmove);
    } else if (! strncmp(line,"quit",4)){
      break;
    }
    fflush(stdout);
  }
}

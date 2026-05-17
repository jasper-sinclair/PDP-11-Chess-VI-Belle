/*
 * winboard.c — WinBoard / XBoard protocol interface
 */
#include "winboard.h"
#include <ctype.h>
#include <fcntl.h>   /* Added for open() flags */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "chess.h"
#include "uci.h"
/* --- Linux Compatibility Fixes --- */
#ifndef O_BINARY
#define O_BINARY 0   /* Not needed on Linux, but required for the code to compile */
#endif
/* Track last move engine made to avoid processing echo */
static int last_engine_move = 0;
static int engine_has_moved = 0;
/* ------------------------------------------------------------------ */
/* WinBoard global variables                                          */
/* ------------------------------------------------------------------ */
int winboard_mode = WB_MODE_UNDEFINED;
int winboard_force_mode = 0;
int winboard_post_mode = 0;
int winboard_time_control = 0;
int winboard_wtime = 0; /* ms remaining for White  */
int winboard_btime = 0; /* ms remaining for Black  */
int winboard_winc = 0; /* White increment per move (ms) */
int winboard_binc = 0; /* Black increment per move (ms) */
int winboard_movestogo = 0;
int winboard_search_depth = 0;
int winboard_search_time = 0; /* Time from 'st' command in ms */
/* Node counter — reset at the start of each search */
long winboard_nodes = 0;
/* engine_side — which colour the engine plays. */
int engine_side = -1;
/* Internal state */
static int winboard_analyze_mode = 0;
static int winboard_stop_search = 0;
static int winboard_move_sent = 0;
/* Last stat update time for periodic output */
static long last_post_time = 0;
/* ------------------------------------------------------------------ */
/* wb_time_limit_ms — single shared formula for how long to think.   */
/* All three callers (wb_count_node, winboard_should_stop,            */
/* winboard_search) use this so they are always consistent.           */
/* ------------------------------------------------------------------ */
static long wb_time_limit_ms(void){
  long tl;

  if (winboard_search_time > 0)
    return winboard_search_time;

  const long remaining = (engine_side == 0)?winboard_wtime:winboard_btime;
  const long inc = (engine_side == 0)?winboard_winc:winboard_binc;

  if (winboard_time_control && winboard_movestogo > 1){
    /* Conventional clock: divide remaining time over moves left */
    tl = remaining / winboard_movestogo;
  } else{
    /* Increment-based (e.g. 4+2) or movestogo==1:
     * Use remaining / est + 80 % of increment.
     * est = expected moves still to be played, at least 10. */
    int est = 40 - moveno;
    if (est < 10) est = 10;
    tl = remaining / est + inc * 80 / 100;
  }

  /* Safety: never plan to use more than 15 % of remaining time
   * in a single move (avoids runaway depth on low-clock positions) */
  long safety_cap = remaining * 15 / 100;
  if (safety_cap < 500) safety_cap = 500; /* at least 500 ms      */
  if (tl > safety_cap) tl = safety_cap;

  if (tl < 100) tl = 100; /* absolute floor */
  return tl;
}

/* ------------------------------------------------------------------ */
/* wb_count_node — count nodes; batch-check time every 4096 nodes.   */
/* ------------------------------------------------------------------ */
void wb_count_node(void){
  winboard_nodes++;
  if ((winboard_nodes & 4095) == 0 &&
    winboard_time_control && ! winboard_stop_search){
    const long _el = get_current_time_ms() - move_start_time;
    const long _tl = wb_time_limit_ms();
    if (_el >= _tl * 90 / 100)
      winboard_stop_search = 1;
  }
}

/* ------------------------------------------------------------------ */
/* winboard_send_move — "move e2e4\n"                                 */
/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
/* winboard_send_move — NOW HANDLES PROMOTIONS CORRECTLY             */
/* ------------------------------------------------------------------ */
void winboard_send_move(const int move){
  const int from = (move >> 8) & 0xFF;
  const int to = move & 0xFF;
  const int piece = board[from];

  last_engine_move = move;
  engine_has_moved = 1;

  /* Standard move */
  printf("move %c%c%c%c",
    'a' + (from % 8),'1' + (7 - from / 8),
    'a' + (to % 8),'1' + (7 - to / 8));

  /* Append promotion letter if it's a pawn promotion */
  if (piece == -1 && to < 8){ /* White pawn promotes */
    printf("q");
  } else if (piece == 1 && to > 55){ /* Black pawn promotes */
    printf("q");
  }

  printf("\n");
  fflush(stdout);
}

/* ------------------------------------------------------------------ */
/* winboard_send_result                                               */
/* ------------------------------------------------------------------ */
void winboard_send_result(char* result){
  printf("%s\n",result);
  fflush(stdout);
}

/* ------------------------------------------------------------------ */
/* winboard_send_thinking — post-mode thinking line                   */
/* ------------------------------------------------------------------ */
void winboard_send_thinking(
  const int move, const int score,
  const int depth_val, const long time_ms){
  printf("%d %d %ld %ld",depth_val,score,time_ms / 10,winboard_nodes);

  if (pv_length[0] > 0){
    for (int i = 0; i < pv_length[0] && i < MAX_PV_LENGTH; i++){
      const int m = pv_table[0][i];
      const int from = (m >> 8) & 0xFF;
      const int to = m & 0xFF;
      if (from < 0 || from > 63 || to < 0 || to > 63) break;
      printf(" %c%c%c%c",
        'a' + (from % 8),'1' + (7 - from / 8),
        'a' + (to % 8),'1' + (7 - to / 8));
    }
  } else if (move != 0){
    const int from = (move >> 8) & 0xFF;
    const int to = move & 0xFF;
    printf(" %c%c%c%c",
      'a' + (from % 8),'1' + (7 - from / 8),
      'a' + (to % 8),'1' + (7 - to / 8));
  }
  printf("\n");
  fflush(stdout);
}

/* ------------------------------------------------------------------ */
/* winboard_send_stat — send stat command with thinking info          */
/* ------------------------------------------------------------------ */
void winboard_send_stat(
  const int depth_val, const int score, const long time_ms,
  const long nodes, const int move){
  printf("stat %d %d %ld %ld",depth_val,score,time_ms / 10,nodes);

  const int pv_len = pv_length[0];

  if (pv_len == 0 && move != 0){
    const int from = (move >> 8) & 0xFF;
    const int to = move & 0xFF;
    printf(" %c%c%c%c",
      'a' + (from % 8),'1' + (7 - from / 8),
      'a' + (to % 8),'1' + (7 - to / 8));
  } else{
    for (int i = 0; i < pv_len && i < MAX_PV_LENGTH; i++){
      const int m = pv_table[0][i];
      if (m == 0) break;

      const int from = (m >> 8) & 0xFF;
      const int to = m & 0xFF;
      if (from < 0 || from > 63 || to < 0 || to > 63) break;

      printf(" %c%c%c%c",
        'a' + (from % 8),'1' + (7 - from / 8),
        'a' + (to % 8),'1' + (7 - to / 8));
    }
  }
  printf("\n");
  fflush(stdout);
}

/* ------------------------------------------------------------------ */
/* winboard_send_periodic_update                                      */
/* ------------------------------------------------------------------ */
void winboard_send_periodic_update(const int depth_val, const int score, const int best_move){
  const long now = get_current_time_ms();
  if (now - last_post_time >= 500){
    last_post_time = now;
    const long elapsed = now - move_start_time;
    winboard_send_thinking(best_move,score,depth_val,elapsed);
  }
}

/* ------------------------------------------------------------------ */
/* winboard_force_move                                                */
/* ------------------------------------------------------------------ */
void winboard_force_move(const int move){
  if (move == 0) return;

  int* p1 = lmp;
  if (mantom) gen_black_legal();
  else gen_white_legal();
  const int* p2 = p1;
  int found = 0;
  while (p2 < lmp - 2){
    const int lm = *(p2 + 1);
    if ((unsigned)lm == 0xfffffffc) break;
    if ((lm & 0xFFFF) == (move & 0xFFFF)){
      found = 1;
      break;
    }
    p2 += 2;
  }
  lmp = p1;

  if (! found){
    printf("Illegal move\n");
    fflush(stdout);
    return;
  }

  if (mantom) black_move(move);
  else white_move(move);
  inc_move();
}

/* ------------------------------------------------------------------ */
/* winboard_set_time_control                                          */
/* ------------------------------------------------------------------ */
void winboard_set_time_control(
  const int wtime, const int btime,
  const int winc, const int binc, const int movestogo){
  winboard_wtime = wtime;
  winboard_btime = btime;
  winboard_winc = winc;
  winboard_binc = binc;
  winboard_movestogo = movestogo;
  winboard_time_control = 1;
  time_control_enabled = 1;
}

/* ------------------------------------------------------------------  */
/* winboard_should_stop                                                */
/* ------------------------------------------------------------------  */
int winboard_should_stop(void){
  if (winboard_stop_search) return 1;
  const long elapsed = get_current_time_ms() - move_start_time;
  const long time_limit = wb_time_limit_ms();
  if (elapsed >= time_limit * 90 / 100){
    winboard_stop_search = 1;
    return 1;
  }
  return 0;
}

/* ------------------------------------------------------------------ */
/* engine_search                                                    */
/* ------------------------------------------------------------------ */
int engine_search(void){
  int best_move = 0;
  int best_value = (engine_side == 0)?3000:-3000;
  const int max_depth = winboard_search_depth > 0?winboard_search_depth:32;
  const int saved_depth = depth;

  if (engine_side == -1) return 0;

  winboard_stop_search = 0;
  winboard_move_sent = 0;
  winboard_nodes = 0;
  last_post_time = 0;

  clear_search_tables();
  ivalue = (engine_side == 0)?value:-value;

  const long start_time = get_current_time_ms();
  move_start_time = start_time;

  /* Single source of truth for time budget */
  const long time_limit = wb_time_limit_ms();

  #ifdef DEBUG
  fprintf(stderr,"WinBoard search: time_limit=%ld ms, wtime=%d, btime=%d, inc=%d/%d, mtg=%d\n",
    time_limit,winboard_wtime,winboard_btime,
    winboard_winc,winboard_binc,winboard_movestogo);
  #endif

  /* Tell Arena we're thinking */
  /*if (winboard_post_mode){
    printf("stat 0 0 0 0\n");
    fflush(stdout);
  }*/

  for (int cur_depth = 1; cur_depth <= max_depth; cur_depth++){
    long elapsed = get_current_time_ms() - start_time;

    /* Don't start a new iteration if we've used ≥ 60 % of budget */
    if (cur_depth > 1 && elapsed >= time_limit * 60 / 100){
      #ifdef DEBUG
      fprintf(stderr,"WinBoard search: skipping depth %d (elapsed=%ld / limit=%ld)\n",
        cur_depth,elapsed,time_limit);
      #endif
      break;
    }

    depth = cur_depth;
    position_setup();

    best_value = (engine_side == 0)?white_play():black_play();

    if (abmove != 0){
      ivalue = best_value;
      best_move = abmove;

      if (winboard_post_mode){
        const long el2 = get_current_time_ms() - start_time;
        const int display_score = (engine_side == 0)?-best_value:best_value;
        if (uci_mode)
			    uci_send_info(cur_depth, display_score, best_move,el2);
		    else
		      winboard_send_thinking(best_move,display_score,cur_depth,el2);
      }
    }

    if (winboard_stop_search) break;

    /* Stop if mate found */
    if (best_value > 2900 || best_value < -2900) break;

    /* Hard stop if over budget */
    elapsed = get_current_time_ms() - start_time;
    if (elapsed >= time_limit * 90 / 100) break;
  }

  /* No minimum-wait: in blitz every millisecond counts. */

  depth = saved_depth;
  abmove = best_move;

  #ifdef DEBUG
  fprintf(stderr,"WinBoard search: returning move %d after %ld ms\n",
    best_move,get_current_time_ms() - start_time);
  #endif

  return best_value;
}

/* ------------------------------------------------------------------ */
/* winboard_check_game_over                                           */
/* ------------------------------------------------------------------ */
static int winboard_check_game_over(void){
  int* saved_lmp = lmp;

  if (mantom) gen_black_legal();
  else gen_white_legal();
  const int has_moves = (lmp > saved_lmp + 2);
  lmp = saved_lmp;

  if (! has_moves){
    if (in_check()){
      if (mantom)
        winboard_send_result("1-0 {White wins by checkmate}");
      else
        winboard_send_result("0-1 {Black wins by checkmate}");
    } else{
      winboard_send_result("1/2-1/2 {Stalemate}");
    }
    return 1;
  }

  if (repetition() >= 3){
    winboard_send_result("1/2-1/2 {Draw by repetition}");
    return 1;
  }

  return 0;
}

/* ------------------------------------------------------------------ */
/* winboard_make_move                                                 */
/* ------------------------------------------------------------------ */
void winboard_make_move(void){
  int move;

  if (winboard_mode == WB_MODE_FORCE) return;
  if (engine_side == -1) return;

  winboard_move_sent = 0;

  if (winboard_mode == WB_MODE_ANALYZE){
    winboard_analyze_mode = 1;
    engine_search();
    return;
  }

  if (winboard_check_game_over()) return;

  if (book_enabled && book_move()){
    move = abmove;
    if (move != 0 && ! winboard_move_sent){
      winboard_send_move(move);
      winboard_move_sent = 1;
      winboard_force_move(move);
      if (engine_side == 0) winboard_wtime += winboard_winc;
      else winboard_btime += winboard_binc;
    }
    return;
  }

  engine_search();
  move = abmove;

  if (move != 0 && ! winboard_move_sent){
    winboard_send_move(move);
    winboard_move_sent = 1;
    winboard_force_move(move);
    if (engine_side == 0) winboard_wtime += winboard_winc;
    else winboard_btime += winboard_binc;
  } else if (move == 0){
    winboard_check_game_over();
  }
}

/* ------------------------------------------------------------------ */
/* reset_board                                                        */
/* ------------------------------------------------------------------ */
void reset_board(void){
  int i;
  for (i = 0; i < 64; i++) board[i] = 0;
  board[0] = 4;
  board[1] = 2;
  board[2] = 3;
  board[3] = 5;
  board[4] = 6;
  board[5] = 3;
  board[6] = 2;
  board[7] = 4;
  for (i = 8; i < 16; i++) board[i] = 1;
  board[56] = -4;
  board[57] = -2;
  board[58] = -3;
  board[59] = -5;
  board[60] = -6;
  board[61] = -3;
  board[62] = -2;
  board[63] = -4;
  for (i = 48; i < 56; i++) board[i] = -1;
  ambuf[0] = -1;
  lmbuf[0] = -1;
  amp = ambuf + 1;
  lmp = lmbuf + 1;
  eppos = 64;
  mantom = 0;
  moveno = 1;
  wkpos = 60;
  bkpos = 4;
  flag = 033;
  value = 0;
  for (i = 0; i < 64; i++)
    value += ipval[6 + board[i]];

  /* Reset time control variables */
  winboard_search_time = 0;
  winboard_time_control = 0;
  time_control_enabled = 0;
  time_per_move = 5; /* Default */
}

/* ------------------------------------------------------------------ */
/* winboard_process_command                                           */
/* ------------------------------------------------------------------ */
void winboard_process_command(char* cmd){
  char* p = cmd;
  while (*p == ' ') p++;

  if (strncmp(p,"xboard",6) == 0){
    printf("\n");
    fflush(stdout);
  } else if (strncmp(p,"protover",8) == 0){
    printf("feature done=0\n");
    printf("feature myname=""\"Belle PDP-11 (chess.6) ""(Thompson/Condon, modern port by Me)\"\n");
    printf("feature variants=\"normal\"\n");
    printf("feature colors=1\n");
    printf("feature analyze=1\n");
    printf("feature ping=1\n");
    printf("feature setboard=1\n");
    printf("feature playother=1\n");
    printf("feature usermove=1\n");
    printf("feature time=1\n");
    printf("feature done=1\n");
    fflush(stdout);
  } else if (strncmp(p,"new",3) == 0){
    reset_board();
    engine_side = -1;
    winboard_mode = WB_MODE_GO;
    winboard_force_mode = 0;
    winboard_analyze_mode = 0;
    winboard_stop_search = 0;
    winboard_move_sent = 0;
    winboard_post_mode = 0;
    winboard_time_control = 0;
    winboard_search_time = 0;
    time_control_enabled = 0;
    time_per_move = 5;
    abmove = 0;
    last_post_time = 0;
    if (bookf > 0){
      unsigned short initial_ptr;
      lseek(bookf,0, SEEK_SET);
      if (read(bookf,&initial_ptr,2) == 2){
        bookp_white = initial_ptr;
        bookp_black = initial_ptr;
        current_bookp = initial_ptr;
      }
    }
  } else if (strncmp(p, "setboard", 8) == 0) {
    p+=8;

    while (*p == ' ')
      p++;

    if (!load_fen(p)) {

      printf("Error (invalid FEN)\n");
      fflush(stdout);

    }
    else {

      engine_side=-1;
      abmove=0;
    }
  }
  else if (strncmp(p,"force",5) == 0){
    winboard_mode = WB_MODE_FORCE;
    winboard_force_mode = 1;
  } else if (strncmp(p,"go",2) == 0 && (p[2] == '\0' || p[2] == ' ')){
    /* Engine should start playing if it's its turn */
    if (engine_side == -1){
      /* No color set yet - this shouldn't happen with proper protocol */
      engine_side = mantom;
    }

    winboard_mode = WB_MODE_GO;
    winboard_force_mode = 0;
    winboard_analyze_mode = 0;
    winboard_post_mode = 1;
    last_post_time = 0;

    /* Only play if it's our turn */
    if (engine_side == mantom){
      #ifdef DEBUG
      fprintf(stderr,"DEBUG: Our turn - making move\n");
      #endif
      winboard_make_move();
    } else{
      #ifdef DEBUG
      fprintf(stderr,"DEBUG: Not our turn (engine=%d, mantom=%d) - waiting\n",
        engine_side,mantom);
      #endif
    }
  } else if (strncmp(p,"playother",9) == 0){
    engine_side = 1 - mantom;
    winboard_mode = WB_MODE_GO;
    winboard_force_mode = 0;
  } else if (strncmp(p,"white",5) == 0){
    engine_side = 0;
    winboard_mode = WB_MODE_PLAY_WHITE;
  } else if (strncmp(p,"black",5) == 0){
    engine_side = 1;
    winboard_mode = WB_MODE_PLAY_BLACK;
  } else if (strncmp(p,"level",5) == 0){
    int mvstg = 4, base_ms = 0, inc_sec = 0;
    char min_str[64] = {0};
    p += 5;
    if (sscanf(p," %d %63s %d",&mvstg,min_str,&inc_sec) >= 2){
      int secs;
      int mins;
      const char* colon = strchr(min_str,':');
      if (colon){
        mins = atoi(min_str);
        secs = atoi(colon + 1);
      } else{
        mins = atoi(min_str);
        secs = 0;
      }
      base_ms = mins * 60000 + secs * 1000;
    }
    winboard_wtime = base_ms;
    winboard_btime = base_ms;
    winboard_winc = inc_sec * 1000;
    winboard_binc = inc_sec * 1000;
    winboard_movestogo = mvstg;
    winboard_time_control = 1;
    time_control_enabled = 1;
    winboard_search_time = 0; /* Clear st time when using level */
  } else if (strncmp(p,"st",2) == 0 && (p[2] == ' ' || p[2] == '\0')){
    p += 2;
    while (*p == ' ') p++;
    const int seconds = atoi(p);
    winboard_search_time = seconds * 1000; /* Convert to ms */
    winboard_time_control = 1;
    time_control_enabled = 1;
    time_per_move = seconds; /* Also set the engine's time per move */

    #ifdef DEBUG
    fprintf(stderr,"WinBoard: st %d -> time_per_move=%d, winboard_search_time=%d\n",
      seconds,time_per_move,winboard_search_time);
    #endif
  } else if (strncmp(p,"sd",2) == 0 && (p[2] == ' ' || p[2] == '\0')){
    p += 2;
    winboard_search_depth = atoi(p);
    depth = winboard_search_depth; /* Also set engine depth */
  } else if (strncmp(p,"time",4) == 0 && (p[4] == ' ' || p[4] == '\0')){
    p += 4;
    while (*p == ' ') p++;
    const int centiseconds = atoi(p);
    const long ms = centiseconds * 10; /* Convert to milliseconds */

    if (engine_side == 0){
      winboard_wtime = ms;
    } else{
      winboard_btime = ms;
    }

    #ifdef DEBUG
    fprintf(stderr,"WinBoard: time %d -> %ld ms for side %d\n",
      centiseconds,ms,engine_side);
    #endif
  } else if (strncmp(p,"otim",4) == 0 && (p[4] == ' ' || p[4] == '\0')){
    p += 4;
    while (*p == ' ') p++;
    const int centiseconds = atoi(p);
    const long ms = centiseconds * 10;

    if (engine_side == 0){
      winboard_btime = ms;
    } else{
      winboard_wtime = ms;
    }
  } else if (strncmp(p,"usermove",8) == 0){
    static int last_engine_move = 0;
    static int engine_has_moved = 0;

    p += 8;
    while (*p == ' ') p++;
    if (p[0] >= 'a' && p[0] <= 'h' && p[1] >= '1' && p[1] <= '8' &&
      p[2] >= 'a' && p[2] <= 'h' && p[3] >= '1' && p[3] <= '8'){
      const int from = (p[0] - 'a') + (7 - (p[1] - '1')) * 8;
      const int to = (p[2] - 'a') + (7 - (p[3] - '1')) * 8;
      abmove = (from << 8) | to;

      /* If this is the move we just played, ignore the echo */
      if (engine_has_moved && abmove == last_engine_move){
        engine_has_moved = 0;
        last_engine_move = 0;
        return; /* Ignore our own move being echoed back */
      }

      /* This is opponent's move */
      engine_has_moved = 0;

      winboard_force_move(abmove);
      if (book_enabled) book_follow_move(abmove);
      if (engine_side == -1) engine_side = mantom;
      if (winboard_mode != WB_MODE_FORCE && winboard_mode != WB_MODE_ANALYZE && engine_side == mantom){
        winboard_make_move();
      }
    } else{
      printf("Error (illegal move): %s\n",p);
      fflush(stdout);
    }
  } else if (strncmp(p,"ping",4) == 0){
    printf("pong %d\n",atoi(p + 4));
    fflush(stdout);
  } else if (strncmp(p,"nopost",6) == 0){
    winboard_post_mode = 0;
  } else if (strncmp(p,"post",4) == 0){
    winboard_post_mode = 1;
    last_post_time = 0;
  } else if (strncmp(p,"analyze",7) == 0){
    winboard_mode = WB_MODE_ANALYZE;
    winboard_analyze_mode = 1;
    winboard_post_mode = 1;
    last_post_time = 0;
  } else if (strncmp(p,"exit",4) == 0 || strncmp(p,"quit",4) == 0){
    exit(0);
  } else if (strncmp(p,"resign",6) == 0){
    winboard_send_result(mantom?"0-1 {White resigns}":"1-0 {Black resigns}");
  } else if (strncmp(p,"hint",4) == 0){
    engine_search();
    if (abmove){
      const int f = (abmove >> 8) & 0xFF, t = abmove & 0xFF;
      printf("Hint: %c%c%c%c\n",'a' + (f % 8),'1' + (7 - f / 8),'a' + (t % 8),'1' + (7 - t / 8));
      fflush(stdout);
    }
  } else if (strncmp(p,"book",4) == 0){
    p += 4;
    while (*p == ' ') p++;
    if (strncmp(p,"on",2) == 0) book_enabled = 1;
    else if (strncmp(p,"off",3) == 0) book_enabled = 0;
    else book_enabled = ! book_enabled;
  }
}

/* ------------------------------------------------------------------ */
/* winboard_init — reset all WinBoard state                          */
/* ------------------------------------------------------------------ */
void winboard_init(void){
  winboard_mode = WB_MODE_UNDEFINED;
  winboard_force_mode = 0;
  winboard_post_mode = 0;
  winboard_time_control = 0;
  winboard_wtime = 0;
  winboard_btime = 0;
  winboard_winc = 0;
  winboard_binc = 0;
  winboard_movestogo = 0;
  winboard_search_depth = 0;
  winboard_search_time = 0;
  winboard_analyze_mode = 0;
  winboard_stop_search = 0;
  winboard_move_sent = 0;
  winboard_nodes = 0;
  engine_side = -1;
  last_post_time = 0;
  time_per_move = 5;
  time_control_enabled = 0;

  if (bookf <= 0){
    bookf = open(BOOK, O_RDONLY | O_BINARY);
    if (bookf > 0){
      unsigned short initial_ptr;
      if (read(bookf,&initial_ptr,2) == 2){
        bookp_white = initial_ptr;
        bookp_black = initial_ptr;
        current_bookp = initial_ptr;
      }
    }
  }
}

/* ------------------------------------------------------------------ */
/* winboard_loop — main command loop                                 */
/* ------------------------------------------------------------------ */
void winboard_loop(void){
  char line[1024];
  winboard_init();
  while (fgets(line,sizeof(line), stdin) != NULL){
    int len = (int)strlen(line);
    if (len > 0 && line[len - 1] == '\n') line[--len] = '\0';
    if (len > 0 && line[len - 1] == '\r') line[--len] = '\0';
    if (len == 0) continue;
    winboard_process_command(line);
  }
}

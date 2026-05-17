#include <stdio.h>
#include <stdlib.h>
#include "chess.h"
/* Wrapper function for print_move1 */
void print_move_wrapper(const int m, const intptr_t a){
  (void)a;
  print_move1(m);
}

/* ========== MAIN PLAY FUNCTION ========== */
void play(const int f){
  get_clock_ms();
  if (f) goto first;

loop:
  intrp = 0;
  move(); /* human move (or command) */

first:
  if (manflg)
    goto loop;

  if (book_enabled){
    if (book_move()){
      printf("Computer plays: ");
      print_move(abmove);
      printf("\n");
      make_book_move(abmove);
      goto loop;
    }
  }

  if (! check_mate(mdepth,1)){
    if (time_control_enabled){
      move_start_time = get_current_time_ms();
      printf("Thinking for up to %d seconds...\n",time_per_move);

      if (mantom){
        timed_black_play(depth);
      } else{
        timed_white_play(depth);
      }
    } else{
      xplay();
    }

    if (abmove){
      printf("Computer plays: ");
      print_move(abmove);
      printf("\n");
      make_book_move(abmove);
      goto loop;
    }
    printf("No move found! Resigning.\n");
    handle_hup(0);
    return;
  }
  if (abmove){
    printf("Computer plays: ");
    print_move(abmove);
    printf("\n");
    make_book_move(abmove);
  }

  printf("Unexpected state in play()\n");
  goto loop;
}

/* ========== MOVE HANDLING ========== */
void move(void){
loop:
  /* Generate legal moves */
  int* saved_lmp = lmp;
  done(); /* populates move list */

  /* Check legality */
  {
    const int a = manual();
    const int* p1 = saved_lmp;
    int found = 0;
    const int* sentinel = lmp - 2;

    while (p1 < sentinel){
      const int mv = *(p1 + 1);
      if ((unsigned)mv == 0xfffffffc || mv == -4) break;
      if (mv == a){
        found = 1;
        break;
      }
      p1 += 2;
    }

    if (found){
      lmp = saved_lmp;
      make_book_move(a);

      if (book_enabled)
        book_follow_move(a);

      return;
    }
  }

  printf("Illegal move\n");
  lmp = saved_lmp;
  goto loop;
}

/* ========== DONE FUNCTION ========== */
void* done(void){
  if (repetition() > 3){
    printf("Draw by repetition\n");
    handle_hup(0);
  }

  int* p = lmp;
  if (mantom) gen_black_legal();
  else gen_white_legal();

  if (p == lmp){
    if (in_check()){
      if (mantom)
        printf("White wins\n");
      else
        printf("Black wins\n");
    } else{
      printf("Stale mate\n");
    }
    handle_hup(0);
  }
  return p;
}

/* ========== XPLAY FUNCTION ========== */
int xplay(void){
  int a;

  position_setup();
  abmove = 0;

  if (book_enabled){
    if (book_move()){
      static int dumped = 0;
      if (! dumped && bookf > 0){
        book_dump_position(2,"Root (White first moves)");
        book_dump_position(18,"After e4 (Black responses)");
        book_dump_position(106,"E4 E5 (White responses)");
        dumped = 1;
      }

      printf("Book move: ");
      print_move(abmove);
      printf("\n");
      ivalue = 0;
      return 0;
    }
  }

  if (mantom){
    a = black_play();
  } else{
    a = white_play();
  }

  ivalue = a;
  return a;
}

/* ========== MANUAL INPUT FUNCTIONS ========== */
int manual(void){
  int a;
  int* saved_amp;

loop:
  intrp = 0;
  position_setup();
  read_line();
  sbufp = sbuf;

  if (match_str("save")){
    save_game();
    goto loop;
  }
  if (match_str("test")){
    testf = ! testf;
    goto loop;
  }
  if (match_str("remove")){
    if (amp[-1] != -1){
      dec_move();
      if (mantom) black_undo();
      else white_undo();
    }
    if (amp[-1] != -1){
      dec_move();
      if (mantom) black_undo();
      else white_undo();
    }
    goto loop;
  }
  if (match_str("exit"))
    exit(0);
  if (match_str("manual")){
    manflg = ! manflg;
    goto loop;
  }
  if (match_str("resign"))
    handle_hup(0);

  if (match_str("book")){
    if (match_str("on")){
      book_enabled = 1;
      printf("Opening book ON\n");
    } else if (match_str("off")){
      book_enabled = 0;
      printf("Opening book OFF\n");
    } else{
      book_enabled = ! book_enabled;
      printf("Opening book %s\n",book_enabled?"ON":"OFF");
    }
    goto loop;
  }

  if (match_str("time")){
    while (*sbufp == ' ') sbufp++;

    if (*sbufp >= '0' && *sbufp <= '9'){
      int seconds = 0;
      while (*sbufp >= '0' && *sbufp <= '9'){
        seconds = seconds * 10 + (*sbufp - '0');
        sbufp++;
      }

      if (seconds > 0){
        time_per_move = seconds;
        time_control_enabled = 1;
        printf("Time control enabled: %d second%s per move\n",
          time_per_move,time_per_move == 1?"":"s");
      } else{
        printf("Invalid time value\n");
      }
    } else{
      if (time_control_enabled){
        time_control_enabled = 0;
        printf("Time control disabled\n");
      } else{
        time_control_enabled = 1;
        printf("Time control enabled: %d second%s per move\n",
          time_per_move,time_per_move == 1?"":"s");
      }
    }
    goto loop;
  }

  if (match_str("mfmt")){
    if (match_str("on")){
      mfmt = 1;
      printf("Algebraic notation ON\n");
    } else if (match_str("off")){
      mfmt = 0;
      printf("Algebraic notation OFF\n");
    } else{
      mfmt = ! mfmt;
      printf("Algebraic notation %s\n",mfmt?"ON":"OFF");
    }
    goto loop;
  }

  if (moveno == 1 && mantom == 0){
    if (match_str("first")){
      if (book_enabled){
        unsigned short root_ptr;
        lseek(bookf,0, SEEK_SET);
        if (read(bookf,&root_ptr,2) == 2){
          bookp = root_ptr;
        }
      }
      play(1);
    }
    if (match_str("alg")){
      mfmt = 1;
      goto loop;
    }
    if (match_str("restore")){
      restore_game();
      goto loop;
    }
  }
  if (match_str("clock")){
    clktim[mantom] += get_clock_ms();
    print_clock("white",clktim[0]);
    print_clock("black",clktim[1]);
    goto loop;
  }
  if (match_str("score")){
    print_score();
    goto loop;
  }
  if (match_str("setup")){
    setup_board();
    goto loop;
  }
  if (match_str("hint")){
    a = xplay();
    print_move(abmove);
    printf(" %d\n",a);
    goto loop;
  }
  if (match_str("repeat")){
    if (amp[-1] != -1){
      saved_amp = amp;
      if (mantom) white_undo();
      else black_undo();
      dec_move();
      position_traverse(print_move_wrapper,saved_amp,0);
    }
    goto loop;
  }
  if (*sbufp == '\0'){
    print_board();
    goto loop;
  }
  if ((a = alg_in()) != 0){
    mfmt = 1;
    return a;
  }
  if ((a = parse_stdin()) != 0){
    mfmt = 0;
    return a;
  }
  printf("eh?\n");
  goto loop;
}

int alg_in(void){
  const int from = coord_in();
  const int to = coord_in();
  if (*sbufp != '\0') return 0;
  return (from << 8) | to;
}

int coord_in(void){
  int a = sbufp[0];
  if (a < 'a' || a > 'h') return 0;
  const int b = sbufp[1];
  if (b < '1' || b > '8') return 0;
  sbufp += 2;
  a = (a - 'a') + 8 * ('8' - b);
  return a;
}

int match_str(const char* s){
  int c;

  char* p1 = sbufp;
  while ((c = *s++) != '\0')
    if (*p1++ != c) return 0;
  sbufp = p1;
  return 1;
}

void term(void){
  exit(0);
}

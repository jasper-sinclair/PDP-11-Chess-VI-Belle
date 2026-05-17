#include <stdio.h>
#include "chess.h"
//
/* Static counter to limit book depth */
static int book_ply_counter = 0;

void book_dump_position(const int offset, const char* name){
  unsigned char buf[4];
  fprintf(stderr,"\n=== %s at offset %d ===\n",name,offset);
  lseek(bookf,offset, SEEK_SET);
  int count = 0;
  while (read(bookf,buf,4) == 4 && count < 10){
    const int swapped = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
    const int move = (swapped >> 16) & 0xFFFF;
    const int next = swapped & 0xFFFF;
    const int from = (move >> 8) & 0xFF;
    const int to = move & 0xFF;
    fprintf(stderr,"  %d: %02x%02x%02x%02x -> %c%d%c%d next=%d\n",
      count + 1,buf[0],buf[1],buf[2],buf[3],
      'a' + (from % 8),8 - (from / 8),'a' + (to % 8),8 - (to / 8),next);
    count++;
  }
  fprintf(stderr,"Total: %d entries\n",count);
}

/* Reset book for a new game */
void book_reset(void){
  book_ply_counter = 0;
  book_enabled = 1;
  bookp_white = 2;
  bookp_black = 2;
  current_bookp = 2;
}

int book_move(void){
  unsigned char buf[4];
  int *p1, *p2;
  int found = 0;
  int saved_bookp;
  int* saved_lmp; /* Save original lmp as pointer */

  #ifdef DEBUG
  fprintf(stderr,"BOOK_MOVE: current_bookp=%d, bookf=%d, mantom=%d\n",
    current_bookp,bookf,mantom);
  #endif

  if (! book_enabled) return 0;
  if (bookf <= 0) return 0;

  /* Limit book depth - stop after 8 ply (4 moves each) */
  if (book_ply_counter >= 8){
    return 0;
  }

  /* Use color-specific pointer based on who is to move */

  //	if (wb) {
  //  saved_bookp = (mantom) ? bookp_white : bookp_black;
  //} 
  //else if (book_enabled) {
  saved_bookp = (mantom)?bookp_black:bookp_white;
  //} 
  //else { // !wb && !book_enabled
  //   saved_bookp = (mantom) ? bookp_black : bookp_white;
  //}

  if (saved_bookp < 2) return 0;

  current_bookp = saved_bookp;
  lseek(bookf,current_bookp, SEEK_SET);

  /* Save current lmp before generating moves */
  saved_lmp = lmp;
  p1 = lmp;

  // if (wb) {
  // if (mantom) gen_white_legal(); else gen_black_legal();
  //} 
  //else if (book_enabled) {
  if (mantom) gen_black_legal();
  else gen_white_legal();
  //} 
  //else { // !wb && !book_enabled
  //  if (mantom) gen_white_legal(); else gen_black_legal();
  //}

  //  int entry_count = 0;
  while (read(bookf,buf,4) == 4){
    // entry_count++;

    const int book_mv = (buf[0] << 8) | buf[1];
    const int next_ptr = (buf[2] << 8) | buf[3];

    if (book_mv == 0 || book_mv == 0xFFFF) break;

    p2 = p1;
    while (p2 < lmp){
      if (*(p2 + 1) == book_mv){
        found = book_mv;
        if (mantom){
          bookp_black = next_ptr;
        } else{
          bookp_white = next_ptr;
        }
        current_bookp = next_ptr;
        break;
      }
      p2 += 2;
    }
    if (found) break;
  }

  /* CRITICAL: Restore lmp to original value */
  lmp = saved_lmp;

  if (found){
    book_ply_counter++;
    abmove = found;
    return 1;
  }

  /* No book move found */
  return 0;
}

void make_book_move(const int m){
  #ifdef DEBUG
  fprintf(stderr,"MAKE_BOOK_MOVE: making move 0x%04x, mantom=%d\n",m,mantom);
  #endif
  print_move1(m);

  const int from = (m >> 8) & 0xFF;
  const int to = m & 0xFF;
  if (from < 0 || from > 63 || to < 0 || to > 63){
    printf("FATAL: Invalid move coordinates from=%d to=%d\n",from,to);
    exit(1);
  }
  if (board[from] == 0){
    printf("FATAL: No piece at from square %d\n",from);
    print_board();
    exit(1);
  }

  /* Correct move execution based on whose turn it is *before* the move */
  if (mantom == 0) /* White to move */
    white_move(m);
  else
    black_move(m);

  /* Update book pointer for the side that just moved */
  if (mantom == 0)
    bookp_white = current_bookp;
  else
    bookp_black = current_bookp;

  inc_move();
}

void book_follow_move(const int move){
  unsigned char buf[4];
  int* saved_lmp = lmp;

  #ifdef DEBUG
  fprintf(stderr,"BOOK_FOLLOW: move=0x%04x, mantom=%d\n",move,mantom);
  #endif

  if (! book_enabled || bookf <= 0){
    current_bookp = 0;
    return;
  }

  /* After inc_move() in force_move, mantom now = engine's turn */
  const int search_ptr = (mantom == 0)?bookp_white:bookp_black;

  current_bookp = search_ptr;
  lseek(bookf,current_bookp, SEEK_SET);

  saved_lmp = lmp;

  while (read(bookf,buf,4) == 4){
    const int book_mv = (buf[0] << 8) | buf[1];
    const int next_ptr = (buf[2] << 8) | buf[3];

    if (book_mv == 0 || book_mv == 0xFFFF) break;

    if (book_mv == (move & 0xFFFF)){
      current_bookp = next_ptr;
      /* Update the pointer for the side that just moved (opponent) */
      if (mantom == 0) /* now White's turn → Black just moved */
        bookp_black = current_bookp;
      else
        bookp_white = current_bookp;
      lmp = saved_lmp;
      return;
    }
  }

  lmp = saved_lmp;
  if (mantom == 0)
    bookp_black = 0;
  else
    bookp_white = 0;
  current_bookp = 0;
}

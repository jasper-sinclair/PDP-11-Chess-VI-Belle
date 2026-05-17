/*
 * main.c  — engine entry point
 */
#include <fcntl.h>   /* For open flags */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "chess.h"
#include "uci.h"
#include "winboard.h"

/* Handle O_BINARY for cross-platform compatibility */
#ifndef O_BINARY
#define O_BINARY 0
#endif

static void catch_sigint(const int s){
  signal(s,catch_sigint);
}

int main(const int argc, char* argv[]) {
  int i;

  /* Disable buffering immediately */
  setbuf(stdout, NULL);
  setbuf(stdin, NULL);
  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stdin, NULL, _IONBF, 0);

  signal(SIGINT, catch_sigint);

  /* --- Parse command-line arguments ----------------------------- */
  for (i=1; i < argc; i++) {

    if (strcmp(argv[i], "-wb") == 0 ||
      strcmp(argv[i], "-xboard") == 0 ||
      strcmp(argv[i], "-winboard") == 0) {

      wb=1;
    }

    else if (strcmp(argv[i], "-uci") == 0) {

      uci=1;
    }

    else if (strcmp(argv[i], "-depth") == 0 &&
      i + 1 < argc) {

      depth=atoi(argv[++i]);
    }

    else if (strcmp(argv[i], "-time") == 0 &&
      i + 1 < argc) {

      time_per_move=atoi(argv[++i]);
      time_control_enabled=1;
    }

    else if (strcmp(argv[i], "-nobook") == 0) {

      book_enabled=0;
    }

    else if (strcmp(argv[i], "-post") == 0) {

      winboard_post_mode=1;
    }
  }

  /* only print startup banner in console mode */
  if (!wb && !uci) {

    printf("PDP-11 Chess Engine\n");

    if (depth > 0)
      printf("Search depth: %d\n", depth);

    if (book_enabled)
      printf("Opening book: ON\n");
  }

  /* --- Initialize engine --------------------------------------- */

  init_signals();

  ambuf[0]=-1;
  lmbuf[0]=-1;

  lmp=lmbuf + 1;
  amp=ambuf + 1;

  /* Initialize book pointers */
  bookp_white=0;
  bookp_black=0;
  current_bookp=0;

  /* Open opening book */
  bookf=open(BOOK, O_RDONLY | O_BINARY);

  if (bookf >= 0) {

    unsigned short initial_ptr;

    const off_t book_size=
      lseek(bookf, 0, SEEK_END);

    lseek(bookf, 0, SEEK_SET);

    /* Never print these in protocol modes */
    if (!wb && !uci) {
      fprintf(stderr,
        "Book file: %s (%ld bytes)\n",
        BOOK,
        (long)book_size);
    }

    if (read(bookf, &initial_ptr, 2) == 2) {

      bookp_white=initial_ptr;
      bookp_black=initial_ptr;
      current_bookp=initial_ptr;

      if (!wb && !uci) {
        printf("Opening book loaded successfully\n");
      }

    }
    else {

      if (!wb && !uci) {
        printf(
          "Warning: Opening book '%s' "
          "has invalid header\n",
          BOOK);
      }
    }

  }
  else {

    if (!wb && !uci) {
      printf(
        "Warning: Opening book '%s' "
        "not found\n",
        BOOK);
    }
  }

  /* Build edge/direction table */
  for (i=0; i < 64; i++) {
    dir[i]=
      (edge[i / 8] << 6) |
      edge[i % 8];
  }

  /* --- Enter protocol mode ------------------------------------- */

  if (uci) {
    uci_mode=1;
    uci_loop();
  }
  else if (wb) {
    winboard_post_mode=1;
    winboard_loop();
  }
  else {
    play(0);
  }
  return 0;
}
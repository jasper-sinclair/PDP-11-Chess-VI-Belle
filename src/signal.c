#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "chess.h"
#ifdef _WIN32
/* Windows doesn't have SIGHUP and SIGQUIT */
#ifndef SIGHUP
#define SIGHUP 1
#endif
#ifndef SIGQUIT
#define SIGQUIT 3
#endif
#endif
static long t_start = 0;

void crash_handler(const int sig){
  printf("\n*** CRASH DETECTED: Signal %d ***\n",sig);
  printf("mantom=%d, ply=%d, depth=%d, value=%d\n",mantom,ply,depth,value);
  printf("lmp=%p, amp=%p\n",(void*)lmp,(void*)amp);
  printf("lmp - lmbuf = %ld, amp - ambuf = %ld\n",
    (long)(lmp - lmbuf),(long)(amp - ambuf));
  fflush(stdout);
  exit(1);
}

void handle_hup(const int sig){
  (void)sig; /* Suppress unused parameter warning */
  signal(SIGHUP,handle_hup);
  signal(SIGINT,handle_hup);
  signal(SIGQUIT,handle_hup);
  term();
}

void handle_int(const int sig){
  (void)sig; /* Suppress unused parameter warning */
  signal(SIGINT,handle_int);
  intrp++;
}

void init_signals(void){
  signal(SIGHUP,handle_hup);
  signal(SIGINT,handle_int);
  signal(SIGQUIT,handle_hup);
  signal(SIGSEGV,crash_handler); // Catch segmentation faults
  signal(SIGABRT,crash_handler); // Catch aborts
}

long get_clock_ms(void){
  const clock_t now = clock();

  if (t_start == 0){
    t_start = now;
    return 0;
  }

  const long diff = now - t_start;
  t_start = now;

  /* Convert to milliseconds (CLOCKS_PER_SEC is 1000 on many systems) */
  return diff * 1000 / CLOCKS_PER_SEC;
}

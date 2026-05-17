// timecontrol.c - With minimum thinking time
#include <stdio.h>
#include "chess.h"
#include "winboard.h"
#define MAX_SEARCH_DEPTH 32
/* Track if we've reached minimum thinking time */
static int reached_min_time = 0;
/* Reset for each new search */
void reset_time_tracking(void){
  reached_min_time = 0;
}

/* Check if we should continue searching (not stop) */
int should_continue_search(void){
  if (! time_control_enabled) return 1;

  const long elapsed_ms = get_current_time_ms() - move_start_time;

  /* Calculate minimum thinking time (50% of allocated time, but at least 1 second) */
  int min_thinking_ms = (time_per_move * 1000) / 2;
  if (min_thinking_ms < 1000) min_thinking_ms = 1000; /* At least 1 second */

  /* If we haven't reached minimum thinking time yet, MUST continue */
  if (elapsed_ms < min_thinking_ms){
    return 1; /* Continue searching - cannot stop yet */
  }

  /* Mark that we've reached minimum time */
  reached_min_time = 1;

  /* Now we can check if we should stop due to time running out */
  const long max_thinking_ms = (time_per_move * 1000) * 90 / 100; /* 90% of time */
  if (elapsed_ms >= max_thinking_ms){
    return 0; /* Stop searching - time's almost up */
  }

  return 1; /* Continue searching - we have more time */
}

/* For backward compatibility - returns 1 if we should stop */
int should_stop_search(void){
  if (winboard_mode != WB_MODE_UNDEFINED){
    return winboard_should_stop();
  }

  if (! time_control_enabled){
    return 0;
  }

  /* Return 1 (stop) if we should NOT continue */
  return ! should_continue_search();
}

/* Check if we've met minimum time (for deciding whether to accept a move) */
int has_thought_enough(void){
  if (! time_control_enabled) return 1; /* No time control = always enough */

  const long elapsed_ms = get_current_time_ms() - move_start_time;
  int min_thinking_ms = (time_per_move * 1000) / 2;
  if (min_thinking_ms < 1000) min_thinking_ms = 1000;

  return (elapsed_ms >= min_thinking_ms);
}

int timed_white_play(const int base_depth){
  int best_move = 0;
  int best_value = 3000;
  const int saved_depth = depth;
  int searched_depth = 0;

  if (winboard_mode != WB_MODE_UNDEFINED){
    return engine_search();
  }

  (void)base_depth;

  /* Reset timing */
  const long start_time = get_current_time_ms();
  move_start_time = start_time;
  const long time_limit_ms = time_per_move * 1000;
  reset_time_tracking();

  printf("\n  Thinking for %d seconds...",time_per_move);
  fflush(stdout);

  const int saved_intrp = intrp;
  intrp = 0;

  clear_search_tables();
  ivalue = value;

  for (int current_depth = 1; current_depth <= MAX_SEARCH_DEPTH; current_depth++){
    long elapsed_ms = get_current_time_ms() - start_time;

    /* Don't start a new depth if we're low on time */
    if (elapsed_ms > (time_limit_ms * 70 / 100)){
      printf(" [time low, stopping at depth %d]",current_depth - 1);
      fflush(stdout);
      break;
    }

    depth = current_depth;
    position_setup();

    {
      const int iter_val = white_play();
      searched_depth = current_depth;
      if (abmove != 0){
        best_move = abmove;
        best_value = iter_val;
        ivalue = iter_val;
        printf(" d%d/%+d",current_depth,-best_value);
        fflush(stdout);
      }
    }

    elapsed_ms = get_current_time_ms() - start_time;

    /* Stop conditions - but ONLY if we've thought enough */
    if (has_thought_enough()){
      if (elapsed_ms > (time_limit_ms * 85 / 100)) break;
      if (intrp) break;
      if (best_value > 2900 || best_value < -2900) break; /* Mate */
    }

    /* Force at least depth 2 if we have time */
    if (current_depth == 1 && elapsed_ms < (time_limit_ms * 20 / 100)){}
  }

  /* If we haven't thought enough yet, wait */
  while (! has_thought_enough()){
    printf(".");
    fflush(stdout);
    sleep_ms(100000); /* Sleep 100ms */
  }

  printf("\n");

  if (best_move != 0){
    abmove = best_move;
  } else{
    /* Emergency move */
    if (mantom) gen_black_legal();
    else gen_white_legal();
    if (lmp > (mantom?lmbuf + 1:lmbuf + 1) + 2){
      abmove = *(lmp - 1);
    }
  }

  depth = saved_depth;
  {
    const long e = get_current_time_ms() - start_time;
    printf("  d%d in %ld.%lds (target: %ds)\n",searched_depth,e / 1000,(e % 1000) / 100,time_per_move);
  }
  intrp = saved_intrp;
  return best_value;
}

int timed_black_play(const int base_depth){
  int best_move = 0;
  int best_value = -3000;
  const int saved_depth = depth;
  int searched_depth = 0;

  if (winboard_mode != WB_MODE_UNDEFINED){
    return engine_search();
  }

  (void)base_depth;

  /* Reset timing */
  const long start_time = get_current_time_ms();
  move_start_time = start_time;
  const long time_limit_ms = time_per_move * 1000;
  reset_time_tracking();

  printf("\n  Thinking for %d seconds...",time_per_move);
  fflush(stdout);

  const int saved_intrp = intrp;
  intrp = 0;

  clear_search_tables();
  ivalue = value;

  for (int current_depth = 1; current_depth <= MAX_SEARCH_DEPTH; current_depth++){
    long elapsed_ms = get_current_time_ms() - start_time;

    if (elapsed_ms > (time_limit_ms * 70 / 100)){
      printf(" [time low, stopping at depth %d]",current_depth - 1);
      fflush(stdout);
      break;
    }

    depth = current_depth;
    position_setup();

    {
      const int iter_val = black_play();
      searched_depth = current_depth;
      if (abmove != 0){
        best_move = abmove;
        best_value = iter_val;
        ivalue = iter_val;
        printf(" d%d/%+d",current_depth,best_value);
        fflush(stdout);
      }
    }

    elapsed_ms = get_current_time_ms() - start_time;

    if (has_thought_enough()){
      if (elapsed_ms > (time_limit_ms * 85 / 100)) break;
      if (intrp) break;
      if (best_value > 2900 || best_value < -2900) break;
    }

    if (current_depth == 1 && elapsed_ms < (time_limit_ms * 20 / 100)){}
  }

  /* Wait for minimum thinking time if needed */
  while (! has_thought_enough()){
    printf(".");
    fflush(stdout);
    sleep_ms(100000);
  }

  printf("\n");

  if (best_move != 0){
    abmove = best_move;
  } else{
    if (mantom) gen_black_legal();
    else gen_white_legal();
    if (lmp > (mantom?lmbuf + 1:lmbuf + 1) + 2){
      abmove = *(lmp - 1);
    }
  }

  depth = saved_depth;
  {
    const long e = get_current_time_ms() - start_time;
    printf("  d%d in %ld.%lds (target: %ds)\n",searched_depth,e / 1000,(e % 1000) / 100,time_per_move);
  }
  intrp = saved_intrp;
  return best_value;
}

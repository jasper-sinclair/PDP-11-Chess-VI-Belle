#ifndef CHESS_H
#define CHESS_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>

#ifdef _WIN32
#include <windows.h>
inline void sleep_ms(const int ms){
  Sleep(ms);
}
#else
#include <unistd.h>
inline void sleep_ms(int ms){
  usleep(ms * 1000);
}
#endif

#ifdef _MSC_VER
#pragma warning(disable: 4244) // 'initializing': conversion from 'int' to 'char', possible loss of data
#else
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wsign-compare" 
#endif

#ifdef _WIN32
#define open   _open
#define read   _read
#define close  _close
#define lseek  _lseek
#endif

/* ------------------------------------------------------------------ */
/* get_current_time_ms — wall-clock time in milliseconds                   */
/* ------------------------------------------------------------------ */
#ifdef _WIN32
inline long get_current_time_ms(void){
  return (long)GetTickCount64();
}
#else
#include <sys/time.h>
inline long get_current_time_ms(void){
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (tv.tv_sec * 1000L) + (tv.tv_usec / 1000L);
}
#endif

#define BOOK "book.dat"
#define NONE 12345

/* Killer move heuristic: 2 killers per ply */
enum{
  MAX_KILLERS = 2, MAX_KILLER_PLY = 32,
  /* Principal Variation table */
  MAX_PV_LENGTH = 40
};

/* Search-path hash array size */
#define MAX_SEARCH_PLY (MAX_PV_LENGTH + 4)

/* WinBoard mode flag */
extern int winboard_mode;
extern int engine_side; /* 0 = engine plays White, 1 = engine plays Black */
/* Global variables */
extern int attacv[64];
extern int center[64];
extern int control[64];
extern int clktim[2];
extern int testf;
extern int qdepth;
extern int mdepth;
extern int bookf;
extern int bookp; /* Legacy - kept for compatibility */
extern int bookp_white; /* Book pointer when engine plays White */
extern int bookp_black; /* Book pointer when engine plays Black */
extern int current_bookp; /* Active book pointer */
extern int book_enabled;
extern int manflg;
extern int matflg;
extern int intrp;
extern int moveno;
extern int gval;
extern int game;
extern int abmove;
extern int* lmp;
extern int* amp;
extern char* sbufp;
extern int lastmov;
extern int mantom;
extern int ply;
extern int value;
extern int ivalue;
extern int mfmt;
extern int depth;
extern int flag;
extern int eppos;
extern int bkpos;
extern int wkpos;
extern int column;
extern int edge[8];
extern int pval[13];
extern int ipval[13];
extern int dir[64];
extern int board[64];
extern int lmbuf[4000];
extern int ambuf[8000];
extern char sbuf[100];
extern int time_per_move;
extern int time_control_enabled;
extern long move_start_time;

/* Direction masks from original PDP-11 chess */
enum {
  uleft=04040, uright=04004, dleft=00440, dright=00404,
  left=00040, right=00004, up=04000, down=00400,
  u2r1=06004, u1r2=04006, d1r2=00406, d2r1=00604,
  d2l1=00640, d1l2=00460, u1l2=04060, u2l1=06040,
  rank2=00200, rank7=02000
};

/* Move types - must match original */
enum{
  MOVE_NORMAL = 0, MOVE_KCASTLE = 1, MOVE_QCASTLE = 2, MOVE_EP = 3, MOVE_PROMO = 4
};

/* Castling flags - must match original */
enum{
  WKCASTLE = 01, WQCASTLE = 02, BKCASTLE = 010, BQCASTLE = 020
};

static int wb;
static int uci;

/* Function prototypes */
void play(int f);
void move(void);
int manual(void);
int alg_in(void);
int coord_in(void);
int match_str(const char* s);
void* done(void);
int xplay(void);
void term(void);
void read_line(void);
void print_board(void);
void print_move1(int m);
void print_move(int m);
void print_piece(int p);
void print_square(int b);
void print_algebraic(int p);
void putchar_c(int c);
void print_time(int a, int b);
void print_score1(int m);
void print_score(void);
/* Extension functions */
int move_gives_check(int move);
int capture_worth_extending(int move, int depth_remaining);
/* Move execution */
void white_move(int m);
void white_undo(void);
void black_move(int m);
void black_undo(void);
/* Move generation */
void gen_white_moves(void);
void gen_black_moves(void);
void gen_white_legal(void);
void gen_black_legal(void);
/* Search functions */
int white_play(void);
int white_play1(int alpha, int beta);
int white_quiesce(int alpha, int beta);
int black_play(void);
int black_play1(int alpha, int beta);
int black_quiesce(int alpha, int beta);
/* Heuristic functions */
int white_heur1(void);
int white_heur2(void);
int white_heur3(void);
int white_heur4(void);
int white_heur5(void);
int white_heur6(void);
int black_heur1(void);
int black_heur2(void);
int black_heur3(void);
int black_heur4(void);
int black_heur5(void);
int black_heur6(void);
/* Evaluation */
int white_static(int f);
int black_static(int f);
int* static_eval(void);
int check_heuristic(int ploc);
int positional_adjustment(void);
void surround_king(int p);
/* Attack detection */
int black_attacks(int pos);
int white_attacks(int pos);
void attackers(int pos);
/* Mate detection */
int check_mate(int n, int f);
int mate_search(int ns);
int mate_recursive(int ns);
/* Static Exchange Evaluator */
int see(int from, int to);
int see_ge(int from, int to, int threshold);
/* Endgame pattern evaluation */
int endgame_eval(void);
/* Book functions */
int book_move(void);
void make_book_move(int m);
int book_swap(int m);
void book_follow_move(int move);
/* Game state */
int repetition(void);
void repetition_check(int m, intptr_t a);
void position_setup(void);
void inc_move(void);
void dec_move(void);
int in_check(void);
void print_clock(char* s, int t);
/* Board setup and save/restore */
void setup_board(void);
void save_game(void);
void restore_game(void);
/* Input parsing */
int parse_stdin(void);
void parse_std_piece(int* ap, int* as, int* ar, int* af);
void parse_std_square(int* as, int* ar, int* af);
int piece_compare(int p, int l, int pp, int sp, int rp, int fp);
int val_compare(int p, int v);
int abs_val(int x);
/* Signal handling */
void handle_hup(int sig);
void handle_int(int sig);
void init_signals(void);
long get_clock_ms(void);
/* Time control */
int timed_white_play(int base_depth);
int timed_black_play(int base_depth);
int should_stop_search(void);
/* Helper functions */
void add_move(int from, int to, int score);
int compare_int(const void* a, const void* b);
int compare_desc(const void* a, const void* b);
int score_move_internal(int move, int cur_ply);
void slide_moves(int pos, int is_white, const int* offsets, int ndirs);
/* Position traversal */
void position_traverse(void (*f)(int, intptr_t), const int* p, intptr_t a);
/* Add WinBoard function declarations */
int winboard_should_stop(void);
int engine_search(void);
void wb_count_node(void);
/* Perft*/
uint64_t perft(int depth);
uint64_t divide(int depth);
void move_to_uci(int move, char* buf);

/* Direction tables */
extern const int knight_offsets[8];
extern const int king_offsets[8];
extern const int bishop_offsets[4];
extern const int rook_offsets[4];
extern const int queen_offsets[8];
extern const int knight_masks[8];
extern const int dir_masks[8];
extern const int dir_offsets[8];

/* Heuristic arrays */
extern int (*white_heur[])(void);
extern int (*black_heur[])(void);
extern long winboard_nodes;
/* Null-move flag: non-zero while inside a null-move subtree */
extern int in_null_move;

extern int pv_table[MAX_PV_LENGTH][MAX_PV_LENGTH];
extern int pv_length[MAX_PV_LENGTH];
extern unsigned int search_pos_hash[MAX_SEARCH_PLY];
extern int killer_moves[MAX_KILLER_PLY][MAX_KILLERS];
/* History heuristic: history[from][to] for quiet-move ordering */
extern int history[64][64];

/* ── REPETITION DETECTION IN SEARCH ─────────────────────────────────
 * pos_fingerprint(): fast 32-bit hash of (board, side-to-move, ep-square)
 */
inline unsigned int pos_fingerprint(void){
  unsigned int h = mantom?0x9e3779b9u:0x517cc1b7u;
  for (int _i = 0; _i < 64; _i++){
    const int _p = board[_i];
    if (_p){
      h ^= (unsigned int)(_p + 7) * 2654435761u ^ (unsigned int)_i * 2246822519u;
      h = (h << 13) | (h >> 19);
    }
  }
  h ^= (unsigned int)(eppos & 0xFF) * 1000003u;
  return h | 1u;
}

/* Functions for killer / history */
void store_killer(int ply_n, int move);
void clear_search_tables(void);
void print_move_wrapper(int m, intptr_t a);
void book_dump_position(int offset, const char* name);
/* Time control functions */
void reset_time_tracking(void);
int should_continue_search(void);
int has_thought_enough(void);
extern int winboard_search_time;
#endif /* CHESS_H */

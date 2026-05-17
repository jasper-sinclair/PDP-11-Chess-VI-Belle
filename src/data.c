#include "chess.h"
/* Center control table from original PDP-11 chess */
int center[] = {
  2,3,4,4,4,4,3,2,
  3,6,8,8,8,8,6,3,
  4,8,12,12,12,12,8,4,
  4,8,12,14,14,12,8,4,
  4,8,12,14,14,12,8,4,
  4,8,12,12,12,12,8,4,
  3,6,8,8,8,8,6,3,
  2,3,4,4,4,4,3,2
};
/* Piece values - matches original PDP-11 order: P,N,B,R,Q,K */
int ipval[] = {
  -3000,-900,-550,-320,-300,-90, /* White pieces: K,Q,R,B,N,P */
  0, /* Empty */
  90,300,320,550,900,3000 /* Black pieces: P,N,B,R,Q,K */
};
int moveno = 1;
int depth = 8;
int qdepth = 8;
int mdepth = 8;
int flag = 033;
int eppos = 64;
int bkpos = 4;
int wkpos = 60;
int edge[] = {040,020,010,0,0,1,2,4};
/* Starting position - standard chess setup */
int board[] = {
  4,2,3,5,6,3,2,4, /* Black pieces (positive) */
  1,1,1,1,1,1,1,1,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  -1,-1,-1,-1,-1,-1,-1,-1, /* White pawns (negative) */
  -4,-2,-3,-5,-6,-3,-2,-4, /* White pieces (negative) */
};
/* Global variables */
int attacv[64];
int control[64];
int clktim[2] = {0,0};
int testf = 0;
int bookf = 0;
int bookp = 0; /* Legacy - kept for compatibility */
int bookp_white = 0; /* Book pointer when engine plays White */
int bookp_black = 0; /* Book pointer when engine plays Black */
int current_bookp = 0; /* Active book pointer */
int book_enabled = 1;
int manflg = 0;
int matflg = 0;
int intrp = 0;
int gval = 0;
int game = 0;
int abmove = 0;
int* lmp = lmbuf + 1; /* Initialize at compile time */
int* amp = ambuf + 1; /* Initialize at compile time */
char* sbufp = sbuf; /* Initialize at compile time */
int lastmov = 0;
int mantom = 0;
int ply = 0;
int value = 0;
int ivalue = 0;
int mfmt = 0;
int column = 0;
int pval[13];
int dir[64];
int lmbuf[4000];
int ambuf[8000];
char sbuf[100];
/* Time control variables */
int time_per_move = 5;
int time_control_enabled = 0;
long move_start_time = 0;
/* Null-move guard */
int in_null_move = 0;
/* Search-path position hashes for repetition detection */
unsigned int search_pos_hash[MAX_SEARCH_PLY];
/* Principal Variation table */
int pv_table[MAX_PV_LENGTH][MAX_PV_LENGTH];
int pv_length[MAX_PV_LENGTH];
/* Killer move and history heuristic tables */
int killer_moves[MAX_KILLER_PLY][MAX_KILLERS];
int history[64][64];
/* Heuristic arrays */
int (*white_heur[])(void) = {
  white_heur1,white_heur2,white_heur3,
  white_heur4,white_heur5,white_heur6, NULL
};
int (*black_heur[])(void) = {
  black_heur1,black_heur2,black_heur3,
  black_heur4,black_heur5,black_heur6, NULL
};

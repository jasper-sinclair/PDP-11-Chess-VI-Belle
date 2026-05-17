#ifndef WINBOARD_H
#define WINBOARD_H
/* WinBoard protocol mode constants */
#define WB_MODE_UNDEFINED   0
#define WB_MODE_FORCE       1
#define WB_MODE_GO          2
#define WB_MODE_ANALYZE     3
#define WB_MODE_PLAY_WHITE  4
#define WB_MODE_PLAY_BLACK  5
/* WinBoard global variables (defined in winboard.c) */
extern int winboard_force_mode;
extern int winboard_post_mode;
extern int winboard_time_control;
extern int winboard_wtime;
extern int winboard_btime;
extern int winboard_winc;
extern int winboard_binc;
extern int winboard_movestogo;
extern int winboard_search_depth;
extern int best_value_so_far;
/* WinBoard interface functions */
void winboard_init(void);
void winboard_loop(void);
void winboard_process_command(char* cmd);
void winboard_make_move(void);
void winboard_send_move(int move);
void winboard_send_result(char* result);
void winboard_force_move(int move);
void winboard_set_time_control(int wtime, int btime, int winc, int binc, int movestogo);
void winboard_send_thinking(int move, int score, int depth_val, long time_ms);
void winboard_send_periodic_update(int depth_val, int score, int best_move);
void winboard_send_stat(int depth_val, int score, long time_ms, long nodes, int move);
#endif /* WINBOARD_H */

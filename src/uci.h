#pragma once
void reset_board(void);
void uci_loop(void);
void parse_position(const char* line);
void uci_send_bestmove(int move);
void uci_send_info(int depth, int score, long time_ms, long nodes);
int load_fen(const char* fen);
extern int uci_mode;
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include "unistd.h"
#include "../chess.h"

#ifndef O_BINARY
#define O_BINARY 0
#endif
#define MAX_POSITIONS 30000
#define MAX_MOVES_PER_POS 32
#define MAX_BOOK_DEPTH 8  /* Limit to 8 ply (4 moves per side) */

typedef struct{
  unsigned short move_val;
  int target_pos_idx;
} MoveEdge;

typedef struct{
  MoveEdge edges[MAX_MOVES_PER_POS];
  int num_edges;
  int file_offset;
  int depth; /* Add depth tracking */
} PositionNode;

PositionNode tree[MAX_POSITIONS];
int position_count = 1;

static int sq(const int file, const int rank){
  return (8 - rank) * 8 + file;
}

static int parse_move(const char* s){
  if (! s || strlen(s) < 4) return 0;

  const int f1 = s[0] - 'a';
  const int r1 = s[1] - '0';
  const int f2 = s[2] - 'a';
  const int r2 = s[3] - '0';

  return (sq(f1,r1) << 8) | sq(f2,r2);
}

void add_line(const char* line){
  char buf[2048];
  strncpy(buf,line,2047);
  buf[2047] = '\0';

  const char* m_str = strtok(buf," \t\r\n");
  int current_idx = 0;
  int current_depth = 0;

  while (m_str != NULL && current_depth < MAX_BOOK_DEPTH){
    if (strlen(m_str) >= 4){
      const unsigned short mv = (unsigned short)parse_move(m_str);
      int next_idx = -1;

      for (int i = 0; i < tree[current_idx].num_edges; i++){
        if (tree[current_idx].edges[i].move_val == mv){
          next_idx = tree[current_idx].edges[i].target_pos_idx;
          break;
        }
      }

      if (next_idx == -1){
        if (position_count >= MAX_POSITIONS) return;
        next_idx = position_count++;
        tree[next_idx].depth = current_depth + 1; /* Set depth */
        const int edge_idx = tree[current_idx].num_edges;
        if (edge_idx < MAX_MOVES_PER_POS){
          tree[current_idx].edges[edge_idx].move_val = mv;
          tree[current_idx].edges[edge_idx].target_pos_idx = next_idx;
          tree[current_idx].num_edges++;
        }
      }
      current_idx = next_idx;
      current_depth++;
    }
    m_str = strtok(NULL," \t\r\n");
  }
}

int create_book(void){
  memset(tree,0,sizeof(tree));
  tree[0].depth = 0;

  // --- REPAIR: ALEKHINE (limited to 4 moves each) ---
  add_line("e2e4 g8f6 c2c3 d7d5 e4d5 d8d5 d2d4 c8f5");

  // --- WHITE REPERTOIRE (limited) ---
  add_line("e2e4 e5 g1f3 b8c6 f1c4 f8c5 c2c3 g8f6");
  add_line("d2d4 d5 c1f4 g8f6 e3 e6 g1f3 f8d6");
  add_line("e2e4 c7c5 c3 d5 e4d5 d8d5 d2d4 g8f6");

  // --- BLACK REPERTOIRE (limited) ---
  add_line("e2e4 c6 d2d4 d5 b1c3 d5e4 c3e4 c8f5");
  add_line("d2d4 d5 c4 e6 b1c3 g8f6 g1f3 f8e7");

  int current_ptr = 2;
  for (int i = 0; i < position_count; i++){
    if (tree[i].num_edges > 0){
      tree[i].file_offset = current_ptr;
      current_ptr += (tree[i].num_edges * 4);
    } else{
      tree[i].file_offset = 0;
    }
  }

  const int fd = open("book.dat", O_WRONLY | O_CREAT | O_TRUNC | O_BINARY,0644);
  if (fd < 0){
    perror("open");
    return 1;
  }

  unsigned char header[2];
  header[0] = (unsigned char)(tree[0].file_offset & 0xFF);
  header[1] = (unsigned char)((tree[0].file_offset >> 8) & 0xFF);
  _write(fd,header,2);

  for (int i = 0; i < position_count; i++){
    if (tree[i].num_edges == 0) continue;
    for (int j = 0; j < tree[i].num_edges; j++){
      const unsigned short mv = tree[i].edges[j].move_val;
      const int target_idx = tree[i].edges[j].target_pos_idx;
      const int next_off = tree[target_idx].file_offset;

      unsigned char buf[4];
      buf[0] = (unsigned char)((mv >> 8) & 0xFF);
      buf[1] = (unsigned char)(mv & 0xFF);
      buf[2] = (unsigned char)((next_off >> 8) & 0xFF);
      buf[3] = (unsigned char)(next_off & 0xFF);
      _write(fd,buf,4);
    }
  }

  close(fd);
  printf("Book Compiled Cleanly! Total Positions: %d\n",position_count);
  return 0;
}

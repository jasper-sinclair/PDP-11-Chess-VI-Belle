#include <stdio.h>
#include "chess.h"

int parse_stdin(void){
  int piece1, piece2, side1, side2, rnk1, rnk2, file1, file2;
  int c, m, *p1, *p2, amb, piece;
  int* sentinel_pos;

  piece1 = piece2 = side1 = side2 = -1;
  rnk1 = rnk2 = file1 = file2 = -1;
  int ckf = 0;

  if (match_str("o-o-o") || match_str("ooo")){
    piece1 = 6;
    file1 = 3;
    side1 = 1;
    file2 = 2;
    side2 = 0;
    goto search;
  }

  if (match_str("o-o") || match_str("oo")){
    piece1 = 6;
    file1 = 3;
    file2 = 1;
    goto search;
  }

  parse_std_piece(&piece1,&side1,&rnk1,&file1);
  c = *sbufp++;

  if (c == '*' || c == 'x')
    parse_std_piece(&piece2,&side2,&rnk2,&file2);
  else if (c == '-')
    parse_std_square(&side2,&rnk2,&file2);
  else
    sbufp--;

search:
  c = *sbufp++;
  if (c == '+'){
    ckf = 1;
    c = *sbufp++;
  }
  if (c != '\0')
    return 0;

  p1 = p2 = lmp;
  if (mantom) gen_black_legal();
  else gen_white_legal();

  /* The sentinel is at lmp-2 (score) and lmp-1 (move) */
  sentinel_pos = lmp - 2;

  m = -1;
  amb = 0;

  /* Process moves until we reach the sentinel */
  while (p2 < sentinel_pos){
    /* Check if we have enough space */
    if (p2 + 1 >= sentinel_pos)
      break;

    /* Get the move value */
    const int move = *(p2 + 1);

    /* Check for sentinel */
    if (move == 0xfffffffc || move == -4)
      break;

    const int from = (move >> 8) & 0xFF;

    piece = board[from];

    if (mantom) black_move(move);
    else white_move(move);

    const int to_square = amp[-3]; // Renamed to avoid conflict

    if (piece_compare(piece,amp[-4],piece1,side1,rnk1,file1) &&
      piece_compare(amp[-2],to_square,piece2,side2,rnk2,file2) &&
      val_compare(ckf,in_check())){
      if (m >= 0){
        if (! amb){
          printf("ambiguous\n");
          amb = 1;
        }
      }
      m = move;
    }

    if (mantom) black_undo();
    else white_undo();

    /* Move to next move entry */
    p2 += 2;
  }

  lmp = p1;
  if (amb) return -1;
  return m;
}

void parse_std_piece(int* ap, int* as, int* ar, int* af){
  const int c = *sbufp++;

  if (c == 'q'){
    *as = 0;
    parse_std_piece(ap,as,ar,af);
    return;
  }
  if (c == 'k'){
    *as = 1;
    parse_std_piece(ap,as,ar,af);
    return;
  }
  if (c == 'p'){
    *ap = 1;
    if (*as >= 0)
      *af = 3;
    goto loc;
  }
  if (c == 'n'){
    *ap = 2;
    goto pie;
  }
  if (c == 'b'){
    *ap = 3;
    goto pie;
  }
  if (c == 'r'){
    *ap = 4;
    goto pie;
  }

  sbufp--;
  goto loc;

pie:
  if (*sbufp == 'p'){
    *af = (*ap - 1) % 3;
    *ap = 1;
    sbufp++;
  }

loc:
  if (*ap < 0 && *as >= 0){
    *ap = *as + 5;
    *as = -1;
  }
  if (*sbufp == '/'){
    sbufp++;
    parse_std_square(as,ar,af);
  }
}

void parse_std_square(int* as, int* ar, int* af){
loop:
  const int c = *sbufp++;

  if (c == 'q'){
    *as = 0;
    goto kq;
  }
  if (c == 'k'){
    *as = 1;
  kq:
    parse_std_square(as,ar,af);
    if (*af < 0)
      *af = 3;
    return;
  }
  if (c == 'r'){
    *af = 0;
    goto loop;
  }
  if (c == 'n'){
    *af = 1;
    goto loop;
  }
  if (c == 'b'){
    *af = 2;
    goto loop;
  }
  if (c > '0' && c < '9')
    *ar = c - '1';
  else
    sbufp--;
}

int piece_compare(const int p, const int l, const int pp, const int sp, const int rp, const int fp){
  int s;

  int f = l % 8;
  int r = l / 8;

  if (! mantom)
    r = 7 - r;

  if (f > 3){
    f = 7 - f;
    s = 1;
  } else{
    s = 0;
  }

  if (val_compare(pp,p) && val_compare(sp,s) &&
    val_compare(rp,r) && val_compare(fp,f))
    return 1;

  return 0;
}

int val_compare(const int p, const int v){
  if (p < 0) return 1;
  return p == abs_val(v);
}

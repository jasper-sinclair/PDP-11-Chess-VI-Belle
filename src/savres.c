#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include "chess.h"

void save_game(void){
  int i;
  const off_t bookp_save = bookp; /* Save as off_t for file writing */

  const int f = _creat("chess.out",0666);
  if (f < 0){
    printf("cannot create file\n");
    return;
  }

  _write(f,&clktim,sizeof(clktim));
  _write(f,&bookp_save,sizeof(off_t));
  _write(f,&moveno,sizeof(moveno));
  _write(f,&game,sizeof(game));

  i = (int)(amp - ambuf);
  _write(f,&i,sizeof(i));
  _write(f,&mantom,sizeof(mantom));
  _write(f,&value,sizeof(value));
  _write(f,&ivalue,sizeof(ivalue));
  _write(f,&depth,sizeof(depth));
  _write(f,&flag,sizeof(flag));
  _write(f,&eppos,sizeof(eppos));
  _write(f,&bkpos,sizeof(bkpos));
  _write(f,&wkpos,sizeof(wkpos));

  _write(f,board,sizeof(board));
  _write(f,ambuf,i * sizeof(int));

  close(f);
}

void restore_game(void){
  int i;
  off_t bookp_loaded;

  const int f = open("chess.out", O_RDONLY);
  if (f < 0){
    printf("cannot open file\n");
    return;
  }

  read(f,&clktim,sizeof(clktim));
  read(f,&bookp_loaded,sizeof(off_t));
  bookp = (int)bookp_loaded;
  read(f,&moveno,sizeof(moveno));
  read(f,&game,sizeof(game));

  read(f,&i,sizeof(i));
  amp = ambuf + i;

  read(f,&mantom,sizeof(mantom));
  read(f,&value,sizeof(value));
  read(f,&ivalue,sizeof(ivalue));
  read(f,&depth,sizeof(depth));
  read(f,&flag,sizeof(flag));
  read(f,&eppos,sizeof(eppos));
  read(f,&bkpos,sizeof(bkpos));
  read(f,&wkpos,sizeof(wkpos));

  read(f,board,sizeof(board));
  read(f,ambuf,i * sizeof(int));

  close(f);
}

#ifndef TETRIS_H
# define TETRIS_H

struct tetris;

void tetris_cleanup_io();

void tetris_signal_quit(int);

void tetris_set_ioconfig();

void tetris_init(struct tetris *t,int w,int h);

void tetris_clean(struct tetris *t);

void tetris_print(struct tetris *t);

void tetris_run(int width, int height);

void tetris_new_block(struct tetris *t);

void tetris_print_block(struct tetris *t);

void tetris_rotate(struct tetris *t, int r);

void tetris_gravity(struct tetris *t);

void tetris_fall(struct tetris *t, int l);

void tetris_check_lines(struct tetris *t);

int tetris_level(struct tetris *t);

void tetris_toggle_pause(struct tetris *t);
void tetris_save_state(struct tetris *t);
void tetris_restore_state(struct tetris *t);

#endif //TETRIS_H
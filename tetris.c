#include <stdio.h>
#include <stdlib.h>
#include <tetris.h>
#include <termios.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>

struct tetris_level {
    int score;
    int nsec;
};

struct tetris {
    char **game;
    int w;
    int h;
    int level;
    int gameover;
    int score;
    int paused;
    struct tetris_block {
        char data[5][5];
        int w;
        int h;
    } current;
    struct tetris_block next_blocks[3];
    int x;
    int y;
    char **prev_game;
    int prev_score;
    int prev_level;
    struct tetris_block prev_next_blocks[3];
    struct tetris_block last_placed_block;
    int undo_state; // 0: no undo, 1: can undo, 2: undo performed, piece falling
};

struct tetris_block blocks[] =
{
    {{"##", 
         "##"},
    2, 2
    },
    {{" X ",
         "XXX"},
    3, 2
    },
    {{"@@@@"},
        4, 1},
    {{"OO",
         "O ",
         "O "},
    2, 3},
    {{"&&",
         " &",
         " &"},
    2, 3},
    {{"ZZ ",
         " ZZ"},
    3, 2},
    {{" SS",
         "SS "},
    3, 2}
};

struct tetris_level levels[]=
{
    {0,
        1200000},
    {1500,
        900000},
    {8000,
        700000},
    {20000,
        500000},
    {40000,
        400000},
    {75000,
        300000},
    {100000,
        200000}
};

#define TETRIS_PIECES (sizeof(blocks)/sizeof(struct tetris_block))
#define TETRIS_LEVELS (sizeof(levels)/sizeof(struct tetris_level))

struct termios save;

void
tetris_cleanup_io() {
    printf("\e[?25h");
    tcsetattr(fileno(stdin),TCSANOW,&save);
}

void
tetris_signal_quit(int s) {
    tetris_cleanup_io();
    exit(0);
}

void
tetris_set_ioconfig() {
    struct termios custom;
    int fd=fileno(stdin);
    printf("\e[?25l");
    tcgetattr(fd, &save);
    custom=save;
    custom.c_lflag &= ~(ICANON|ECHO);
    tcsetattr(fd,TCSANOW,&custom);
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0)|O_NONBLOCK);
}

void
tetris_init(struct tetris *t,int w,int h) {
    int x, y, i;
    t->level = 1;
    t->score = 0;
    t->gameover = 0;
    t->paused = 0;
    t->w = w;
    t->h = h;
    t->game = malloc(sizeof(char *)*w);
    t->prev_game = malloc(sizeof(char *)*w);
    for (x=0; x<w; x++) {
        t->game[x] = malloc(sizeof(char)*h);
        t->prev_game[x] = malloc(sizeof(char)*h);
        for (y=0; y<h; y++)
            t->game[x][y] = ' ';
            t->prev_game[x][y] = ' ';
    }
    for (i = 0; i < 3; i++) {
        t->next_blocks[i] = blocks[random()%TETRIS_PIECES];
    }
    t->undo_state = 0;
}

void
tetris_clean(struct tetris *t) {
    int x;
    for (x=0; x<t->w; x++) {
        free(t->game[x]);
        free(t->prev_game[x]);
    }
    free(t->game);
    free(t->prev_game);
}

void
tetris_save_state(struct tetris *t) {
    int x, y;
    for (x = 0; x < t->w; x++) {
        for (y = 0; y < t->h; y++) {
            t->prev_game[x][y] = t->game[x][y];
        }
    }
    t->prev_score = t->score;
    t->prev_level = t->level;
    for (int i = 0; i < 3; i++) {
        t->prev_next_blocks[i] = t->next_blocks[i];
    }
    t->last_placed_block = t->current;
    t->undo_state = 1; // Can undo
}

void
tetris_restore_state(struct tetris *t) {
    int x, y;
    for (x = 0; x < t->w; x++) {
        for (y = 0; y < t->h; y++) {
            t->game[x][y] = t->prev_game[x][y];
        }
    }
    t->score = t->prev_score;
    t->level = t->prev_level;
    for (int i = 0; i < 3; i++) {
        t->next_blocks[i] = t->prev_next_blocks[i];
    }
    t->current = t->last_placed_block;
    t->x = (t->w / 2) - (t->current.w / 2);
    t->y = 0;
    t->undo_state = 2;
}

void
tetris_print(struct tetris *t) {
    int x,y,i,j;
    printf("\e[H");
    printf("[LEVEL: %d | SCORE: %d]\n", t->level, t->score);
    for (x=0; x<2*t->w+2; x++)
        printf("~");
    printf("\n");
    for (y=0; y<t->h; y++) {
        printf ("!");
        for (x=0; x<t->w; x++) {
            if (x>=t->x && y>=t->y 
                    && x<(t->x+t->current.w) && y<(t->y+t->current.h) 
                    && t->current.data[y-t->y][x-t->x]!=' ')
                printf("%c ", t->current.data[y-t->y][x-t->x]);
            else
                printf("%c ", t->game[x][y]);
        }
        printf ("!");
        if (y < 21) {
            if (y == 0) {
                printf("  +----------+");
            } else if (y == 1) {
                printf("  |   NEXT   |");
            } else if (y == 2 || y == 8 || y == 14) {
                printf("  +----------+");
            } else if (y >= 3 && y < 19) {
                printf("  |");
                int block_index = (y - 3) / 6;
                int row_in_block = (y - 3) % 6;

                if (row_in_block < 5) {
                    struct tetris_block next_block = t->next_blocks[block_index];
                    int block_display_width = next_block.w * 2;
                    int padding_left = (10 - block_display_width) / 2;
                    int padding_right = 10 - block_display_width - padding_left;

                    for (j = 0; j < padding_left; j++) {
                        printf(" ");
                    }

                    for (j = 0; j < next_block.w; j++) {
                        if (row_in_block < next_block.h && next_block.data[row_in_block][j] != ' ') {
                            printf("%c ", next_block.data[row_in_block][j]);
                        } else {
                            printf("  ");
                        }
                    }
                    for (j = 0; j < padding_right; j++) {
                        printf(" ");
                    }
                } else {
                    printf("          ");
                }
                printf("|");
            } else if (y == 20) {
                printf("  +----------+");
            }
        }
        printf ("\n");
    }
    for (x=0; x<2*t->w+2; x++)
        printf("~");
    printf("\n");
}

int
tetris_hittest(struct tetris *t) {
    int x,y,X,Y;
    struct tetris_block b=t->current;
    for (x=0; x<b.w; x++)
        for (y=0; y<b.h; y++) {
            X=t->x+x;
            Y=t->y+y;
            if (X<0 || X>=t->w)
                return 1;
            if (b.data[y][x]!=' ') {
                if ((Y>=t->h) || 
                        (X>=0 && X<t->w && Y>=0 && t->game[X][Y]!=' ')) {
                    return 1;
                }
            }
        }
    return 0;
}

void
tetris_new_block(struct tetris *t) {
    t->current = t->next_blocks[0];
    t->next_blocks[0] = t->next_blocks[1];
    t->next_blocks[1] = t->next_blocks[2];
    t->next_blocks[2] = blocks[random()%TETRIS_PIECES];
    t->x=(t->w/2) - (t->current.w/2);
    t->y=0;
    if (tetris_hittest(t)) {
        t->gameover=1;
    }
}

void
tetris_print_block(struct tetris *t) {
    int x,y,X,Y;
    struct tetris_block b=t->current;
    for (x=0; x<b.w; x++)
        for (y=0; y<b.h; y++) {
            if (b.data[y][x]!=' ')
                t->game[t->x+x][t->y+y]=b.data[y][x];
        }
}

void
tetris_rotate(struct tetris *t, int r) {
    struct tetris_block b=t->current;
    struct tetris_block s=b;
    int x,y;
    b.w=s.h;
    b.h=s.w;
    for (x=0; x<s.w; x++)
        for (y=0; y<s.h; y++) {
            if (r>0)
                b.data[x][y]=s.data[s.h-y-1][x];
            else
                b.data[x][y]=s.data[y][s.w-x-1];
        }
    x=t->x;
    y=t->y;
    t->x-=(b.w-s.w)/2;
    t->y-=(b.h-s.h)/2;
    t->current=b;
    if (tetris_hittest(t)) {
        t->current=s;
        t->x=x;
        t->y=y;
    }
}

void
tetris_gravity(struct tetris *t) {
    int x,y;
    t->y++;
    if (tetris_hittest(t)) {
        t->y--;

        if (t->undo_state != 2) {
            tetris_save_state(t);
        }

        tetris_print_block(t);

        if (t->undo_state == 2) {
            t->undo_state = 0;
            tetris_new_block(t);
        } else {
            tetris_new_block(t);
        }
    }
}

void
tetris_fall(struct tetris *t, int l) {
    int x,y;
    for (y=l; y>0; y--) {
        for (x=0; x<t->w; x++)
            t->game[x][y]=t->game[x][y-1];
    }
    for (x=0; x<t->w; x++)
        t->game[x][0]=' ';
}

void
tetris_check_lines(struct tetris *t) {
    int x,y,l;
    int p=100;
    for (y=t->h-1; y>=0; y--) {
        l=1;
        for (x=0; x<t->w && l; x++) {
            if (t->game[x][y]==' ') {
                l=0;
            }
        }
        if (l) {
            t->score += p;
            p*=2;
            tetris_fall(t, y);
            y++;
        }
    }
}

void
tetris_toggle_pause(struct tetris *t) {
    t->paused = !t->paused;
}

int
tetris_level(struct tetris *t) {
    int i;
    for (i=0; i<TETRIS_LEVELS; i++) {
        if (t->score>=levels[i].score) {
            t->level = i+1;
        } else break;
    }
    return levels[t->level-1].nsec;
}

void
tetris_run(int w, int h) {
    struct timespec tm;
    struct tetris t;
    char cmd;
    int count=0;
    
    printf("\033[2J\033[H");
    fflush(stdout);
    
    tetris_set_ioconfig();
    signal(SIGINT, tetris_signal_quit);
    tetris_init(&t, w, h);
    srand(time(NULL));

    tm.tv_sec=0;
    tm.tv_nsec=1000000;

    tetris_new_block(&t);
    while (!t.gameover) {
        nanosleep(&tm, NULL);
        if (!t.paused) {
            count++;
            if (count%50 == 0) {
                tetris_print(&t);
            }
            if (count%350 == 0) {
                tetris_gravity(&t);
                tetris_check_lines(&t);
            }
        }
        while ((cmd=getchar())>0) {
            switch (cmd) {
                case 'p':
                    tetris_toggle_pause(&t);
                    break;
                case 'q':
                case 68:
                    if (!t.paused) {
                        t.x--;
                        if (tetris_hittest(&t))
                            t.x++;
                    }
                    break;
                case 'd':
                case 67:
                    if (!t.paused) {
                        t.x++;
                        if (tetris_hittest(&t))
                            t.x--;
                    }
                    break;
                case 's':
                case 66:
                    if (!t.paused) {
                        tetris_gravity(&t);
                    }
                    break;
                case ' ':
                case 65:
                    if (!t.paused) {
                        while(!tetris_hittest(&t))
                            t.y++;
                        t.y--;
                    }
                    break;
                case 'z':
                    if (!t.paused) {
                        tetris_rotate(&t, 0);
                    }
                    break;
                case 'x':
                    if (!t.paused) {
                        tetris_rotate(&t, 1);
                    }
                    break;
                case 'c':
                    if (!t.paused) {
                        tetris_rotate(&t, 1);
                        tetris_rotate(&t, 1);
                    }
                    break;
                case 'r':
                    if (!t.paused) {
                        printf("\033[2J\033[H");
                        fflush(stdout);
                        tetris_clean(&t);
                        tetris_init(&t, w, h);
                        tetris_new_block(&t);
                    }
                    break;
                case 'u':
                    if (!t.paused) {
                        if (t.undo_state == 1) {
                            tetris_restore_state(&t);
                        }
                    }
                    break;
            }
        }
        if (!t.paused) {
            tm.tv_nsec=tetris_level(&t);
        }
    }

    tetris_print(&t);
    printf("*** GAME OVER ***\n");

    tetris_clean(&t);
    tetris_cleanup_io();
}
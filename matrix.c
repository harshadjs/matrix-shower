#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <wchar.h>
#include <stdint.h>

typedef struct streak {
	int on;
	int highlight;
} streak_t;

typedef struct point {
	int value;
	int on;
	int highlight;
	int sparkled;
	uint64_t ticks;
} point_t;

#define N_ROWS 28
#define N_COLS 51
#define BOUND(__value) (((__value) >= N_ROWS) ? (N_ROWS - 1) : (__value))

#define MODE_NUMBERS 0
#define MODE_FILE 1

point_t screen[N_ROWS][N_COLS] = {0};
uint64_t ticks = 0;
int matrix_mode;
FILE *gfp;
int offsets[N_COLS];
int sparkle;

int select_character(int value)
{
	int start = '.';
	int end = '.' + 1;

	if(isgraph(value))
		return value;
	else
		return start + (value % (end - start));
}

int set_value(int column)
{
	int ch;

	if(matrix_mode == MODE_FILE) {
		fseek(gfp, offsets[column % N_COLS], SEEK_SET);

		ch = fgetc(gfp);
		offsets[column]++;

		if(ch == EOF) {
			fseek(gfp, 0, SEEK_SET);
			offsets[column] = 0;
		}

		return ch;
	} else {
		return '1' + (random() % 9);
	}
}

int prob(float percentage)
{
	int num = random() % 10000;

	if(num < 100*(percentage))
		return 1;

	return 0;
}


#define WHITE_FG "\e[1;97m"
#define GREEN_FG "\e[1;92m"
#define RESET "\e[0m"
#define BG_START "\e[40m"

#define HIGHLIGHT_FG WHITE_FG
#define NORMAL_FG GREEN_FG

#define GET_HIGHLIGHT(__bool) ((__bool) ? (HIGHLIGHT_FG) : (NORMAL_FG))

void display_screen(void)
{
	int i, j;

	for(i = 0; i < N_ROWS; i++) {
		for(j = 0; j < N_COLS; j++) {
			printf("%s", BG_START);
			if(screen[i][j].on)
				printf("%s%lc ", GET_HIGHLIGHT(screen[i][j].highlight),
					   select_character(screen[i][j].value));

			else
				printf("  ");
			printf("%s", RESET);
		}
		printf("\n");
	}
}

void handle_column(int column)
{
	int i, j;
	int lengths[N_ROWS], streaks[N_ROWS];
	int n_streaks = 0;
	int in_streak = 0;

	/* Find out all streaks in this column */
	for(i = 0; i < N_ROWS; i++) {
		if((!in_streak) && (screen[i][column].on)) {
			streaks[n_streaks] = i;
			in_streak = 1;
		}

		if((in_streak) && (!screen[i][column].on)) {
			in_streak = 0;
			lengths[n_streaks] = i - streaks[n_streaks];
			n_streaks++;
		}
	}

	if(in_streak) {
		lengths[n_streaks] = i - streaks[n_streaks];
		n_streaks ++;
	}

	if(n_streaks == 0) {
		/* Start streaks at random */
		if(prob(70)) {
			i = (random() % 14);
			if(prob(20))
				i += 14;

//			i = 0;
			screen[i][column].on = 1;
			screen[i][column].value = set_value(column);
			screen[i][column].ticks = ticks;
			if(prob(30))
				screen[i][column].highlight = 1;
		}
		return;
	}

	for(i = 0; i < n_streaks; i++) {
		if((ticks - screen[streaks[i]][column].ticks) <= 5) {
			int collision = 0;
			if(prob(10)) {
				/* Turn off this streak */
				continue;
			}

			/* Carry on with this! */
			for(j = 0; j < lengths[i]; j++) {
				if(j + streaks[i] + 1>= streaks[i + 1]) {
					collision = 1;
					break;
				}
				screen[BOUND(j + streaks[i])][column].on = 1;
				screen[BOUND(j + streaks[i])][column].ticks = ticks;

			}
			if(!collision) {
				screen[BOUND(streaks[i] + lengths[i])][column].on = 1;
				screen[BOUND(streaks[i] + lengths[i])][column].value = set_value(column);
				screen[BOUND(streaks[i] + lengths[i])][column].ticks = ticks;
				if(BOUND(j + streaks[i]) - 1 >= 0) {
					if(screen[BOUND(j + streaks[i]) - 1][column].highlight) {
						screen[BOUND(j + streaks[i]) - 1][column].highlight = 0;
						screen[BOUND(j + streaks[i])][column].highlight = 1;
					}
				}
			}
		} else if(ticks - screen[streaks[i]][column].ticks > 25) {
			/* Must die */
			if(prob(70))
				screen[streaks[i]][column].on = 0;
		}
	}
}

void matrix_shower(void)
{
	int i, j;
	int streaks[N_ROWS], n_streaks = 0, in_streak;

	while(1) {
		ticks++;
		if((ticks % 3) == 0) {
			for(i = 0; i < N_COLS; i++) {
				printf("=====================%d\n", i);
				handle_column(i);
			}
		} else {
			for(i = 0; i < N_ROWS; i++) {
				for(j = 0; j < N_COLS; j++) {
					if(prob(0.2)) {
						screen[i][j].value++;
					}
					if((sparkle && prob(1)) ||
					   (screen[i][j].sparkled && prob(10))) {
						/* Sparkle */
						screen[i][j].highlight ^= 1;
						screen[i][j].sparkled = 1;
					}
				}
			}
		}
		display_screen();
		usleep(40000);
	}
}

int main(int argc, char *argv[])
{
	int i, opt;
	char *filename;

	matrix_mode = MODE_NUMBERS;
	while((opt = getopt(argc, argv, "snf:")) != -1) {
		switch(opt) {
		case 'n':
			matrix_mode = MODE_NUMBERS;
			break;
		case 'f':
			matrix_mode = MODE_FILE;
			filename = (char *)optarg;
			break;
		case 's':
			sparkle = 1;
			break;
		default:
			printf("Usage: %s -n -f [Filename] -s\n", argv[0]);
			return 0;
		}
	}
	if(matrix_mode == MODE_FILE) {
		gfp = fopen(filename, "r+");
		if(!gfp)
			matrix_mode = MODE_NUMBERS;
	}

	setlocale(LC_ALL, "");
	for(i = 0; i < N_COLS; i++) {
		offsets[i] = i*10;
	}

	matrix_shower();
	return 0;
}

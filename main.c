#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>

#define NUM_ROWS 4
#define NUM_COLS 20

// ------------------
//  Type Definitions
// ------------------
typedef struct {
	char cells[NUM_ROWS][NUM_COLS];
} LcdScreen;

typedef enum { NEUTRAL_X, LEFT, RIGHT } Joystick_X;
typedef enum { NEUTRAL_Y, UP, DOWN } Joystick_Y;

typedef struct {
	Joystick_X x;
	Joystick_Y y;
} Joystick;

typedef struct {
	int x;
	int y;
} Cell;
// ------------------


// ------------------
//  LCD Screen Functions
// ------------------

// Wipes the terminal to all redrawing the screen
void clear_screen(void) {
	// Clear screen + move cursor to top-left
	printf("\x1b[2J\x1b[H");
}


// Initializes the screen with all . characters
LcdScreen initScreen() {
	LcdScreen screen;
	for (int r = 0; r < NUM_ROWS; r++) {
		for (int c = 0; c < NUM_COLS; c++) {
			screen.cells[r][c] = '.';
		}
	}
	return screen;
}


// Display  the screen contents
void printScreen(LcdScreen screen) {
	clear_screen();

	printf("----------------------\n");

	for (int r = 0; r < NUM_ROWS; r++) {
		printf("|");
		for (int c = 0; c < NUM_COLS; c++) {
			printf("%c", screen.cells[r][c]);
		}
		printf("|\n");
	}
	printf("----------------------\n\n");
	printf("Press CTRL+C to quit\n");
}



// --------------------------------------------------
// NON-BLOCKING KEY INPUT (POSIX)
//     These functions will not be used on our microcontroller
//     They are only used for displaying the screen on the console
//     Do not edit these functions
// --------------------------------------------------

static struct termios g_orig_termios;

// Restore the terminal to its original (canonical, echoing) input mode
static void disable_raw_mode(void) {
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_orig_termios);
}

// Configure the terminal for non-blocking, non-echoed raw keyboard input
static void enable_raw_mode(void) {
	tcgetattr(STDIN_FILENO, &g_orig_termios);
	atexit(disable_raw_mode);

	struct termios raw = g_orig_termios;
	raw.c_lflag &= ~(ICANON | ECHO); // no line buffering, no echo
	raw.c_cc[VMIN] = 0;              // read() returns immediately
	raw.c_cc[VTIME] = 0;
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

// Check whether a key has been pressed without blocking program execution
static int kbhit(void) {
	fd_set set;
	FD_ZERO(&set);
	FD_SET(STDIN_FILENO, &set);
	struct timeval tv = { 0, 0 }; // don't wait
	return select(STDIN_FILENO + 1, &set, NULL, NULL, &tv) > 0;
}

// Read a single key press from standard input without blocking, or return -1 if none
static int read_key(void) {
	unsigned char c;
	ssize_t n = read(STDIN_FILENO, &c, 1);
	if (n == 1) return (int)c;
	return -1;
}

// --------------------------------------------------


// Reads WASD character and returns movement (UP, LEFT, DOWN, RIGHT, NEUTRAL)
//      joystick axes are indepedent, so not there is a seperate joystick.x and joystick.y
Joystick pollJoystick() {

	Joystick joystick;
	joystick.x = NEUTRAL_X;
	joystick.y = NEUTRAL_Y;

	while (kbhit()) {             // drain all pending keys this frame
		int ch = read_key();

		if (ch == 'a')
			joystick.x = LEFT;
		if (ch == 'd')
			joystick.x = RIGHT;
		if (ch == 'w')
			joystick.y = UP;
		if (ch == 's')
			joystick.y = DOWN;
	}

	return joystick;
}




int main(void) {

	//----------------------
	// Initialization
	//----------------------
	enable_raw_mode();

	LcdScreen screen = initScreen();
	//----------------------

	// Keep track of the hero 'O' character
	Cell hero;
	hero.x = 0;
	hero.y = 0;

	// Keep track of the old hero's location (to support erasing old location)
	Cell prevHero = hero;

	// Loop Forever
	while (1) {

			Joystick joystick = pollJoystick();
		if (joystick.x == LEFT && hero.x > 0) {
			hero.x--;
		}
		if (joystick.x == RIGHT && hero.x < NUM_COLS - 1) {
			hero.x++;
		}
		if (joystick.y == UP && hero.y > 0) {
			hero.y--;
		}
		if (joystick.y == DOWN && hero.y < NUM_ROWS - 1) {
			hero.y++;
		}

		screen.cells[prevHero.y][prevHero.x] = '.';

		screen.cells[hero.y][hero.x] = 'O';

		prevHero = hero;

		printScreen(screen);

		fflush(stdout);      // Flush - Make sure screen is displayed immediately
		usleep(250000);      // 250ms delay
	}

	return EXIT_SUCCESS;
}

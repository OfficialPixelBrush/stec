/*  ___ ____ ___  ___
 * /__   /  /__  /
 * __/  /  /__  /__ 
 * by T. Virtmann
 *
 * A down to earth text-editor that is straight-forward,
 * simple and has no obscure keybindings. 
 */
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

// Some very important variables
unsigned char *fileMem; // The actual file in Memory
unsigned int fileSize; // The size of the loaded file in Bytes
unsigned int rows, cols; // The size of the terminal in characters (e.g. 80x25)
unsigned int cursorX, cursorY = 1; // The current position of the cursor relative to the first character of the file
unsigned char running = 1;

// I hate myself for wanting this on both Linux and Windows
// And that I'm refusing to use ncurses or similar
#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#endif

// This Code has been generated by ChatGPT (Jan 30 Version). Thank it for this blessing.
// For the arrow keys we'll be using Device Control 1-4
#ifdef _WIN32
// Into this bit we put the Windows versions of functions
// Disable Echo and waiting for return to accept Keyboard input
void disable_echo_and_buffering()
{
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hStdin, &mode);
    mode &= ~ENABLE_ECHO_INPUT;
    mode &= ~ENABLE_LINE_INPUT;
	// Also disable Ctrl+C fucking stuff up
    mode &= ~ENABLE_PROCESSED_INPUT;
    SetConsoleMode(hStdin, mode);
}

// Enable Echo and waiting for return to accept Keyboard input
void enable_echo_and_buffering(void)
{
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode;
    GetConsoleMode(hStdin, &mode);
    mode |= ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT;
    SetConsoleMode(hStdin, mode);
}

// Get current Terminal Size automatically 
void get_terminal_size(int *rows, int *cols)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    *cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    *rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
}

// Decode Arrow Keys to Device Control Characters
int decodeKeycodes() {
  int ch = _getch();
  if (ch != 0 && ch != 224) {
    return ch;
  }

  ch = _getch();
  switch (ch) {
    case 72:
      return 17;
    case 80:
      return 18;
    case 77:
      return 19;
    case 75:
      return 20;
    default:
      return ch;
  }
}
#else
// Into this bit we put the Linux versions of functions
// Disable Echo and waiting for return to accept Keyboard input
void disable_echo_and_buffering()
{
    struct termios tty;
    tcgetattr(STDIN_FILENO, &tty);
    tty.c_lflag &= ~ECHO;
    tty.c_lflag &= ~ICANON;
    tcsetattr(STDIN_FILENO, TCSANOW, &tty);
	// Also disable Ctrl+C fucking stuff up
	signal(SIGINT, SIG_IGN);
}

// Enable Echo and waiting for return to accept Keyboard input
void enable_echo_and_buffering(void)
{
    struct termios t;
    tcgetattr(STDIN_FILENO, &t);
    t.c_lflag |= ECHO | ICANON;
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
}

// Get current Terminal Size automatically 
void get_terminal_size(int *rows, int *cols)
{
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    *cols = w.ws_col;
    *rows = w.ws_row;
}

// Decode Arrow Keys to Device Control Characters
int decodeKeycodes() {
  int ch = getchar();
  if (ch != 27) {
    return ch;
  }

  ch = getchar();
  if (ch != 91) {
    return ch;
  }

  ch = getchar();
  switch (ch) {
    case 65:
      return 17;
    case 66:
      return 18;
    case 67:
      return 19;
    case 68:
      return 20;
    default:
      return ch;
  }
}
#endif
// Past this, I wrote everything

// Get Size of File
int fsize(FILE *fp){
    int prev=ftell(fp);
    fseek(fp, 0L, SEEK_END);
    int sz=ftell(fp);
    rewind(fp); // Go back to the start of the filestream
    return sz;
}

// Redraw the entire screen relative to start of file
// TODO: Figure out some way to format the file based on the lines,
// To make it easier to draw the screen without going through the entire file
int printScreenAtZero() {
	// Clear the Screen
	printf("\x1b[2J");
	int x = 1;
	int y = 1;
	// Draw that shit
	for (int i = 0; i < fileSize; i++) {
		// Check if y is larger than the max number of rows possible
		if (y <= rows) {
			// If you encounter a new-line, go to the next line
			switch(fileMem[i]) {
				case 9: // Tab
					x += 4;
					break;
				case '\n': // New-line 
					y++;
					x = 1;
					break;
				default:
					printf("\x1b[%u;%uH",y,x);
					putchar(fileMem[i]);
					x++;
					break;
			}
		} else {
			break;
		}
	}
}

// Print the Screen with offset defined by cursor position
int printScreen() {
	// Clear the Screen
	printf("\x1b[2J");
	int x = 1;
	int y = 1;
	// Draw that shit
	for (int i = 0; i < fileSize; i++) {
		// Check if y is larger than the max number of rows possible
		if (y <= rows) {
			// If you encounter a new-line, go to the next line
			switch(fileMem[i]) {
				case 9: // Tab
					x += 4;
					break;
				case '\n': // New-line 
					y++;
					x = 1;
					break;
				default:
					printf("\x1b[%u;%uH",y,x);
					putchar(fileMem[i]);
					x++;
					break;
			}
		} else {
			break;
		}
	}
}

// Place the cursor at the specified x,y screen coorindate
int placeCursor() {
	if (cursorX < 1) {
		cursorX = 1;
	}
	
	if (cursorY < 1) {
		cursorY = 1;
	}
	
	printf("\x1b[%u;%uH", cursorY%rows, cursorX%cols);
	return 0;
}

// Type a single character
int typeCharacter(char character, char character2) {
	// Set cursor position to reflect cursorX and cursorY
	// Make space in the array for a character/
	// Insert the Character
	// TODO: Figure this shit out
	/*
	fileSize++;
	fileMem = realloc(fileMem, fileSize); // reallocations until size settles
	int i;
	int pos = 0; // Later to be replaced by x,y position
    for (i = fileSize; i >= pos; i--)
        fileMem[i] = fileMem[i - 1];
	fileMem[pos] = character;
	printScreen();*/
}
	
// Main Program function
int main(int argc, char *argv[]) {
	// Figure out the Terminal Size
    get_terminal_size(&rows, &cols);
    printf("The terminal size is %d rows by %d columns.\n", rows, cols);
	// This is here as a debug feature, just to check if I'm dumb
	
	// Argunent related parameters
	// I'll put shit like help params and stuff here	
	if (argc < 1) {
		printf("Too few arguments!\n");
		return 1;
	}
	
	// ****** Load file into Memory ****** 
	/* While this is worse for large files,
	* A) we're only dealing with smaller files here
	* B) nano and vi seem to do this too, and nobody's complaining
	* C) it's faster than accessing the hard drive repeatedly
	*/
	
	// Load the file
	FILE *fptr;
	// Best to check if the file doesn't exist when loading it in
	if ((fptr = fopen(argv[1],"rb")) == NULL) {
		printf("File doesn't exist!");   
		// Routine for creating a file that doesn't yet exist will go here
		exit(1);             
	}
	
	// Get Filesize to determine array size
	fileSize = fsize(fptr);
	printf("File is %dB Big",fileSize);
	
	//fileMem = malloc (fileSize);
	// Read the file into Memory
    //fread (fileMem, 1, fileSize, fptr);
	
	/* Debug code for printing the entire file
	int i = 0;
	while(fileMem[i] != NULL) {
		printf("%c", fileMem[i]);
		i++;
	}*/
	
	// ****** Init the Screen ****** 
	// Disablecharacter echo and buffering
	disable_echo_and_buffering(fptr);
	// Print until width of tty is reached, then read until next new-line
	printf("\x1b[0m");
	printScreen(cursorX,cursorY);
	// Reset Cursor
	printf("\x1b[H");
	
	// This loop is for testing only
	while(running) { 
		// Place cursor where it should be according to page
		placeCursor();
		// Check what character was pressed
		unsigned int currentCharacter = decodeKeycodes();
		/* Handle special cases with a switch
		 *
		 * In thise case, certain characters, like the arrows keys
		 * are handled differently. They get mapped to the Device Control
		 * Characters of the ASCII Table (Character 17-20)
		 * I know it's a cursed solution, but it's simple and good enough
		 * for my needs.
		 */
		 
		 // Check for screen movement
		 
		switch(currentCharacter) {
			case 9: // Tab
				cursorX += 4;
				break;
			case 13: // New-line
				cursorY++;
				break;
			case 17: // Up Arrow
				// printf("\x1b[1A");
				cursorY--;
				break;
			case 18: // Down Arrow
				// printf("\x1b[1B");
				cursorY++;
				break;
			case 19: // Right Arrow
				// printf("\x1b[1C");
				cursorX++;
				break;
			case 20: // Left Arrow
				// printf("\x1b[1D");
				cursorX--;
				break;
			default: // Handling for literally anything else
				//printScreen();
				// This code is bullshit and I can't figure out how to resize an array
				/*printf("%c",currentCharacter);
				fileSize++;
				fileMem = (char *) realloc(fileMem, fileSize);
				for (int i = fileSize - 1; i >= cursorX; i--) {
					fileMem[i + 1] = fileMem[i];
				}
				fileMem[cursorX-1] = currentCharacter;
				cursorX++;
				printScreen(cursorX,cursorY);
				// So fucken close to just using C++*/
				//printf("%d\n", currentCharacter);
				break;
		}
		// Use 9 to exit
		if (currentCharacter == '9') {
			running = 0;
		}
	}
	
	// Determine Cursor Position
	// Determine position in File based on Cursor Position
	// Draw screen based on cursor position
	// 
	
	// Close Application
	free(fileMem); // Remove the file from memory
	fclose(fptr);  // Close the filestream
	printf("\x1b[0m"); // Reset Colors etc
	enable_echo_and_buffering(); // Re-enable echo and buffering
	return 0;
}
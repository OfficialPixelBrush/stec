#include <stdio.h>
#include <stdlib.h>
#include <conio.h>

// This Code has been generated by ChatGPT (Jan 30 Version). Thank it for this blessing.
#ifdef _WIN32
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#endif

// This too
#ifdef _WIN32
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

void get_terminal_size(int *rows, int *cols)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    *cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    *rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
}
#else
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

void get_terminal_size(int *rows, int *cols)
{
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    *cols = w.ws_col;
    *rows = w.ws_row;
}
#endif
// Past this, I wrote everything

// Get Size of File
int fsize(FILE *fp){
    int prev=ftell(fp);
    fseek(fp, 0L, SEEK_END);
    int sz=ftell(fp);
    rewind(fp);
    return sz;
}

char *fileMem;
int rows, cols;
int fileSize;
int cursorX, cursorY = 0;

// Redraw the entire screen relative to the current cursor position
int printScreen() {
	// Clear the Screen
	printf("\x1b[2J");
	// Draw that shit
	int x,y = 0;
	for (int i = 0; i < fileSize; i++) {
		if (y < rows) {
			if (x < cols) {
				printf("\x1b[%d;%dH",y+1,x+1);
				printf("%c",fileMem[i]);
				x++;
			}
			if (fileMem[i] == '\n') {
				y++;
				x = 0;
			}
		}
	}
}

// Type a single character
int typeCharacter(char character) {
	// TODO: Figure this shit out
	/*
	fileSize++;
	fileMem = realloc(fileMem, fileSize); // reallocations until size settles
	int i;
	int pos = 0; // Later to be replaced by x,y position
    for (i = fileSize; i >= pos; i--)
        fileMem[i] = fileMem[i - 1];
	fileMem[pos] = character;
	printScreen();
	*/
}
	
int main(int argc, char *argv[]) {
    get_terminal_size(&rows, &cols);
    printf("The terminal size is %d rows by %d columns.\n", rows, cols);
	
	// Argunent related parameters
	// I'll put shit like help params and stuff here	
	if (argc < 1) {
		printf("Too few arguments!\n");
		return 1;
	}
	
	// Load file into Memory
	/* While this is worse for large files,
	* A) we're only dealing with smaller files here
	* B) nano and vi seem to do this too, and nobody's complaining
	* C) it's faster than repeatedly accessing the hard drive repeatedly
	* 
	* For simplicity, we'll be doing exactly what nano does too, as slow as it sounds.
	*/
	
	// Load the file
	FILE *fptr;
	if ((fptr = fopen(argv[1],"rb")) == NULL) {
		printf("File doesn't exist!");   
		// Routine for creating a file that doesn't yet exist will go here
		exit(1);             
	}
	
	// Get Filesize to determine array size
	fileSize = fsize(fptr);
	printf("File is %dB Big",fileSize);
	
	fileMem = malloc (fileSize);
    fread (fileMem, 1, fileSize, fptr);
	
	/* Debug code for printing the entire file
	int i = 0;
	while(fileMem[i] != NULL) {
		printf("%c", fileMem[i]);
		i++;
	}*/
	
	// Init the Screen
	// Disablecharacter echo and buffering
	disable_echo_and_buffering(fptr);
	// Print until width of tty is reached, then read until next new-line
	printScreen(cursorX,cursorY);
	// Reset Cursor
	printf("\x1b[H");
	
	char currentCharacter;
	while(1) {
		if (kbhit()) {
			// loop to wait
			currentCharacter = getch();
			if (currentCharacter == '9') {
				break;
			} else {
				typeCharacter(currentCharacter);
			}
		}
	}
	
	// Determine Cursor Position
	// Determine position in File based on Cursor Position
	// Draw screen based on cursor position
	// 
	
	// Close Application
	free(fileMem);
	fclose(fptr);
	return 0;
}
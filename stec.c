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

// Linked list Implementation and how files are loaded
/* Say we have a simple file with 3 lines of text:
 * Line 1\n
 * Line 2\n
 * Line 3
 *
 * The file will be split into it's individual lines.
 * Those lines will be loaded as part of a linked list,
 * where each line is part of one element.
 * Inside each of these elements, the actual text resides in an array.
 * So our document is stored as follows:
 * nodeL1
 *	  \_*lineptr -> (points to array for line 1)
 *      *nextNode -> nodeL2;
 * nodeL2
 *    \_*lineptr -> (points to array for line 2)
 *      *nextNode -> nodeL3;
 * nodeL3
 *    \_*lineptr -> (points to array for line 3)
 *      *nextNode -> null;
 * The size of the line arrays are changed using typical array resizing operations.
 */
typedef struct node {
    unsigned char *lineptr;
    struct node * next;
} node_t;

// Get node at index
node_t *get_node_at_index(node_t *head, unsigned long int index) {
  node_t *current = head; // Load in the currently viewed node
  for (unsigned long int i = 0; i < index; i++) { 
    if (current->next == NULL) { // Iterate over all the nodes until the relevant node is reached
      return NULL; // If index is out of bounds, return NULL
    }
    current = current->next; // Move to next index if one is found
  }
  return current;
}


// Get Size of File
int fsize(FILE *fp){
    int prev=ftell(fp);
    fseek(fp, 0L, SEEK_END);
    int sz=ftell(fp);
    rewind(fp); // Go back to the start of the filestream
    return sz;
}

// Redraw the entire screen
int printScreen(node_t *head) {
	// Clear the Screen
	printf("\x1b[2J");
	for (int y = 1; y <= rows; y++) {
		node_t *current = get_node_at_index(head,y-1);
		printf("\x1b[%u;%uH", y, 1);
		for (int x = 1; x <= cols; x++) { 
			char character = current->lineptr[x-1];
			if (character != 0) {
				printf("%c", character);
			} else {
				break;
			}
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
	// Load the file
	FILE *fptr;
	// Best to check if the file doesn't exist when loading it in
	if ((fptr = fopen(argv[1],"rb")) == NULL) {
		printf("File doesn't exist!\n");   
		// Routine for creating a file that doesn't yet exist will go here
		exit(1);             
	}
	
	// Get Filesize to determine array size
	fileSize = fsize(fptr);
	printf("File is %dB Big\n",fileSize);
	
	// Read the file into Memory
	/* Read chars until new-line is hit
	 * Example:
	 * Line 1\n<- Line end found!
	 * 
	 * Load from start of line until new-line into string
	 */	
	node_t * head = (node_t *) malloc(sizeof(node_t)); // Allocate space for head node
	if (head == NULL) {
		printf("Head node allocation error\n");
		return 1;
	}
	head->lineptr = NULL;
	head->next = NULL;

	// Load other chars
	unsigned long int lineStart = 0;
	unsigned long int lineEnd = 0;
	node_t *current = head;
	for (unsigned long int i = 0; i <= fileSize; i++) {
		unsigned char character = fgetc(fptr);
		// TODO: The last line won't render, probably because it doesn't have a new-line character
		// Attached to it, signaling it's the end of a line. Using EOF doesn't seem to do anything here.
		// Hacky Fix: Checking if i >= fileSize.
		if ( (character == '\n') || (i>=fileSize) ) {
			lineEnd = i; // Set end of current line
			// Allocate memory for line, including the null terminator
			char *arrayPtr = malloc(lineEnd - lineStart + 1);
			if (arrayPtr == NULL) {
				printf("Line allocation error\n");
				return 1;
			}

			fseek(fptr, lineStart, SEEK_SET); // Read from start of line
			for (unsigned long int j = lineStart; j <= lineEnd; j++) { // Iterate over line and copy into array
				arrayPtr[j - lineStart] = fgetc(fptr);
			}
			arrayPtr[lineEnd - lineStart] = 0; // Set the null terminator
			
			// Create new node
			current->lineptr = arrayPtr; // Point to new string
			node_t *newNode = (node_t *) malloc(sizeof(node_t)); // Allocate space for new node
			if (newNode == NULL) { // Error Check
				printf("New node allocation error\n");
				return 1;
			}
			current->next = newNode; // Point to new node
			newNode->next = NULL;
			current = newNode;
			lineStart = i+1; // Set start of next line
		}
	}
	// Done loading the file
	fclose(fptr);  // Close the filestream
	
	// ****** Init the Screen ****** 
	// Disablecharacter echo and buffering
	disable_echo_and_buffering(fptr);
	// Print until width of tty is reached, then read until next new-line
	printf("\x1b[0m");
	printScreen(head);
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
	// TODO: Free memory from the nodes
	printf("\x1b[0m"); // Reset Colors etc
	enable_echo_and_buffering(); // Re-enable echo and buffering
	return 0;
}
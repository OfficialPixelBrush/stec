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
int rows, cols; // The size of the terminal in characters (e.g. 80x25)
long int currentRow, currentCollumn = 0; // The current position of the cursor relative to the first character of the file
unsigned char running = 1; // Set if application is running
unsigned char query = 0; // Set if application is asking for user input
unsigned long int maxRows = 0;

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
 */
typedef struct lineNode {
    unsigned char *lineptr;
	unsigned int linelength;
    struct lineNode * prev;
    struct lineNode * next;
} lineNode_t;

// Struct for a character Node
typedef struct charNode {
	unsigned char letter;
	struct charNode * prev;
	struct charNode * next;
} charNode_t;

// TODO: Conversion from array to linked-list of nodes
charNode_t arrayToNodes() {
	// Load in each char and turn it into a charNode
}

unsigned char * nodesToArray() {
	// Iterate over the nodes to find out how long it is (alternatively, just keep track of it using lineLength)
	// Make an appropriately sized array with the characters this lineNode contains
	// Free the memory the nodes took up
	// Replace the pointer of the array the current line's lineNode had
	// Free the memory the old array used
}

// Get Size of File
int fsize(FILE *fp){
    int prev=ftell(fp);
    fseek(fp, 0L, SEEK_END);
    int sz=ftell(fp);
    rewind(fp); // Go back to the start of the filestream
    return sz;
}

// This was patched by ChatGPT Feb 13 Version
// Redraw the entire screen
int printScreen(lineNode_t *head) {
    printf("\x1b[?25l"); // Turn off cursor

    char character;
    int rowsHalf = rows / 2;
    lineNode_t *current = head;

    // Iterate over top part of the screen
    for (int y = rowsHalf; y >= 1; y--) {
        // Position Cursor
        printf("\x1b[%u;%uH", y, 1);
        // Clear line
        printf("\x1b[2K");
        if ((current != NULL) && (current->lineptr != NULL)) {
            for (int x = 1; x <= cols; x++) {
                character = current->lineptr[x - 1];
                if (character != 0) {
                    putchar(character);
                }
                else {
                    break;
                }
            }
        }
        if (current->prev != NULL) {
            current = current->prev;
        }
        else {
            break;
        }
    }
	
    current = head; // Re-center head
    // Iterate over bottom part of the screen
    for (int y = rowsHalf; y <= rows; y++) {
        // Position Cursor
        printf("\x1b[%u;%uH", y, 1);
        // Clear line
        printf("\x1b[2K");
        if ((current != NULL) && (current->lineptr != NULL)) {
            for (int x = 1; x <= cols; x++) {
                character = current->lineptr[x - 1];
                if (character != 0) {
                    putchar(character);
                }
                else {
                    break;
                }
            }
        }
        current = current->next;
    }

    printf("\x1b[?25h"); // Enable Cursor
    return 0;
}

// Place the cursor at the specified x,y screen coorindate
int placeCursor() {	
	printf("\x1b[%u;%uH", rows/2, currentCollumn+1);
	return 0;
}

// Type a single character
int typeCharacter(char character) {
	// Set cursor position to reflect currentCollumn and currentRow
	// Make space in the array for a character/
	// Insert the Character
	// TODO: Figure this shit out
	// Will be a lot easier with charNodes!
}

// Load the File into Nodes
int loadFileIntoNodes(FILE *fptr,lineNode_t *current) {
	// Load other chars
	unsigned long int lineStart = 0;
	unsigned long int lineEnd = 0;
	lineNode_t *previous = NULL;
	for (unsigned long int i = 0; i <= fileSize; i++) {
		unsigned char character = fgetc(fptr);
		// Make new nodes until EOF
		if ( (character == '\n') || (i >= fileSize) ) {
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
			maxRows++; // Increase number of max rows in file
			
			// Create new lineNode
			lineNode_t *newNode = (lineNode_t *) malloc(sizeof(lineNode_t)); // Allocate space for new lineNode
			if (newNode == NULL) { // Error Check
				printf("New lineNode allocation error\n");
				return 1;
			}
			newNode->lineptr = arrayPtr; // Point to new string
			newNode->linelength = lineEnd-lineStart; // Get size of line for convenience
			newNode->prev = previous; // Point to previous lineNode
			newNode->next = NULL; // Define next lineNode as NULL
			
			// Make the next lineNode of the current lineNode the new Node
			current->next = newNode;
			
			// The current lineNode is now the current lineNode
			previous = newNode;
			
			// The current lineNode is now the next lineNode
			current = newNode;
			lineStart = i+1; // Set start of next line
		}
	}
	return 0;
}
	
// TODO: Put most of these into their own functions
// Main Program function
int main(int argc, char *argv[]) {
	
	// ****** Load file into Memory ****** 	
	// Load the file
	FILE *fptr;
	// Best to check if the file doesn't exist when loading it in
	
	// Shoutout to http://users.cms.caltech.edu/~mvanier/CS11_C/misc/cmdline_args.html
	for (int i = 1; i < argc; i++) {  /* Skip argv[0] (program name). */
		/*
		 * Use the 'strcmp' function to compare the argv values
		 * to a string of your choice (here, it's the optional
		 * argument "-q").  When strcmp returns 0, it means that the
		 * two strings are identical.
		 */
		if (strcmp(argv[i], "-h") == 0) {  /* Process optional arguments. */
			printf("stec <filename>\n");
			return 0;
		} else {
			if ((fptr = fopen(argv[i],"rb")) == NULL) {
				printf("File doesn't exist!\n");   
				// Routine for creating a file that doesn't yet exist will go here
				exit(1);             
			}
		}
	}
	
	// Figure out the Terminal Size
    get_terminal_size(&rows, &cols);
    printf("The terminal size is %d rows by %d columns.\n", rows, cols);
	// This is here as a debug feature, just to check if I'm dumb
	
	// Get Filesize to determine array size
	fileSize = fsize(fptr);
	printf("File is %dB Big\n",fileSize);
	
	// ****** Read the file into Memory ******
	// Set up lineNode for the currently selected line
	lineNode_t * current = (lineNode_t *) malloc(sizeof(lineNode_t)); // Allocate space for current lineNode
	if (current == NULL) {
		printf("Current lineNode allocation error\n");
		return 1;
	}
	current->prev = NULL;
	current->linelength = 0;
	current->lineptr = NULL;
	current->next = NULL;
	
	// Fill up the nodes with data
	loadFileIntoNodes(fptr,current);
	// Close the filestream
	fclose(fptr); 
	
	// ****** Init the Screen ****** 
	// Disable  character echo and buffering
	disable_echo_and_buffering(fptr);
	// Print until width of tty is reached, then read until next new-line
	printf("\x1b[0m");
	printScreen(current);
	// Reset Cursor
	printf("\x1b[H");
	
	// This loop is for testing only
	// TODO: Make use of currentCollumn and currentRow for cursor positioning
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
		// TODO: Make a general method for moving the cursor
		switch(currentCharacter) {
			case 9: // Tab
				// TODO: Create Tab function
				currentCollumn += 4;
				break;
			case 13: // New-line
				// TODO: Create New-line function
				//createNewLine();
				break;
			case 17: // Up Arrow
				if (currentRow-1 > 0) {
					currentRow--;
					current = current->prev;
				}
				break;
			case 18: // Down Arrow
				// TODO: Keep track of the number of lines for limiting scrolling
				if (currentRow<maxRows-1) {
					currentRow++;
					current = current->next;
				}
				break;
			case 19: // Right Arrow
				//if (currentCollumn+1 < get_node_at_index(head,currentRow)->linelength) {
					currentCollumn++;
				//}
				break;
			case 20: // Left Arrow
				if (currentCollumn != 0) {
					currentCollumn--;
				}
				break;
			case 71: // Home
				currentCollumn = 0;
				break;
			case 73: // Up Page
				if (currentRow - rows >= 0) {
					currentRow -= rows;
				} else {
					currentRow = 0;
				}
				break;
			case 81: // Down Page
				if (currentRow + rows > maxRows-1) {
					currentRow = maxRows-1;

				} else {
					currentRow += rows;
				}
				break;
			// TODO: Figure out end
			/*case 79: // End 
				for (int i = 0; i >= 1000; i++) {
					if (get_node_at_index(current,currentRow-1)->lineptr[i] == 0) {
						currentCollumn = i;
						break;
					}
				}
				break;*/
			default: // Handling for literally anything else
				// TODO: Print Characters already
				break;
		}
		// Check if there's enough of a reason to update the screen
		//checkIfReRenderNecessary();
		printScreen(current);
		
		// Use Ctrl+X to exit
		if (currentCharacter == 24) {//'9') {
			query = 1;
			printf("\x1b[%u;%uH\x1b[2K", rows, 1);
			printf("Are you sure you want to exit? y/n");
			while (query == 1) {
				switch ( _getch()) {
					case 'y': // Exit Application
					case 'Y':
						query = 0;
						running = 0;
						break;
					case 'n': // Resume Application
					case 'N':
						query = 0;
						break;
				};
			}
		}
	}
	
	// Close Application
	// TODO: Free memory from the nodes
	printf("\x1b[0m"); // Reset Colors etc
	enable_echo_and_buffering(); // Re-enable echo and buffering
	return 0;
}
#include <stdio.h>
#include <stdlib.h>
//#include <conio.h>
//#include <windows.h>
#include "rs232.h"
#include "serial.h"

#define bdrate 115200               /* 115200 baud */
#define NUM_ROWS 1027               // The number of rows in the file
#define COLUMNS 3                   // The number of columns in each row
#define LINE_WIDTH 100              // Maximum width of a line
#define ASCII_MARKER 999            // Marker indicating the start of a new batch

//Structure for our array
struct DataRow 
{
    int col1;
    int col2;
    int col3;
};

// Function prototypes
void SendCommands (char *buffer );
struct DataRow* allocateMemoryForDataArray(int numRows); 
int getHeightFromUser(); 
void readFile(const char *filename, struct DataRow *dataArray, int numRows);
void findAsciiData(struct DataRow *dataArray, int numRows, int asciiValue, int height, double xOffset, double yOffset);
FILE* openTextFile(const char *filename);
void processWordsFromFile(FILE *asciiFile, struct DataRow *dataArray, int numRows, int height, double *xOffset, double *yOffset);
int readNextWordFromFile(FILE *asciiFile, char *word);
int calculateWordWidth(const char *word, int height);
void processWord(struct DataRow *dataArray, int numRows, char *word, int height, double *xOffset, double yOffset);


int main()
{
    //char mode[]= {'8','N','1',0};
    char buffer[100];

    // If we cannot open the port then give up immediately
    if ( CanRS232PortBeOpened() == -1 )
    {
        printf ("\nUnable to open the COM port (specified in serial.h) ");
        exit (0);
    }

    // Time to wake up the robot
    printf ("\nAbout to wake up the robot\n");

    // We do this by sending a new-line
    sprintf (buffer, "\n");
     // printf ("Buffer to send: %s", buffer); // For diagnostic purposes only, normally comment out
    PrintBuffer (&buffer[0]);
    Sleep(100);

    // This is a special case - we wait  until we see a dollar ($)
    WaitForDollar();

    printf ("\nThe robot is now ready to draw\n");

        //These commands get the robot into 'ready to draw mode' and need to be sent before any writing commands
    sprintf (buffer, "G1 X0 Y0 F1000\n");
    SendCommands(buffer);
    sprintf (buffer, "M3\n");
    SendCommands(buffer);
    sprintf (buffer, "S0\n");
    SendCommands(buffer);

    // Allocate memory for NUM_ROWS rows using the new function
    struct DataRow *dataArray = allocateMemoryForDataArray(NUM_ROWS);

    // Read the font data from the file
    readFile("SingleStrokeFont.txt", dataArray, NUM_ROWS);

    // Ask the user for the height multiplier
    int height = getHeightFromUser();  // Use the function to get the height

    // Open the text file that contains words using the new function
    FILE *asciiFile = openTextFile("text.txt");

    // Initialise xOffset to 0
    double xOffset = 0.0;
    // Start Y-offset at height
    double yOffset = 0 - height;

    // Call the new function to process words from the file
    processWordsFromFile(asciiFile, dataArray, NUM_ROWS, height, &xOffset, &yOffset);

    // Close the text file
    fclose(asciiFile);

    // Free the allocated memory
    free(dataArray);

    // Before we exit the program we need to close the COM port
    CloseRS232Port();
    printf("Com port now closed\n");

    return (0);
}

// Send the data to the robot - note in 'PC' mode you need to hit space twice
// as the dummy 'WaitForReply' has a getch() within the function.
void SendCommands (char *buffer )
{
    // printf ("Buffer to send: %s", buffer); // For diagnostic purposes only, normally comment out
    PrintBuffer (&buffer[0]);
    WaitForReply();
    Sleep(100); // Can omit this when using the writing robot but has minimal effect
    // getch(); // Omit this once basic testing with emulator has taken place
}


// All Functions

// Function to allocate memory for the DataRow array
struct DataRow* allocateMemoryForDataArray(int numRows) 
{
    struct DataRow *dataArray = (struct DataRow *)malloc((size_t)numRows * sizeof(struct DataRow));
    if (dataArray == NULL) 
    {
        perror("Memory allocation failed");  //print error message
        CloseRS232Port(); // Close the port before exiting
        exit(EXIT_FAILURE);
    }
    return dataArray;
}

// Function to read the stroke data file and load font data into the array
void readFile(const char *filename, struct DataRow *dataArray, int numRows) 
{
    FILE *inputFile = fopen(filename, "r");  // Open the file for reading
    if (inputFile == NULL) 
    {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    // Read the rows of the file
    for (int i = 0; i < numRows; i++) 
    {
        fscanf(inputFile, "%d %d %d", &dataArray[i].col1, &dataArray[i].col2, &dataArray[i].col3);
    }

    fclose(inputFile);  // Close the file
}

// Function to prompt the user for a height value between 4 and 10
int getHeightFromUser() 
{
    int height;
    do 
    {
        printf("Enter a height value (between 4 and 10): ");
        scanf("%d", &height);

        if (height < 4 || height > 10) 
        {
            printf("Scale is out of limits. Please enter a value between 4 and 10.\n");
        }
    } 
    while (height < 4 || height > 10);

    return height;
}

// Function to open a text file and return a file pointer
FILE* openTextFile(const char *filename) 
{
    FILE *asciiFile = fopen(filename, "r");
    if (asciiFile == NULL)
    {
        perror("Error opening text file");
        exit(EXIT_FAILURE);
    }
    return asciiFile;
}

// Function to process words from the file
void processWordsFromFile(FILE *asciiFile, struct DataRow *dataArray, int numRows, int height, double *xOffset, double *yOffset) 
{
    char word[100];  // Buffer to store a word (assuming words are not longer than 100 characters)

    // Read and process words one by one
    while (readNextWordFromFile(asciiFile, word)) 
    {
        // Calculate the width of the word
        int wordWidth = calculateWordWidth(word, height);

        if (*xOffset + wordWidth > LINE_WIDTH) 
        {
            // Move to the next line
            *xOffset = 0.0;
            *yOffset -= ((36.0 / 18.0) * height); 
        }   

        // Process each word
        processWord(dataArray, numRows, word, height, xOffset, *yOffset);
    }
}

// Function to open a text file and read the next word (one at a time)
int readNextWordFromFile(FILE *asciiFile, char *word) 
{
    return fscanf(asciiFile, "%s", word) == 1 ? 1 : 0;
}

// Function to calculate the width of a word
int calculateWordWidth(const char *word, int height) 
{
    int width = 0;
    for (int i = 0; word[i] != '\0'; i++) 
    {
        if ((int)word[i] != 32) 
        {
            width += 18;
        }
    }
    return (width / 18) * height;
}

// Function to process a single word
void processWord(struct DataRow *dataArray, int numRows, char *word, int height, double *xOffset, double yOffset) 
{
    for (int i = 0; word[i] != '\0'; i++) 
    {
        int asciiValue = (int)word[i];  
        findAsciiData(dataArray, numRows, asciiValue, height, *xOffset, yOffset);
        *xOffset += (18.0 / 18.0) * height;
    }

    findAsciiData(dataArray, NUM_ROWS, 32, height, *xOffset, yOffset);
    *xOffset += (18.0 / 18.0) * height;
}

// Function to find data for a specific ASCII value and send it to the robot
void findAsciiData(struct DataRow *dataArray, int numRows, int asciiValue, int height, double xOffset, double yOffset) 
{
    
    for (int i = 0; i < numRows; i++) 
    {
        if (dataArray[i].col1 == ASCII_MARKER && dataArray[i].col2 == asciiValue) 
        {
            for (int j = i + 1; j < numRows; j++) 
            {
                if (dataArray[j].col1 == ASCII_MARKER) 
                {
                    break;
                }

                double adjustedCol1 = (dataArray[j].col1 / 18.0) * height + xOffset;
                double adjustedCol2 = (dataArray[j].col2 / 18.0) * height + yOffset;

                const char *sValue = (dataArray[j].col3 == 1) ? "S1000" : "S0";
                const char *gValue = (dataArray[j].col3 == 1) ? "G1" : "G0";

                // Send adjusted data to the robot
                char buffer[100];
                sprintf(buffer, "%s\n%s X%.1f Y%.1f\n", sValue, gValue, adjustedCol1, adjustedCol2);
                SendCommands(buffer);
            }
            break;
        }
    }
}
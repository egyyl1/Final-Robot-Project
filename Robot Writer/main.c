#include <stdio.h>
#include <stdlib.h>
//#include <conio.h>
//#include <windows.h>
#include "rs232.h"
#include "serial.h"

#define bdrate 115200               /* 115200 baud */
#define NUM_ROWS 1027               // The number of rows in the file
#define COLUMNS 3                   // The number of columns in each row

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
void readFile(const char *filename, struct DataRow *dataArray, int numRows);
void findAsciiData(struct DataRow *dataArray, int numRows, int asciiValue, int height, double xOffset, double yOffset);



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
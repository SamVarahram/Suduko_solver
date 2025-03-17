/*
All the standerd error handling and comments are added by Copilot.
*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#ifdef _OPENMP
    #include <omp.h>
    #define OPENMP_ENABLED 1
#else
    #define OPENMP_ENABLED 0
#endif

// Global variables for solution found
volatile int solution_found = 0;
unsigned char *solution_board = NULL;


// A structure to hold the coordinates of an empty cell.
typedef struct {
    unsigned char row;
    unsigned char col;
    unsigned char possibilities;
} Position;

// Function declarations.
void print_board(unsigned char *board, unsigned char side_length, unsigned char base);
int validate_finished_board(unsigned char* board, unsigned char side_length, unsigned char base);
int validate_input(unsigned char *board, unsigned char side_length, unsigned char base, unsigned char row, unsigned char col);
unsigned char *copy_board(unsigned char *board, unsigned char side_length);
int solve(unsigned char *board, Position *unAssignInd, unsigned short N_unAssign, unsigned char side_length, unsigned char base, unsigned short serial_threshold);
static double get_wall_seconds();


int main(int argc, char *argv[]) {
    double non_solving_time = get_wall_seconds();

    // Check if a filename is provided.
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <board_file>\n", argv[0]);
        return 1;
    }

    // Open the file in binary mode.
    FILE *file = fopen(argv[1], "rb");
    if (file == NULL) {
        fprintf(stderr, "Error opening file\n");
        return 1;
    }

    // Read the first two 1-byte integers: base and side length.
    unsigned char base;
    unsigned char side_length;
    if (fread(&base, sizeof(unsigned char), 1, file) != 1 ||
        fread(&side_length, sizeof(unsigned char), 1, file) != 1) {
        fprintf(stderr, "Error reading board parameters\n");
        fclose(file);
        return 1;
    }

    // Allocate memory for the 2D board (matrix).
    unsigned char *board = malloc(side_length * side_length * sizeof(unsigned char));
    if (board == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        fclose(file);
        return 1;
    }

    // Read the board data from the file into the matrix.
    if (fread(board, sizeof(unsigned char), side_length * side_length, file) != side_length * side_length) {
        fprintf(stderr, "Error reading board data\n");
        free(board);
        fclose(file);
        return 1;
    }
    fclose(file);

    // Count the number of empty cells.
    unsigned short N_unAssign = 0;
    for (unsigned char i = 0; i < side_length; i++) {
        for (unsigned char j = 0; j < side_length; j++) {
            if (board[i * side_length + j] == 0) {
                N_unAssign++;
            }
        }
    }

    // Set the threshold for switching to sequential backtracking.
    unsigned short serial_threshold;
    if (OPENMP_ENABLED) {
        serial_threshold = N_unAssign - N_unAssign / 100;
    } else {
        serial_threshold = N_unAssign;
    }
    
    printf("Serial threshold: %d\n", serial_threshold);

    // Create an array to store the positions of empty cells.
    Position *unAssignInd = malloc(N_unAssign * sizeof(Position));
    if (unAssignInd == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        free(board);
        return 1;
    }

    // Fill the array with the positions of empty cells.
    unsigned short count = 0;
    for (unsigned char i = 0; i < side_length; i++) {
        for (unsigned char j = 0; j < side_length; j++) {
            if (board[i * side_length + j] == 0) {
                unAssignInd[count].row = i;
                unAssignInd[count].col = j;
                count++;
            }
        }
    }

    // Sort the array of empty cells
    

    // Finish the timer for reading the file and preparing the board.
    non_solving_time = get_wall_seconds() - non_solving_time;

    double solving_time = get_wall_seconds();

    // Solve the Sudoku board using backtracking.
    // The solution is stored in the global variable solution_board.
    // The solution_found flag is also set to 1 if a solution is found.
    #pragma omp parallel
    {
        // Start the recursive backtracking function.
        #pragma omp single nowait
        {
            solve(board, unAssignInd, N_unAssign, side_length, base, serial_threshold);
        }
    }
    solving_time = get_wall_seconds() - solving_time;

    if (solution_found) {
        printf("Board is solved\n");
    } else {
        printf("No solution found\n");
    }

    // Validate the finished board.
    unsigned char result = validate_finished_board(solution_board, side_length, base);
    if (result == 1) {
        fprintf(stderr, "Invalid row sum\n");
    } else if (result == 2) {
        fprintf(stderr, "Invalid column sum\n");
    } else if (result == 3) {
        fprintf(stderr, "Invalid block sum\n");
    } else {
        printf("Board passed the last test!\n");
        printf("Non-solving time: %.6f seconds\n", non_solving_time);
        printf("Solving time: %.6f seconds\n", solving_time);
    }
    


    // Print the solved board.
    // printf("Board:\n");
    // print_board(solution_board, side_length, base);

    // Free the allocated memory.
    free(board);
    free(unAssignInd);

    if (solution_board) {
        free(solution_board);
    }
    
    return 0;
}


// Validates the board after placing a number at (row, col).
// Returns 1 if valid, 0 if there is a duplicate in the row, column, or sub-box.
int validate_input(unsigned char *board, unsigned char side_length, unsigned char base, unsigned char row, unsigned char col) {
    unsigned char num = board[row * side_length + col];

    // Check the row for duplicates.
    for (unsigned char j = 0; j < col; j++) {
        if (board[row * side_length + j] == num)
            return 0;
    }
    // Split the loop to avoid checking the same cell twice.
    for (unsigned char j = col + 1; j < side_length; j++) {
        if (board[row * side_length + j] == num)
            return 0;
    }
    
    // Check the column for duplicates.
    for (unsigned char j = 0; j < row; j++) {
        if (board[j * side_length + col] == num)
            return 0;
    }
    // Split the loop to avoid checking the same cell twice.
    for (unsigned char j = row + 1; j < side_length; j++) {
        if (board[j * side_length + col] == num)
            return 0;
    }

    // Check the sub-box.
    unsigned char startRow = row - row % base;
    unsigned char startCol = col - col % base;
    for (unsigned char i = startRow; i < row; i++) {
        for (unsigned char j = startCol; j < startCol + base; j++) {
            if (board[i * side_length + j] == num)
                return 0;
        }
    }
    for (unsigned char i = row + 1; i < startRow + base; i++) {
        for (unsigned char j = startCol; j < startCol + base; j++) {
            if (board[i * side_length + j] == num)
                return 0;
        }
    }
    return 1;
}

// Creates a deep copy of the Sudoku board.
unsigned char *copy_board(unsigned char *board, unsigned char side_length) {
    unsigned char *new_board = malloc(side_length * side_length * sizeof(unsigned char));
    memcpy(new_board, board, side_length * side_length * sizeof(unsigned char));
    return new_board;
}


// Recursively solves the Sudoku board using backtracking.
// unAssignInd is an array of empty cell positions, and N_unAssign is the number of such cells left.
int solve(unsigned char *board, Position *unAssignInd, unsigned short N_unAssign, unsigned char side_length, unsigned char base, unsigned short serial_threshold) {
    
    // Check if a solution has been found by another thread.
    if (solution_found) {
        return 1;
    }
    
    // Base case: no unassigned positions remain so the board is solved.
    if (N_unAssign == 0) {
        // Store the solution in the global variable.
        #pragma omp critical
        {
            solution_board = copy_board(board, side_length);
            solution_found = 1;
        }
        return 1;
    }
    
    // Get the next empty cell from the unassigned list.
    Position pos = unAssignInd[N_unAssign - 1];
    unsigned char row = pos.row;
    unsigned char col = pos.col;

    if (N_unAssign > serial_threshold) {
        
    int local_solution_found = 0;
    
    // Try every possible value from 1 to side_length.
    for (unsigned char val = 1; val <= side_length; val++) {
        #pragma omp task firstprivate(val) shared(solution_found, local_solution_found)
        {
            // Check if a solution has been found by another thread.
            if (!solution_found) {
                // Create a deep copy of the board.
                unsigned char *new_board = copy_board(board, side_length);
                new_board[row * side_length + col] = val; // Set the guess.
                // Check if the guess is valid.
                if (validate_input(new_board, side_length, base, row, col)) {
                    // Recursively try to solve with one fewer unassigned cell.
                    if (solve(new_board, unAssignInd, N_unAssign - 1, side_length, base, serial_threshold)) {
                        #pragma omp critical
                        {
                            local_solution_found = 1;
                        }
                    }
                }
                // Free the memory for the new board.
                free(new_board);
            }
        }
    }
    // Wait for all tasks to finish before returning.
    #pragma omp taskwait
    return local_solution_found;
}
else {
    // Sequential in-place backtracking: modify board directly and restore afterward.
    for (unsigned char val = 1; val <= side_length; val++) {
        board[row * side_length + col] = val;
        if (validate_input(board, side_length, base, row, col)) {
            if (solve(board, unAssignInd, N_unAssign - 1, side_length, base, serial_threshold))
                return 1;
        }
    }
    board[row * side_length + col] = 0; // Reset the cell.
    return 0;
}
}


int validate_finished_board(unsigned char* board, unsigned char side_length, unsigned char base) {
    // Calculate the target sum for each row, column, and block.
    int target_sum = side_length * (side_length + 1) / 2;
    int row_sum, col_sum, block_sum;
    for (int i = 0; i < side_length; i++) {
        row_sum = 0;
        col_sum = 0;
        for (int j = 0; j < side_length; j++) {
            row_sum += board[i * side_length + j];
            col_sum += board[j * side_length + i];
        }
        if (row_sum != target_sum) return 1;
        if (col_sum != target_sum) return 2;
    }
    for (int i = 0; i < side_length; i += base) {
        for (int j = 0; j < side_length; j += base) {
            block_sum = 0;
            for (int k = i; k < i + base; k++) {
                for (int l = j; l < j + base; l++) {
                    block_sum += board[k * side_length + l];
                }
            }
            if (block_sum != target_sum) return 3;
        }
    }
    return 0;
}


// Taken from Lab6 Task 02!
static double get_wall_seconds() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    double seconds = tv.tv_sec + (double)tv.tv_usec / 1000000;
    return seconds;
  }

// _______________________________________________________________________________________
// _______________________________________________________________________________________

// Functions to print the suduko board was used to understand the board structure
// and to debug the program.
// The functions are not used in the final program.
// These are created by Chatgpt 03-mini-high

// Helper function to print a horizontal divider line.
static void print_horizontal_divider(int base, int mini_width) {
    printf("+");
    for (int b = 0; b < base; b++) {
        for (int k = 0; k < mini_width; k++) {
            printf("-");
        }
        printf("+");
    }
    printf("\n");
}

// Print the Sudoku board stored in a 2D matrix.
void print_board(unsigned char *board, unsigned char side_length, unsigned char base) {
    int cellWidth = 1;
    int max_val = side_length;
    while (max_val >= 10) {
        cellWidth++;
        max_val /= 10;
    }
    int mini_width = (base - 1) * (cellWidth + 1) + cellWidth;
    for (int i = 0; i < side_length; i++) {
        if (i % base == 0)
            print_horizontal_divider(base, mini_width);
        printf("|");
        for (int j = 0; j < side_length; j++) {
            printf("%*d", cellWidth, board[i * side_length + j]);
            if ((j + 1) % base == 0)
                printf("|");
            else
                printf(" ");
        }
        printf("\n");
    }
    print_horizontal_divider(base, mini_width);
}

/*
All the standerd error handling and comments are added by Copilot.
*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#ifdef _OPENMP
    #include <omp.h>
    #define OPENMP_ENABLED 1
#else
    #define OPENMP_ENABLED 0
#endif

// Global variables for solution found
volatile int solution_found = 0;
unsigned char **solution_board = NULL;
// Global bit arrays for validation
uint64_t *row_bits = NULL;
uint64_t *col_bits = NULL;
uint64_t *box_bits = NULL;

// A structure to hold the coordinates of an empty cell.
typedef struct {
    unsigned char row;
    unsigned char col;
} Position;

// Function declarations.
void print_board(unsigned char **board, unsigned char side_length, unsigned char base);
int validate_finished_board(unsigned char** board, unsigned char side_length, unsigned char base);
int validate_input(unsigned char **board, unsigned char side_length, unsigned char base, unsigned char row, unsigned char col);
unsigned char **copy_board(unsigned char **board, unsigned char side_length);
int solve(unsigned char **board, Position *unAssignInd, unsigned short N_unAssign, unsigned char side_length, unsigned char base, unsigned short serial_threshold, uint64_t *row_bits, uint64_t *col_bits, uint64_t *box_bits);
void update_bits(unsigned char row, unsigned char col, unsigned char val, unsigned char base, uint64_t *row_bits, uint64_t *col_bits, uint64_t *box_bits, int set_or_clear);
int validate_input_bits(unsigned char **board, unsigned char side_length, unsigned char base, unsigned char row, unsigned char col, uint64_t *row_bits, uint64_t *col_bits, uint64_t *box_bits);
void init_bit_arrays(unsigned char **board, unsigned char side_length, unsigned char base);
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
    unsigned char **board = malloc(side_length * sizeof(unsigned char *));
    if (board == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        fclose(file);
        return 1;
    }
    for (unsigned char i = 0; i < side_length; i++) {
        board[i] = malloc(side_length * sizeof(unsigned char));
        if (board[i] == NULL) {
            fprintf(stderr, "Memory allocation failed\n");
            // Free already allocated rows before exiting.
            for (int j = 0; j < i; j++) {
                free(board[j]);
            }
            free(board);
            fclose(file);
            return 1;
        }
    }

    // Read the board data from the file into the matrix.
    for (unsigned char i = 0; i < side_length; i++) {
        if (fread(board[i], sizeof(unsigned char), side_length, file) != side_length) {
            fprintf(stderr, "Error reading board data\n");
            // Free allocated memory on error.
            for (unsigned char j = 0; j < side_length; j++) {
                free(board[j]);
            }
            free(board);
            fclose(file);
            return 1;
        }
    }
    fclose(file);
    init_bit_arrays(board, side_length, base);

    // Count the number of empty cells.
    unsigned short N_unAssign = 0;
    for (unsigned char i = 0; i < side_length; i++) {
        for (unsigned char j = 0; j < side_length; j++) {
            if (board[i][j] == 0) {
                N_unAssign++;
            }
        }
    }

    // Set the threshold for switching to sequential backtracking.
    unsigned short serial_threshold;
    if (OPENMP_ENABLED) {
        serial_threshold = N_unAssign - N_unAssign / 10;
    } else {
        serial_threshold = N_unAssign+1;
    }
    
    printf("Serial threshold: %d\n", serial_threshold);

    // Create an array to store the positions of empty cells.
    Position *unAssignInd = malloc(N_unAssign * sizeof(Position));
    if (unAssignInd == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        // Free the allocated board memory before exiting.
        for (unsigned char i = 0; i < side_length; i++) {
            free(board[i]);
        }
        free(board);
        return 1;
    }

    // Fill the array with the positions of empty cells.
    unsigned short count = 0;
    for (unsigned char i = 0; i < side_length; i++) {
        for (unsigned char j = 0; j < side_length; j++) {
            if (board[i][j] == 0) {
                unAssignInd[count].row = i;
                unAssignInd[count].col = j;
                count++;
            }
        }
    }

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
            solve(board, unAssignInd, N_unAssign, side_length, base, serial_threshold, row_bits, col_bits, box_bits);
        }
    }
    solving_time = get_wall_seconds() - solving_time;

    if (solution_found) {
        printf("Board is solved\n");
    } else {
        printf("No solution found\n");
    }


    // Validate the finished board.
    if (solution_found && solution_board) {
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

    } else {
        printf("No solution to validate\n");
    }

    // Print the solved board.
    // printf("Board:\n");
    // print_board(soluction_board, side_length, base);

    // Free the allocated memory.
    for (unsigned char i = 0; i < side_length; i++) {
        free(board[i]);
    }
    free(board);
    free(unAssignInd);
    free(row_bits);
    free(col_bits);
    free(box_bits);

    if (solution_board) {
        for (unsigned char i = 0; i < side_length; i++) {
            free(solution_board[i]);
        }
        free(solution_board);
    }
    
    return 0;
}

// Initialize bit arrays for fast validation
void init_bit_arrays(unsigned char **board, unsigned char side_length, unsigned char base) {
    // Allocate memory for bit arrays
    row_bits = calloc(side_length, sizeof(uint64_t));
    col_bits = calloc(side_length, sizeof(uint64_t));
    box_bits = calloc(side_length, sizeof(uint64_t));
    
    if (!row_bits || !col_bits || !box_bits) {
        fprintf(stderr, "Memory allocation failed for bit arrays\n");
        return;
    }

    // Initialize with board values
    for (unsigned char i = 0; i < side_length; i++) {
        for (unsigned char j = 0; j < side_length; j++) {
            unsigned char val = board[i][j];
            if (val > 0) {
                // Calculate box index
                unsigned char box_idx = (i / base) * base + (j / base);
                
                // Set bits
                row_bits[i] |= (1ULL << (val - 1));
                col_bits[j] |= (1ULL << (val - 1));
                box_bits[box_idx] |= (1ULL << (val - 1));
            }
        }
    }
}

// Validates the board after placing a number at (row, col).
// Returns 1 if valid, 0 if there is a duplicate in the row, column, or sub-box.
// Optimized validation using bit arrays
int is_valid_input(unsigned char row, unsigned char col, unsigned char val, unsigned char base,
    uint64_t *row_bits, uint64_t *col_bits, uint64_t *box_bits) {
    uint64_t mask = 1ULL << (val - 1);
    unsigned char box_idx = (row / base) * base + (col / base);
    if (row_bits[row] & mask) return 0;
    if (col_bits[col] & mask) return 0;
    if (box_bits[box_idx] & mask) return 0;
    return 1;
}


// Update bit arrays when setting or unsetting a value
void update_bits(unsigned char row, unsigned char col, unsigned char val, unsigned char base,
    uint64_t *row_bits, uint64_t *col_bits, uint64_t *box_bits, int set_or_clear) {

    if (val == 0) return; // No update needed for empty cells

    // Calculate box index
    unsigned char box_idx = (row / base) * base + (col / base);

    // Create mask for this value
    uint64_t mask = 1ULL << (val - 1);

    if (set_or_clear) {
    // Set the bits
    row_bits[row] |= mask;
    col_bits[col] |= mask;
    box_bits[box_idx] |= mask;
    } else {
    // Clear the bits
    row_bits[row] &= ~mask;
    col_bits[col] &= ~mask;
    box_bits[box_idx] &= ~mask;
    }
}

// Creates a deep copy of the Sudoku board.
unsigned char **copy_board(unsigned char **board, unsigned char side_length) {
    unsigned char **new_board = malloc(side_length * sizeof(unsigned char *));
    for (int i = 0; i < side_length; i++) {
        new_board[i] = malloc(side_length * sizeof(unsigned char));
        memcpy(new_board[i], board[i], side_length * sizeof(unsigned char));
    }
    return new_board;
}

// Recursively solves the Sudoku board using backtracking.
// unAssignInd is an array of empty cell positions, and N_unAssign is the number of such cells left.
int solve(unsigned char **board, Position *unAssignInd, unsigned short N_unAssign, unsigned char side_length, unsigned char base, unsigned short serial_threshold, uint64_t *row_bits, uint64_t *col_bits, uint64_t *box_bits) {
    
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
            unsigned char **new_board = copy_board(board, side_length);
            // Create local bit arrays for validation
            uint64_t *local_row_bits = malloc(side_length * sizeof(uint64_t));
            uint64_t *local_col_bits = malloc(side_length * sizeof(uint64_t));
            uint64_t *local_box_bits = malloc(side_length * sizeof(uint64_t));
            
            // Copy current bit state
            memcpy(local_row_bits, row_bits, side_length * sizeof(uint64_t));
            memcpy(local_col_bits, col_bits, side_length * sizeof(uint64_t));
            memcpy(local_box_bits, box_bits, side_length * sizeof(uint64_t));
            

            // Check if the guess is valid.
            if (is_valid_input(row, col, val, base, local_row_bits, local_col_bits, local_box_bits)) {
                
                new_board[row][col] = val; // Set the guess.
                // Update bit arrays
                update_bits(row, col, val, base, local_row_bits, local_col_bits, local_box_bits, 1);
                // Recursively try to solve with one fewer unassigned cell.
                if (solve(new_board, unAssignInd, N_unAssign - 1, side_length, base, serial_threshold, local_row_bits, local_col_bits, local_box_bits)) {
                    #pragma omp critical
                    {
                    local_solution_found = 1;
                    }}
        }
        // Free local bit arrays
        free(local_row_bits);
        free(local_col_bits);
        free(local_box_bits);

        // Free the memory for the new board.
        for (int i = 0; i < side_length; i++) {
            free(new_board[i]);
        }
        free(new_board);
        }}
    }
    // Wait for all tasks to finish before returning.
    #pragma omp taskwait
    return local_solution_found;
}
else {
    // Sequential in-place backtracking: modify board directly and restore afterward.
    for (unsigned char val = 1; val <= side_length; val++) {

        // Check if valid
        if (is_valid_input(row, col, val, base, row_bits, col_bits, box_bits)) {
            board[row][col] = val; // Set the guess.
            // Update bit arrays
            update_bits(row, col, val, base, row_bits, col_bits, box_bits, 1);

            if (solve(board, unAssignInd, N_unAssign - 1, side_length, base, serial_threshold, row_bits, col_bits, box_bits)) {
                return 1;
        }
        
        // Undo the bit update when backtracking
        update_bits(row, col, val, base, row_bits, col_bits, box_bits, 0);
        
        board[row][col] = 0; // Reset the cell
    }}
    return 0;
}
}


int validate_finished_board(unsigned char** board, unsigned char side_length, unsigned char base) {
    // Calculate the target sum for each row, column, and block.
    int64_t target_sum = side_length * (side_length + 1) / 2;
    int64_t row_sum, col_sum, block_sum;

    // Check the sum of each row and column.
    for (int i = 0; i < side_length; i++) {
        row_sum = 0;
        col_sum = 0;
        // Using matrix geometry to calculate the sum of each row and column.
        for (int j = 0; j < side_length; j++) {
            row_sum += board[i][j];
            col_sum += board[j][i];
        }
        if (row_sum != target_sum) return 1;
        if (col_sum != target_sum) return 2;
    }

    // Check the sum of each block.
    // Looping over each block column wise
    for (int i = 0; i < side_length; i += base) {
        // Here we loop row wise
        for (int j = 0; j < side_length; j += base) {
            block_sum = 0;
            // Looping inside each block
            for (int k = i; k < i + base; k++) {
                for (int l = j; l < j + base; l++) {
                    block_sum += board[k][l];
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
void print_board(unsigned char **board, unsigned char side_length, unsigned char base) {
    // Determine the fixed field width for each cell.
    int cellWidth = 1;
    int max_val = side_length;
    while (max_val >= 10) {
        cellWidth++;
        max_val /= 10;
    }
    
    // Calculate the width of one mini board section.
    int mini_width = (base - 1) * (cellWidth + 1) + cellWidth;
    
    // Print each row.
    for (int i = 0; i < side_length; i++) {
        // Print horizontal divider at the beginning of each mini block row.
        if (i % base == 0) {
            print_horizontal_divider(base, mini_width);
        }
        
        // Start the row with a vertical divider.
        printf("|");
        for (int j = 0; j < side_length; j++) {
            // Print the cell value in a fixed-width field.
            printf("%*d", cellWidth, board[i][j]);
            
            // Print a vertical divider after each mini block.
            if ((j + 1) % base == 0) {
                printf("|");
            } else {
                printf(" ");
            }
        }
        printf("\n");
    }
    // Print the final horizontal divider.
    print_horizontal_divider(base, mini_width);
}
/*
All the standerd error handling and comments are added by Copilot.
*/
#include <stdio.h>
#include <stdlib.h>

// Function declarations.
void print_board(unsigned char **board, unsigned char side_length, unsigned char base);
int validate_board(unsigned char** board, unsigned char side_length, unsigned char base);


int main(int argc, char *argv[]) {
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
    for (int i = 0; i < side_length; i++) {
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
    for (int i = 0; i < side_length; i++) {
        if (fread(board[i], sizeof(unsigned char), side_length, file) != side_length) {
            fprintf(stderr, "Error reading board data\n");
            // Free allocated memory on error.
            for (int j = 0; j < side_length; j++) {
                free(board[j]);
            }
            free(board);
            fclose(file);
            return 1;
        }
    }
    fclose(file);


    
    // Validate the board.
    /*
    int result = validate_board(board, side_length, base);
    if (result == 1) {
        fprintf(stderr, "Invalid row sum\n");
    } else if (result == 2) {
        fprintf(stderr, "Invalid column sum\n");
    } else if (result == 3) {
        fprintf(stderr, "Invalid block sum\n");
    } else {
        printf("Board is valid\n");
    }
    */


    // Print board information and the Sudoku board.
    printf("Board Base: %d, Side Length: %d\n", base, side_length);
    printf("Board:\n");
    print_board(board, side_length, base);

    // Free the allocated memory.
    for (int i = 0; i < side_length; i++) {
        free(board[i]);
    }
    free(board);
    
    return 0;
}


int validate_board(unsigned char** board, unsigned char side_length, unsigned char base) {
    // Calculate the target sum for each row, column, and block.
    int target_sum = side_length * (side_length + 1) / 2;
    int row_sum, col_sum, block_sum;

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
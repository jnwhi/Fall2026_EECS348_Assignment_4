/*
 * ============================================================================
 * Program Name : EECS 348 Assignment 4 - Recursive Object-Oriented Sudoku Solver
 * File         : Assignment4.cpp
 * Description  : Reads five Sudoku puzzles from text files, prints each puzzle,
 *                and uses a recursive depth-first search with backtracking to
 *                find and print EVERY solution to each puzzle. If a puzzle has
 *                no solution, "No solution found" is printed instead.
 * Inputs       : puzzle1.txt, puzzle2.txt, puzzle3.txt, puzzle4.txt, puzzle5.txt
 *                (each file is a 9x9 grid of digits 1-9 separated by spaces,
 *                with an underscore '_' marking a blank cell). The files must be
 *                in the same directory the program is run from.
 * Output       : For each file: the puzzle file name, the original puzzle, and
 *                every solution followed by "Solutions found: N", or
 *                "No solution found" when the puzzle cannot be solved.
 * Collaborators: None (no other people).
 * Other sources: - Claude Opus (Anthropic) generative AI, accessed through the
 *                  Google Antigravity IDE agent chat (https://antigravity.google).
 *                  Its generated program was selected as the base design
 *                  (SudokuBoard / SudokuSolver / PuzzleRunner classes).
 *                - Gemini (Google) generative AI, accessed through the Google
 *                  Antigravity IDE (https://antigravity.google). Its program was
 *                  used for comparison; its idea of a "used digit" lookup table
 *                  inspired the rowUsed/colUsed/boxUsed tables below.
 *                - The comparison of both programs is in Assignment4GenAI.pdf.
 * Author       : Justin Whitaker
 * Creation Date: 10/15/2026
 * Build        : g++ -std=c++11 -Wall -o Assignment4.exe Assignment4.cpp
 * Run          : ./Assignment4.exe
 * ============================================================================
 */

#include <iostream>   // std::cout and std::ostream for printing
#include <fstream>    // std::ifstream for reading the puzzle files
#include <string>     // std::string for file names and tokens
#include <vector>     // std::vector to hold the list of puzzle file names

// ============================================================================
// Class SudokuBoard
// Stores one 9x9 Sudoku grid and answers "can this digit go here?" questions.
// Source: Class layout from Claude's generated code; the O(1) lookup tables,
//         stricter file validation, and conflicting-clue check were added by
//         Justin Whitaker (lookup-table idea inspired by Gemini's code).
// ============================================================================
class SudokuBoard {
public:
    static const int SIZE = 9;            // the grid is 9 rows by 9 columns
    static const int BOX = 3;             // each block is 3 rows by 3 columns
    static const int EMPTY = 0;           // 0 is stored for a blank ('_') cell
    static const int CELL_COUNT = SIZE * SIZE; // 81 cells in the whole grid

    // Constructor: start with a completely empty board.
    SudokuBoard() {
        clearBoard();                     // set every cell and table to empty
    }

    // ------------------------------------------------------------------------
    // loadFromFile: reads exactly 81 tokens ('_' or a digit 1-9) from a file.
    // Returns false (and fills errorMessage) if the file is missing or badly
    // formatted. A clue that breaks a Sudoku rule is still loaded so the puzzle
    // can be printed, but it is remembered in givensValid so the solver knows
    // the puzzle has no solution.
    // Source: Claude (base), validation rules added by Justin Whitaker.
    // ------------------------------------------------------------------------
    bool loadFromFile(const std::string& filename, std::string& errorMessage) {
        clearBoard();                                 // forget any previous puzzle
        std::ifstream inFile(filename.c_str());       // open the puzzle file
        if (!inFile) {                                // the file could not be opened
            errorMessage = "Could not open " + filename; // explain the problem
            return false;                             // tell the caller loading failed
        }

        std::string token;                            // holds one '_' or digit at a time
        int cellIndex = 0;                            // which of the 81 cells we are filling
        while (cellIndex < CELL_COUNT && inFile >> token) { // read until 81 tokens or end of file
            int row = cellIndex / SIZE;               // convert the cell index to a row
            int col = cellIndex % SIZE;               // convert the cell index to a column
            if (token == "_") {                       // underscore means a blank cell
                cells[row][col] = EMPTY;              // store the blank as 0
            } else if (token.size() == 1 && token[0] >= '1' && token[0] <= '9') { // a single digit 1-9
                int digit = token[0] - '0';           // convert the character to its number
                if (canPlace(row, col, digit)) {      // the clue agrees with the clues so far
                    place(row, col, digit);           // store it and update the lookup tables
                } else {                              // the clue repeats a digit in a row/column/block
                    cells[row][col] = digit;          // keep it so the puzzle prints as given
                    givensValid = false;              // remember that this puzzle is unsolvable
                }
            } else {                                  // anything else is not allowed
                errorMessage = "Invalid entry '" + token + "' in " + filename; // describe the bad token
                return false;                         // stop loading this file
            }
            cellIndex++;                              // move on to the next cell
        }

        if (cellIndex < CELL_COUNT) {                 // the file ran out before 81 cells
            errorMessage = filename + " has fewer than 81 cells"; // describe the problem
            return false;                             // loading failed
        }
        if (inFile >> token) {                        // there is still data after 81 cells
            errorMessage = filename + " has more than 81 cells";  // describe the problem
            return false;                             // loading failed
        }
        return true;                                  // the whole grid was read successfully
    }

    // Returns true when every clue in the file obeyed the Sudoku rules.
    bool hasValidGivens() const {
        return givensValid;                           // set to false by loadFromFile on a conflict
    }

    // Returns the digit stored at (row, col); 0 means the cell is blank.
    int get(int row, int col) const {
        return cells[row][col];                       // read straight from the grid
    }

    // ------------------------------------------------------------------------
    // canPlace: true if digit is not already used in this row, column, or
    // block. Uses the three lookup tables, so it costs O(1) instead of
    // scanning 27 cells. Source: Justin Whitaker (replaces Claude's scan).
    // ------------------------------------------------------------------------
    bool canPlace(int row, int col, int digit) const {
        return !rowUsed[row][digit]                   // digit is not in this row
            && !colUsed[col][digit]                   // digit is not in this column
            && !boxUsed[boxIndex(row, col)][digit];   // digit is not in this 3x3 block
    }

    // place: writes a digit into a cell and marks it as used in the tables.
    // Source: Justin Whitaker.
    void place(int row, int col, int digit) {
        cells[row][col] = digit;                      // put the digit in the grid
        rowUsed[row][digit] = true;                   // the row now contains this digit
        colUsed[col][digit] = true;                   // the column now contains this digit
        boxUsed[boxIndex(row, col)][digit] = true;    // the block now contains this digit
    }

    // removeDigit: erases a cell (used when backtracking) and un-marks the tables.
    // Source: Justin Whitaker.
    void removeDigit(int row, int col) {
        int digit = cells[row][col];                  // remember which digit is being removed
        cells[row][col] = EMPTY;                      // blank the cell again
        rowUsed[row][digit] = false;                  // the row no longer has this digit
        colUsed[col][digit] = false;                  // the column no longer has this digit
        boxUsed[boxIndex(row, col)][digit] = false;   // the block no longer has this digit
    }

    // ------------------------------------------------------------------------
    // getCandidates: fills the caller's array with every digit that could
    // legally go in (row, col) and returns how many there are (0 to 9).
    // A fixed-size array is used instead of a std::vector so no memory is
    // allocated on each recursive call.
    // Source: Claude (idea), changed to a fixed array by Justin Whitaker.
    // ------------------------------------------------------------------------
    int getCandidates(int row, int col, int candidates[SIZE]) const {
        int count = 0;                                // no candidates found yet
        for (int digit = 1; digit <= SIZE; digit++) { // try each digit 1 through 9
            if (canPlace(row, col, digit)) {          // the digit breaks no rule here
                candidates[count] = digit;            // add it to the candidate list
                count++;                              // one more candidate found
            }
        }
        return count;                                 // tell the caller how many were found
    }

    // print: shows the grid in the same format as the puzzle files.
    // Source: Claude (unchanged except for comments).
    void print(std::ostream& out) const {
        for (int row = 0; row < SIZE; row++) {        // go through each row
            for (int col = 0; col < SIZE; col++) {    // go through each column in the row
                if (cells[row][col] == EMPTY) {       // a blank cell
                    out << '_';                       // print an underscore like the input file
                } else {                              // a filled cell
                    out << cells[row][col];           // print its digit
                }
                out << ' ';                           // separate cells with a space
            }
            out << '\n';                              // end of this row
        }
    }

private:
    int cells[SIZE][SIZE];                // the digits in the grid (0 = blank)
    bool rowUsed[SIZE][SIZE + 1];         // rowUsed[r][d] is true if digit d is in row r
    bool colUsed[SIZE][SIZE + 1];         // colUsed[c][d] is true if digit d is in column c
    bool boxUsed[SIZE][SIZE + 1];         // boxUsed[b][d] is true if digit d is in block b
    bool givensValid;                     // false if the file's clues break a Sudoku rule

    // boxIndex: numbers the nine 3x3 blocks 0-8, left to right, top to bottom.
    // Source: Justin Whitaker.
    static int boxIndex(int row, int col) {
        return (row / BOX) * BOX + (col / BOX);       // block row * 3 + block column
    }

    // clearBoard: resets the grid and all lookup tables to empty.
    // Source: Justin Whitaker.
    void clearBoard() {
        for (int i = 0; i < SIZE; i++) {              // for each row / column / block number
            for (int j = 0; j < SIZE; j++) {          // for each column of the grid
                cells[i][j] = EMPTY;                  // blank the grid cell
            }
            for (int d = 0; d <= SIZE; d++) {         // for each digit slot 0-9
                rowUsed[i][d] = false;                // no digit used in row i yet
                colUsed[i][d] = false;                // no digit used in column i yet
                boxUsed[i][d] = false;                // no digit used in block i yet
            }
        }
        givensValid = true;                           // assume the clues are fine until proven otherwise
    }
};

// ============================================================================
// Class SudokuSolver
// Recursive depth-first search with backtracking that finds ALL solutions.
// Each solution is printed the moment it is found instead of being stored,
// so memory use does not grow with the number of solutions.
// Source: Recursive structure from Claude's generated code; resuming the
//         search from the next cell, printing instead of storing solutions,
//         and the conflicting-clue check were added by Justin Whitaker.
// ============================================================================
class SudokuSolver {
public:
    // Constructor: copy the starting board and remember where to print solutions.
    SudokuSolver(const SudokuBoard& startBoard, std::ostream& output)
        : board(startBoard),                  // work on a private copy of the puzzle
          out(output),                        // stream that solutions are printed to
          solutionCount(0) {                  // no solutions found yet
    }

    // solveAll: prints every solution and returns how many were found.
    int solveAll() {
        solutionCount = 0;                    // reset the counter for this puzzle
        if (!board.hasValidGivens()) {        // the clues already break a rule
            return 0;                         // so there can be no solution
        }
        search(0);                            // start the recursive search at cell 0
        return solutionCount;                 // report the total number of solutions
    }

private:
    SudokuBoard board;                        // the grid being filled in during the search
    std::ostream& out;                        // where solutions are printed
    int solutionCount;                        // how many solutions have been found so far

    // ------------------------------------------------------------------------
    // search: the recursive step. cellIndex (0-80) is where to start looking
    // for the next empty cell; every cell before it is already filled, so it
    // never re-scans the grid from the top.
    //   - If no empty cell remains, all 81 cells are filled: print a solution.
    //   - Otherwise list the candidates for the empty cell. If there are none,
    //     an earlier guess was wrong, so return (backtrack).
    //   - Otherwise try each candidate, recurse on the next cell, then undo it
    //     so the next candidate can be tried (this is what finds ALL solutions).
    // ------------------------------------------------------------------------
    void search(int cellIndex) {
        // Skip over cells that already contain a digit.
        while (cellIndex < SudokuBoard::CELL_COUNT &&                       // still inside the grid
               board.get(cellIndex / SudokuBoard::SIZE,                     // row of this cell
                         cellIndex % SudokuBoard::SIZE) != SudokuBoard::EMPTY) { // column; cell is filled
            cellIndex++;                                                    // move to the next cell
        }

        if (cellIndex == SudokuBoard::CELL_COUNT) {   // no empty cells left (k == 81)
            solutionCount++;                          // count this solution
            out << "Solution " << solutionCount << ":\n"; // label the solution
            board.print(out);                         // print the completed grid
            out << '\n';                              // blank line after the grid
            return;                                   // keep searching for other solutions
        }

        int row = cellIndex / SudokuBoard::SIZE;      // row of the empty cell
        int col = cellIndex % SudokuBoard::SIZE;      // column of the empty cell

        int candidates[SudokuBoard::SIZE];            // room for up to 9 candidate digits
        int candidateCount = board.getCandidates(row, col, candidates); // list legal digits
        if (candidateCount == 0) {                    // nothing can go here
            return;                                   // dead end: backtrack
        }

        for (int i = 0; i < candidateCount; i++) {    // try every candidate in turn
            board.place(row, col, candidates[i]);     // guess this digit
            search(cellIndex + 1);                    // recursively fill the rest of the grid
            board.removeDigit(row, col);              // undo the guess before trying the next one
        }
    }
};

// ============================================================================
// Class PuzzleRunner
// Drives the program: for each file, print its name and puzzle, solve it,
// and print the result.
// Source: Claude's generated code; error-message handling and streaming the
//         solutions through SudokuSolver were changed by Justin Whitaker.
// ============================================================================
class PuzzleRunner {
public:
    // Constructor: remember the list of puzzle files to process.
    explicit PuzzleRunner(const std::vector<std::string>& files)
        : fileNames(files) {                          // copy the file names
    }

    // run: process every puzzle file in order.
    void run() const {
        for (size_t i = 0; i < fileNames.size(); i++) { // go through each file name
            processFile(fileNames[i]);                // load, print, and solve that puzzle
        }
    }

private:
    std::vector<std::string> fileNames;               // names of the puzzle files

    // processFile: handles one puzzle file from start to finish.
    void processFile(const std::string& fileName) const {
        std::cout << "==============================\n"; // separator line
        std::cout << "Puzzle file: " << fileName << "\n"; // print the puzzle file name
        std::cout << "==============================\n"; // separator line

        SudokuBoard board;                            // empty board to load into
        std::string errorMessage;                     // filled in if loading fails
        if (!board.loadFromFile(fileName, errorMessage)) { // try to read the file
            std::cout << "Error: " << errorMessage << "\n\n"; // explain what went wrong
            return;                                   // skip to the next file
        }

        std::cout << "Puzzle:\n";                     // heading for the original puzzle
        board.print(std::cout);                       // print the puzzle as read from the file
        std::cout << '\n';                            // blank line after the puzzle

        SudokuSolver solver(board, std::cout);        // solver prints solutions to the screen
        int solutionCount = solver.solveAll();        // find and print every solution

        if (solutionCount == 0) {                     // the search found nothing
            std::cout << "No solution found\n\n";     // required message for unsolvable puzzles
        } else {                                      // at least one solution was printed
            std::cout << "Solutions found: " << solutionCount << "\n\n"; // print the total
        }
    }
};

// ============================================================================
// main: builds the list of the five puzzle files and runs the solver on them.
// Source: Claude (rewritten without an initializer list so it also builds
//         on older compilers) by Justin Whitaker.
// ============================================================================
int main() {
    std::vector<std::string> files;                   // list of puzzle files to solve
    files.push_back("puzzle1.txt");                   // first puzzle
    files.push_back("puzzle2.txt");                   // second puzzle
    files.push_back("puzzle3.txt");                   // third puzzle
    files.push_back("puzzle4.txt");                   // fourth puzzle
    files.push_back("puzzle5.txt");                   // fifth puzzle

    PuzzleRunner runner(files);                       // create the program driver
    runner.run();                                     // solve and print all five puzzles
    return 0;                                         // report successful completion
}

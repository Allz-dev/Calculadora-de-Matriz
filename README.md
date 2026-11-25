# Matrix Operations in C

An interactive C program for entering two or more matrices and applying
mathematical operations to them in the order entered.

## Files

- `main.c` - program entry point.
- `funcoes.c` - input, matrix operations, and result display.
- `funcoes.h` - declaration shared by the C files.

## Requirements

- A C compiler with C11 support.
- GCC users on Windows can compile from PowerShell in the project folder:

```powershell
gcc main.c funcoes.c -o matrizes.exe -lm
```

Run the program with:

```powershell
.\matrizes.exe
```

Compile both `main.c` and `funcoes.c`; `funcoes.h` is included by the source.
With another compiler, add both `.c` files to the same project.

## Using the program

1. Enter the number of matrices (at least two).
2. For each matrix, enter its row and column counts, then each element.
3. Choose an operation from the menu. The operation is applied from left to
   right across the entered matrices.
4. Choose `0` to exit.

## Operations

- **Addition and subtraction:** each matrix must have the same dimensions as
  the current result.
- **Multiplication:** the current result's column count must equal the next
  matrix's row count.
- **Matrix division:** defined as `A / B = A * B^-1`. Each divisor must be
  square and invertible, and its dimensions must allow multiplication.

The program reports when dimensions or mathematical conditions prevent an
operation.

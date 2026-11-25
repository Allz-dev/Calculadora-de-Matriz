#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "funcoes.h"

typedef struct {
    size_t rows;
    size_t cols;
    double *values;
} Matrix;

typedef enum {
    OPERATION_OK,
    OPERATION_DIMENSIONS,
    OPERATION_NOT_SQUARE,
    OPERATION_SINGULAR,
    OPERATION_MEMORY,
    OPERATION_OVERFLOW
} OperationStatus;

static double *element(Matrix *matrix, size_t row, size_t col)
{
    return &matrix->values[row * matrix->cols + col];
}

static const double *const_element(const Matrix *matrix, size_t row, size_t col)
{
    return &matrix->values[row * matrix->cols + col];
}

static int matrix_create(Matrix *matrix, size_t rows, size_t cols)
{
    if (cols != 0 && rows > SIZE_MAX / cols) {
        return 0;
    }

    size_t count = rows * cols;
    if (count > SIZE_MAX / sizeof(double)) {
        return 0;
    }

    matrix->values = malloc(count * sizeof(double));
    if (matrix->values == NULL) {
        return 0;
    }

    matrix->rows = rows;
    matrix->cols = cols;
    return 1;
}

static void matrix_free(Matrix *matrix)
{
    free(matrix->values);
    matrix->values = NULL;
    matrix->rows = 0;
    matrix->cols = 0;
}

static void clear_line(void)
{
    int character;
    while ((character = getchar()) != '\n' && character != EOF) {
    }
}

static int read_integer(
    const char *prompt, long long minimum, long long maximum, long long *value)
{
    for (;;) {
        long long input;
        printf("%s", prompt);
        if (scanf("%lld", &input) != 1) {
            if (feof(stdin)) {
                return 0;
            }
            clear_line();
            puts("Entrada invalida. Tente novamente.");
            continue;
        }
        clear_line();

        if (input < minimum || input > maximum) {
            printf("Informe um numero entre %lld e %lld.\n", minimum, maximum);
            continue;
        }

        *value = input;
        return 1;
    }
}

static int read_size(const char *prompt, size_t minimum, size_t *value)
{
    long long input;
    if (!read_integer(prompt, (long long)minimum, LLONG_MAX, &input)) {
        return 0;
    }

    if ((uintmax_t)input > SIZE_MAX) {
        puts("Dimensao muito grande.");
        return 0;
    }

    *value = (size_t)input;
    return 1;
}

static int read_number(const char *prompt, double *value)
{
    for (;;) {
        double input;
        printf("%s", prompt);
        if (scanf("%lf", &input) != 1) {
            if (feof(stdin)) {
                return 0;
            }
            clear_line();
            puts("Entrada invalida. Informe um numero.");
            continue;
        }
        clear_line();

        if (!isfinite(input)) {
            puts("Informe um numero finito.");
            continue;
        }

        *value = input;
        return 1;
    }
}

static int read_choice(long long *choice)
{
    return read_integer("Opcao: ", 0, 4, choice);
}

static OperationStatus matrix_add_or_subtract(
    const Matrix *left, const Matrix *right, Matrix *result, int subtract)
{
    if (left->rows != right->rows || left->cols != right->cols) {
        return OPERATION_DIMENSIONS;
    }
    if (!matrix_create(result, left->rows, left->cols)) {
        return OPERATION_MEMORY;
    }

    for (size_t row = 0; row < left->rows; ++row) {
        for (size_t col = 0; col < left->cols; ++col) {
            double a = *const_element(left, row, col);
            double b = *const_element(right, row, col);
            *element(result, row, col) = subtract ? a - b : a + b;

            if (!isfinite(*element(result, row, col))) {
                matrix_free(result);
                return OPERATION_OVERFLOW;
            }
        }
    }
    return OPERATION_OK;
}

static OperationStatus matrix_multiply(
    const Matrix *left, const Matrix *right, Matrix *result)
{
    if (left->cols != right->rows) {
        return OPERATION_DIMENSIONS;
    }
    if (!matrix_create(result, left->rows, right->cols)) {
        return OPERATION_MEMORY;
    }

    for (size_t row = 0; row < left->rows; ++row) {
        for (size_t col = 0; col < right->cols; ++col) {
            double sum = 0.0;
            for (size_t k = 0; k < left->cols; ++k) {
                double product = *const_element(left, row, k) *
                    *const_element(right, k, col);
                sum += product;
                if (!isfinite(product) || !isfinite(sum)) {
                    matrix_free(result);
                    return OPERATION_OVERFLOW;
                }
            }
            *element(result, row, col) = sum;
        }
    }
    return OPERATION_OK;
}

static OperationStatus matrix_inverse(const Matrix *source, Matrix *inverse)
{
    if (source->rows != source->cols) {
        return OPERATION_NOT_SQUARE;
    }

    Matrix work = {0, 0, NULL};
    double *scales = NULL;
    OperationStatus status = OPERATION_OK;

    if (!matrix_create(&work, source->rows, source->cols) ||
        !matrix_create(inverse, source->rows, source->cols)) {
        status = OPERATION_MEMORY;
        goto cleanup;
    }

    size_t n = source->rows;
    scales = malloc(n * sizeof(double));
    if (scales == NULL) {
        status = OPERATION_MEMORY;
        goto cleanup;
    }

    for (size_t row = 0; row < n; ++row) {
        double scale = 0.0;
        for (size_t col = 0; col < n; ++col) {
            double value = *const_element(source, row, col);
            *element(&work, row, col) = value;
            if (fabs(value) > scale) {
                scale = fabs(value);
            }
            *element(inverse, row, col) = row == col ? 1.0 : 0.0;
        }
        scales[row] = scale;
    }

    for (size_t col = 0; col < n; ++col) {
        size_t pivot_row = col;
        double best_ratio = -1.0;
        for (size_t row = col; row < n; ++row) {
            double ratio = scales[row] == 0.0
                ? 0.0
                : fabs(*element(&work, row, col)) / scales[row];
            if (ratio > best_ratio) {
                best_ratio = ratio;
                pivot_row = row;
            }
        }

        double pivot = *element(&work, pivot_row, col);
        if (scales[pivot_row] == 0.0 ||
            fabs(pivot) <= DBL_EPSILON * (double)n * scales[pivot_row]) {
            status = OPERATION_SINGULAR;
            goto cleanup;
        }

        if (pivot_row != col) {
            for (size_t k = 0; k < n; ++k) {
                double temp = *element(&work, col, k);
                *element(&work, col, k) = *element(&work, pivot_row, k);
                *element(&work, pivot_row, k) = temp;

                temp = *element(inverse, col, k);
                *element(inverse, col, k) = *element(inverse, pivot_row, k);
                *element(inverse, pivot_row, k) = temp;
            }
            double temp_scale = scales[col];
            scales[col] = scales[pivot_row];
            scales[pivot_row] = temp_scale;
        }

        pivot = *element(&work, col, col);
        for (size_t k = 0; k < n; ++k) {
            *element(&work, col, k) /= pivot;
            *element(inverse, col, k) /= pivot;
            if (!isfinite(*element(&work, col, k)) ||
                !isfinite(*element(inverse, col, k))) {
                status = OPERATION_OVERFLOW;
                goto cleanup;
            }
        }

        for (size_t row = 0; row < n; ++row) {
            if (row == col) {
                continue;
            }
            double factor = *element(&work, row, col);
            for (size_t k = 0; k < n; ++k) {
                *element(&work, row, k) -= factor * *element(&work, col, k);
                *element(inverse, row, k) -= factor * *element(inverse, col, k);
                if (!isfinite(*element(&work, row, k)) ||
                    !isfinite(*element(inverse, row, k))) {
                    status = OPERATION_OVERFLOW;
                    goto cleanup;
                }
            }
        }
    }

cleanup:
    free(scales);
    matrix_free(&work);
    if (status != OPERATION_OK) {
        matrix_free(inverse);
    }
    return status;
}

static OperationStatus matrix_divide(
    const Matrix *left, const Matrix *right, Matrix *result)
{
    if (right->rows != right->cols) {
        return OPERATION_NOT_SQUARE;
    }
    if (left->cols != right->rows) {
        return OPERATION_DIMENSIONS;
    }

    Matrix inverse = {0, 0, NULL};
    OperationStatus status = matrix_inverse(right, &inverse);
    if (status != OPERATION_OK) {
        return status;
    }

    status = matrix_multiply(left, &inverse, result);
    matrix_free(&inverse);
    return status;
}

static void print_matrix(const Matrix *matrix)
{
    for (size_t row = 0; row < matrix->rows; ++row) {
        printf("|");
        for (size_t col = 0; col < matrix->cols; ++col) {
            printf("%10.6g", *const_element(matrix, row, col));
            if (col + 1 < matrix->cols) {
                printf(" ");
            }
        }
        printf(" |\n");
    }
}

static void print_operation_error(OperationStatus status, size_t first, size_t second)
{
    switch (status) {
    case OPERATION_DIMENSIONS:
        printf("Nao e possivel operar as matrizes %zu e %zu: dimensoes incompativeis.\n",
               first, second);
        break;
    case OPERATION_NOT_SQUARE:
        printf("A matriz %zu, que seria divisora, nao e quadrada.\n", second);
        break;
    case OPERATION_SINGULAR:
        printf("A matriz %zu nao e invertivel; a divisao nao pode ser realizada.\n",
               second);
        break;
    case OPERATION_MEMORY:
        puts("Nao foi possivel alocar memoria para calcular o resultado.");
        break;
    case OPERATION_OVERFLOW:
        puts("O resultado excede o intervalo de valores numericos representavel.");
        break;
    case OPERATION_OK:
        break;
    }
}

int executar_programa(void)
{
    Matrix *matrices = NULL;
    size_t matrix_count = 0;
    int exit_status = EXIT_FAILURE;

    puts("=== Operacoes com matrizes ===");
    if (!read_size("Quantas matrizes deseja cadastrar (minimo 2)? ",
                   2, &matrix_count)) {
        puts("\nEntrada encerrada.");
        goto cleanup;
    }
    if (matrix_count > SIZE_MAX / sizeof(Matrix)) {
        puts("Quantidade de matrizes muito grande.");
        goto cleanup;
    }

    matrices = calloc(matrix_count, sizeof(Matrix));
    if (matrices == NULL) {
        puts("Nao foi possivel alocar memoria para as matrizes.");
        goto cleanup;
    }

    for (size_t i = 0; i < matrix_count; ++i) {
        char prompt[128];
        size_t rows;
        size_t cols;
        snprintf(prompt, sizeof(prompt), "Matriz %zu - quantidade de linhas: ", i + 1);
        if (!read_size(prompt, 1, &rows)) {
            puts("\nEntrada encerrada.");
            goto cleanup;
        }
        snprintf(prompt, sizeof(prompt), "Matriz %zu - quantidade de colunas: ", i + 1);
        if (!read_size(prompt, 1, &cols)) {
            puts("\nEntrada encerrada.");
            goto cleanup;
        }

        if (!matrix_create(&matrices[i], rows, cols)) {
            printf("Nao foi possivel alocar a matriz %zu; verifique suas dimensoes.\n",
                   i + 1);
            goto cleanup;
        }

        for (size_t row = 0; row < rows; ++row) {
            for (size_t col = 0; col < cols; ++col) {
                snprintf(prompt, sizeof(prompt),
                         "Matriz %zu - elemento [%zu][%zu]: ",
                         i + 1, row + 1, col + 1);
                if (!read_number(prompt, element(&matrices[i], row, col))) {
                    puts("\nEntrada encerrada.");
                    goto cleanup;
                }
            }
        }
    }

    for (;;) {
        long long choice;
        puts("\nMenu de operacoes:");
        puts("1 - Adicao");
        puts("2 - Subtracao");
        puts("3 - Multiplicacao");
        puts("4 - Divisao matricial (A / B = A * B^-1)");
        puts("0 - Encerrar");
        if (!read_choice(&choice)) {
            puts("\nEntrada encerrada.");
            break;
        }
        if (choice == 0) {
            break;
        }

        Matrix result = {0, 0, NULL};
        if (!matrix_create(&result, matrices[0].rows, matrices[0].cols)) {
            puts("Nao foi possivel alocar memoria para o resultado.");
            continue;
        }
        for (size_t row = 0; row < result.rows; ++row) {
            for (size_t col = 0; col < result.cols; ++col) {
                *element(&result, row, col) =
                    *const_element(&matrices[0], row, col);
            }
        }

        OperationStatus status = OPERATION_OK;
        size_t failed_first = 1;
        size_t failed_second = 2;
        for (size_t i = 1; i < matrix_count; ++i) {
            Matrix next = {0, 0, NULL};
            failed_first = i;
            failed_second = i + 1;

            if (choice == 1 || choice == 2) {
                status = matrix_add_or_subtract(
                    &result, &matrices[i], &next, choice == 2);
            } else if (choice == 3) {
                status = matrix_multiply(&result, &matrices[i], &next);
            } else {
                status = matrix_divide(&result, &matrices[i], &next);
            }

            if (status != OPERATION_OK) {
                print_operation_error(status, failed_first, failed_second);
                matrix_free(&next);
                break;
            }
            matrix_free(&result);
            result = next;
        }

        if (status == OPERATION_OK) {
            puts("\nMatriz resultante:");
            print_matrix(&result);
        }
        matrix_free(&result);
    }

    exit_status = EXIT_SUCCESS;

cleanup:
    if (matrices != NULL) {
        for (size_t i = 0; i < matrix_count; ++i) {
            matrix_free(&matrices[i]);
        }
        free(matrices);
    }
    return exit_status;
}
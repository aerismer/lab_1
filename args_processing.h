#pragma once

#include <stdio.h>


typedef enum {
    TYPE_NULL,
    TYPE_INT,
    TYPE_DOUBLE,
    TYPE_STRING
} Type;

typedef struct {
    char* name;
    Type column_type;
} Column;

typedef enum {
    END_BY_COMMA,
    END_BY_EOL,
    END_BY_ERROR_QUOTES
} End;

typedef struct{
    char* name;
    Type column_type;
    long long nulls;
    long long min;
    long long max;
    double mean;
    long long positive;
    long long negative;
    long long even;
    long long not_nulls;
} Block_INT;

typedef struct{
    char* name;
    Type column_type;
    long long nulls;
    double min;
    double max;
    double mean;
    long long positive;
    long long negative;
    long long integer;
    long long not_nulls;
} Block_DOUBLE;

typedef struct{
    char* name;
    Type column_type;
    long long nulls;
    size_t min_length;
    size_t max_length;
    double mean_length;
    long long empty;
    long long not_nulls;
} Block_STRING;

typedef struct{
    char* name;
    Type column_type;
    long long rows_count;
} Block_NULLS;

void parse_args(int argc, char* argv[]);

Column* schema(FILE* file, int* out_column_count, int* out_row_count, int silent);

int fields(char** buffer, char* field, int* quoted);

Type detect_cell_type(const char* value, int quoted, Column* col);

void stats(FILE* file);

void process_stats(
    Block_INT* block_int, 
    Block_DOUBLE* block_double, 
    Block_STRING* block_string, 
    Block_NULLS* block_nulls,
    Column* columns, 
    int column_count, 
    char** fields_array, 
    int* quoted_array
);

void print_stats(
Block_INT* block_int, 
Block_DOUBLE* block_double, 
Block_STRING* block_string, 
Block_NULLS* block_nulls,
Column* columns,
int column_count,
int row_count
);
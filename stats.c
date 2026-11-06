#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "args_processing.h"

void stats(FILE* file) {
    int column_count = 0;
    int row_count = 0;

    Column* columns = schema(file, &column_count, &row_count, 1);
    if (columns == NULL || column_count == 0) {
        return; 
    }

    fseek(file, 0, SEEK_SET);

    Block_INT* block_int = calloc(column_count, sizeof(Block_INT));
    Block_DOUBLE* block_double = calloc(column_count, sizeof(Block_DOUBLE));
    Block_STRING* block_string = calloc(column_count, sizeof(Block_STRING));
    Block_NULLS* block_nulls = calloc(column_count, sizeof(Block_NULLS));

    for (int i = 0; i < column_count; i++) {
        block_int[i].mean = 0.0L;
        block_double[i].mean = 0.0L;
        block_string[i].mean_length = 0.0L;
    }

    int len_max = 100;
    char* buffer = malloc(sizeof(char) * len_max);
    char* field = malloc(sizeof(char) * len_max);
    
    char** fields_array = malloc(sizeof(char*) * column_count);
    int* quoted_array = malloc(sizeof(int) * column_count);

    int current_row = 0;

    while (!feof(file)) {
        int len_s = 0;
        
        while (1) {
            if (len_s + 99 >= len_max) {
                char* temp_b = realloc(buffer, len_max * 2);
                char* temp_f = realloc(field, len_max * 2);
                if (temp_b && temp_f) {
                    buffer = temp_b;
                    field = temp_f;
                    len_max *= 2;
                } else {
                    abort();
                }
            }
            if (fgets(buffer + len_s, 99, file) == NULL) {
                break;
            }
            len_s += strlen(buffer + len_s);
            if (len_s > 0 && buffer[len_s - 1] == '\n') {
                break;
            }
        }

        if (len_s == 0) break;

        if (current_row == 0) {
            current_row++;
            continue;
        }

        char* buff_temp = buffer;
        int current_col = 0;

        for (int i = 0; i < column_count; i++) {
            fields_array[i] = NULL;
            quoted_array[i] = 0;
        }

        while (current_col < column_count) {
            int quoted;
            int end_reason = fields(&buff_temp, field, &quoted);
            
            fields_array[current_col] = strdup(field);
            quoted_array[current_col] = quoted;
            current_col++;

            if (end_reason == END_BY_EOL || end_reason == END_BY_ERROR_QUOTES) {
                break;
            }
        }

        for (int i = current_col; i < column_count; i++) {
            fields_array[i] = strdup("");
            quoted_array[i] = 0;
        }

        process_stats(block_int, block_double, block_string, block_nulls, 
                      columns, column_count, fields_array, quoted_array);

        for (int i = 0; i < column_count; i++) {
            free(fields_array[i]);
        }
        current_row++;
    }

    print_stats(block_int, block_double, block_string, block_nulls, columns, column_count, row_count);

    free(buffer);
    free(field);
    free(fields_array);
    free(quoted_array);
    free(block_int);
    free(block_double);
    free(block_string);
    free(block_nulls);
    
    for (int i = 0; i < column_count; i++) {
        free(columns[i].name);
    }
    free(columns);
}


void process_stats(
    Block_INT* block_int, 
    Block_DOUBLE* block_double, 
    Block_STRING* block_string, 
    Block_NULLS* block_nulls,
    Column* columns, 
    int column_count, 
    char** fields_array, 
    int* quoted_array
){

for(int i = 0; i < column_count; i++){

    if(columns[i].column_type == TYPE_NULL){
        block_nulls[i].rows_count++;
        continue;
    }

    const char* field = fields_array[i];
    int quoted = quoted_array[i];

    if(field == NULL){
        continue;
    }

    const char* k = field;
    while(*k == ' ' || *k == '\t'){
        k++;
    }

    if(!quoted && *k == '\0'){

        switch(columns[i].column_type){

            case TYPE_INT:
            if(block_int == NULL){
                break;
            }
            block_int[i].nulls++;
            break;

            case TYPE_DOUBLE:
            if(block_double == NULL){
                break;
            }
            block_double[i].nulls++;
            break;

            case TYPE_STRING:
            if(block_string == NULL){
                break;
            }
            block_string[i].nulls++;
            break;

            default:
            break;
        }
        continue;
    }

    switch(columns[i].column_type){

        case TYPE_INT:

            if(block_int == NULL){
                break;
            }

            char* end_i;
            long long value_i = strtoll(field, &end_i, 10);

            if (end_i == field) {
                block_int[i].nulls++;
                break; 
            }

            if(value_i > 0){
                block_int[i].positive++;
            }
            if(value_i < 0){
                block_int[i].negative++;
            }
            if(value_i % 2 == 0){
                block_int[i].even++;
            }

            if(block_int[i].not_nulls == 0) {
                block_int[i].min = value_i;
                block_int[i].max = value_i;
                block_int[i].not_nulls = 1;
                block_int[i].mean = (double)value_i;
            }
            else{
                if(value_i < block_int[i].min){
                    block_int[i].min = value_i;
                }
                if(value_i > block_int[i].max){
                    block_int[i].max = value_i;
                }

                block_int[i].not_nulls++;
                block_int[i].mean += ((double)value_i - block_int[i].mean) / block_int[i].not_nulls;; 
            }
            break;

        case TYPE_DOUBLE:

            if (block_double == NULL){
                break;
            }
            
            char* end_d;
            double value_d = strtod(field, &end_d); 

            if (end_d == field) {
                block_double[i].nulls++;
                break;
            }

            if (value_d > 0.0){
                block_double[i].positive++;
            }
            if (value_d < 0.0){ 
                block_double[i].negative++;
            }

            if (fmod(value_d, 1.0) == 0.0) {
                block_double[i].integer++;
            }

            if (block_double[i].not_nulls == 0) {
                block_double[i].min = (long double)value_d;
                block_double[i].max = (long double)value_d;
                block_double[i].not_nulls = 1;
                block_double[i].mean = (double)value_d;
            } 
            else {
                if (value_d < block_double[i].min){
                    block_double[i].min = value_d;
                }
                if (value_d > block_double[i].max){
                    block_double[i].max = value_d;
                }

                block_double[i].not_nulls++;
                block_double[i].mean += ((double)value_d - block_double[i].mean) / block_double[i].not_nulls;; 
            }
            break;

        case TYPE_STRING:

            if (block_string == NULL) {
                break;
            }
            
            size_t len = strlen(field);

            if (quoted && len == 0) {
                block_string[i].empty++;
            }

            if (block_string[i].not_nulls == 0) {
                block_string[i].min_length = len;
                block_string[i].max_length = len;
                block_string[i].not_nulls = 1;
                block_string[i].mean_length = (double)len;
            } 
            else {
                if (len < block_string[i].min_length){
                        block_string[i].min_length = len;
                }
                if (len > block_string[i].max_length){
                    block_string[i].max_length = len;
                }
                
                block_string[i].not_nulls++;
                block_string[i].mean_length += ((double)len - block_string[i].mean_length) / block_string[i].not_nulls;
            }
            break;

        case TYPE_NULL:
            break;
        }
    }    
}


void print_stats(
Block_INT* block_int, 
Block_DOUBLE* block_double, 
Block_STRING* block_string, 
Block_NULLS* block_nulls,
Column* columns,
int column_count,
int row_count
) {

    printf("columns %d\n", column_count);
    printf("rows %d\n", row_count);

    printf("\n");

for (int i = 0; i < column_count; i++){
    printf("column %d\n", i);
    printf("name ");

    putchar('"');
    char* k = columns[i].name;
    if(k != NULL){
        while(*k != '\0'){
            if(*k == '"'){
                putchar('"');
            }
            putchar(*k);
            k++;
        }
    }
    printf("\"\n");

    switch (columns[i].column_type){
        
        case TYPE_NULL:
            printf("type NULL\n");
            if (block_nulls != NULL) {
                printf("rows %lld\n", block_nulls[i].rows_count);
            } 
            else {
                printf("rows 0\n");
            }
            break;

        case TYPE_INT:

            printf("type INT\n");

            if (block_int != NULL){
                printf("nulls %lld\n", block_int[i].nulls);

                if (block_int[i].not_nulls > 0){
                    printf("min %lld\n", block_int[i].min);
                    printf("max %lld\n", block_int[i].max);
           
                    printf("mean %.6f\n", block_int[i].mean); 
                    printf("positive %lld\n", block_int[i].positive);
                    printf("negative %lld\n", block_int[i].negative);
                    printf("even %lld\n", block_int[i].even);
                } 
                else {
                    printf("min -\nmax -\nmean -\npositive 0\nnegative 0\neven 0\n");
                }
            }
            break;

        case TYPE_DOUBLE:

            printf("type DOUBLE\n");

            if (block_double != NULL) {
                printf("nulls %lld\n", block_double[i].nulls);

                if (block_double[i].not_nulls > 0) {
                    if (block_double[i].min == 0.0){
                        block_double[i].min = 0.0;
                    }
                    if (block_double[i].max == 0.0) {
                        block_double[i].max = 0.0;
                    }

                    printf("min %.6f\n", block_double[i].min);
                    printf("max %.6f\n", block_double[i].max);
                    printf("mean %.6f\n", block_double[i].mean);
                    
                    printf("positive %lld\n", block_double[i].positive);
                    printf("negative %lld\n", block_double[i].negative);
                    printf("integer %lld\n", block_double[i].integer);
                }
                else {
                    printf("min -\nmax -\nmean -\npositive 0\nnegative 0\ninteger 0\n");
                }
            }
            break;

        case TYPE_STRING:

            printf("type STRING\n");

            if (block_string != NULL) {
                printf("nulls %lld\n", block_string[i].nulls);

                if (block_string[i].not_nulls > 0) {
                    printf("min_length %zu\n", block_string[i].min_length);
                    printf("max_length %zu\n", block_string[i].max_length);                
                    printf("mean_length %.6f\n", block_string[i].mean_length);
                    
                    printf("empty %lld\n", block_string[i].empty);
                } 
                else {
                    printf("min_length -\nmax_length -\nmean_length -\nempty 0\n");
                }
            }
            break;

        default:
        break;
    }
    printf("\n");
  }
}


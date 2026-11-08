#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>

#include "args_processing.h"

Column* schema(FILE* file, int* out_column_count, int* out_row_count, int silent){

    Column* columns = malloc(sizeof(Column) * 256);

    for(int i = 0; i < 256; i++){
        columns[i].column_type = TYPE_NULL;
        columns[i].name = NULL;
    }

    int first_row_columns = -1;
    int len_max = 256;
    
    int column_count = 0;
    int row_count = 0;

    char* buffer = malloc(sizeof(char) * len_max);

    char* field = malloc(sizeof(char) * len_max);

    while(!feof(file)) {

    int len_s = 0;
    column_count = 0;

    while(1){
        if(len_s + 256 >= len_max){
            char* temp_b = realloc(buffer, len_max * 2);
            char* temp_f = realloc(field, len_max * 2);

            if(temp_b != NULL && temp_f != NULL) {
                buffer = temp_b;
                field = temp_f;
                len_max *= 2;
            }
            else {
                fprintf(stderr, "Ошибка: не удалось выделить память.\n");
                fclose(file);
                exit(1);
            }
        }

        if(fgets(buffer + len_s, 256, file) == NULL) {
            break;
        } 
        
        len_s += strlen(buffer + len_s);

        if(len_s > 0 && buffer[len_s - 1] == '\n') {
            break;
        }
    }

    if(len_s == 0) {
        if(row_count == 0){
            fprintf(stderr, "Ошибка: файл не содержит данных.\n");
            fclose(file);
            exit(1);
        }
        else{
            break;
        }
    }

    char* buff_temp = buffer;

    while(1){

        int quoted;
        int end_reason = fields(&buff_temp, field, &quoted);

        if(row_count == 0){
        columns[column_count].name = strdup(field);
        }
        else {
            detect_cell_type(field, quoted, &columns[column_count]);
        }
        column_count++;

        if(end_reason == END_BY_ERROR_QUOTES){
            fprintf(stderr, "Ошибка в столбце %d, строке %d: кавычки не были закрыты или их использование неуместно.\n", column_count, row_count);
            
            int L;
            if(row_count == 0){
                L = first_row_columns;
            }
            else{
                L = column_count;
            }

            for (int i = 0;i < L; i++){
               free(columns[i].name);
            }
            free(buffer);
            free(columns);
            free(field);
            fclose(file);
            exit(1);
        }
        else if(end_reason == END_BY_COMMA){
            continue;
        }
        else if(end_reason == END_BY_EOL){
            row_count++;
            break;
        }
      }

       if(row_count == 1 && first_row_columns == -1){
        first_row_columns = column_count;
    }

    if(column_count != first_row_columns){
        fprintf(stderr, "Ошибка: неверный формат таблицы.\n");

        for (int i = 0;i < first_row_columns; i++){
        free(columns[i].name);
        }
        free(buffer);
        free(columns);
        free(field);
        fclose(file);
        exit(1);
        }
    }

    if(!silent){
    printf("columns %d\n", column_count);
    printf("rows %d\n", row_count);

    const char* type_names[] = {"NULL", "INT", "DOUBLE", "STRING"};
    
    for(int i = 0; i < column_count; i++) {
        printf("%d ", i);
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
        printf("\" %s\n",type_names[columns[i].column_type]);
      }
   }
    free(buffer);
    free(field);

    if (out_column_count != NULL) {
        *out_column_count = column_count;
    }
    if (out_row_count != NULL) {
        *out_row_count = row_count;
    }
   
    return columns;
}



int fields(char** buffer, char* field, int* quoted) {
    char c;
    int outer_quotes = 0;
    int quotes_count = 0;
    int len = 0;
    *quoted = 0;

    while(**buffer == ' ' || **buffer == '\t'){
        (*buffer)++;
    }

    if(**buffer == '"'){
        outer_quotes = 1;
        *quoted = 1;
        quotes_count++;
        (*buffer)++;
    }

    while(1) {
        c = **buffer;
        if(c == '\n' || c == '\r' || c == '\0'){
           break;
        }

        if(outer_quotes == 1){
            if(c == '"'){
                if(*(*buffer + 1) == '"'){
                    field[len++] = '"';
                    quotes_count += 2;
                    (*buffer) += 2; 
                    continue;
                }
                else {
                    (*buffer)++; 
                    quotes_count++;

                    while(**buffer == ' ' || **buffer == '\t'){
                        (*buffer)++;
                    }
                    if(**buffer == ','){
                        (*buffer)++;
                        field[len] = '\0';
                        return END_BY_COMMA;
                    }
                    else if(**buffer == '\n' || **buffer == '\r' || **buffer == '\0'){
                        field[len] = '\0';
                        return END_BY_EOL;
                    }
                    else{
                        return END_BY_ERROR_QUOTES;
                    }
                }
            }
            field[len++] = c;
            (*buffer)++;
        }
        else {
        
            if(c == ',') {
                field[len] = '\0';
                (*buffer)++; 
                return END_BY_COMMA;
            }
            if(c == '"') {
                return END_BY_ERROR_QUOTES; 
            }
            
            if(c == ' ' || c == '\t') {
                (*buffer)++;
            }
            else{
            field[len++] = c;
            (*buffer)++;
            }
        }
    }
    
    field[len] = '\0';
    if(quotes_count % 2 != 0){
        return END_BY_ERROR_QUOTES;
    }
    return END_BY_EOL;
}

Type detect_cell_type(const char* value, int quoted, Column* col) {

    Type cell_type = TYPE_NULL;

    if (value != NULL) {

        if (quoted && *value == '\0') {
            cell_type = TYPE_STRING;
        }
        else if (*value != '\0') {
            
            const char* start = value;
            
            if (!quoted) {
                while (*start == ' ' || *start == '\t') {
                    start++;
                }
            }

            if (*start == '\0') {
                cell_type = TYPE_NULL;
            } 
            else {
                char* end;
                errno = 0;

                strtoll(start, &end, 10);
                while (*end == ' ' || *end == '\t') {
                    end++;
                }
                
                if (*end == '\0' && start != end) {

                    if (errno != ERANGE) {
                        cell_type = TYPE_INT;
                    } 
                    else {
                        cell_type = TYPE_STRING;
                    }
                } 
                else {
                    errno = 0;
                    double d = strtod(start, &end);

                    while (*end == ' ' || *end == '\t') {
                        end++;
                    }
                    
                    if (*end == '\0' && start != end) {
                        if (errno == ERANGE && (d == HUGE_VAL || d == -HUGE_VAL)) {
                            cell_type = TYPE_STRING; 
                        } 
                        else {
                            cell_type = TYPE_DOUBLE; 
                        }
                    } 
                    else {
                        cell_type = TYPE_STRING;
                    }
                }
            }
        }
    }
    if (cell_type > col->column_type) {
        col->column_type = cell_type;
    }

    return col->column_type;
}

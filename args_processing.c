#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "args_processing.h"

void parse_args(int argc, char* argv[]) {

    if(argc < 2) {
        fprintf(stderr, "Ошибка: количество аргументов меньше ожидаемого.\n");
        exit(2);
    }

    else if(!strcmp(argv[1], "--help")) {

        if(argc == 2) {
            printf("Справка:\n‾‾‾‾‾‾‾‾\nschema <cvs-file>         Выводит число строк данных, число столбцов, имя и тип столбца.\nstats <cvs-file>          Выводит общую информацию о таблице и статистики столбцов.\n\n");
        }
        else {
            fprintf(stderr,"Ошибка: количество аргументов больше ожидаемого.\n");
            exit(2);
        }
    }

    else if(!strcmp(argv[1], "schema")) {
        if(argc == 3) {
           FILE* file = fopen(argv[2], "r");

           if(file == NULL) {
            fprintf(stderr, "Ошибка: не удалось открыть файл.\n");
            abort();
           }
           else {
            int* column_count;
            int* row_count;
            schema(file, column_count, row_count, 0);
           }
        }
        else {
            fprintf(stderr, "Ошибка: количество аргументов не соответствует ожидаемому.\n");
        }
    }

    else if(!strcmp(argv[1], "stats")) {
        if(argc == 3) {
            FILE* file = fopen(argv[2], "r");

           if(file == NULL) {
            fprintf(stderr, "Ошибка: не удалось открыть файл.\n");
            abort();
           }
           else {
            stats(file);
           }

        }
        else {
            fprintf(stderr, "Ошибка: количество аргументов не соответствует ожидаемому.\n");
        }
    }

    else{
        fprintf(stderr, "Ошибка: неизвестная команда.\n");
    }
}
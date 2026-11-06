#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include "args_processing.h"

int main(int argc, char* argv[]) {

    #ifdef _WIN32
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);
    #endif

    parse_args(argc, argv);

    return 0;
}
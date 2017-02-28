#ifndef UTILITY_INCLUDED
#define UTILITY_INCLUDED 

#include <iostream>      // for io
#include <string>        // for string
#include <cstdlib>       // definitions of exit status

void dieWithError(const std::string& errorMsg) {
    std::cerr << errorMsg << std::endl;
    exit(EXIT_FAILURE);
}

#endif
#include "uci/uci.h"
#include "movegen/attacks.h"
#include <iostream>

int main() {
    // Unbuffered stdout - critical for UCI GUIs (e.g. Arena) on Windows
    std::cout << std::unitbuf;

    // Initialize attack tables
    chess::Attacks::init();
    
    // Start UCI loop
    chess::UCI uci;
    uci.loop();
    
    return 0;
}


#include "uci/uci.h"
#include "movegen/attacks.h"
#include <iostream>

int main() {
    // Initialize attack tables
    chess::Attacks::init();
    
    // Start UCI loop
    chess::UCI uci;
    uci.loop();
    
    return 0;
}


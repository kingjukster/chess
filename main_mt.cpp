#include "uci/uci_new.h"
#include <iostream>

int main() {
    chess::UCINew uci;
    uci.loop();
    return 0;
}

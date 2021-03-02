#include <iostream>

#include "vm/globalState.h"
#include "object/luaString.h"
#include "vm/runner.h"


using namespace std;

int main(int argc, const char* argv[]) {
    if(argc != 2){
        throw runtime_error("Expected filename!");
    }

    GlobalState::InitAll();
    Runner runner;
    runner.RunClosure(argv[1]);

    return 0;
}

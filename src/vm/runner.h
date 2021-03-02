//
// Created by gpf on 2020/12/20.
//

#ifndef LUUA2_RUNNER_H
#define LUUA2_RUNNER_H

#include "VM.h"
#include "globalState.h"

#include <memory>

class Runner {
public:
    Runner();
    ~Runner();
    void RunClosure(const char* fname);
    void RunClosure(Proto* proto);


private:
    std::unique_ptr<VM> vm_;

};


#endif //LUUA2_RUNNER_H

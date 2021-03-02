//
// Created by gpf on 2020/12/10.
//

#ifndef LUUA2_DEBUG_H
#define LUUA2_DEBUG_H

#include <stdio.h>
#include <stdlib.h>

#define panic(str) {fprintf(stderr,"panic!%s:%d--%s\n",__FILE__,__LINE__,str);exit(EXIT_FAILURE);}
#define panic_if(exp,str) if(exp) panic(str)


class debug {

};


#endif //LUUA2_DEBUG_H

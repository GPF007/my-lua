//
// Created by gpf on 2020/12/9.
//

#ifndef LUUA2_ARITH_H
#define LUUA2_ARITH_H


//一些简单的计算函数
namespace arith{
    inline int lmod(int s, int size){
        return (s & (size-1));
    }

    int ceillog2(unsigned int x);
};



#endif //LUUA2_ARITH_H

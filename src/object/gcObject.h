//
// Created by gpf on 2020/12/10.
//

#ifndef LUUA2_GCOBJECT_H
#define LUUA2_GCOBJECT_H
#include "utils/typeAlias.h"
#include <string>

class GCObject{
    friend class GC;
public:

    //getters
    LuaByte type()    const   {return type_;}


protected:
    GCObject* next_;
    LuaByte makked_;
    LuaByte type_;
};


#endif //LUUA2_GCOBJECT_H

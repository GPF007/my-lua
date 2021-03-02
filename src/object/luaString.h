//
// Created by gpf on 2020/12/8.
//

#ifndef LUUA2_LUASTRING_H
#define LUUA2_LUASTRING_H

#include "gcObject.h"
#include "Value.h"
#include <string.h>

class LuaString: public GCObject {
    friend class GC;
    friend class StringTable;
public:

    //getters
    const char* data()   const      {return data_;}
    LuaByte shrlen()     const      {return shrlen_;}
    size_t lnglen()      const      {return lnglen_;}
    unsigned int hash()  const      {return hash_;}
    LuaByte extra()     const       {return extra_;}
    bool isReserved()      const      {return extra_;}

    //setters
    void setExtra(int extra)        {extra_ = extra;}

    void markSelf() {makked_ = GC_BLACK;}
    void destroySelf();


    //methods
    //convert method
    Value convertToNumber() const;
    Value convertToInteger() const;


    friend bool operator <(const LuaString& l, const  LuaString& r);
    friend bool operator <=(const LuaString& l, const  LuaString& r);





    //static functions
    static unsigned int hash(const char* str, size_t len, unsigned int seed);
    static unsigned int hash(LuaString* lstr);
    static bool equalLongString(LuaString* l, LuaString* r);
    static LuaString* CreateLongString(const char* str, size_t l);
    static LuaString* CreateString(const char* str, size_t l);
    static LuaString* CreateString(const char* str){
        return CreateString(str, strlen(str));
    }
    static void DestroyLongString(LuaString* s);


    //friend functions
    friend bool operator == (const LuaString& l, const LuaString& r);

private:
    LuaByte extra_; // reversed words for short stringl; hash for longs
    LuaByte shrlen_; //短字符串的长度
    unsigned int hash_;
    union {
        size_t lnglen_; //长字符串的长度
        LuaString* shrNext_; //或者是段字符串的next指针
    };
    char* data_; //具体的数据

    //private methods
    static LuaString* createStringObject(size_t l, int tag, unsigned int h);
    static LuaString* createShortString(const char* str, size_t l);

};


#endif //LUUA2_LUASTRING_H

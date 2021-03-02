//
// Created by gpf on 2020/12/9.
//

#ifndef LUUA2_LUATABLE_H
#define LUUA2_LUATABLE_H
#include "utils/typeAlias.h"
#include "Value.h"
#include "arith.h"




class LuaTable : public GCObject {
    friend class GC;
public:

    //methods
    static LuaTable* CreateTable();
    static void DestroyTable(LuaTable* t);
    const Value* Get(const Value* key);
    const Value* Get(const Value& key)          {return Get(&key);}
    Value* Set(const Value* key);
    void Set(const Value& key, const Value& val){
        Value* tmp = Set(&key);
        *tmp = val;
    }
    Value* NewKey(const Value* key);
    const Value* getShortString(LuaString* s);
    const Value* getInt(LuaInteger key);
    void setInt(LuaInteger key, Value* value);
    void resize(unsigned int na, unsigned int nh);
    bool Next(Value* key, Value* val);
    bool Next(Value& key, Value& val);
    int Size();

    //getters
    LuaByte lsizenode() const       {return lsizenode_;}
    unsigned int sizearray() const  {return sizearray_;}
    unsigned int sizenode()  const  {return sizenode_;}
    LuaTable* metatable()   const   {return metatable_;}
    void setmetatable(LuaTable* mt) {metatable_ = mt;}

    //for gc
    void markSelf();



private:
    //some type
    //Tkey由两部分组成，一个val和一个next，next是可选的
    struct TKey{
        Value val;
        int next;
    };
    typedef Value TValue;

    struct Node{
        TKey key;
        TValue val;
    };

private:
    //members
    LuaByte flags_;
    LuaByte lsizenode_; //以2为底hash part的size 2 => 4; 3=> 8
    unsigned int sizearray_;     //数组部分的大小
    unsigned int sizenode_;      //hash table部分的大小

    Value* array_; //array part
    Node* node_;    //hash part
    Node* lastfree_; //一个可用的节点
    LuaTable* metatable_;
    static Node dummyNode;

private:


    //methods
    inline int hashIndex(int n){    return arith::lmod(n,sizenode_);}
    inline Node* nodeAt(int idx)                {return &node_[idx];}
    inline Value* arrayAt(LuaInteger idx)      {return &array_[idx];}
    bool isDummy()                              {return lastfree_ == nullptr;}

    const Value* getGeneric(const Value* key);
    Node* freepos();

    //rehash methods
    void rehash(const Value* newkey);
    unsigned int countArray(unsigned int bitmap[]);
    int countHash(unsigned int bitmap[], unsigned int* na);
    bool moveToArray(const Value* key, unsigned int bitmap[]);
    unsigned int optArraySize(unsigned int bitmap[], unsigned int* oldsize);
    void setNodeVector(unsigned int size);
    unsigned int findIndex(Value* key);


};


#endif //LUUA2_LUATABLE_H

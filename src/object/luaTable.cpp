//
// Created by gpf on 2020/12/9.
//

#include "luaTable.h"
#include "luaString.h"
#include "vm/globalState.h"
#include "utils/debug.h"
#include <limits.h>



#define MAXABITS	(int)(sizeof(int) * CHAR_BIT - 1)
#define MAXASIZE	(1u << MAXABITS)
#define dummynode   (&dummyNode)

LuaTable::Node LuaTable::dummyNode = {
        {Value(), 0},
        {Value()},
};


//返回对应的value指针
/**
 * 需要根据不同的key分类讨论
 */
const Value * LuaTable::Get(const Value *key) {
    switch (key->type) {
        case LUA_TSHRSTR: return getShortString(tostring(key));
        case LUA_TNUMINT: return getInt(key->ival);
        case LUA_TNIL:    return kNil;
        default:{
            return getGeneric(key);
        }
    }
}

const Value * LuaTable::getShortString(LuaString* s) {
    //找到节点, idx是hash值 % size
    Node* n = nodeAt(hashIndex(s->hash()));
    for(;;){
        auto key = n->key.val;
        //如果是短字符串且相等则退出
        if(key.isShortString() && key.obj == static_cast<GCObject*>(s))
            return &n->val;
        //尝试链表下一个节点
        int nx = n->key.next;
        if(nx==0)
            return kNil;
        n += nx; //下一个位置

    }
}

//得到key为int类型的value，如果没有找到返回nil
const Value * LuaTable::getInt(LuaInteger key) {
    if(key>0 && key-1 < sizearray_ ){
        return arrayAt(key-1);
    }
    //此时尝试在hash表中查找
    Node* n = nodeAt(hashIndex(key));
    for(;;){
        auto curkey = n->key.val;
        //如果是短字符串且相等则退出
        if(curkey.isInteger() && curkey.ival == key)
            return &n->val;
        //尝试链表下一个节点
        int nx = n->key.next;
        if(nx==0)
            return kNil;
        n += nx; //下一个位置
    }
}

//通常情况下的查找 只在 hashpart中查找
const Value * LuaTable::getGeneric(const Value *key) {
    Node* n = nodeAt(hashIndex(key->HashValue()));
    for(;;){
        auto curkey = n->key.val;
        //如果是短字符串且相等则退出
        if(curkey.equalto(key))
            return &n->val;
        //尝试链表下一个节点
        int nx = n->key.next;
        if(nx==0)
            return kNil;
        n += nx; //下一个位置
    }
}

/**
 * 在表中插入一个新的key，这个方法可能会rehash表,
 * 一定是在hash part中查找
 * @param key
 * @return 返回一个value指针，这个指针是新插入key对于的value可以直接进行赋值操作哟
 */
Value * LuaTable::NewKey(const Value *key) {
    if(key->isNil()){
        panic("table index is nil!");
    }
    //找到key所在的node
    Node* n = nodeAt(hashIndex(key->HashValue()));
    if(!n->key.val.isNil() || isDummy()){
        //n所在的位置key不为nil，说明需要新建一个node连接到对应的链表中
        Node* other;
        Node* freeNode = freepos();
        if(freeNode== nullptr){
            //没有可用的需要rehash
            rehash(key);
            return Set(key);
        }else{
            //此时key应该在的位置node已经被n占据了，计算n的key的hashvalue是否应该在当前位置
            //如果不是的化那么说明发生冲突了，
            //如果是直接替换即可
            other = nodeAt(hashIndex(n->key.val.HashValue()));
            if(other != n){
                //此时发生了冲突，other是n应该所处的位置，但是它占据了即将插入key的位置,讲当前n移动到freepos
                //找到n节点的前一个节点
                while(other + other->key.next != n){
                    other += other->key.next;
                }
                //前一个节点的下一个位置指向freepos，然后将原来的内容复制到frennod中
                other->key.next = static_cast<int>(freeNode - other);
                *freeNode = *n;
                if(n->key.next !=0){
                    freeNode->key.next += static_cast<int>(n - freeNode);
                    n->key.next = 0;
                }
                n->val.setnil();
            }else{
                //n和key的hash值是一样的，应该用链表连接起来
                if(n->key.next!=0){
                    freeNode->key.next = (n + n->key.next - freeNode);
                }
                n->key.next = static_cast<int>(freeNode - n);
                n = freeNode;
            }
        }
    }

    //更新 n的key为要插入的key返回对应的value
    n->key.val = *key;
    return &n->val;
}

//返回当前表中第一个可用的freepos位置,如果没有返回nullptr
//lasfree永远是可用node的下一个节点
LuaTable::Node * LuaTable::freepos() {
    if(!isDummy()){
        while(lastfree_ > node_){
            lastfree_--;
            if(lastfree_->key.val.isNil())
                return lastfree_;
        }
    }
    return nullptr;
}

//尝试插入一个key，返回对应的value指针
//首先尝试是否能找到对于的key，如果能直接返回value指针，否则插入一个key
Value * LuaTable::Set(const Value *key) {
    const Value* val = Get(key);
    if(val != kNil){
        return const_cast<Value*>(val);
    }
    return NewKey(key);
}


//先进行rehash然后插入一个key
void LuaTable::rehash(const Value *newkey) {
    unsigned int arraySize;
    unsigned int arrayKeyCount;
    unsigned int bitmap[MAXABITS + 1];
    for(int i=0;i<=MAXABITS;i++)
        bitmap[i] = 0;
    int totalUsed = 0;

    //首先统计arraypart
    arrayKeyCount = countArray(bitmap);
    totalUsed = arrayKeyCount;
    totalUsed += countHash(bitmap, &arrayKeyCount);

    //然后判断新加入的key是否可以加入arraypart
    if(moveToArray(newkey, bitmap))
        arrayKeyCount++;
    totalUsed++;

    //计算optim的arraysize，然后进行resize操作
    arraySize = optArraySize(bitmap, & arrayKeyCount);
    resize(arraySize, totalUsed - arrayKeyCount);

}

//计算array中键的个数，填充到nums数组中，返回总个数
//  nums是一个位图，nums数组中第i个元素存放的key在2^(i-1) 和 2^(i)之间的元素数量
//  nums[0] ([0~1] 之间元素的个数)
//  nums[1] ([2~4] 之间元素的个数)
//  nums[2] ([5~8] 之间元素的个数)
// 返回数组中有效item的总个数
unsigned int LuaTable::countArray(unsigned int *bitmap) {
    int lg;
    unsigned int ttlg; // 2^log
    unsigned int arrayUsed = 0;
    unsigned int i=1;
    //每次循环 lg+1 ttlg*=2
    for(lg=0, ttlg = 1; lg<=MAXABITS; lg++, ttlg*=2){
        unsigned int counter= 0;
        auto lim = ttlg;
        if(lim > sizearray_){
            lim = sizearray_;
            if(i>lim)
                break;
        }
        while(i<=lim){
            if(!arrayAt(i-1)->isNil())
                counter++;
            ++i;
        }

        //在循环的最后加上counter
        bitmap[lg] += counter;
        arrayUsed += counter;
    }
    return arrayUsed;
}

int LuaTable::countHash(unsigned int *bitmap, unsigned int* na) {
    int hashUsed = 0;
    int arrayUsed = 0;
    for(int i = sizenode_;i>=0;i--){
        Node* n = nodeAt(i);
        if(!n->val.isNil()){
            if(moveToArray(&n->key.val, bitmap))
                arrayUsed++;
            hashUsed++;
        }
    }
    *na += arrayUsed;

    return hashUsed;
}

//尝试讲keymove到arraypart，如果成功返回true并更新bitmap
bool LuaTable::moveToArray(const Value *key, unsigned int *bitmap) {
    //key是整数且符合条件
    if(key->isInteger() && key->ival > 0 && key->ival <MAXASIZE){
        bitmap[arith::ceillog2(key->ival)]++;
        return true;
    }
    return false;
}

//计算最好的数组大小
unsigned int LuaTable::optArraySize(unsigned int *bitmap, unsigned int *oldsize) {
    unsigned int twotoi; // 2^i
    unsigned int a = 0;
    unsigned int na = 0;
    unsigned int optimal = 0;
    int i;
    for(i=0, twotoi=1; *oldsize>twotoi/2; i++, twotoi*=2){
        if(bitmap[i] > 0){
            a+=bitmap[i];
            if( a > twotoi /2){ //如果a超过了它的half
                optimal = twotoi;
                na = a;
            }
        }
    }
    *oldsize = na;
    return optimal;
}

//从新设置大小，arraypart的大小应该是na，hashpart是nh
void LuaTable::resize(unsigned int na, unsigned int nh) {
    //na一定是 2 n 次方
    //首先从新设置数组部分
    auto oldArraySize = sizearray_;
    auto oldHashSize  = isDummy()?0:sizenode_;
    Node* oldHashStart = node_;
    if(na > oldArraySize){// grow array part
        array_ = reinterpret_cast<Value*>(Allocator::ReallocVector(array_, oldArraySize, na, sizeof(Value)));
        //新增加的部分每个值设置为nil
        for(int i= oldArraySize; i< (int)na;i++)
            arrayAt(i)->setnil();
        sizearray_ = na;
    }
    //在shrink之前必须创建好node数组
    setNodeVector(nh);

    if(na < oldArraySize){
        //shrink array part
        sizearray_ = na;
        for(int i=na;i<(int)oldArraySize;i++){
            if(!arrayAt(i)->isNil())
                setInt(i+1, arrayAt(i));
        }
        //shrink array
        array_ = reinterpret_cast<Value*>(Allocator::ReallocVector(array_, oldArraySize, na, sizeof(Value)));
    }

    //最后插入原来的hash部分
    for(int i=oldHashSize-1; i>=0;i--){
        Node* old = oldHashStart + i;
        if(!old->val.isNil()){
            Value* val = Set(&old->key.val);
            *val = old->val;
        }
    }

    //free原来的array
    if(oldHashSize> 0)
        Allocator::Free(oldHashStart, sizeof(Node) * oldArraySize);

}

//重新设置nodevector的大小为size
void LuaTable::setNodeVector(unsigned int size) {
    if(size == 0){//空表
        node_ = dummynode;
        lsizenode_ = 0;
        sizenode_ = 1;
        lastfree_ = nullptr;
    }else{
        int logsize = arith::ceillog2(size);
        size = 1<<(logsize);
        //从新分配node一个数组大小为 size * node
        node_ = reinterpret_cast<Node*>(Allocator::Alloc(size * sizeof(Node)));
        for(int i=0;i<(int)size;i++){
            Node* n = nodeAt(i);
            n->key.next=0;
            n->key.val.setnil();
            n->val.setnil();
        }

        lsizenode_ = logsize;
        sizenode_ = size;
        lastfree_ = nodeAt(size);

    }
}

//插入一个int类型的key，value是value
void LuaTable::setInt(LuaInteger key, Value *value) {
    auto p = getInt(key);
    Value* cell;
    if(p != kNil)
        cell = const_cast<Value*>(p);
    else{
        Value k;
        k.setInt(key);
        cell = NewKey(&k);
    }
    *cell = *value;

}

LuaTable * LuaTable::CreateTable() {
    auto gc = GlobalState::gc;
    auto obj = gc->createObject(LUA_TTABLE, sizeof(LuaTable));
    auto tbl = static_cast<LuaTable*>(obj);
    //初始化table
    tbl->lastfree_ = nullptr;
    tbl->array_ = nullptr;
    tbl->sizearray_ = 0;
    tbl->node_ = dummynode;
    tbl->lsizenode_ = 0;
    tbl->sizenode_ = 1;

    return tbl;

}
//销毁一个表
void LuaTable::DestroyTable(LuaTable *t) {
    if(!t->isDummy()){
        Allocator::Free(t->node_, sizeof(Node)* t->sizenode_);
    }
    Allocator::Free(t->array_, sizeof(Value) * t->sizearray_);
    Allocator::Free(t, sizeof(LuaTable));

}

/**
 *  newkey = table.next(key)
 *  *key = *newkey
 *  *val = table[key]
 *  每次得到的是当前key的下一个新的键值对
0 *
 */
bool LuaTable::Next(Value *key,Value* val) {
    auto i = findIndex(key);

    for(;i<sizearray_;i++){
        if(!arrayAt(i)->isNil()){
            key->setInt(i+1);
            *val = *arrayAt(i);
            return true;
        }
    }
    //减去数组的大小
    i-= sizearray_;
    for(;i<sizenode_;i++){
        auto n = nodeAt(i);
        if(! n->val.isNil()){
            *key = n->key.val;
            *val = n->val;
            return true;
        }
    }


    return false;
}

/**
 *  newkey = table.next(key)
 *  *key = *newkey
 *  *val = table[key]
 *  每次得到的是当前key的下一个新的键值对
0 *
 */
bool LuaTable::Next(Value &key,Value& val) {
    auto i = findIndex(&key);

    for(;i<sizearray_;i++){
        if(!arrayAt(i)->isNil()){
            key.setInt(i+1);
            val = *arrayAt(i);
            return true;
        }
    }
    //减去数组的大小
    i-= sizearray_;
    for(;i<sizenode_;i++){
        auto n = nodeAt(i);
        if(! n->val.isNil()){
            key = n->key.val;
            val = n->val;
            return true;
        }
    }


    return false;
}

//找到key在table中的索引，如果在数组中返回数组索引，否则返回arraysize + nodeidx
unsigned int LuaTable::findIndex(Value *key) {
    unsigned int i = 0;
    if(key->isNil()) return 0; //表示第一次迭代
    if(key->isInteger())
        i = key->ival;
    if(i!=0 && i<=sizearray_)
        return i;

    //尝试在hashtable中
    int nx;
    Node* n = nodeAt(hashIndex(key->HashValue()));
    for(;;){
        if(key->equalto(&n->key.val)){
            i = static_cast<int>(n - node_);
            return i+1 + sizearray_; //这里后移了一个位置
        }
        nx = n->key.next;
        if(nx == 0){
            panic("invalid key to 'next'");
        }
        n+=nx;
    }
}

/**
 * 得到该表的size，现在只考虑数组部分
 * @return
 */
int LuaTable::Size() {
    auto hi = sizearray_;
    //数组部分存在且最后一个位置为nil
    if(hi>0 && array_[hi-1].isNil()){
        //用二分查找找到最前面的那个nil值
        auto lo = 0;
        while(hi - lo>1){
            auto m = (lo + hi)/2;
            if(array_[m-1].isNil())
                hi  = m; //前半段
            else
                lo = m;
        }
        return lo;
    }
    return hi;
}

//标记自己的所有子对象
void LuaTable::markSelf() {
    type_ = GC_BLACK;
    //首先是数组部分
    for(size_t i=0;i<sizearray_;i++){
        if(array_[i].needGC()){
            array_[i].markSelf();
        }
    }
    //然后遍历hash部分
    for(size_t i=0;i<sizenode_;i++){
        auto& key = node_[i].key;
        //如果该key是nil说明该slot是空的啊
        if(key.val.isNil()) continue;
        auto& val = node_[i].val;
        val.markSelf();
    }
}
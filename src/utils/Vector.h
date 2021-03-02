//
// Created by gpf on 2020/12/14.
//

#ifndef LUUA2_VECTOR_H
#define LUUA2_VECTOR_H

#include "memory/allocator.h"
#include <assert.h>

#define DEFAULT_SIZE 2
#define DEFAULT_CAP 2*DEFAULT_SIZE

/**
 * Vector是一个可以扩展的的array
 */
template<typename T, typename Alloc=Allocator>
class Vector {
public:

    Vector(){
        data_ = (T*)Allocator::ReallocVector(nullptr, 0, DEFAULT_CAP, sizeof(T));
        size_ = DEFAULT_SIZE;
        cap_ = DEFAULT_CAP;
    }

    //构造函数，分配n个 value为val的vector，设置size = n,cap = 2*n
    Vector(int n, T&& val){
        data_ = (T*) Allocator::ReallocVector(nullptr, 0, 2*n, sizeof(T));
        size_ = n;
        cap_ = 2 * n;
        for(int i=0;i<size_;i++)
            data_[i] = val;
    }

    Vector(int n){
        data_ = (T*) Allocator::ReallocVector(nullptr, 0, 2*n, sizeof(T));
        size_ = n;
        cap_ = 2 * n;
    }

    //push back
    void push_back(T&& val){
        if(size_< cap_){
            data_[size_++] = val;
        }else{//need relloc
            data_ = (T*)Allocator::ReallocVector(data_, cap_ , 2*cap_, sizeof(T));
            cap_ *=2;
            data_[size_++] = val;
        }
    }

    void Destroy(){
        Alloc::Free(data_, cap_*sizeof(T));
    }

    /*
    ~Vector(){
        if(data_){
            Alloc::Free(data_, cap_*sizeof(T));
        }
    }
     */

    T& operator[](int idx){
        assert(idx>=0 && idx < size_);
        return data_[idx];
    }
    T& operator[](int idx)  const{
        assert(idx>=0 && idx < size_);
        return data_[idx];
    }

    //getters
    T* data()   const   {return data_;}
    int size()  const   {return size_;}
    int cap()   const   {return cap_;}


private:
    T* data_;
    int size_;
    int cap_;

};


#endif //LUUA2_VECTOR_H

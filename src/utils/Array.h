//
// Created by gpf on 2020/12/8.
//

#ifndef LUUA2_ARRAY_H
#define LUUA2_ARRAY_H

#include "memory/allocator.h"
#include <assert.h>

/**
 *
 */
template<typename T, typename Alloc=Allocator>
class Array {
public:
    Array(){
        len_ = 0;
        cap_ = 2;
        data_ = (T*) Allocator::ReallocVector(nullptr, 0, cap_, sizeof(T));
    }

    Array(int n){
        len_ = n;
        cap_ = 2 * n;
        data_ = (T*) Allocator::ReallocVector(nullptr, 0, cap_, sizeof(T));

    }


    Array(int n, T&& val){
        len_ = n;
        cap_ = 2 * n;
        data_ = (T*) Allocator::ReallocVector(nullptr, 0, cap_, sizeof(T));
        for(int i=0;i<len_;i++)
            data_[i] = val;
    }
    //浅的拷贝 只拷贝指针和大小
    Array(const Array& other){
        len_ = other.len_;
        cap_ = other.cap_;
        data_ = other.data_;
    }

    Array& operator=(const Array& other){
        len_ = other.len_;
        data_ = other.data_;
        cap_ = other.cap_;
        return *this;
    }


    void Destroy(){
        if(data_)
            Alloc::Free(data_, cap_* sizeof(T));
    }

    /*
    ~Array(){
        if(data_){
            Alloc::Realloc(data_, len_*sizeof(T), 0);
        }
    }
     */

    T& operator[](int idx){
        assert(idx>=0 && idx < len_);
        return data_[idx];
    }
    T& operator[](int idx) const{
        assert(idx>=0 && idx < len_);
        return data_[idx];
    }

    //getters
    T* data()       const {return data_;}
    int len()    const {return len_;}
    int cap()       const {return cap_;}

    //push back
    void push_back(T& val){
        if(len_< cap_){
            data_[len_++] = val;
        }else{//need relloc

            data_ = (T*)Allocator::ReallocVector(data_, cap_ , 2*cap_, sizeof(T));
            cap_ *=2;
            data_[len_++] = val;
        }
    }

    T& push_back(){
        if(len_< cap_){
           return data_[len_++];
        }else{//need relloc
            data_ = (T*)Allocator::ReallocVector(data_, cap_ , 2*cap_, sizeof(T));
            cap_ *=2;
            return data_[len_++];
        }
    }




    //如果大于cap要重置
    void resize(int nsize){
        if(nsize>cap_){
            cap_ = 2* nsize;
            len_ = nsize;
            data_ = (T*)Allocator::ReallocVector(data_, cap_ , cap_, sizeof(T));
        }else{
            len_ = nsize;
        }
    }



    //比较==
    bool operator==(const Array& right){
        if(len_ != right.len_)
            return false;
        for(int i=0;i<len_;i++){
            if(data_[i] != right[i])
                return false;
        }
        return true;
    }

private:

    //为了对齐这里是16bytes
    T* data_;
    int len_;
    int cap_;
};





#endif //LUUA2_ARRAY_H

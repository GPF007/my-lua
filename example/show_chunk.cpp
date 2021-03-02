//
// Created by gpf on 2020/12/14.
//

#include <iostream>
#include "dump/undump.h"
#include "object/proto.h"
#include "vm/globalState.h"
using namespace std;


int main(int argc, char* argv[])
{
    if(argc!=2){
        fprintf(stderr,"Epxected file name \n");
        exit(1);
    }
    GlobalState::InitAll();



    const char* fname = argv[1];
    Proto* p = Undumper::Undump(fname);
    cout<<p->toString()<<endl;

    cout<<"this is dump"<<endl;


}
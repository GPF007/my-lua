//
// Created by gpf on 2021/1/28.
//

#include <iostream>
#include "dump/undump.h"
#include "object/proto.h"
#include "vm/globalState.h"
#include "compile/compiler.h"
using namespace std;


int main(int argc, char* argv[])
{
    if(argc<2){
        fprintf(stderr,"Epxected file name \n");
        exit(1);
    }
    GlobalState::InitAll();



    const char* fname = argv[1];
    //如果有第二个参数的话，输出到第二个参数的文件，否则输出到stdout
    FILE * outf = stdout;
    if(argc==3){
        const char* output = argv[2];
        outf = fopen(output,"w");
    }
    Compiler compiler;
    Proto* p = compiler.compile(fname);
    //cout<<compiler.block()->toString()<<endl;
    fprintf(outf, "%s",p->toString().data());
    //cout<<p->toString()<<endl;

    //cout<<"this is dump"<<endl;


}
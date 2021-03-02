//
// Created by gpf on 2020/12/26.
//


#include <iostream>
#include "compile/lexer.h"
#include "vm/globalState.h"
#include <fmt/core.h>
using namespace std;


int main(int argc, char* argv[])
{
    if(argc!=2){
        fprintf(stderr,"Epxected file name \n");
        exit(1);
    }
    GlobalState::InitAll();



    const char* fname = argv[1];
    Lexer lexer(fname);
    fmt::print("Lexer create done!\n");
    int i=1;
    for(;;){
        auto tok = lexer.GetToken();
        if(tok->type() == Token::TK_EOS)
            break;
        i++;
        //if(i==10)
        //    break;
        auto str = tok->toString();
        cout<<str<<endl;
        //fmt::print("{}\n",tok->toString());
    }



}
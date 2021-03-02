//
// Created by gpf on 2020/12/14.
//

#ifndef LUUA2_UNDUMP_H
#define LUUA2_UNDUMP_H

/**
 * Undumper 用来读取二进制文件中的proto
 */
#include <string>
#include "object/proto.h"

class Undumper{
public:

    static Proto* Undump(const char* fname);

private:
    std::string fname_;
    size_t idx_;
    std::string buf_;
    size_t len_;

    //private methods
    LuaByte     readByte();
    int         readInt();
    unsigned int readUInt();
    LuaInteger  readInteger();
    LuaNumber   readNumber();
    LuaString*  readString();
    std::string readChars(size_t len);
    Array<LuaByte> readBytes(size_t len);

    //void read functon 直接操纵 proto
    void readCodes(Proto* p);
    void readConstants(Proto* p);
    void readUpvalues(Proto* p);
    void readDebug(Proto* p);
    void readProtos(Proto* p);
    void readFunction(Proto* p, LuaString* psource);

    void checkHeader();

    Undumper(std::string fname);
    Undumper(const char* fname): Undumper(std::string(fname)){}

    Proto* GetProto();

};

#endif //LUUA2_UNDUMP_H

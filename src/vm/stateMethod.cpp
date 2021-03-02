//
// Created by gpf on 2020/12/24.
//

#include "luaState.h"
#include "object/luaTable.h"


/**
 * idx处是一个表，得到该表的next值
 * @param idx
 * @return
 */
bool LuaState::next(int idx) {
    ASSERT(stack_[idx].isTable());
    auto tbl = static_cast<LuaTable*>(stack_[idx].obj);
    //next的key是top-1的位置，生成的value放在top位置，如果生成的value不为null那么递增top
    bool hasMore = tbl->Next(stack_[top_-1], stack_[top_]);
    if(hasMore)
        top_++;
    return hasMore;

}
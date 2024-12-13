#pragma once
#include <unordered_map>
#include <memory>
#include <cassert>
#include <iostream>
#include "koopa.h"

/**********************************************************************************************************/
/************************************************Stack*****************************************************/
/**********************************************************************************************************/

class Stack
{
public:
    int len;
    int pos;

    Stack()
    {
        len = 0;
        pos = 0;
    }
    void alloc_value(koopa_raw_value_t value, int loc);
    int get_loc(koopa_raw_value_t value);
    void init();

private:
    std::unordered_map<koopa_raw_value_t, int> value_loc;
};

static Stack stack;

/**********************************************************************************************************/
/************************************************Visit*****************************************************/
/**********************************************************************************************************/

// 访问 raw program
void Visit(const koopa_raw_program_t &program);

// 访问 raw slice
void Visit(const koopa_raw_slice_t &slice);

// 访问函数
void Visit(const koopa_raw_function_t &func);

// 访问基本块
void Visit(const koopa_raw_basic_block_t &bb);

// 访问指令
void Visit(const koopa_raw_value_t &value);

// 访问 return 指令
void Visit(const koopa_raw_return_t &value);

// 访问 integer 指令
void Visit(const koopa_raw_integer_t &value);

// 访问 binary 指令
void Visit(const koopa_raw_binary_t &binary,
           const koopa_raw_value_t &value);

// 访问 load 指令
void Visit(const koopa_raw_load_t &load,
           const koopa_raw_value_t &value);

// 访问 store 指令
void Visit(const koopa_raw_store_t &value);

// 访问 branch 指令
void Visit(const koopa_raw_branch_t &branch);

// 访问 jump 指令
void Visit(const koopa_raw_jump_t &jump);

/**********************************************************************************************************/
/************************************************Utils*****************************************************/
/**********************************************************************************************************/

// 将 value 加载到 reg 中
void load_reg(const koopa_raw_value_t &value, std::string reg);
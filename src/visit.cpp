#include <cassert>
#include <iostream>
#include "visit.h"
#include "koopa.h"

static int reg_count = 0;
// 加载寄存器
void load_reg(const koopa_raw_value_t &value, std::string reg)
{
    // 根据指令类型判断后续需要如何访问
    const auto &kind = value->kind;
    switch (kind.tag)
    {
    case KOOPA_RVT_INTEGER:
        // 访问 integer 指令
        reg_count++;
        std::cout << "  li " << reg << ", " << kind.data.integer.value << std::endl;
        break;
    default:
        break;
    }
}
// 访问 raw program
void Visit(const koopa_raw_program_t &program)
{
    // 执行一些其他的必要操作
    // ...
    // 访问所有全局变量
    Visit(program.values);
    // 访问所有函数
    Visit(program.funcs);
}

// 访问 raw slice
void Visit(const koopa_raw_slice_t &slice)
{
    for (size_t i = 0; i < slice.len; ++i)
    {
        auto ptr = slice.buffer[i];
        // 根据 slice 的 kind 决定将 ptr 视作何种元素
        switch (slice.kind)
        {
        case KOOPA_RSIK_FUNCTION:
            // 访问函数
            Visit(reinterpret_cast<koopa_raw_function_t>(ptr));
            break;
        case KOOPA_RSIK_BASIC_BLOCK:
            // 访问基本块
            Visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr));
            break;
        case KOOPA_RSIK_VALUE:
            // 访问指令
            Visit(reinterpret_cast<koopa_raw_value_t>(ptr));
            break;
        default:
            // 我们暂时不会遇到其他内容, 于是不对其做任何处理
            assert(false);
        }
    }
}

// 访问函数
void Visit(const koopa_raw_function_t &func)
{
    // 执行一些其他的必要操作
    std::cout << "  .text" << std::endl;
    std::cout << "  .globl " << func->name + 1 << std::endl;
    std::cout << func->name + 1 << ":" << std::endl;

    // 访问所有基本块
    Visit(func->bbs);
}

// 访问基本块
void Visit(const koopa_raw_basic_block_t &bb)
{
    // 执行一些其他的必要操作
    // ...
    // 访问所有指令
    Visit(bb->insts);
}

// 访问指令
void Visit(const koopa_raw_value_t &value)
{
    // 根据指令类型判断后续需要如何访问
    const auto &kind = value->kind;
    switch (kind.tag)
    {
    case KOOPA_RVT_RETURN:
        // 访问 return 指令
        Visit(kind.data.ret);
        break;
    case KOOPA_RVT_INTEGER:
        // 访问 integer 指令
        Visit(kind.data.integer);
        break;
    case KOOPA_RVT_BINARY:
        // 访问 binary 指令
        Visit(kind.data.binary);
        break;
    default:
        // 其他类型暂时遇不到
        assert(false);
    }
}

// 访问 return 指令
void Visit(const koopa_raw_return_t &value)
{
    // 访问返回值
    Visit(value.value);
    std::cout << "  ret" << std::endl;
}

// 访问 integer 指令
void Visit(const koopa_raw_integer_t &value)
{
    // 访问返回值
    // std::cout << "visit integer" << std::endl;
    std::cout << "  li a0, " << value.value << std::endl;
}

// 访问 binary 指令
void Visit(const koopa_raw_binary_t &value)
{
    // 访问左右操作数
    std::cout << "访问左式" << value.lhs->kind.tag << std::endl;
    load_reg(value.lhs, std::string("a") + std::to_string(reg_count));
    std::cout << "访问右式" << value.rhs->kind.tag << std::endl;
    load_reg(value.rhs, std::string("a") + std::to_string(reg_count));
    // 进行运算
    switch (value.op)
    {
    case KOOPA_RBO_ADD:
        std::cout << "  add a" << reg_count - 2 << " a" << reg_count - 2 << " a" << reg_count - 1 << std::endl;
        break;
    case KOOPA_RBO_SUB:
        std::cout << "  sub a" << reg_count - 2 << " a" << reg_count - 2 << " a" << reg_count - 1 << std::endl;
        break;
    case KOOPA_RBO_MUL:
        std::cout << "  mul a" << reg_count - 2 << " a" << reg_count - 2 << " a" << reg_count - 1 << std::endl;
        break;
    // case KOOPA_RBO_DIV:
    //     ret += "  div " + rd + ", " + rs1 + ", " + rs2 + "\n";
    //     break;
    // case KOOPA_RBO_MOD:
    //     ret += "  rem " + rd + ", " + rs1 + ", " + rs2 + "\n";
    //     break;
    // case KOOPA_RBO_AND:
    //     ret += "  and " + rd + ", " + rs1 + ", " + rs2 + "\n";
    //     break;
    // case KOOPA_RBO_OR:
    //     ret += "  or " + rd + ", " + rs1 + ", " + rs2 + "\n";
    //     break;
    // case KOOPA_RBO_EQ:
    //     ret += "  xor " + rd + ", " + rs1 + ", " + rs2 + "\n";
    //     ret += "  seqz " + rd + ", " + rd + "\n";
    //     break;
    // case KOOPA_RBO_NOT_EQ:
    //     ret += "  xor " + rd + ", " + rs1 + ", " + rs2 + "\n";
    //     ret += "  snez " + rd + ", " + rd + "\n";
    //     break;
    // case KOOPA_RBO_GT:
    //     ret += "  sgt " + rd + ", " + rs1 + ", " + rs2 + "\n";
    //     break;
    // case KOOPA_RBO_LT:
    //     ret += "  slt " + rd + ", " + rs1 + ", " + rs2 + "\n";
    //     break;
    // case KOOPA_RBO_GE:
    //     ret += "  slt " + rd + ", " + rs1 + ", " + rs2 + "\n";
    //     ret += "  seqz " + rd + ", " + rd + "\n";
    //     break;
    // case KOOPA_RBO_LE:
    //     ret += "  sgt " + rd + ", " + rs1 + ", " + rs2 + "\n";
    //     ret += "  seqz " + rd + ", " + rd + "\n";
    //     break;
    default:
        break;
    }
}
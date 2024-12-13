#include "visit.h"

/**********************************************************************************************************/
/************************************************Stack*****************************************************/
/**********************************************************************************************************/

void Stack::alloc_value(koopa_raw_value_t value, int loc)
{
    value_loc[value] = loc;
}

int Stack::get_loc(koopa_raw_value_t value)
{
    return value_loc[value];
}

void Stack::init()
{
    len = 0;
    pos = 0;
    value_loc.clear();
}

/**********************************************************************************************************/
/************************************************Visit*****************************************************/
/**********************************************************************************************************/

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

    // 初始化栈帧
    stack.init();

    // 计算栈帧大小
    for (size_t i = 0; i < func->bbs.len; ++i)
    {
        auto bbs = reinterpret_cast<koopa_raw_basic_block_t>(func->bbs.buffer[i])->insts;
        for (size_t j = 0; j < bbs.len; ++j)
        {
            auto inst = reinterpret_cast<koopa_raw_value_t>(bbs.buffer[j]);
            // 把所有变量都放在栈上
            if (inst->ty->tag != KOOPA_RTT_UNIT)
            {
                stack.len += 4;
            }
        }
    }

    // 将栈帧长度对齐到16字节
    stack.len = (stack.len + 15) / 16 * 16;

    // 分配栈帧
    // addi 指令中立即数的范围是 -2048 到 2047
    if (stack.len != 0)
    {
        deal_offset_exceed(stack.len, "addi-", "sp");
    }
    // 访问所有基本块
    Visit(func->bbs);
}

// 访问基本块
void Visit(const koopa_raw_basic_block_t &bb)
{
    // 执行一些其他的必要操作
    // ...
    // 访问所有指令
    std::cout << bb->name + 1 << ":" << std::endl;
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
        Visit(kind.data.binary, value);
        break;
    case KOOPA_RVT_ALLOC:
        // 分配栈帧
        stack.alloc_value(value, stack.pos);
        stack.pos += 4;
        break;
    case KOOPA_RVT_LOAD:
        // 访问 load 指令
        Visit(kind.data.load, value);
        break;
    case KOOPA_RVT_STORE:
        // 访问 store 指令
        Visit(kind.data.store);
        break;
    case KOOPA_RVT_BRANCH:
        Visit(kind.data.branch);
        break;
    case KOOPA_RVT_JUMP:
        Visit(kind.data.jump);
        break;
    default:
        // 其他类型暂时遇不到
        std::cout << kind.tag << std::endl;
        assert(false);
    }
}

// 访问 return 指令
void Visit(const koopa_raw_return_t &value)
{
    // 访问返回值
    load_reg(value.value, "a0");
    // 恢复栈帧
    if (stack.len != 0)
    {
        deal_offset_exceed(stack.len, "addi+", "sp");
    }
    std::cout << "  ret" << std::endl;
}

// 访问 integer 指令
void Visit(const koopa_raw_integer_t &value)
{
    std::cout << "  li t0, " << value.value << std::endl;
}

// 访问 binary 指令
void Visit(const koopa_raw_binary_t &binary, const koopa_raw_value_t &value)
{
    // 访问左右操作数
    load_reg(binary.lhs, "t0");
    load_reg(binary.rhs, "t1");

    // 进行运算
    switch (binary.op)
    {
    case KOOPA_RBO_ADD:
        std::cout << "  add t0, t0, t1" << std::endl;
        break;
    case KOOPA_RBO_SUB:
        std::cout << "  sub t0, t0, t1" << std::endl;
        break;
    case KOOPA_RBO_MUL:
        std::cout << "  mul t0, t0, t1" << std::endl;
        break;
    case KOOPA_RBO_DIV:
        std::cout << "  div t0, t0, t1" << std::endl;
        break;
    case KOOPA_RBO_MOD:
        std::cout << "  rem t0, t0, t1" << std::endl;
        break;
    case KOOPA_RBO_AND:
        std::cout << "  and t0, t0, t1" << std::endl;
        break;
    case KOOPA_RBO_OR:
        std::cout << "  or t0, t0, t1" << std::endl;
        break;
    case KOOPA_RBO_XOR:
        std::cout << "  xor t0, t0, t1" << std::endl;
        break;
    case KOOPA_RBO_SHL:
        std::cout << "  sll t0, t0, t1" << std::endl;
        break;
    case KOOPA_RBO_SHR:
        std::cout << "  srl t0, t0, t1" << std::endl;
        break;
    case KOOPA_RBO_SAR:
        std::cout << "  sra t0, t0, t1" << std::endl;
        break;
    case KOOPA_RBO_EQ:
        std::cout << " xor t0, t0, t1" << std::endl;
        std::cout << "  seqz t0, t0" << std::endl;
        break;
    case KOOPA_RBO_NOT_EQ:
        std::cout << " xor t0, t0, t1" << std::endl;
        std::cout << "  snez t0, t0" << std::endl;
        break;
    case KOOPA_RBO_GT:
        std::cout << "  slt t0, t1, t0" << std::endl;
        break;
    case KOOPA_RBO_LT:
        std::cout << "  slt t0, t0, t1" << std::endl;
        break;
    case KOOPA_RBO_GE:
        std::cout << "  slt t0, t0, t1" << std::endl;
        std::cout << "  xori t0, t0, 1" << std::endl;
        break;
    case KOOPA_RBO_LE:
        std::cout << "  slt t0, t1, t0" << std::endl;
        std::cout << "  xori t0, t0, 1" << std::endl;
        break;
    }

    // 将结果存回
    stack.alloc_value(value, stack.pos);
    stack.pos += 4;
    int offset = stack.get_loc(value);
    deal_offset_exceed(offset, "sw", "t0");
}

// 访问 load 指令
void Visit(const koopa_raw_load_t &load, const koopa_raw_value_t &value)
{
    load_reg(load.src, "t0");

    // 将结果存回
    stack.alloc_value(value, stack.pos);
    stack.pos += 4;
    int offset = stack.get_loc(value);
    deal_offset_exceed(offset, "sw", "t0");
}

// 访问 store 指令
void Visit(const koopa_raw_store_t &value)
{
    load_reg(value.value, "t0");

    // 将结果存回
    int offset = stack.get_loc(value.dest);
    deal_offset_exceed(offset, "sw", "t0");
}

// 访问 branch 指令
void Visit(const koopa_raw_branch_t &branch)
{
    load_reg(branch.cond, "t0");
    std::cout << "  bnez t0, " << branch.true_bb->name + 1 << std::endl;
    std::cout << "  j " << branch.false_bb->name + 1 << std::endl;
}

void Visit(const koopa_raw_jump_t &jump)
{
    std::cout << "  j " << jump.target->name + 1 << std::endl;
}
/**********************************************************************************************************/
/************************************************Utils*****************************************************/
/**********************************************************************************************************/

void load_reg(const koopa_raw_value_t &value, std::string reg)
{
    int offset = 0;
    switch (value->kind.tag)
    {
    case KOOPA_RVT_INTEGER:
        std::cout << "  li " << reg << ", " << value->kind.data.integer.value << std::endl;
        break;
    case KOOPA_RVT_ALLOC:
    case KOOPA_RVT_LOAD:
    case KOOPA_RVT_BINARY:
        offset = stack.get_loc(value);
        deal_offset_exceed(offset, "lw", reg);
        break;
    default:
        break;
    }
}

void deal_offset_exceed(int offset, std::string inst, std::string reg)
{

    if (inst == "lw" || inst == "sw")
    {
        if (offset < -2048 || offset > 2047)
        {
            // 偏移量超出范围，逐步调整基址寄存器
            int new_base_offset = offset & ~0x7FF;
            int remaining_offset = offset & 0x7FF;

            if (new_base_offset < -2048 || new_base_offset > 2047)
            {
                std::cout << "  li t1, " << new_base_offset << std::endl;
                std::cout << "  add t1, sp, t1" << std::endl;
            }
            else
            {
                std::cout << "  addi t1, sp, " << new_base_offset << std::endl;
            }
            std::cout << "  " << inst << " " << reg << ", " << remaining_offset << "(t1)" << std::endl;
        }
        else
        {
            std::cout << "  " << inst << " " << reg << ", " << offset << "(sp)" << std::endl;
        }
    }
    else if (inst == "addi-")
    {
        if (offset < -2048 || offset > 2047)
        {
            std::cout << "  li " << reg << ", " << offset << std::endl;
            std::cout << "  sub sp, sp, " << reg << std::endl;
        }
        else
        {
            std::cout << "  addi sp, sp, -" << offset << std::endl;
        }
    }
    else if (inst == "addi+")
    {
        if (offset < -2048 || offset > 2047)
        {
            std::cout << "  li " << reg << ", " << offset << std::endl;
            std::cout << "  add sp, sp, " << reg << std::endl;
        }
        else
        {
            std::cout << "  addi sp, sp, " << offset << std::endl;
        }
    }
}
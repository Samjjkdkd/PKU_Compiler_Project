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

// 记录ra是否被使用
static int ra_count = 0;

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
    if (func->bbs.len == 0)
    {
        return;
    }
    // 执行一些其他的必要操作
    std::cout << "  .text" << std::endl;
    std::cout << "  .globl " << func->name + 1 << std::endl;
    std::cout << func->name + 1 << ":" << std::endl;

    // 初始化栈帧
    stack.init();

    int var_count = 0;
    ra_count = 0;
    int arg_count = 0;
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
                var_count++;
            }
            if (inst->kind.tag == KOOPA_RVT_CALL)
            {
                ra_count = 1;
                arg_count =
                    std::max(arg_count, std::max(0, (int)inst->kind.data.call.args.len - 8));
            }
        }
    }
    stack.len = (var_count + ra_count + arg_count) * 4;
    // 将栈帧长度对齐到16字节
    stack.len = (stack.len + 15) / 16 * 16;
    stack.pos = arg_count * 4;
    // 分配栈帧
    // addi 指令中立即数的范围是 -2048 到 2047
    if (stack.len != 0)
    {
        deal_offset_exceed(stack.len, "addi-", "t0");
    }
    if (ra_count)
    {
        deal_offset_exceed(stack.len - 4, "sw", "ra");
    }
    // 访问所有基本块
    Visit(func->bbs);
    std::cout << std::endl;
}

// 访问基本块
void Visit(const koopa_raw_basic_block_t &bb)
{
    // 执行一些其他的必要操作
    // ...
    // 访问所有指令
    if (strncmp(bb->name + 1, "entry", 5) != 0)
    {
        std::cout << bb->name + 1 << ":" << std::endl;
    }
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
    case KOOPA_RVT_GLOBAL_ALLOC:
        // 全局变量分配
        Visit(kind.data.global_alloc, value);
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
    case KOOPA_RVT_CALL:
        Visit(kind.data.call, value);
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
    if (value.value != nullptr)
    {
        load_reg(value.value, "a0");
    }
    // 恢复ra
    if (ra_count)
    {
        deal_offset_exceed(stack.len - 4, "lw", "ra");
    }
    // 恢复栈帧
    if (stack.len != 0)
    {
        deal_offset_exceed(stack.len, "addi+", "t0");
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
    if (value->ty->tag != KOOPA_RTT_UNIT)
    {
        stack.alloc_value(value, stack.pos);
        stack.pos += 4;
        save_reg(value, "t0");
    }
}

// 访问 load 指令
void Visit(const koopa_raw_load_t &load, const koopa_raw_value_t &value)
{
    load_reg(load.src, "t0");

    // 将结果存回
    if (value->ty->tag != KOOPA_RTT_UNIT)
    {
        stack.alloc_value(value, stack.pos);
        stack.pos += 4;
        save_reg(value, "t0");
    }
}

// 访问 store 指令
void Visit(const koopa_raw_store_t &value)
{
    load_reg(value.value, "t0");

    // 将结果存回
    save_reg(value.dest, "t0");
}

// 访问 branch 指令
void Visit(const koopa_raw_branch_t &branch)
{
    load_reg(branch.cond, "t0");
    std::cout << "  bnez t0, " << branch.true_bb->name + 1 << std::endl;
    std::cout << "  j " << branch.false_bb->name + 1 << std::endl;
}

// 访问 jump 指令
void Visit(const koopa_raw_jump_t &jump)
{
    std::cout << "  j " << jump.target->name + 1 << std::endl;
}

// 访问 call 指令
void Visit(const koopa_raw_call_t &call, const koopa_raw_value_t &value)
{
    for (size_t i = 0; i < call.args.len; ++i)
    {
        auto arg = reinterpret_cast<koopa_raw_value_t>(call.args.buffer[i]);
        if (i < 8)
        {
            load_reg(arg, "a" + std::to_string(i));
        }
        else
        {
            load_reg(arg, "t0");
            deal_offset_exceed((i - 8) * 4, "sw", "t0");
        }
    }
    std::cout << "  call " << call.callee->name + 1 << std::endl;

    // 将结果存回
    if (value->ty->tag != KOOPA_RTT_UNIT)
    {
        stack.alloc_value(value, stack.pos);
        stack.pos += 4;
        save_reg(value, "a0");
    }
}

// 访问 global_alloc 指令
void Visit(const koopa_raw_global_alloc_t &global_alloc, const koopa_raw_value_t &value)
{
    std::cout << "  .data" << std::endl;
    std::cout << "  .globl " << value->name + 1 << std::endl;
    std::cout << value->name + 1 << ":" << std::endl;
    if (global_alloc.init->kind.tag == KOOPA_RVT_ZERO_INIT)
    {
        std::cout << "  .zero 4" << std::endl;
    }
    else if (global_alloc.init->kind.tag == KOOPA_RVT_INTEGER)
    {
        std::cout << "  .word " << global_alloc.init->kind.data.integer.value << std::endl;
    }
    std::cout << std::endl;
}

/**********************************************************************************************************/
/************************************************Utils*****************************************************/
/**********************************************************************************************************/

void load_reg(const koopa_raw_value_t &value, std::string reg)
{
    int offset = 0;
    size_t index = 0;
    switch (value->kind.tag)
    {
    case KOOPA_RVT_INTEGER:
        std::cout << "  li " << reg << ", " << value->kind.data.integer.value << std::endl;
        break;
    case KOOPA_RVT_ALLOC:
    case KOOPA_RVT_LOAD:
    case KOOPA_RVT_BINARY:
    case KOOPA_RVT_CALL:
        offset = stack.get_loc(value);
        deal_offset_exceed(offset, "lw", reg);
        break;
    case KOOPA_RVT_FUNC_ARG_REF:
        index = value->kind.data.func_arg_ref.index;
        if (index < 8)
        {
            std::cout << "  mv " << reg << ", a" << index << std::endl;
        }
        else
        {
            deal_offset_exceed(stack.len + (index - 8) * 4, "lw", reg);
        }
        break;
    case KOOPA_RVT_GLOBAL_ALLOC:
        std::cout << "  la t1, " << value->name + 1 << std::endl;
        std::cout << "  lw " << reg << ", 0(t1)" << std::endl;
        break;
    default:
        assert(false);
        break;
    }
}

void save_reg(const koopa_raw_value_t &value, std::string reg)
{
    assert(value->kind.tag != KOOPA_RVT_INTEGER);
    if (value->kind.tag == KOOPA_RVT_GLOBAL_ALLOC)
    {
        std::cout << "  la t1, " << value->name + 1 << std::endl;
        std::cout << "  sw " << reg << ", 0(t1)" << std::endl;
        return;
    }
    else
    {
        int offset = stack.get_loc(value);
        deal_offset_exceed(offset, "sw", reg);
    }
}

void deal_offset_exceed(int offset, std::string inst, std::string reg)
{
    if (inst == "lw" || inst == "sw")
    {
        if (offset < -2048 || offset > 2047)
        {
            int new_base_offset = offset & ~0x7FF;
            int remaining_offset = offset & 0x7FF;

            if (new_base_offset < -2048 || new_base_offset > 2047)
            {
                std::cout << "  li t1, " << new_base_offset << std::endl;
                std::cout << "  add t1, t1, sp" << std::endl;
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
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
/**********************************************PtrSizeVec**************************************************/
/**********************************************************************************************************/

void PtrSizeVec::push_size(koopa_raw_value_t value, int size)
{
    size_vec[value].push_back(size);
}

void PtrSizeVec::push_size_vec_iter(koopa_raw_value_t value)
{
    size_vec_iter[value] = std::make_pair(size_vec[value].begin(), size_vec[value].end());
}

int PtrSizeVec::get_value_total_size(koopa_raw_value_t value)
{
    int offset = 4;
    for (auto i : size_vec[value])
    {
        offset *= i;
    }
    return offset;
}

void PtrSizeVec::copy_size_vec(koopa_raw_value_t dest, koopa_raw_value_t src)
{
    if (size_vec_iter.find(src) != size_vec_iter.end())
    {
        size_vec_iter[dest] = size_vec_iter[src];
    }
}

void PtrSizeVec::copy_size_vec_ptr(koopa_raw_value_t dest, koopa_raw_value_t src)
{
    const auto &begin_end = size_vec_iter[src];
    auto first = begin_end.first;
    first++;
    size_vec_iter[dest] = std::make_pair(first, begin_end.second);
}

int PtrSizeVec::get_value_offset(koopa_raw_value_t src)
{
    const auto &begin_end = size_vec_iter[src];
    int offset = 4;
    auto it = begin_end.first;
    ++it;
    for (; it != begin_end.second; it++)
    {
        offset *= (*it);
    }
    return offset;
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
    // 忽略函数声明
    if (func->bbs.len == 0)
        return;

    // 执行一些其他的必要操作
    std::cout << "  .text" << std::endl;
    std::cout << "  .globl " << func->name + 1 << std::endl;
    std::cout << func->name + 1 << ":" << std::endl;

    // 清空
    stack.init();

    // 计算栈帧长度需要的值
    // 局部变量个数
    int var_count = 0;
    // 是否需要为 ra 分配栈空间
    ra_count = 0;
    // 需要为传参预留几个变量的栈空间
    int arg_count = 0;

    // 遍历基本块
    for (size_t i = 0; i < func->bbs.len; ++i)
    {
        const auto &insts = reinterpret_cast<koopa_raw_basic_block_t>(func->bbs.buffer[i])->insts;
        var_count += insts.len;
        for (size_t j = 0; j < insts.len; ++j)
        {
            auto inst = reinterpret_cast<koopa_raw_value_t>(insts.buffer[j]);
            if (inst->ty->tag != KOOPA_RTT_UNIT)
            {
                var_count++;
            }
            if (inst->kind.tag == KOOPA_RVT_CALL)
            {
                ra_count = 1;
                arg_count = std::max(arg_count, std::max(0, int(inst->kind.data.call.args.len) - 8));
            }
            else if (inst->kind.tag == KOOPA_RVT_ALLOC &&
                     inst->ty->data.pointer.base->tag == KOOPA_RTT_ARRAY)
            {
                var_count--;
                int arrmem = 1;
                auto base = inst->ty->data.pointer.base;
                while (base->tag == KOOPA_RTT_ARRAY)
                {
                    ptr_size_vec.push_size(inst, base->data.array.len);
                    arrmem *= base->data.array.len;
                    base = base->data.array.base;
                }
                ptr_size_vec.push_size_vec_iter(inst);
                var_count += arrmem;
            }
        }
    }

    stack.len = (var_count + ra_count + arg_count) * 4;
    // 将栈帧长度对齐到 16
    stack.len = (stack.len + 15) / 16 * 16;
    stack.pos = arg_count * 4;

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
    // 当前块的label, %entry开头的不打印
    if (strncmp(bb->name + strlen(bb->name) - 5, "entry", 5) != 0)
    {
        std::cout << bb->name + 1 << ":" << std::endl;
    }
    // 访问所有指令
    Visit(bb->insts);
}

// 访问指令
void Visit(const koopa_raw_value_t &value)
{
    // 根据指令类型判断后续需要如何访问
    const auto &kind = value->kind;
    koopa_raw_type_t base;
    switch (kind.tag)
    {
    case KOOPA_RVT_INTEGER:
        // 访问 integer 指令
        Visit(kind.data.integer);
        break;
    case KOOPA_RVT_ALLOC:
        base = value->ty->data.pointer.base;
        if (base->tag == KOOPA_RTT_INT32)
        {
            stack.alloc_value(value, stack.pos);
            stack.pos += 4;
        }
        else if (base->tag == KOOPA_RTT_ARRAY)
        {
            int arrmem = ptr_size_vec.get_value_total_size(value);
            stack.alloc_value(value, stack.pos);
            stack.pos += arrmem;
        }
        else if (base->tag == KOOPA_RTT_POINTER)
        {
            while (base->tag == KOOPA_RTT_POINTER)
            {
                ptr_size_vec.push_size(value, 1);
                base = base->data.array.base;
            }
            while (base->tag == KOOPA_RTT_ARRAY)
            {
                ptr_size_vec.push_size(value, base->data.array.len);
                base = base->data.array.base;
            }
            ptr_size_vec.push_size_vec_iter(value);
            stack.alloc_value(value, stack.pos);
            stack.pos += 4;
        }
        break;
    case KOOPA_RVT_GLOBAL_ALLOC:
        // 访问 global alloc 指令
        Visit(value->kind.data.global_alloc, value);
        break;
    case KOOPA_RVT_LOAD:
        // 访问 load 指令
        Visit(kind.data.load, value);
        break;
    case KOOPA_RVT_STORE:
        // 访问 store 指令
        Visit(kind.data.store);
        break;
    case KOOPA_RVT_GET_PTR:
        // 访问 getptr 指令
        Visit(kind.data.get_ptr, value);
        break;
    case KOOPA_RVT_GET_ELEM_PTR:
        // 访问 getelemptr 指令
        Visit(kind.data.get_elem_ptr, value);
        break;
    case KOOPA_RVT_BINARY:
        // 访问 binary 指令
        Visit(kind.data.binary, value);
        break;
    case KOOPA_RVT_BRANCH:
        // 访问 branch 指令
        Visit(kind.data.branch);
        break;
    case KOOPA_RVT_JUMP:
        // 访问 jump 指令
        Visit(kind.data.jump);
        break;
    case KOOPA_RVT_CALL:
        // 访问 call 指令
        Visit(kind.data.call, value);
        break;
    case KOOPA_RVT_RETURN:
        // 访问 return 指令
        Visit(kind.data.ret);
        break;
    default:
        // 其他类型暂时遇不到
        assert(false);
        break;
    }
}

// 访问 return 指令
void Visit(const koopa_raw_return_t &ret)
{
    // 返回值存入 a0
    if (ret.value != nullptr)
    {
        loadstack_reg(ret.value, "a0");
    }
    // 从栈帧中恢复 ra 寄存器
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
void Visit(const koopa_raw_integer_t &integer)
{
    std::cout << "  li a0, " << integer.value << std::endl;
}

// 访问 binary 指令
void Visit(const koopa_raw_binary_t &binary, const koopa_raw_value_t &value)
{
    // 将运算数存入 t0 和 t1
    loadstack_reg(binary.lhs, "t0");
    loadstack_reg(binary.rhs, "t1");

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
    loadstack_reg(load.src, "t0");

    if (value_is_ptr(load.src))
    {
        std::cout << "  lw t0, 0(t0)" << std::endl;
    }

    if (value->ty->tag != KOOPA_RTT_UNIT)
    {
        ptr_size_vec.copy_size_vec(value, load.src);
        // 存入栈
        stack.alloc_value(value, stack.pos);
        stack.pos += 4;
        save_reg(value, "t0");
    }
}

// 访问 store 指令
void Visit(const koopa_raw_store_t &store)
{
    loadstack_reg(store.value, "t0");

    if (value_is_ptr(store.value))
    {
        std::cout << "  lw t0, 0(t0)" << std::endl;
    }
    loadaddr_reg(store.dest, "t1");
    if (value_is_ptr(store.dest))
    {
        std::cout << "  lw t1, 0(t1)" << std::endl;
    }
    std::cout << "  sw t0, 0(t1)" << std::endl;
}

// 访问 branch 指令
void Visit(const koopa_raw_branch_t &branch)
{
    loadstack_reg(branch.cond, "t0");
    std::cout << "  bnez t0, DOUBLE_JUMP_" << branch.true_bb->name + 1 << std::endl;
    std::cout << "  j " << branch.false_bb->name + 1 << std::endl;
    std::cout << "DOUBLE_JUMP_" << branch.true_bb->name + 1 << ":" << std::endl;
    std::cout << "  j " << branch.true_bb->name + 1 << std::endl;
}

// 访问 jump 指令
void Visit(const koopa_raw_jump_t &jump)
{
    std::cout << "  j " << jump.target->name + 1 << std::endl;
}

// 访问 call 指令
void Visit(const koopa_raw_call_t &call, const koopa_raw_value_t &value)
{
    // 处理参数
    for (size_t i = 0; i < call.args.len; ++i)
    {
        auto arg = reinterpret_cast<koopa_raw_value_t>(call.args.buffer[i]);
        if (i < 8)
        {
            loadstack_reg(arg, "a" + std::to_string(i));
        }
        else
        {
            loadstack_reg(arg, "t0");
            deal_offset_exceed((i - 8) * 4, "sw", "t0");
        }
    }
    // call half
    std::cout << "  call " << call.callee->name + 1 << std::endl;

    // 若有返回值则将 a0 中的结果存入栈
    if (value->ty->tag != KOOPA_RTT_UNIT)
    {
        stack.alloc_value(value, stack.pos);
        stack.pos += 4;
        save_reg(value, "a0");
    }
}

// 访问 global alloc 指令
void Visit(const koopa_raw_global_alloc_t &global_alloc, const koopa_raw_value_t &value)
{
    std::cout << "  .data" << std::endl;
    std::cout << "  .globl " << value->name + 1 << std::endl;
    std::cout << value->name + 1 << ":" << std::endl;
    if (global_alloc.init->kind.tag == KOOPA_RVT_ZERO_INIT)
    {
        // 初始化为 0
        auto base = value->ty->data.pointer.base;
        if (base->tag == KOOPA_RTT_INT32)
        {
            std::cout << "  .zero 4" << std::endl;
        }
        else if (base->tag == KOOPA_RTT_ARRAY)
        {
            int size = 4;
            while (base->tag == KOOPA_RTT_ARRAY)
            {
                ptr_size_vec.push_size(value, base->data.array.len);
                size *= base->data.array.len;
                base = base->data.array.base;
            }
            ptr_size_vec.push_size_vec_iter(value);
            std::cout << "  .zero " << size << std::endl;
        }
    }
    if (global_alloc.init->kind.tag == KOOPA_RVT_INTEGER)
    {
        std::cout << "  .word " << global_alloc.init->kind.data.integer.value << std::endl;
    }
    else if (global_alloc.init->kind.tag == KOOPA_RVT_AGGREGATE)
    {
        // 数组，初始化为 Aggregate
        auto base = value->ty->data.pointer.base;
        while (base->tag == KOOPA_RTT_ARRAY)
        {
            ptr_size_vec.push_size(value, base->data.array.len);
            base = base->data.array.base;
        }
        ptr_size_vec.push_size_vec_iter(value);
        aggregate_init(global_alloc.init);
    }
    std::cout << std::endl;
}

// 访问 getptr 指令
void Visit(const koopa_raw_get_ptr_t &get_ptr, const koopa_raw_value_t &value)
{
    loadaddr_reg(get_ptr.src, "t0");
    if (value_is_ptr(get_ptr.src))
    {
        std::cout << "  lw t0, 0(t0)" << std::endl;
    }

    int offset = ptr_size_vec.get_value_offset(get_ptr.src);
    loadstack_reg(get_ptr.index, "t1");
    loadint_reg(offset, "t2");
    std::cout << "  mul t1, t1, t2" << std::endl;
    std::cout << "  add t0, t0, t1" << std::endl;

    // 若有返回值则将 t0 中的结果存入栈
    if (value->ty->tag != KOOPA_RTT_UNIT)
    {
        ptr_size_vec.copy_size_vec_ptr(value, get_ptr.src);
        // 存入栈
        stack.alloc_value(value, stack.pos);
        stack.pos += 4;
        save_reg(value, "t0");
    }
}

// 访问 getelemptr 指令
void Visit(const koopa_raw_get_elem_ptr_t &get_elem_ptr, const koopa_raw_value_t &value)
{
    loadaddr_reg(get_elem_ptr.src, "t0");
    if (value_is_ptr(get_elem_ptr.src))
    {
        std::cout << "  lw t0, 0(t0)" << std::endl;
    }

    int offset = ptr_size_vec.get_value_offset(get_elem_ptr.src);
    loadstack_reg(get_elem_ptr.index, "t1");
    loadint_reg(offset, "t2");
    std::cout << "  mul t1, t1, t2" << std::endl;
    std::cout << "  add t0, t0, t1" << std::endl;

    // 若有返回值则将 t0 中的结果存入栈
    if (value->ty->tag != KOOPA_RTT_UNIT)
    {
        ptr_size_vec.copy_size_vec_ptr(value, get_elem_ptr.src);
        // 存入栈
        stack.alloc_value(value, stack.pos);
        stack.pos += 4;
        save_reg(value, "t0");
    }
}

/**********************************************************************************************************/
/************************************************Utils*****************************************************/
/**********************************************************************************************************/

void loadstack_reg(const koopa_raw_value_t &value, const std::string &reg)
{
    int index;
    switch (value->kind.tag)
    {
    case KOOPA_RVT_INTEGER:
        loadint_reg(value->kind.data.integer.value, reg);
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
        // 一定保存在栈里, 或者是全局符号
        deal_offset_exceed(stack.get_loc(value), "lw", reg);
        break;
    }
}

// 将 value 的值放置在标号为 reg 的寄存器中
void loadint_reg(int value, const std::string &reg)
{
    std::cout << "  li " << reg << ", " << value << std::endl;
}

// 将 value 的地址放置在标号为 reg 的寄存器中
void loadaddr_reg(const koopa_raw_value_t &value, const std::string &reg)
{
    int index;
    switch (value->kind.tag)
    {
    case KOOPA_RVT_FUNC_ARG_REF:
        index = value->kind.data.func_arg_ref.index;
        assert(index >= 8);
        loadint_reg(stack.len + (index - 8) * 4, reg);
        std::cout << "  add " << reg << ", " << reg << ", sp" << std::endl;
        break;
    case KOOPA_RVT_GLOBAL_ALLOC:
        std::cout << "  la " << reg << ", " << value->name + 1 << std::endl;
        break;
    default:
        loadint_reg(stack.get_loc(value), reg);
        std::cout << "  add " << reg << ", " << reg << ", sp" << std::endl;
        break;
    }
}

// 将标号为 reg 的寄存器中的value的值保存在内存中
void save_reg(const koopa_raw_value_t &value, const std::string &reg)
{
    assert(value->kind.tag != KOOPA_RVT_INTEGER);
    int offset;
    int index;
    switch (value->kind.tag)
    {
    case KOOPA_RVT_FUNC_ARG_REF:
        index = value->kind.data.func_arg_ref.index;
        assert(index >= 8);
        deal_offset_exceed(stack.len + (index - 8) * 4, "sw", reg);
        break;
    case KOOPA_RVT_GLOBAL_ALLOC:
        std::cout << "  la " << reg << ", " << value->name + 1 << std::endl;
        std::cout << "  sw " << reg << ", 0(t1)" << std::endl;
        break;
    default:
        offset = stack.get_loc(value);
        deal_offset_exceed(offset, "sw", reg);
        break;
    }
}

// 遍历 agg 并输出为一系列 .word 格式
void aggregate_init(const koopa_raw_value_t &value)
{
    if (value->kind.tag == KOOPA_RVT_INTEGER)
    {
        // 到叶子了
        std::cout << "  .word " << value->kind.data.integer.value << std::endl;
    }
    else if (value->kind.tag == KOOPA_RVT_AGGREGATE)
    {
        const auto &agg = value->kind.data.aggregate;
        for (int i = 0; i < agg.elems.len; i++)
        {
            aggregate_init(reinterpret_cast<koopa_raw_value_t>(agg.elems.buffer[i]));
        }
    }
}

// value->kind.tag 为 KOOPA_RVT_GET_PTR 或 KOOPA_RVT_GET_ELEM_PTR,
// 或者是 KOOPA_RVT_GET_LOAD 且 load 的是函数开始保存数组参数的位置
bool value_is_ptr(const koopa_raw_value_t &value)
{
    if (value->kind.tag == KOOPA_RVT_GET_PTR)
        return 1;
    if (value->kind.tag == KOOPA_RVT_GET_ELEM_PTR)
        return 1;
    if (value->kind.tag == KOOPA_RVT_LOAD)
    {
        const auto &load = value->kind.data.load;
        if (load.src->kind.tag == KOOPA_RVT_ALLOC)
            if (load.src->ty->data.pointer.base->tag == KOOPA_RTT_POINTER)
                return 1;
    }
    return 0;
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
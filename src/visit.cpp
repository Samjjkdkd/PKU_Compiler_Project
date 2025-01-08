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

int PtrSizeVec::get_value_total_size(koopa_raw_value_t value)
{
    int offset = 4;
    for (auto i : size_vec[value])
    {
        offset *= i;
    }
    return offset;
}

int PtrSizeVec::get_value_offset(koopa_raw_value_t src)
{
    auto it = size_vec[src].begin();
    ++it;
    int offset = 4;
    for (; it != size_vec[src].end(); it++)
    {
        offset *= (*it);
    }
    return offset;
}

void PtrSizeVec::push_size(koopa_raw_value_t value, int size)
{
    size_vec[value].push_back(size);
}

void PtrSizeVec::copy_size_vec(koopa_raw_value_t dest, koopa_raw_value_t src)
{
    if (size_vec.find(src) != size_vec.end())
    {
        size_vec[dest] = size_vec[src];
    }
}

void PtrSizeVec::copy_size_vec_ptr(koopa_raw_value_t dest, koopa_raw_value_t src)
{
    auto it = size_vec[src].begin();
    it++;
    std::vector<int> tmp;
    for (; it != size_vec[src].end(); it++)
    {
        tmp.push_back(*it);
    }
    size_vec[dest] = tmp;
}

/**********************************************************************************************************/
/************************************************Visit*****************************************************/
/**********************************************************************************************************/

// 访问 raw program
std::string Visit(const koopa_raw_program_t &program)
{
    // 执行一些其他的必要操作
    // ...
    // 访问所有全局变量
    std::string ret = "";

    ret += Visit(program.values);
    // 访问所有函数
    ret += Visit(program.funcs);

    return ret;
}

// 访问 raw slice
std::string Visit(const koopa_raw_slice_t &slice)
{
    std::string ret = "";
    for (size_t i = 0; i < slice.len; ++i)
    {
        auto ptr = slice.buffer[i];
        // 根据 slice 的 kind 决定将 ptr 视作何种元素
        switch (slice.kind)
        {
        case KOOPA_RSIK_FUNCTION:
            // 访问函数
            ret += Visit(reinterpret_cast<koopa_raw_function_t>(ptr));
            break;
        case KOOPA_RSIK_BASIC_BLOCK:
            // 访问基本块
            ret += Visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr));
            break;
        case KOOPA_RSIK_VALUE:
            // 访问指令
            ret += Visit(reinterpret_cast<koopa_raw_value_t>(ptr));
            break;
        default:
            // 我们暂时不会遇到其他内容, 于是不对其做任何处理
            assert(false);
        }
    }
    return ret;
}

// 访问函数
std::string Visit(const koopa_raw_function_t &func)
{
    // 忽略函数声明
    std::string ret = "";
    if (func->bbs.len == 0)
        return ret;

    // 执行一些其他的必要操作
    ret += "  .text\n";
    ret += "  .globl " + std::string(func->name + 1) + "\n";
    ret += std::string(func->name + 1) + ":\n";

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
        ret += deal_offset_exceed(stack.len, "addi-", "t0");
    }

    if (ra_count)
    {
        ret += deal_offset_exceed(stack.len - 4, "sw", "ra");
    }

    // 访问所有基本块
    ret += Visit(func->bbs);
    ret += "\n";
    return ret;
}

// 访问基本块
std::string Visit(const koopa_raw_basic_block_t &bb)
{
    std::string ret = "";
    // 执行一些其他的必要操作
    // 当前块的label, %entry开头的不打印
    if (strncmp(bb->name + strlen(bb->name) - 5, "entry", 5) != 0)
    {
        ret += std::string(bb->name + 1) + ":\n";
    }
    // 访问所有指令
    ret += Visit(bb->insts);
    return ret;
}

// 访问指令
std::string Visit(const koopa_raw_value_t &value)
{
    // 根据指令类型判断后续需要如何访问
    std::string ret = "";
    const auto &kind = value->kind;
    koopa_raw_type_t base;
    switch (kind.tag)
    {
    case KOOPA_RVT_INTEGER:
        // 访问 integer 指令
        ret += Visit(kind.data.integer);
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
            stack.alloc_value(value, stack.pos);
            stack.pos += 4;
        }
        break;
    case KOOPA_RVT_GLOBAL_ALLOC:
        // 访问 global alloc 指令
        ret += Visit(value->kind.data.global_alloc, value);
        break;
    case KOOPA_RVT_LOAD:
        // 访问 load 指令
        ret += Visit(kind.data.load, value);
        break;
    case KOOPA_RVT_STORE:
        // 访问 store 指令
        ret += Visit(kind.data.store);
        break;
    case KOOPA_RVT_GET_PTR:
        // 访问 getptr 指令
        ret += Visit(kind.data.get_ptr, value);
        break;
    case KOOPA_RVT_GET_ELEM_PTR:
        // 访问 getelemptr 指令
        ret += Visit(kind.data.get_elem_ptr, value);
        break;
    case KOOPA_RVT_BINARY:
        // 访问 binary 指令
        ret += Visit(kind.data.binary, value);
        break;
    case KOOPA_RVT_BRANCH:
        // 访问 branch 指令
        ret += Visit(kind.data.branch);
        break;
    case KOOPA_RVT_JUMP:
        // 访问 jump 指令
        ret += Visit(kind.data.jump);
        break;
    case KOOPA_RVT_CALL:
        // 访问 call 指令
        ret += Visit(kind.data.call, value);
        break;
    case KOOPA_RVT_RETURN:
        // 访问 return 指令
        ret += Visit(kind.data.ret);
        break;
    default:
        // 其他类型暂时遇不到
        assert(false);
        break;
    }
    return ret;
}

// 访问 return 指令
std::string Visit(const koopa_raw_return_t &ret_inst)
{
    std::string ret = "";
    // 返回值存入 a0
    if (ret_inst.value != nullptr)
    {
        ret += loadstack_reg(ret_inst.value, "a0");
    }
    // 从栈帧中恢复 ra 寄存器
    if (ra_count)
    {
        ret += deal_offset_exceed(stack.len - 4, "lw", "ra");
    }
    // 恢复栈帧
    if (stack.len != 0)
    {
        ret += deal_offset_exceed(stack.len, "addi+", "t0");
    }
    ret += "  ret\n";
    return ret;
}

// 访问 integer 指令
std::string Visit(const koopa_raw_integer_t &integer)
{
    return "  li a0, " + std::to_string(integer.value) + "\n";
}

// 访问 binary 指令
std::string Visit(const koopa_raw_binary_t &binary, const koopa_raw_value_t &value)
{
    std::string ret = "";
    // 将运算数存入 t0 和 t1
    ret += loadstack_reg(binary.lhs, "t0");
    ret += loadstack_reg(binary.rhs, "t1");

    // 进行运算
    switch (binary.op)
    {
    case KOOPA_RBO_ADD:
        ret += "  add t0, t0, t1\n";
        break;
    case KOOPA_RBO_SUB:
        ret += "  sub t0, t0, t1\n";
        break;
    case KOOPA_RBO_MUL:
        ret += "  mul t0, t0, t1\n";
        break;
    case KOOPA_RBO_DIV:
        ret += "  div t0, t0, t1\n";
        break;
    case KOOPA_RBO_MOD:
        ret += "  rem t0, t0, t1\n";
        break;
    case KOOPA_RBO_AND:
        ret += "  and t0, t0, t1\n";
        break;
    case KOOPA_RBO_OR:
        ret += "  or t0, t0, t1\n";
        break;
    case KOOPA_RBO_XOR:
        ret += "  xor t0, t0, t1\n";
        break;
    case KOOPA_RBO_SHL:
        ret += "  sll t0, t0, t1\n";
        break;
    case KOOPA_RBO_SHR:
        ret += "  srl t0, t0, t1\n";
        break;
    case KOOPA_RBO_SAR:
        ret += "  sra t0, t0, t1\n";
        break;
    case KOOPA_RBO_EQ:
        ret += "  xor t0, t0, t1\n";
        ret += "  seqz t0, t0\n";
        break;
    case KOOPA_RBO_NOT_EQ:
        ret += "  xor t0, t0, t1\n";
        ret += "  snez t0, t0\n";
        break;
    case KOOPA_RBO_GT:
        ret += "  slt t0, t1, t0\n";
        break;
    case KOOPA_RBO_LT:
        ret += "  slt t0, t0, t1\n";
        break;
    case KOOPA_RBO_GE:
        ret += "  slt t0, t0, t1\n";
        ret += "  xori t0, t0, 1\n";
        break;
    case KOOPA_RBO_LE:
        ret += "  slt t0, t1, t0\n";
        ret += "  xori t0, t0, 1\n";
        break;
    }

    // 将结果存回
    if (value->ty->tag != KOOPA_RTT_UNIT)
    {
        stack.alloc_value(value, stack.pos);
        stack.pos += 4;
        ret += save_reg(value, "t0");
    }
    return ret;
}

// 访问 load 指令
std::string Visit(const koopa_raw_load_t &load, const koopa_raw_value_t &value)
{
    std::string ret = "";
    ret += loadstack_reg(load.src, "t0");

    if (value_is_ptr(load.src))
    {
        ret += "  lw t0, 0(t0)\n";
    }

    if (value->ty->tag != KOOPA_RTT_UNIT)
    {
        ptr_size_vec.copy_size_vec(value, load.src);
        // 存入栈
        stack.alloc_value(value, stack.pos);
        stack.pos += 4;
        ret += save_reg(value, "t0");
    }
    return ret;
}

// 访问 store 指令
std::string Visit(const koopa_raw_store_t &store)
{
    std::string ret = "";
    ret += loadstack_reg(store.value, "t0");

    if (value_is_ptr(store.value))
    {
        ret += "  lw t0, 0(t0)\n";
    }
    ret += loadaddr_reg(store.dest, "t1");
    if (value_is_ptr(store.dest))
    {
        ret += "  lw t1, 0(t1)\n";
    }
    ret += "  sw t0, 0(t1)\n";
    return ret;
}

// 访问 branch 指令
std::string Visit(const koopa_raw_branch_t &branch)
{
    std::string ret = "";
    ret += loadstack_reg(branch.cond, "t0");
    ret += "  bnez t0, DOUBLE_JUMP_" + std::string(branch.true_bb->name + 1) + "\n";
    ret += "  j " + std::string(branch.false_bb->name + 1) + "\n";
    ret += "DOUBLE_JUMP_" + std::string(branch.true_bb->name + 1) + ":\n";
    ret += "  j " + std::string(branch.true_bb->name + 1) + "\n";
    return ret;
}

// 访问 jump 指令
std::string Visit(const koopa_raw_jump_t &jump)
{
    std::string ret = "";
    ret += "  j " + std::string(jump.target->name + 1) + "\n";
    return ret;
}

// 访问 call 指令
std::string Visit(const koopa_raw_call_t &call, const koopa_raw_value_t &value)
{
    std::string ret = "";
    // 处理参数
    for (size_t i = 0; i < call.args.len; ++i)
    {
        auto arg = reinterpret_cast<koopa_raw_value_t>(call.args.buffer[i]);
        if (i < 8)
        {
            ret += loadstack_reg(arg, "a" + std::to_string(i));
        }
        else
        {
            ret += loadstack_reg(arg, "t0");
            ret += deal_offset_exceed((i - 8) * 4, "sw", "t0");
        }
    }
    // call half
    ret += "  call " + std::string(call.callee->name + 1) + "\n";
    // 若有返回值则将 a0 中的结果存入栈
    if (value->ty->tag != KOOPA_RTT_UNIT)
    {
        stack.alloc_value(value, stack.pos);
        stack.pos += 4;
        ret += save_reg(value, "a0");
    }
    return ret;
}

// 访问 global alloc 指令
std::string Visit(const koopa_raw_global_alloc_t &global_alloc, const koopa_raw_value_t &value)
{
    std::string ret = "";
    ret += "  .data\n";
    ret += "  .globl " + std::string(value->name + 1) + "\n";
    ret += std::string(value->name + 1) + ":\n";
    if (global_alloc.init->kind.tag == KOOPA_RVT_ZERO_INIT)
    {
        // 初始化为 0
        auto base = value->ty->data.pointer.base;
        if (base->tag == KOOPA_RTT_INT32)
        {
            ret += "  .zero 4\n";
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
            ret += "  .zero " + std::to_string(size) + "\n";
        }
    }
    if (global_alloc.init->kind.tag == KOOPA_RVT_INTEGER)
    {
        ret += "  .word " + std::to_string(global_alloc.init->kind.data.integer.value) + "\n";
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
        ret += aggregate_init(global_alloc.init);
    }
    ret += "\n";
    return ret;
}

// 访问 getptr 指令
std::string Visit(const koopa_raw_get_ptr_t &get_ptr, const koopa_raw_value_t &value)
{
    std::string ret = "";
    ret += loadaddr_reg(get_ptr.src, "t0");
    if (value_is_ptr(get_ptr.src))
    {
        ret += "  lw t0, 0(t0)\n";
    }

    int offset = ptr_size_vec.get_value_offset(get_ptr.src);
    ret += loadstack_reg(get_ptr.index, "t1");
    ret += loadint_reg(offset, "t2");
    ret += "  mul t1, t1, t2\n";
    ret += "  add t0, t0, t1\n";

    // 若有返回值则将 t0 中的结果存入栈
    if (value->ty->tag != KOOPA_RTT_UNIT)
    {
        ptr_size_vec.copy_size_vec_ptr(value, get_ptr.src);
        // 存入栈
        stack.alloc_value(value, stack.pos);
        stack.pos += 4;
        ret += save_reg(value, "t0");
    }
    return ret;
}

// 访问 getelemptr 指令
std::string Visit(const koopa_raw_get_elem_ptr_t &get_elem_ptr, const koopa_raw_value_t &value)
{
    std::string ret = "";
    ret += loadaddr_reg(get_elem_ptr.src, "t0");
    if (value_is_ptr(get_elem_ptr.src))
    {
        ret += "  lw t0, 0(t0)\n";
    }

    int offset = ptr_size_vec.get_value_offset(get_elem_ptr.src);
    ret += loadstack_reg(get_elem_ptr.index, "t1");
    ret += loadint_reg(offset, "t2");
    ret += "  mul t1, t1, t2\n";
    ret += "  add t0, t0, t1\n";

    // 若有返回值则将 t0 中的结果存入栈
    if (value->ty->tag != KOOPA_RTT_UNIT)
    {
        ptr_size_vec.copy_size_vec_ptr(value, get_elem_ptr.src);
        // 存入栈
        stack.alloc_value(value, stack.pos);
        stack.pos += 4;
        ret += save_reg(value, "t0");
    }
    return ret;
}

/**********************************************************************************************************/
/************************************************Utils*****************************************************/
/**********************************************************************************************************/

std::string loadstack_reg(const koopa_raw_value_t &value, const std::string &reg)
{
    int index;
    std::string ret = "";
    switch (value->kind.tag)
    {
    case KOOPA_RVT_INTEGER:
        ret += loadint_reg(value->kind.data.integer.value, reg);
        break;
    case KOOPA_RVT_FUNC_ARG_REF:
        index = value->kind.data.func_arg_ref.index;
        if (index < 8)
        {
            ret += "  mv " + reg + ", a" + std::to_string(index) + "\n";
        }
        else
        {
            ret += deal_offset_exceed(stack.len + (index - 8) * 4, "lw", reg);
        }
        break;
    case KOOPA_RVT_GLOBAL_ALLOC:
        ret += "  la t1, " + std::string(value->name + 1) + "\n";
        ret += "  lw " + reg + ", 0(t1)\n";
        break;
    default:
        // 一定保存在栈里, 或者是全局符号
        ret += deal_offset_exceed(stack.get_loc(value), "lw", reg);
        break;
    }
    return ret;
}

// 将 value 的值放置在标号为 reg 的寄存器中
std::string loadint_reg(int value, const std::string &reg)
{
    std::string ret = "";
    ret += "  li " + reg + ", " + std::to_string(value) + "\n";
    return ret;
}

// 将 value 的地址放置在标号为 reg 的寄存器中
std::string loadaddr_reg(const koopa_raw_value_t &value, const std::string &reg)
{
    std::string ret = "";
    int index;
    switch (value->kind.tag)
    {
    case KOOPA_RVT_FUNC_ARG_REF:
        index = value->kind.data.func_arg_ref.index;
        assert(index >= 8);
        ret += loadint_reg(stack.len + (index - 8) * 4, reg);
        ret += "  add " + reg + ", " + reg + ", sp\n";
        break;
    case KOOPA_RVT_GLOBAL_ALLOC:
        ret += "  la " + reg + ", " + std::string(value->name + 1) + "\n";
        break;
    default:
        ret += loadint_reg(stack.get_loc(value), reg);
        ret += "  add " + reg + ", " + reg + ", sp\n";
        break;
    }
    return ret;
}

// 将标号为 reg 的寄存器中的value的值保存在内存中
std::string save_reg(const koopa_raw_value_t &value, const std::string &reg)
{
    std::string ret = "";
    assert(value->kind.tag != KOOPA_RVT_INTEGER);
    int offset;
    int index;
    switch (value->kind.tag)
    {
    case KOOPA_RVT_FUNC_ARG_REF:
        index = value->kind.data.func_arg_ref.index;
        assert(index >= 8);
        ret += deal_offset_exceed(stack.len + (index - 8) * 4, "sw", reg);
        break;
    case KOOPA_RVT_GLOBAL_ALLOC:
        ret += "  la t1, " + std::string(value->name + 1) + "\n";
        ret += "  sw " + reg + ", 0(t1)\n";
        break;
    default:
        offset = stack.get_loc(value);
        ret += deal_offset_exceed(offset, "sw", reg);
        break;
    }
    return ret;
}

// 遍历 agg 并输出为一系列 .word 格式
std::string aggregate_init(const koopa_raw_value_t &value)
{
    std::string ret = "";
    if (value->kind.tag == KOOPA_RVT_INTEGER)
    {
        // 到叶子了
        ret += "  .word " + std::to_string(value->kind.data.integer.value) + "\n";
    }
    else if (value->kind.tag == KOOPA_RVT_AGGREGATE)
    {
        const auto &agg = value->kind.data.aggregate;
        for (int i = 0; i < agg.elems.len; i++)
        {
            ret += aggregate_init(reinterpret_cast<koopa_raw_value_t>(agg.elems.buffer[i]));
        }
    }
    return ret;
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

std::string deal_offset_exceed(int offset, std::string inst, std::string reg)
{
    std::string ret = "";
    if (inst == "lw" || inst == "sw")
    {
        if (offset < -2048 || offset > 2047)
        {
            int new_base_offset = offset & ~0x7FF;
            int remaining_offset = offset & 0x7FF;

            if (new_base_offset < -2048 || new_base_offset > 2047)
            {
                ret += "  li t1, " + std::to_string(new_base_offset) + "\n";
                ret += "  add t1, t1, sp\n";
            }
            else
            {
                ret += "  addi t1, sp, " + std::to_string(new_base_offset) + "\n";
            }
            ret += "  " + inst + " " + reg + ", " + std::to_string(remaining_offset) + "(t1)\n";
        }
        else
        {
            ret += "  " + inst + " " + reg + ", " + std::to_string(offset) + "(sp)\n";
        }
    }
    else if (inst == "addi-")
    {
        if (offset < -2048 || offset > 2047)
        {
            ret += "  li " + reg + ", " + std::to_string(offset) + "\n";
            ret += "  sub sp, sp, " + reg + "\n";
        }
        else
        {
            ret += "  addi sp, sp, -" + std::to_string(offset) + "\n";
        }
    }
    else if (inst == "addi+")
    {
        if (offset < -2048 || offset > 2047)
        {
            ret += "  li " + reg + ", " + std::to_string(offset) + "\n";
            ret += "  add sp, sp, " + reg + "\n";
        }
        else
        {
            ret += "  addi sp, sp, " + std::to_string(offset) + "\n";
        }
    }
    return ret;
}
#include "ast.h"

void *CompUnitAST::GenerateIR() const
{
    // generate the function definitions
    std::vector<const void *> funcs{func_def->GenerateIR()};
    koopa_raw_program_t *ret = new koopa_raw_program_t();
    // initialize the global values
    ret->values = slice(KOOPA_RSIK_VALUE);
    // initialize the function definitions
    ret->funcs = slice(funcs, KOOPA_RSIK_FUNCTION);
    return ret;
}

void *FuncDefAST::GenerateIR() const
{
    koopa_raw_function_data_t *ret = new koopa_raw_function_data_t();

    // generate the function type
    koopa_raw_type_kind_t *ty = new koopa_raw_type_kind_t();
    // the function type is a function
    ty->tag = KOOPA_RTT_FUNCTION;
    // the function has no parameters
    ty->data.function.params = slice(KOOPA_RSIK_TYPE);
    // generate the function return type
    ty->data.function.ret = (const struct koopa_raw_type_kind *)func_type->GenerateIR();
    // set the function type
    ret->ty = ty;

    // set the function name
    char *name = new char[ident.length() + 1];
    ("@" + ident).copy(name, ident.length() + 1);
    ret->name = name;

    // initialize params
    ret->params = slice(KOOPA_RSIK_VALUE);
    // generate the function body
    std::vector<const void *> blocks{block->GenerateIR()};
    // set the basic blocks
    ret->bbs = slice(blocks, KOOPA_RSIK_BASIC_BLOCK);

    return ret;
}

void *FuncTypeAST::GenerateIR() const
{
    // the function returns an integer
    if (func_type == "int")
        return (void *)type_kind(KOOPA_RTT_INT32);
    return nullptr; // not implemented
}

void *BlockAST::GenerateIR() const
{
    koopa_raw_basic_block_data_t *ret = new koopa_raw_basic_block_data_t();
    // the block name
    ret->name = "\%entry";
    // the block parameters
    ret->params = slice(KOOPA_RSIK_VALUE);
    // the block used by
    ret->used_by = slice(KOOPA_RSIK_VALUE);

    std::vector<const void *> stmts;
    stmt->GenerateIR(stmts);
    ret->insts = slice(stmts, KOOPA_RSIK_VALUE);

    return ret;
}

void *StmtAST::GenerateIR(std::vector<const void *> &inst_buf) const
{
    koopa_raw_value_data *ret = new koopa_raw_value_data();
    koopa_raw_slice_t used_by = slice(ret, KOOPA_RSIK_VALUE);
    ret->ty = type_kind(KOOPA_RTT_UNIT);
    ret->name = nullptr;
    ret->used_by = slice(KOOPA_RSIK_VALUE);
    ret->kind.tag = KOOPA_RVT_RETURN;
    ret->kind.data.ret.value =
        (const koopa_raw_value_data *)exp->GenerateIR(used_by, inst_buf);
    inst_buf.push_back(ret);
    return ret;
}

void *ExpAST::GenerateIR(koopa_raw_slice_t parent,
                         std::vector<const void *> &inst_buf) const
{
    return lor_exp->GenerateIR(parent, inst_buf);
}

void *PrimaryExpAST::GenerateIR(koopa_raw_slice_t parent,
                                std::vector<const void *> &inst_buf) const
{
    if (type == 1)
        return exp->GenerateIR(parent, inst_buf);
    koopa_raw_value_data *ret = new koopa_raw_value_data();
    ret->ty = type_kind(KOOPA_RTT_INT32);
    ret->name = nullptr;
    ret->used_by = parent;
    ret->kind.tag = KOOPA_RVT_INTEGER;
    ret->kind.data.integer.value = number;
    return ret;
}

void *UnaryExpAST::GenerateIR(koopa_raw_slice_t parent,
                              std::vector<const void *> &inst_buf) const
{
    if (type == 1)
        return primary_exp->GenerateIR(parent, inst_buf);
    if (unary_op == "+")
        return unary_exp->GenerateIR(parent, inst_buf);

    koopa_raw_value_data *ret = new koopa_raw_value_data();
    koopa_raw_slice_t used_by = slice(ret, KOOPA_RSIK_VALUE);

    koopa_raw_value_data *zero = new koopa_raw_value_data();
    zero->ty = type_kind(KOOPA_RTT_INT32);
    zero->name = nullptr;
    zero->used_by = used_by;
    zero->kind.tag = KOOPA_RVT_INTEGER;
    zero->kind.data.integer.value = 0;

    auto &binary = ret->kind.data.binary;
    binary.lhs = (koopa_raw_value_t)zero;

    ret->ty = type_kind(KOOPA_RTT_INT32);
    ret->name = nullptr;
    ret->used_by = parent;
    ret->kind.tag = KOOPA_RVT_BINARY;
    if (unary_op == "-")
        binary.op = KOOPA_RBO_SUB;
    else if (unary_op == "!")
        binary.op = KOOPA_RBO_EQ;
    binary.rhs = (koopa_raw_value_t)unary_exp->GenerateIR(parent, inst_buf);
    inst_buf.push_back(ret);
    return ret;
}

void *AddExpAST::GenerateIR(koopa_raw_slice_t parent,
                            std::vector<const void *> &inst_buf) const
{
    if (type == 1)
        return mul_exp->GenerateIR(parent, inst_buf);
    koopa_raw_value_data *ret = new koopa_raw_value_data();
    koopa_raw_slice_t used_by = slice(ret, KOOPA_RSIK_VALUE);
    ret->ty = type_kind(KOOPA_RTT_INT32);
    ret->name = nullptr;
    ret->used_by = parent;
    ret->kind.tag = KOOPA_RVT_BINARY;
    auto &binary = ret->kind.data.binary;
    if (add_op == "+")
    {
        binary.op = KOOPA_RBO_ADD;
    }
    if (add_op == "-")
    {
        binary.op = KOOPA_RBO_SUB;
    }
    binary.lhs = (koopa_raw_value_t)add_exp->GenerateIR(used_by, inst_buf);
    binary.rhs = (koopa_raw_value_t)mul_exp->GenerateIR(used_by, inst_buf);
    inst_buf.push_back(ret);
    return ret;
}

void *MulExpAST::GenerateIR(koopa_raw_slice_t parent,
                            std::vector<const void *> &inst_buf) const
{
    if (type == 1)
        return unary_exp->GenerateIR(parent, inst_buf);
    koopa_raw_value_data *ret = new koopa_raw_value_data();
    koopa_raw_slice_t used_by = slice(ret, KOOPA_RSIK_VALUE);
    ret->ty = type_kind(KOOPA_RTT_INT32);
    ret->name = nullptr;
    ret->used_by = parent;
    ret->kind.tag = KOOPA_RVT_BINARY;
    auto &binary = ret->kind.data.binary;
    if (mul_op == "*")
    {
        binary.op = KOOPA_RBO_MUL;
    }
    if (mul_op == "/")
    {
        binary.op = KOOPA_RBO_DIV;
    }
    if (mul_op == "%")
    {
        binary.op = KOOPA_RBO_MOD;
    }
    binary.lhs = (koopa_raw_value_t)mul_exp->GenerateIR(used_by, inst_buf);
    binary.rhs = (koopa_raw_value_t)unary_exp->GenerateIR(used_by, inst_buf);
    inst_buf.push_back(ret);
    return ret;
}

void *RelExpAST::GenerateIR(koopa_raw_slice_t parent,
                            std::vector<const void *> &inst_buf) const
{
    if (type == 1)
        return add_exp->GenerateIR(parent, inst_buf);
    koopa_raw_value_data *ret = new koopa_raw_value_data();
    koopa_raw_slice_t used_by = slice(ret, KOOPA_RSIK_VALUE);
    ret->ty = type_kind(KOOPA_RTT_INT32);
    ret->name = nullptr;
    ret->used_by = parent;
    ret->kind.tag = KOOPA_RVT_BINARY;
    auto &binary = ret->kind.data.binary;
    if (rel_op == "<")
    {
        binary.op = KOOPA_RBO_LT;
    }
    if (rel_op == ">")
    {
        binary.op = KOOPA_RBO_GT;
    }
    if (rel_op == "<=")
    {
        binary.op = KOOPA_RBO_LE;
    }
    if (rel_op == ">=")
    {
        binary.op = KOOPA_RBO_GE;
    }
    binary.lhs = (koopa_raw_value_t)rel_exp->GenerateIR(used_by, inst_buf);
    binary.rhs = (koopa_raw_value_t)add_exp->GenerateIR(used_by, inst_buf);
    inst_buf.push_back(ret);
    return ret;
}

void *EqExpAST::GenerateIR(koopa_raw_slice_t parent,
                           std::vector<const void *> &inst_buf) const
{
    if (type == 1)
        return rel_exp->GenerateIR(parent, inst_buf);
    koopa_raw_value_data *ret = new koopa_raw_value_data();
    koopa_raw_slice_t used_by = slice(ret, KOOPA_RSIK_VALUE);
    ret->ty = type_kind(KOOPA_RTT_INT32);
    ret->name = nullptr;
    ret->used_by = parent;
    ret->kind.tag = KOOPA_RVT_BINARY;
    auto &binary = ret->kind.data.binary;
    if (eq_op == "==")
    {
        binary.op = KOOPA_RBO_EQ;
    }
    if (eq_op == "!=")
    {
        binary.op = KOOPA_RBO_NOT_EQ;
    }
    binary.lhs = (koopa_raw_value_t)eq_exp->GenerateIR(used_by, inst_buf);
    binary.rhs = (koopa_raw_value_t)rel_exp->GenerateIR(used_by, inst_buf);
    inst_buf.push_back(ret);
    return ret;
}

void *LAndExpAST::make_bool(koopa_raw_slice_t parent,
                            std::vector<const void *> &inst_buf,
                            koopa_raw_value_t exp) const
{
    // A != 0
    koopa_raw_value_data *ret = new koopa_raw_value_data();
    koopa_raw_slice_t used_by = slice(ret, KOOPA_RSIK_VALUE);
    ret->ty = type_kind(KOOPA_RTT_INT32);
    ret->name = nullptr;
    ret->used_by = parent;
    ret->kind.tag = KOOPA_RVT_BINARY;
    auto &binary = ret->kind.data.binary;
    binary.op = KOOPA_RBO_NOT_EQ;
    binary.lhs = exp;
    koopa_raw_value_data *zero = new koopa_raw_value_data();
    zero->ty = type_kind(KOOPA_RTT_INT32);
    zero->name = nullptr;
    zero->used_by = used_by;
    zero->kind.tag = KOOPA_RVT_INTEGER;
    zero->kind.data.integer.value = 0;
    binary.rhs = (koopa_raw_value_t)zero;
    inst_buf.push_back(ret);
    return ret;
}

void *LAndExpAST::GenerateIR(koopa_raw_slice_t parent,
                             std::vector<const void *> &inst_buf) const
{
    if (type == 1)
        return eq_exp->GenerateIR(parent, inst_buf);
    koopa_raw_value_data *ret = new koopa_raw_value_data();
    koopa_raw_slice_t used_by = slice(ret, KOOPA_RSIK_VALUE);
    ret->ty = type_kind(KOOPA_RTT_INT32);
    ret->name = nullptr;
    ret->used_by = parent;
    ret->kind.tag = KOOPA_RVT_BINARY;
    auto &binary = ret->kind.data.binary;
    binary.op = KOOPA_RBO_AND;
    binary.lhs = (koopa_raw_value_t)make_bool(used_by, inst_buf, (koopa_raw_value_t)land_exp->GenerateIR(used_by, inst_buf));
    binary.rhs = (koopa_raw_value_t)make_bool(used_by, inst_buf, (koopa_raw_value_t)eq_exp->GenerateIR(used_by, inst_buf));
    inst_buf.push_back(ret);
    return ret;
}

void *LOrExpAST::make_bool(koopa_raw_slice_t parent,
                           std::vector<const void *> &inst_buf,
                           koopa_raw_value_t exp) const
{
    koopa_raw_value_data *ret = new koopa_raw_value_data();
    koopa_raw_slice_t used_by = slice(ret, KOOPA_RSIK_VALUE);
    ret->ty = type_kind(KOOPA_RTT_INT32);
    ret->name = nullptr;
    ret->used_by = parent;
    ret->kind.tag = KOOPA_RVT_BINARY;
    auto &binary = ret->kind.data.binary;
    binary.op = KOOPA_RBO_NOT_EQ;
    binary.lhs = exp;
    koopa_raw_value_data *zero = new koopa_raw_value_data();
    zero->ty = type_kind(KOOPA_RTT_INT32);
    zero->name = nullptr;
    zero->used_by = used_by;
    zero->kind.tag = KOOPA_RVT_INTEGER;
    zero->kind.data.integer.value = 0;
    binary.rhs = (koopa_raw_value_t)zero;
    inst_buf.push_back(ret);
    return ret;
}

void *LOrExpAST::GenerateIR(koopa_raw_slice_t parent,
                            std::vector<const void *> &inst_buf) const
{
    if (type == 1)
        return land_exp->GenerateIR(parent, inst_buf);
    koopa_raw_value_data *ret = new koopa_raw_value_data();
    koopa_raw_slice_t used_by = slice(ret, KOOPA_RSIK_VALUE);
    ret->ty = type_kind(KOOPA_RTT_INT32);
    ret->name = nullptr;
    ret->used_by = parent;
    ret->kind.tag = KOOPA_RVT_BINARY;
    auto &binary = ret->kind.data.binary;
    binary.op = KOOPA_RBO_OR;
    binary.lhs = (koopa_raw_value_t)make_bool(used_by, inst_buf, (koopa_raw_value_t)lor_exp->GenerateIR(used_by, inst_buf));
    binary.rhs = (koopa_raw_value_t)make_bool(used_by, inst_buf, (koopa_raw_value_t)land_exp->GenerateIR(used_by, inst_buf));
    inst_buf.push_back(ret);
    return ret;
}
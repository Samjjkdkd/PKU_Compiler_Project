#include "ast.h"

void *CompUnitAST::GenerateIR() const
{
    std::vector<const void *> funcs{func_def->GenerateIR()};
    koopa_raw_program_t *ret = new koopa_raw_program_t();
    ret->values = generate_slice(KOOPA_RSIK_VALUE);
    ret->funcs = generate_slice(funcs, KOOPA_RSIK_FUNCTION);
    return ret;
}

void *FuncDefAST::GenerateIR() const
{
    koopa_raw_function_data_t *ret = new koopa_raw_function_data_t();
    // init ty
    koopa_raw_type_kind_t *ty = new koopa_raw_type_kind_t();
    ty->tag = KOOPA_RTT_FUNCTION;
    ty->data.function.params = generate_slice(KOOPA_RSIK_TYPE);
    ty->data.function.ret = (const struct koopa_raw_type_kind *)func_type->GenerateIR();
    ret->ty = ty;
    // init name
    char *name = new char[ident.length() + 1];
    ("@" + ident).copy(name, ident.length() + 1);
    ret->name = name;
    // init params
    ret->params = generate_slice(KOOPA_RSIK_VALUE);
    // init blocks
    std::vector<const void *> blocks{block->GenerateIR()};
    ret->bbs = generate_slice(blocks, KOOPA_RSIK_BASIC_BLOCK);

    return ret;
}

void *FuncTypeAST::GenerateIR() const
{
    if (func_type == "int")
        return (void *)generate_type(KOOPA_RTT_INT32);
    return nullptr;
}

void *BlockAST::GenerateIR() const
{
    koopa_raw_basic_block_data_t *ret = new koopa_raw_basic_block_data_t();
    ret->name = "%entry";
    ret->params = generate_slice(KOOPA_RSIK_VALUE);
    ret->used_by = generate_slice(KOOPA_RSIK_VALUE);

    std::vector<const void *> stmts;
    stmt->GenerateIR(stmts);
    ret->insts = generate_slice(stmts, KOOPA_RSIK_VALUE);

    return ret;
}

void *StmtAST::GenerateIR(std::vector<const void *> &inst_buf) const
{
    koopa_raw_value_data_t *ret = new koopa_raw_value_data();
    koopa_raw_slice_t used_by = generate_slice(ret, KOOPA_RSIK_VALUE);
    ret->ty = generate_type(KOOPA_RTT_UNIT);
    ret->name = nullptr;
    ret->used_by = generate_slice(KOOPA_RSIK_VALUE);
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
    return generate_number(parent, number);
}

void *UnaryExpAST::GenerateIR(koopa_raw_slice_t parent,
                              std::vector<const void *> &inst_buf) const
{
    if (type == 1)
        return primary_exp->GenerateIR(parent, inst_buf);
    if (unary_op == "+")
        return unary_exp->GenerateIR(parent, inst_buf);

    koopa_raw_value_data_t *ret = new koopa_raw_value_data();
    koopa_raw_slice_t used_by = generate_slice(ret, KOOPA_RSIK_VALUE);

    koopa_raw_value_data_t *zero = generate_number(used_by, 0);

    auto &binary = ret->kind.data.binary;
    binary.lhs = (koopa_raw_value_t)zero;

    ret->ty = generate_type(KOOPA_RTT_INT32);
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
    koopa_raw_value_data_t *ret = new koopa_raw_value_data();
    koopa_raw_slice_t used_by = generate_slice(ret, KOOPA_RSIK_VALUE);
    ret->ty = generate_type(KOOPA_RTT_INT32);
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
    koopa_raw_value_data_t *ret = new koopa_raw_value_data();
    koopa_raw_slice_t used_by = generate_slice(ret, KOOPA_RSIK_VALUE);
    ret->ty = generate_type(KOOPA_RTT_INT32);
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
    koopa_raw_value_data_t *ret = new koopa_raw_value_data();
    koopa_raw_slice_t used_by = generate_slice(ret, KOOPA_RSIK_VALUE);
    ret->ty = generate_type(KOOPA_RTT_INT32);
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
    koopa_raw_value_data_t *ret = new koopa_raw_value_data();
    koopa_raw_slice_t used_by = generate_slice(ret, KOOPA_RSIK_VALUE);
    ret->ty = generate_type(KOOPA_RTT_INT32);
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

void *LAndExpAST::GenerateIR(koopa_raw_slice_t parent,
                             std::vector<const void *> &inst_buf) const
{
    if (type == 1)
        return eq_exp->GenerateIR(parent, inst_buf);
    koopa_raw_value_data_t *ret = new koopa_raw_value_data();
    koopa_raw_slice_t used_by = generate_slice(ret, KOOPA_RSIK_VALUE);
    ret->ty = generate_type(KOOPA_RTT_INT32);
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

void *LOrExpAST::GenerateIR(koopa_raw_slice_t parent,
                            std::vector<const void *> &inst_buf) const
{
    // A || B = A!=0 | B!=0
    if (type == 1)
        return land_exp->GenerateIR(parent, inst_buf);
    koopa_raw_value_data_t *ret = new koopa_raw_value_data();
    koopa_raw_slice_t used_by = generate_slice(ret, KOOPA_RSIK_VALUE);
    ret->ty = generate_type(KOOPA_RTT_INT32);
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

void *make_bool(koopa_raw_slice_t parent,
                std::vector<const void *> &inst_buf,
                koopa_raw_value_t exp)
{
    koopa_raw_value_data_t *ret = new koopa_raw_value_data();
    koopa_raw_slice_t used_by = generate_slice(ret, KOOPA_RSIK_VALUE);
    ret->ty = generate_type(KOOPA_RTT_INT32);
    ret->name = nullptr;
    ret->used_by = parent;
    ret->kind.tag = KOOPA_RVT_BINARY;
    auto &binary = ret->kind.data.binary;
    binary.op = KOOPA_RBO_NOT_EQ;
    binary.lhs = exp;
    koopa_raw_value_data *zero = generate_number(used_by, 0);
    binary.rhs = (koopa_raw_value_t)zero;
    inst_buf.push_back(ret);
    return ret;
}

koopa_raw_slice_t generate_slice(koopa_raw_slice_item_kind_t kind)
{
    koopa_raw_slice_t ret;
    ret.kind = kind;
    ret.buffer = nullptr;
    ret.len = 0;
    return ret;
}
koopa_raw_slice_t generate_slice(std::vector<const void *> &vec,
                                 koopa_raw_slice_item_kind_t kind)
{
    koopa_raw_slice_t ret;
    ret.kind = kind;
    ret.buffer = new const void *[vec.size()];
    std::copy(vec.begin(), vec.end(), ret.buffer);
    ret.len = vec.size();
    return ret;
}

koopa_raw_slice_t generate_slice(const void *data, koopa_raw_slice_item_kind_t kind)
{
    koopa_raw_slice_t ret;
    ret.kind = kind;
    ret.buffer = new const void *[1];
    ret.buffer[0] = data;
    ret.len = 1;
    return ret;
}

koopa_raw_type_t generate_type(koopa_raw_type_tag_t tag)
{
    koopa_raw_type_kind_t *ret = new koopa_raw_type_kind_t();
    ret->tag = tag;
    return (koopa_raw_type_t)ret;
}

koopa_raw_value_data_t *generate_number(koopa_raw_slice_t parent, int32_t number)
{
    koopa_raw_value_data_t *ret = new koopa_raw_value_data();
    ret->ty = generate_type(KOOPA_RTT_INT32);
    ret->name = nullptr;
    ret->used_by = parent;
    ret->kind.tag = KOOPA_RVT_INTEGER;
    ret->kind.data.integer.value = number;
    return ret;
}
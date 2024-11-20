#include "ast.h"

/****************************************************************************************************************/
/************************************************SymbolTable*****************************************************/
/****************************************************************************************************************/

void SymbolTable::add_symbol(std::string name, SymbolTable::Value value)
{
    symbol_table[name] = value;
}

SymbolTable::Value SymbolTable::get_value(std::string name)
{
    return symbol_table[name];
}

void SymbolTable::init()
{
    symbol_table.clear();
}

/*********************************************************************************************************/
/************************************************Dump*****************************************************/
/*********************************************************************************************************/

void CompUnitAST::Dump() const
{
    std::cout << "CompUnit{";
    func_def->Dump();
    std::cout << "}";
}

void FuncDefAST::Dump() const
{
    std::cout << "FuncDef{";
    func_type->Dump();
    std::cout << ident << "()";
    block->Dump();
    std::cout << "}";
}

void FuncTypeAST::Dump() const
{
    std::cout << "FuncType{";
    std::cout << func_type;
    std::cout << "}";
}

void BlockAST::Dump() const
{
    std::cout << "Block{";
    for (auto block_item = (*block_item_list).begin();
         block_item != (*block_item_list).end(); block_item++)
    {
        (*block_item)->Dump();
    }
    std::cout << "}";
}

void BlockItemAST::Dump() const
{
    std::cout << "BlockItem{";
    if (type == 1)
        decl->Dump();
    else
        stmt->Dump();
    std::cout << "}" << std::endl;
}

void DeclAST::Dump() const
{
    std::cout << "Decl{";
    if (type == 1)
        const_decl->Dump();
    else
        var_decl->Dump();
    std::cout << "}";
}

void ConstDeclAST::Dump() const
{
    std::cout << "ConstDecl{";
    btype->Dump();
    for (auto const_def = (*const_def_list).begin();
         const_def != (*const_def_list).end(); const_def++)
    {
        (*const_def)->Dump();
    }
    std::cout << "}";
}

void BTypeAST::Dump() const
{
    std::cout << "BType{";
    std::cout << btype;
    std::cout << "}";
}

void ConstDefAST::Dump() const
{
    std::cout << "ConstDef{";
    std::cout << ident << "=";
    const_init_val->Dump();
    std::cout << "}";
}

void ConstInitValAST::Dump() const
{
    std::cout << "ConstInitVal{";
    const_exp->Dump();
    std::cout << "}";
}

void ConstExpAST::Dump() const
{
    std::cout << "ConstExp{";
    exp->Dump();
    std::cout << "}";
}

void VarDeclAST::Dump() const
{
    btype->Dump();
    for (auto var_def = (*var_def_list).begin();
         var_def != (*var_def_list).end(); var_def++)
    {
        std::cout << "VarDecl{";
        (*var_def)->Dump();
    }
    std::cout << "}";
}

void VarDefAST::Dump() const
{
    std::cout << "VarDef{";
    std::cout << ident;
    if (type == 2)
    {
        std::cout << "=";
        init_val->Dump();
    }
    std::cout << "}";
}

void InitValAST::Dump() const
{
    std::cout << "Init{";
    exp->Dump();
    std::cout << "}";
}

void StmtAST::Dump() const
{
    std::cout << "Stmt{";
    if (type == 1)
    {
        lval->Dump();
        std::cout << "=";
        exp->Dump();
    }
    else
    {
        std::cout << "return ";
        exp->Dump();
    }
    std::cout << ";}";
}

void LValAST::Dump() const
{
    std::cout << "LVal{";
    std::cout << ident;
    std::cout << "}";
}

void ExpAST::Dump() const
{
    std::cout << "Exp{";
    lor_exp->Dump();
    std::cout << "}";
}

void LOrExpAST::Dump() const
{
    std::cout << "LOrExp{";
    if (type == 1)
        land_exp->Dump();
    else
    {
        lor_exp->Dump();
        std::cout << "||";
        land_exp->Dump();
    }
    std::cout << "}";
}

void LAndExpAST::Dump() const
{
    std::cout << "LAndExp{";
    if (type == 1)
        eq_exp->Dump();
    else
    {
        land_exp->Dump();
        std::cout << "&&";
        eq_exp->Dump();
    }
    std::cout << "}";
}

void EqExpAST::Dump() const
{
    std::cout << "EqExp{";
    if (type == 1)
        rel_exp->Dump();
    else
    {
        eq_exp->Dump();
        std::cout << eq_op;
        rel_exp->Dump();
    }
    std::cout << "}";
}

void RelExpAST::Dump() const
{
    std::cout << "RelExp{";
    if (type == 1)
        add_exp->Dump();
    else
    {
        rel_exp->Dump();
        std::cout << rel_op;
        add_exp->Dump();
    }
    std::cout << "}";
}

void AddExpAST::Dump() const
{
    std::cout << "AddExp{";
    if (type == 1)
        mul_exp->Dump();
    else
    {
        add_exp->Dump();
        std::cout << add_op;
        mul_exp->Dump();
    }
    std::cout << "}";
}

void MulExpAST::Dump() const
{
    std::cout << "MulExp{";
    if (type == 1)
        unary_exp->Dump();
    else
    {
        mul_exp->Dump();
        std::cout << mul_op;
        unary_exp->Dump();
    }
    std::cout << "}";
}

void UnaryExpAST::Dump() const
{
    std::cout << "UnaryExp{";
    if (type == 1)
        primary_exp->Dump();
    else
    {
        std::cout << unary_op;
        unary_exp->Dump();
    }
    std::cout << "}";
}

void PrimaryExpAST::Dump() const
{
    std::cout << "PrimaryExp{";
    if (type == 1)
    {
        std::cout << "(";
        exp->Dump();
        std::cout << ")";
    }
    else if (type == 2)
        lval->Dump();
    else
        std::cout << number;
    std::cout << "}";
}

/***************************************************************************************************************/
/************************************************GenerateIR*****************************************************/
/***************************************************************************************************************/

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
    char *name = new char[ident.size() + 1];
    ret->name = strcpy(name, ("@" + ident).c_str());
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
    symbol_table.init();
    koopa_raw_basic_block_data_t *ret = new koopa_raw_basic_block_data_t();
    ret->name = "%entry";
    ret->params = generate_slice(KOOPA_RSIK_VALUE);
    ret->used_by = generate_slice(KOOPA_RSIK_VALUE);

    std::vector<const void *> blockitems;
    for (auto block_item = (*block_item_list).begin();
         block_item != (*block_item_list).end(); block_item++)
    {
        (*block_item)->GenerateIR(blockitems);
        if (blockitems.size() > 0)
        {
            auto last = (koopa_raw_value_t)blockitems.back();
            if (last->kind.tag == KOOPA_RVT_RETURN)
            {
                ret->insts = generate_slice(blockitems, KOOPA_RSIK_VALUE);
                return ret;
            }
        }
    }
    auto last = (koopa_raw_value_t)blockitems.back();
    if (last->kind.tag != KOOPA_RVT_RETURN)
    {
        koopa_raw_value_data_t *ret = new koopa_raw_value_data();
        ret->ty = generate_type(KOOPA_RTT_UNIT);
        ret->name = nullptr;
        ret->used_by = generate_slice(KOOPA_RSIK_VALUE);
        ret->kind.tag = KOOPA_RVT_RETURN;
        ret->kind.data.ret.value = generate_number(generate_slice(KOOPA_RSIK_VALUE), 0);
        blockitems.push_back(ret);
    }
    ret->insts = generate_slice(blockitems, KOOPA_RSIK_VALUE);
    return ret;
}

void *BlockItemAST::GenerateIR(std::vector<const void *> &inst_buf) const
{
    if (type == 1)
        return decl->GenerateIR(inst_buf);
    return stmt->GenerateIR(inst_buf);
}

void *DeclAST::GenerateIR(std::vector<const void *> &inst_buf) const
{
    if (type == 1)
        return const_decl->GenerateIR(inst_buf);
    return var_decl->GenerateIR(inst_buf);
}

void *ConstDeclAST::GenerateIR(std::vector<const void *> &inst_buf) const
{
    // koopa_raw_type_t type = (koopa_raw_type_t)btype->GenerateIR();
    for (auto const_def = (*const_def_list).begin();
         const_def != (*const_def_list).end(); const_def++)
    {
        (*const_def)->GenerateIR();
    }
    return nullptr;
}

void *BTypeAST::GenerateIR() const
{
    assert(0);
    if (btype == "int")
        return (void *)generate_type(KOOPA_RTT_INT32);
    return nullptr;
}

void *ConstDefAST::GenerateIR() const
{
    int val = const_init_val->CalculateValue();
    SymbolTable::Value value = SymbolTable::Value(SymbolTable::Value::ValueType::Const, val);
    symbol_table.add_symbol(ident, value);
    return nullptr;
}

void *ConstInitValAST::GenerateIR() const
{
    assert(0);
    return nullptr;
}

void *ConstExpAST::GenerateIR(koopa_raw_slice_t parent,
                              std::vector<const void *> &inst_buf) const
{
    assert(0);
    return nullptr;
}

void *VarDeclAST::GenerateIR(std::vector<const void *> &inst_buf) const
{
    // koopa_raw_type_t type = (koopa_raw_type_t)btype->GenerateIR();
    for (auto var_def = (*var_def_list).begin();
         var_def != (*var_def_list).end(); var_def++)
    {
        (*var_def)->GenerateIR(inst_buf);
    }
    return nullptr;
}

void *VarDefAST::GenerateIR(std::vector<const void *> &inst_buf) const
{
    koopa_raw_value_data_t *ret = new koopa_raw_value_data();
    koopa_raw_slice_t used_by = generate_slice(ret, KOOPA_RSIK_VALUE);
    ret->ty = generate_type(KOOPA_RTT_POINTER, KOOPA_RTT_INT32);
    char *name = new char[ident.size() + 1];
    ret->name = strcpy(name, ("@" + ident).c_str());
    ret->used_by = generate_slice(KOOPA_RSIK_VALUE);
    ret->kind.tag = KOOPA_RVT_ALLOC;
    inst_buf.push_back(ret);
    symbol_table.add_symbol(ident, SymbolTable::Value(SymbolTable::Value::Var, ret));
    if (type == 2)
    {
        koopa_raw_value_data_t *store = new koopa_raw_value_data();
        store->name = nullptr;
        store->used_by = generate_slice(KOOPA_RSIK_VALUE);
        store->kind.tag = KOOPA_RVT_STORE;
        auto &store_data = store->kind.data.store;
        store_data.value = (koopa_raw_value_t)init_val->GenerateIR(used_by, inst_buf);
        store_data.dest = (koopa_raw_value_t)ret;
        inst_buf.push_back(store);
    }
    return nullptr;
}

void *InitValAST::GenerateIR(koopa_raw_slice_t parent, std::vector<const void *> &inst_buf) const
{
    return exp->GenerateIR(parent, inst_buf);
}

void *StmtAST::GenerateIR(std::vector<const void *> &inst_buf) const
{
    koopa_raw_value_data_t *ret = new koopa_raw_value_data();
    koopa_raw_slice_t used_by = generate_slice(ret, KOOPA_RSIK_VALUE);
    ret->ty = generate_type(KOOPA_RTT_UNIT);
    ret->name = nullptr;
    ret->used_by = generate_slice(KOOPA_RSIK_VALUE);
    if (type == 1)
    {
        ret->kind.tag = KOOPA_RVT_STORE;
        auto &store = ret->kind.data.store;
        store.value = (koopa_raw_value_t)exp->GenerateIR(used_by, inst_buf);
        store.dest = (koopa_raw_value_t)symbol_table.get_value(lval->GetIdent()).data.var_value;
    }
    else
    {
        ret->kind.tag = KOOPA_RVT_RETURN;
        ret->kind.data.ret.value =
            (const koopa_raw_value_data *)exp->GenerateIR(used_by, inst_buf);
    }
    inst_buf.push_back(ret);
    return ret;
}

void *LValAST::GenerateIR(koopa_raw_slice_t parent,
                          std::vector<const void *> &inst_buf) const
{
    SymbolTable::Value value = symbol_table.get_value(ident);
    koopa_raw_value_data_t *ret = new koopa_raw_value_data();
    ret->ty = generate_type(KOOPA_RTT_INT32);
    ret->name = nullptr;
    ret->used_by = parent;
    if (value.type == SymbolTable::Value::Const)
    {
        ret->kind.tag = KOOPA_RVT_INTEGER;
        ret->kind.data.integer.value = value.data.const_value;
    }
    else if (value.type == SymbolTable::Value::Var)
    {
        ret->kind.tag = KOOPA_RVT_LOAD;
        ret->kind.data.load.src = (koopa_raw_value_t)value.data.var_value;
        inst_buf.push_back(ret);
    }
    return ret;
}

void *ExpAST::GenerateIR(koopa_raw_slice_t parent,
                         std::vector<const void *> &inst_buf) const
{
    return lor_exp->GenerateIR(parent, inst_buf);
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
    binary.lhs = (koopa_raw_value_t)generate_bool(used_by, inst_buf, (koopa_raw_value_t)lor_exp->GenerateIR(used_by, inst_buf));
    binary.rhs = (koopa_raw_value_t)generate_bool(used_by, inst_buf, (koopa_raw_value_t)land_exp->GenerateIR(used_by, inst_buf));
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
    binary.lhs = (koopa_raw_value_t)generate_bool(used_by, inst_buf, (koopa_raw_value_t)land_exp->GenerateIR(used_by, inst_buf));
    binary.rhs = (koopa_raw_value_t)generate_bool(used_by, inst_buf, (koopa_raw_value_t)eq_exp->GenerateIR(used_by, inst_buf));
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

void *PrimaryExpAST::GenerateIR(koopa_raw_slice_t parent,
                                std::vector<const void *> &inst_buf) const
{
    if (type == 1)
        return exp->GenerateIR(parent, inst_buf);
    else if (type == 2)
        return lval->GenerateIR(parent, inst_buf);
    return generate_number(parent, number);
}

/*******************************************************************************************************************/
/************************************************CalculateValue*****************************************************/
/*******************************************************************************************************************/

std::int32_t ConstInitValAST::CalculateValue() const
{
    return const_exp->CalculateValue();
}

std::int32_t ConstExpAST::CalculateValue() const
{
    return exp->CalculateValue();
}

std::int32_t ExpAST::CalculateValue() const
{
    return lor_exp->CalculateValue();
}

std::int32_t LOrExpAST::CalculateValue() const
{
    if (type == 1)
        return land_exp->CalculateValue();
    else
        return lor_exp->CalculateValue() || land_exp->CalculateValue();
    return 0;
}

std::int32_t LAndExpAST::CalculateValue() const
{
    if (type == 1)
        return eq_exp->CalculateValue();
    else
        return land_exp->CalculateValue() && eq_exp->CalculateValue();
    return 0;
}

std::int32_t EqExpAST::CalculateValue() const
{
    if (type == 1)
        return rel_exp->CalculateValue();
    if (eq_op == "==")
        return eq_exp->CalculateValue() == rel_exp->CalculateValue();
    if (eq_op == "!=")
        return eq_exp->CalculateValue() != rel_exp->CalculateValue();
    return 0;
}

std::int32_t RelExpAST::CalculateValue() const
{
    if (type == 1)
        return add_exp->CalculateValue();
    if (rel_op == "<")
        return rel_exp->CalculateValue() < add_exp->CalculateValue();
    if (rel_op == ">")
        return rel_exp->CalculateValue() > add_exp->CalculateValue();
    if (rel_op == "<=")
        return rel_exp->CalculateValue() <= add_exp->CalculateValue();
    if (rel_op == ">=")
        return rel_exp->CalculateValue() >= add_exp->CalculateValue();
    return 0;
}

std::int32_t AddExpAST::CalculateValue() const
{
    if (type == 1)
        return mul_exp->CalculateValue();
    if (add_op == "+")
        return add_exp->CalculateValue() + mul_exp->CalculateValue();
    if (add_op == "-")
        return add_exp->CalculateValue() - mul_exp->CalculateValue();
    return 0;
}

std::int32_t MulExpAST::CalculateValue() const
{
    if (type == 1)
        return unary_exp->CalculateValue();
    if (mul_op == "*")
        return mul_exp->CalculateValue() * unary_exp->CalculateValue();
    if (mul_op == "/")
        return mul_exp->CalculateValue() / unary_exp->CalculateValue();
    if (mul_op == "%")
        return mul_exp->CalculateValue() % unary_exp->CalculateValue();
    return 0;
}

std::int32_t UnaryExpAST::CalculateValue() const
{
    if (type == 1)
        return primary_exp->CalculateValue();
    if (unary_op == "+")
        return unary_exp->CalculateValue();
    if (unary_op == "-")
        return -unary_exp->CalculateValue();
    if (unary_op == "!")
        return !unary_exp->CalculateValue();
    return 0;
}

std::int32_t PrimaryExpAST::CalculateValue() const
{
    if (type == 1)
        return exp->CalculateValue();
    else if (type == 2)
        return lval->CalculateValue();
    return (std::int32_t)number;
}

std::int32_t LValAST::CalculateValue() const
{
    return symbol_table.get_value(ident).data.const_value;
}

/**********************************************************************************************************/
/************************************************Utils*****************************************************/
/**********************************************************************************************************/

std::string LValAST::GetIdent() const
{
    return ident;
}

void *generate_bool(koopa_raw_slice_t parent,
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

koopa_raw_type_t generate_type(koopa_raw_type_tag_t tag, koopa_raw_type_tag_t base)
{
    koopa_raw_type_kind_t *ret = new koopa_raw_type_kind_t();
    ret->tag = tag;
    ret->data.pointer.base = generate_type(base);
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
#include "ast.h"

/****************************************************************************************************************/
/************************************************SymbolTable*****************************************************/
/****************************************************************************************************************/

void SymbolTable::add_symbol(std::string name, SymbolTable::Value value)
{
#ifdef DEBUG
    std::cout << "SymbolTable::add_symbol" << std::endl;
    std::cout << name << " ";
    if (value.type == SymbolTable::Value::Var)
        std::cout << "Var " << (void *)value.data.var_value << std::endl;
    else if (value.type == SymbolTable::Value::Const)
        std::cout << "Const " << value.data.const_value << std::endl;
    else if (value.type == SymbolTable::Value::Func)
        std::cout << "Func " << (void *)value.data.func_value << std::endl;
    else
        std::cout << "Error" << std::endl;

#endif
    symbol_table_stack.back()[name] = value;
}

SymbolTable::Value SymbolTable::get_value(std::string name)
{
#ifdef DEBUG
    std::cout << "SymbolTable::get_value" << std::endl;
    std::cout << name << " ";
#endif
    for (auto it = symbol_table_stack.rbegin();
         it != symbol_table_stack.rend(); it++)
    {
        auto value = it->find(name);
        if (value != it->end())
        {
#ifdef DEBUG
            if (value->second.type == SymbolTable::Value::Var)
                std::cout << "Var " << (void *)value->second.data.var_value << std::endl;
            else if (value->second.type == SymbolTable::Value::Const)
                std::cout << "Const " << value->second.data.const_value << std::endl;
            else if (value->second.type == SymbolTable::Value::Func)
                std::cout << "Func " << (void *)value->second.data.func_value << std::endl;
            else
                std::cout << "Error" << std::endl;

#endif
            return value->second;
        }
    }
    // std::cout << "Error: " << name << " not found" << std::endl;
    assert(0);
}

void SymbolTable::add_table()
{
#ifdef DEBUG
    std::cout << "SymbolTable::add_table" << std::endl;
#endif
    std::unordered_map<std::string, SymbolTable::Value> new_table;
    symbol_table_stack.push_back(new_table);
}

void SymbolTable::del_table()
{
#ifdef DEBUG
    std::cout << "SymbolTable::del_table" << std::endl;
    std::unordered_map<std::string, SymbolTable::Value> mp;
    mp = symbol_table_stack.back();
    for (std::unordered_map<std::string, SymbolTable::Value>::iterator it = mp.begin();
         it != mp.end(); it++)
    {
        std::cout << it->first << " ";
        if (it->second.type == SymbolTable::Value::Var)
            std::cout << "Var " << (void *)it->second.data.var_value << std::endl;
        else if (it->second.type == SymbolTable::Value::Const)
            std::cout << "Const " << it->second.data.const_value << std::endl;
        else if (it->second.type == SymbolTable::Value::Func)
            std::cout << "Func " << (void *)it->second.data.func_value << std::endl;
    }
#endif
    symbol_table_stack.pop_back();
}

/**************************************************************************************************************/
/************************************************BlockList*****************************************************/
/**************************************************************************************************************/

void BlockList::init()
{
#ifdef DEBUG
    std::cout << "BlockList::init" << std::endl;
#endif
    block_list.clear();
    tmp_inst_buf.clear();
}

std::vector<const void *> BlockList::get_block_list()
{
#ifdef DEBUG
    std::cout << "BlockList::get_block_list" << std::endl;
#endif
    return block_list;
}

void BlockList::add_block(koopa_raw_basic_block_data_t *block)
{
#ifdef DEBUG
    std::cout << "BlockList::add_block" << std::endl;
#endif
    block->insts.buffer = nullptr;
    block->insts.len = 0;
    block_list.push_back(block);
}

void BlockList::add_inst(const void *inst)
{
#ifdef DEBUG
    std::cout << "BlockList::add_inst" << std::endl;
#endif
    tmp_inst_buf.push_back(inst);
}

void BlockList::push_tmp_inst()
{
    // 这里是在保证每个block的最后一条指令是return或者branch或者jump
    // 其实这个判断可以放到rearrange_block_list里面。但是这里还是加上了
#ifdef DEBUG
    std::cout << "BlockList::push_tmp_inst" << std::endl;
#endif
    if (block_list.size() == 0)
    {
        return;
    }
    if (tmp_inst_buf.size() == 0)
    {
        return;
    }
    for (unsigned i = 0; i < tmp_inst_buf.size(); i++)
    {
        koopa_raw_value_t inst = (koopa_raw_value_t)tmp_inst_buf[i];
        if (inst->kind.tag == KOOPA_RVT_RETURN || inst->kind.tag == KOOPA_RVT_BRANCH || inst->kind.tag == KOOPA_RVT_JUMP)
        {
            tmp_inst_buf.erase(tmp_inst_buf.begin() + i + 1, tmp_inst_buf.end());
            break;
        }
    }
    koopa_raw_basic_block_data_t *last = (koopa_raw_basic_block_data_t *)block_list.back();
    assert(!last->insts.buffer);
    last->insts = generate_slice(tmp_inst_buf, KOOPA_RSIK_VALUE);
    tmp_inst_buf.clear();
    return;
}

bool BlockList::check_return()
{
#ifdef DEBUG
    std::cout << "BlockList::check_return" << std::endl;
#endif
    // if there is no return, return true
    if (block_list.size() == 0 || tmp_inst_buf.size() == 0)
    {
        return true;
    }
    for (unsigned i = 0; i < tmp_inst_buf.size(); i++)
    {
        koopa_raw_value_t inst = (koopa_raw_value_t)tmp_inst_buf[i];
        if (inst->kind.tag == KOOPA_RVT_RETURN)
        {
            return false;
        }
    }
    return true;
}

void BlockList::rearrange_block_list()
{
#ifdef DEBUG
    std::cout << "BlockList::rearrange_block_list" << std::endl;
#endif
    std::unordered_map<koopa_raw_basic_block_t, bool> is_visited;
    if (block_list.size() == 0)
        return;
    is_visited[(koopa_raw_basic_block_data_t *)block_list[0]] = true;
    for (size_t i = 1; i < block_list.size(); i++)
    {
        auto block = (koopa_raw_basic_block_data_t *)block_list[i];
        is_visited[block] = false;
    }
    for (size_t i = 0; i < block_list.size(); i++)
    {
        auto block = (koopa_raw_basic_block_data_t *)block_list[i];
        for (size_t j = 0; j < block->insts.len; j++)
        {
            auto inst = (koopa_raw_value_t)block->insts.buffer[j];
            if (inst->kind.tag == KOOPA_RVT_JUMP)
            {
                is_visited[inst->kind.data.jump.target] = true;
            }
            else if (inst->kind.tag == KOOPA_RVT_BRANCH)
            {
                is_visited[inst->kind.data.branch.true_bb] = true;
                is_visited[inst->kind.data.branch.false_bb] = true;
            }
        }
    }
    for (size_t i = 0; i < block_list.size(); i++)
    {
        auto block = (koopa_raw_basic_block_data_t *)block_list[i];
        if (!is_visited[block])
        {
            block_list.erase(block_list.begin() + i);
            i--;
        }
    }
}

/******************************************************************************************************************/
/************************************************LoopStack*********************************************************/
/******************************************************************************************************************/

void LoopStack::add_loop(koopa_raw_basic_block_data_t *cond, koopa_raw_basic_block_data_t *end)
{
#ifdef DEBUG
    std::cout << "LoopStack::add_loop" << std::endl;
#endif
    LoopBlocks loop(cond, end);
    loop_stack.push_back(loop);
}

koopa_raw_basic_block_data_t *LoopStack::get_cond_block()
{
#ifdef DEBUG
    std::cout << "LoopList::get_cond_block" << std::endl;
#endif
    return loop_stack.back().cond_block;
}

koopa_raw_basic_block_data_t *LoopStack::get_end_block()
{
#ifdef DEBUG
    std::cout << "LoopStack::get_end_block" << std::endl;
#endif
    return loop_stack.back().end_block;
}

void LoopStack::del_loop()
{
#ifdef DEBUG
    std::cout << "LoopStack::del_loop" << std::endl;
#endif
    loop_stack.pop_back();
}

bool LoopStack::is_inside_loop()
{
#ifdef DEBUG
    std::cout << "LoopStack::is_inside_loop" << std::endl;
#endif
    return loop_stack.size() > 0;
}

/***************************************************************************************************************/
/************************************************GenerateIR*****************************************************/
/***************************************************************************************************************/

void *CompUnitAST::GenerateIR_ret() const
{
#ifdef DEBUG
    std::cout << "CompUnit" << std::endl;
#endif
    symbol_table.add_table();
    std::vector<const void *> values;
    std::vector<const void *> funcs;
    this->load_lib_funcs(funcs);
    for (auto def = (*def_list).begin();
         def != (*def_list).end(); def++)
    {
        (*def)->GenerateIR_void(funcs, values);
    }
    symbol_table.del_table();

    koopa_raw_program_t *ret = new koopa_raw_program_t();
    if (values.size() == 0)
    {
        ret->values = generate_slice(KOOPA_RSIK_VALUE);
    }
    else
    {
        ret->values = generate_slice(values, KOOPA_RSIK_VALUE);
    }
    ret->funcs = generate_slice(funcs, KOOPA_RSIK_FUNCTION);

    return ret;
}

void DefAST::GenerateIR_void(std::vector<const void *> &funcs, std::vector<const void *> &values) const
{
#ifdef DEBUG
    std::cout << "Def" << std::endl;
#endif
    if (type == FUNC_DEF)
    {
        func_def->GenerateIR_void(funcs);
    }
    else if (type == DECL)
    {
        decl->GenerateGlobalValues(values);
    }
    return;
}

void FuncDefAST::GenerateIR_void(std::vector<const void *> &funcs) const
{
#ifdef DEBUG
    std::cout << "FuncDef" << std::endl;
#endif
    koopa_raw_type_t func_ty = (koopa_raw_type_t)func_type->GenerateIR_ret();

    std::vector<const void *> params;
    for (auto func_fparam = (*func_fparam_list).begin();
         func_fparam != (*func_fparam_list).end(); func_fparam++)
    {
        koopa_raw_value_data_t *func_arg_ref = (koopa_raw_value_data_t *)((*func_fparam)->GenerateIR_ret());
        size_t index = std::distance((*func_fparam_list).begin(), func_fparam);
        func_arg_ref->kind.data.func_arg_ref.index = index;
        params.push_back(func_arg_ref);
    }
    koopa_raw_function_data_t *ret = generate_function(ident, params, func_ty);
    symbol_table.add_symbol(ident,
                            SymbolTable::Value(SymbolTable::Value::Func, (koopa_raw_function_t)ret));

    block_list.init();
    koopa_raw_basic_block_data_t *entry = generate_block("%entry");
    block_list.add_block(entry);
    symbol_table.add_table();

    for (size_t i = 0; i < params.size(); i++)
    {
        koopa_raw_value_data_t *param = (koopa_raw_value_data_t *)params[i];
        koopa_raw_value_data_t *alloc = generate_alloc_inst(param->name + 1, KOOPA_RTT_INT32);
        symbol_table.add_symbol(alloc->name + 1,
                                SymbolTable::Value(SymbolTable::Value::Var, (koopa_raw_value_t)alloc));
        block_list.add_inst(alloc);
        koopa_raw_value_data_t *store =
            generate_store_inst((koopa_raw_value_t)alloc, (koopa_raw_value_t)param);
        block_list.add_inst(store);
    }

    block->GenerateIR_void();

    // if there is no return, add a return
    koopa_raw_value_data_t *ret_inst = nullptr;
    if (func_ty->tag == KOOPA_RTT_UNIT)
    {
        ret_inst = generate_return_inst((koopa_raw_value_t) nullptr);
    }
    else
    {
        assert(func_ty->tag == KOOPA_RTT_INT32);
        ret_inst = generate_return_inst(generate_number(0));
    }
    assert(ret_inst != nullptr);
    block_list.add_inst(ret_inst);

    symbol_table.del_table();
    // if there already has a return, push_tmp_inst will erase the return 0
    block_list.push_tmp_inst();
    block_list.rearrange_block_list();
    std::vector<const void *> blocks = block_list.get_block_list();
    ret->bbs = generate_slice(blocks, KOOPA_RSIK_BASIC_BLOCK);
    funcs.push_back(ret);
    return;
}

void *TypeAST::GenerateIR_ret() const
{
#ifdef DEBUG
    std::cout << "Type" << std::endl;
#endif
    if (type == INT)
        return (void *)generate_type(KOOPA_RTT_INT32);
    else if (type == VOID)
        return (void *)generate_type(KOOPA_RTT_UNIT);
    assert(0);
    return nullptr;
}

void *FuncFParamAST::GenerateIR_ret() const
{
#ifdef DEBUG
    std::cout << "FuncFParam" << std::endl;
#endif
    koopa_raw_type_t param_ty = (koopa_raw_type_t)btype->GenerateIR_ret();
    koopa_raw_value_data_t *ret = generate_func_arg_ref(param_ty, ident);
    return ret;
}

void BlockAST::GenerateIR_void() const
{
#ifdef DEBUG
    std::cout << "Block----------------------" << std::endl;
#endif
    symbol_table.add_table();

    for (auto block_item = (*block_item_list).begin();
         block_item != (*block_item_list).end(); block_item++)
    {
        (*block_item)->GenerateIR_void();
    }

    symbol_table.del_table();
    return;
}

void BlockItemAST::GenerateIR_void() const
{
#ifdef DEBUG
    std::cout << "BlockItem-------------------------" << std::endl;
#endif
    if (type == 1)
    {
        decl->GenerateIR_void();
    }
    else
    {
        stmt->GenerateIR_void();
    }
    return;
}

void DeclAST::GenerateIR_void() const
{
#ifdef DEBUG
    std::cout << "Decl" << std::endl;
#endif
    if (type == 1)
    {
        const_decl->GenerateIR_void();
    }
    else
    {
        var_decl->GenerateIR_void();
    }
    return;
}

void ConstDeclAST::GenerateIR_void() const
{
#ifdef DEBUG
    std::cout << "ConstDecl" << std::endl;
#endif
    // koopa_raw_type_t type = (koopa_raw_type_t)btype->GenerateIR();
    for (auto const_def = (*const_def_list).begin();
         const_def != (*const_def_list).end(); const_def++)
    {
        (*const_def)->GenerateIR_void();
    }
    return;
}

void ConstDefAST::GenerateIR_void() const
{
#ifdef DEBUG
    std::cout << "ConstDef" << std::endl;
#endif
    int val = const_init_val->CalculateValue();
    SymbolTable::Value value = SymbolTable::Value(SymbolTable::Value::ValueType::Const, val);
    symbol_table.add_symbol(ident, value);
    return;
}

void VarDeclAST::GenerateIR_void() const
{
#ifdef DEBUG
    std::cout << "VarDecl" << std::endl;
#endif
    koopa_raw_type_t type = (koopa_raw_type_t)btype->GenerateIR_ret();
    for (auto var_def = (*var_def_list).begin();
         var_def != (*var_def_list).end(); var_def++)
    {
        (*var_def)->GenerateIR_void(type->tag);
    }
    return;
}

void VarDefAST::GenerateIR_void(koopa_raw_type_tag_t tag) const
{
#ifdef DEBUG
    std::cout << "VarDef" << std::endl;
#endif
    koopa_raw_value_data_t *dest = generate_alloc_inst(ident, tag);
    block_list.add_inst(dest);
    symbol_table.add_symbol(ident, SymbolTable::Value(SymbolTable::Value::Var, (koopa_raw_value_t)dest));
    if (type == 2)
    {
        koopa_raw_value_t value = (koopa_raw_value_t)init_val->GenerateIR_ret();
        koopa_raw_value_data_t *store = generate_store_inst(dest, value);
        block_list.add_inst(store);
    }
    return;
}

void *InitValAST::GenerateIR_ret() const
{
#ifdef DEBUG
    std::cout << "InitVal" << std::endl;
#endif
    return exp->GenerateIR_ret();
}

void StmtAST::GenerateIR_void() const
{
#ifdef DEBUG
    std::cout << "Stmt" << std::endl;
#endif
    if (type == ASSIGN)
    {
        koopa_raw_value_t dest = (koopa_raw_value_t)symbol_table.get_value(lval->GetIdent()).data.var_value;
        koopa_raw_value_t value = (koopa_raw_value_t)exp->GenerateIR_ret();
        koopa_raw_value_data_t *ret = generate_store_inst(dest, value);
        block_list.add_inst(ret);
    }
    else if (type == EXP)
    {
        if (exp != nullptr)
            exp->GenerateIR_ret();
    }
    else if (type == BLOCK)
    {
        block->GenerateIR_void();
    }
    else if (type == RETURN)
    {
        koopa_raw_value_t value = nullptr;
        if (exp != nullptr)
            value = (koopa_raw_value_t)exp->GenerateIR_ret();
        koopa_raw_value_data_t *ret = generate_return_inst(value);
        block_list.add_inst(ret);
    }
    else if (type == IF_ELSE)
    {
        // check_return 是因为有可能有if，else里面出现return导致后面的块不会执行的问题
        // rearrange_block_list 其实可以解决这个问题。但是这里还是加上了check_return
        koopa_raw_value_data_t *ret = (koopa_raw_value_data_t *)exp->GenerateIR_ret();
        koopa_raw_basic_block_data_t *false_block =
            (koopa_raw_basic_block_data_t *)ret->kind.data.branch.false_bb;
        bool true_block_no_return = block_list.check_return();
        if (stmt != nullptr)
        {
            koopa_raw_basic_block_data_t *end_block = generate_block("%end");
            if (true_block_no_return)
            {
                block_list.add_inst(generate_jump_inst(end_block));
            }
            block_list.push_tmp_inst();
            block_list.add_block(false_block);
            stmt->GenerateIR_void();
            bool false_block_no_return = block_list.check_return();
            if (false_block_no_return)
            {
                block_list.add_inst(generate_jump_inst(end_block));
            }
            if (true_block_no_return || false_block_no_return)
            {
                block_list.push_tmp_inst();
                block_list.add_block(end_block);
            }
        }
        else
        {
            if (true_block_no_return)
            {
                block_list.add_inst(generate_jump_inst(false_block));
            }
            block_list.push_tmp_inst();
            block_list.add_block(false_block);
        }
    }
    else if (type == StmtAST::WHILE)
    {
        exp->GenerateIR_void();
    }
    return;
}

void *IfExpAST::GenerateIR_ret() const
{
#ifdef DEBUG
    std::cout << "IfExp" << std::endl;
#endif
    koopa_raw_value_t cond = (koopa_raw_value_t)exp->GenerateIR_ret();
    koopa_raw_value_data_t *ret =
        generate_branch_inst(cond, generate_block("%true"), generate_block("%false"));
    block_list.add_inst(ret);
    block_list.push_tmp_inst();
    block_list.add_block((koopa_raw_basic_block_data_t *)ret->kind.data.branch.true_bb);
    stmt->GenerateIR_void();
    return ret;
}

void WhileExpAST::GenerateIR_void() const
{
#ifdef DEBUG
    std::cout << "WhileExp" << std::endl;
#endif
    if (type == WHILE)
    {
        koopa_raw_basic_block_data_t *cond_block = generate_block("%cond");
        koopa_raw_basic_block_data_t *body_block = generate_block("%body");
        koopa_raw_basic_block_data_t *end_block = generate_block("%end");
        loop_stack.add_loop(cond_block, end_block);

        block_list.add_inst(generate_jump_inst(cond_block));
        block_list.push_tmp_inst();
        block_list.add_block(cond_block);
        koopa_raw_value_t cond = (koopa_raw_value_t)exp->GenerateIR_ret();
        koopa_raw_value_data_t *ret = generate_branch_inst(cond, body_block, end_block);
        block_list.add_inst(ret);
        block_list.push_tmp_inst();
        block_list.add_block(body_block);
        stmt->GenerateIR_void();
        block_list.add_inst(generate_jump_inst(cond_block));
        block_list.push_tmp_inst();
        block_list.add_block(end_block);

        loop_stack.del_loop();
    }
    if (type == CONTINUE)
    {
        if (loop_stack.is_inside_loop())
        {
            koopa_raw_basic_block_data_t *cond_block = loop_stack.get_cond_block();
            block_list.add_inst(generate_jump_inst(cond_block));
        }
        else
        {
            // std::cout << "Error: continue not in loop" << std::endl;
            assert(0);
        }
    }
    if (type == BREAK)
    {
        if (loop_stack.is_inside_loop())
        {
            koopa_raw_basic_block_data_t *end_block = loop_stack.get_end_block();
            block_list.add_inst(generate_jump_inst(end_block));
        }
        else
        {
            // std::cout << "Error: break not in loop" << std::endl;
            assert(0);
        }
    }
    return;
}

void *LValAST::GenerateIR_ret() const
{
#ifdef DEBUG
    std::cout << "LVal" << std::endl;
#endif
    SymbolTable::Value value = symbol_table.get_value(ident);
    koopa_raw_value_data_t *ret = nullptr;
    if (value.type == SymbolTable::Value::Const)
    {
        ret = generate_number(value.data.const_value);
    }
    else if (value.type == SymbolTable::Value::Var)
    {
        ret = generate_load_inst(value.data.var_value);
        block_list.add_inst(ret);
    }
    return ret;
}

void *ExpAST::GenerateIR_ret() const
{
#ifdef DEBUG
    std::cout << "Exp" << std::endl;
#endif
    return lor_exp->GenerateIR_ret();
}

void *LOrExpAST::GenerateIR_ret() const
{
// int result = 1;
// if (lhs == 0) {
//   result = rhs != 0;
// }
// 表达式的结果即是 result
#ifdef DEBUG
    std::cout << "LOrExp" << std::endl;
#endif
    if (type == 1)
        return land_exp->GenerateIR_ret();
    koopa_raw_value_data_t *result = generate_alloc_inst("result", KOOPA_RTT_INT32);
    block_list.add_inst(result);

    koopa_raw_value_data_t *store_1 = generate_store_inst((koopa_raw_value_t)result, (koopa_raw_value_t)generate_number(1));
    block_list.add_inst(store_1);

    koopa_raw_value_data_t *branch =
        generate_branch_inst((koopa_raw_value_t)lor_exp->GenerateIR_ret(), generate_block("%end"), generate_block("%true"));
    block_list.add_inst(branch);

    block_list.push_tmp_inst();
    block_list.add_block((koopa_raw_basic_block_data_t *)branch->kind.data.branch.false_bb);
    koopa_raw_value_data_t *true_exp =
        generate_binary_inst((koopa_raw_value_t)land_exp->GenerateIR_ret(), (koopa_raw_value_t)generate_number(0), KOOPA_RBO_NOT_EQ);
    block_list.add_inst(true_exp);

    koopa_raw_value_data_t *store_rhs =
        generate_store_inst((koopa_raw_value_t)result, (koopa_raw_value_t)true_exp);
    block_list.add_inst(store_rhs);
    block_list.add_inst(generate_jump_inst((koopa_raw_basic_block_data_t *)branch->kind.data.branch.true_bb));

    block_list.push_tmp_inst();
    block_list.add_block((koopa_raw_basic_block_data_t *)branch->kind.data.branch.true_bb);
    koopa_raw_value_data_t *load_result = generate_load_inst((koopa_raw_value_t)result);
    block_list.add_inst(load_result);
    return load_result;
}

void *LAndExpAST::GenerateIR_ret() const
{
// int result = 0;
// if (lhs == 1) {
//   result = rhs != 0;
// }
// 表达式的结果即是 result
#ifdef DEBUG
    std::cout << "LAndExp" << std::endl;
#endif
    if (type == 1)
        return eq_exp->GenerateIR_ret();
    koopa_raw_value_data_t *result = generate_alloc_inst("result", KOOPA_RTT_INT32);
    block_list.add_inst(result);

    koopa_raw_value_data_t *store_0 = generate_store_inst((koopa_raw_value_t)result, (koopa_raw_value_t)generate_number(0));
    block_list.add_inst(store_0);

    koopa_raw_value_data_t *branch =
        generate_branch_inst((koopa_raw_value_t)land_exp->GenerateIR_ret(), generate_block("%true"), generate_block("%end"));
    block_list.add_inst(branch);

    block_list.push_tmp_inst();
    block_list.add_block((koopa_raw_basic_block_data_t *)branch->kind.data.branch.true_bb);
    koopa_raw_value_data_t *true_exp =
        generate_binary_inst((koopa_raw_value_t)eq_exp->GenerateIR_ret(), (koopa_raw_value_t)generate_number(0), KOOPA_RBO_NOT_EQ);
    block_list.add_inst(true_exp);

    koopa_raw_value_data_t *store_rhs =
        generate_store_inst((koopa_raw_value_t)result, (koopa_raw_value_t)true_exp);
    block_list.add_inst(store_rhs);
    block_list.add_inst(generate_jump_inst((koopa_raw_basic_block_data_t *)branch->kind.data.branch.false_bb));

    block_list.push_tmp_inst();
    block_list.add_block((koopa_raw_basic_block_data_t *)branch->kind.data.branch.false_bb);
    koopa_raw_value_data_t *load_result = generate_load_inst((koopa_raw_value_t)result);
    block_list.add_inst(load_result);
    return load_result;
}

void *EqExpAST::GenerateIR_ret() const
{
#ifdef DEBUG
    std::cout << "EqExp" << std::endl;
#endif
    if (type == 1)
        return rel_exp->GenerateIR_ret();
    koopa_raw_binary_op_t op;
    if (eq_op == "==")
    {
        op = KOOPA_RBO_EQ;
    }
    if (eq_op == "!=")
    {
        op = KOOPA_RBO_NOT_EQ;
    }
    koopa_raw_value_t lhs = (koopa_raw_value_t)eq_exp->GenerateIR_ret();
    koopa_raw_value_t rhs = (koopa_raw_value_t)rel_exp->GenerateIR_ret();
    koopa_raw_value_data_t *ret = generate_binary_inst(lhs, rhs, op);
    block_list.add_inst(ret);
    return ret;
}

void *RelExpAST::GenerateIR_ret() const
{
#ifdef DEBUG
    std::cout << "RelExp" << std::endl;
#endif
    if (type == 1)
        return add_exp->GenerateIR_ret();
    koopa_raw_binary_op_t op;
    if (rel_op == "<")
    {
        op = KOOPA_RBO_LT;
    }
    if (rel_op == ">")
    {
        op = KOOPA_RBO_GT;
    }
    if (rel_op == "<=")
    {
        op = KOOPA_RBO_LE;
    }
    if (rel_op == ">=")
    {
        op = KOOPA_RBO_GE;
    }
    koopa_raw_value_t lhs = (koopa_raw_value_t)rel_exp->GenerateIR_ret();
    koopa_raw_value_t rhs = (koopa_raw_value_t)add_exp->GenerateIR_ret();
    koopa_raw_value_data_t *ret = generate_binary_inst(lhs, rhs, op);
    block_list.add_inst(ret);
    return ret;
}

void *AddExpAST::GenerateIR_ret() const
{
#ifdef DEBUG
    std::cout << "AddExp" << std::endl;
#endif
    if (type == 1)
        return mul_exp->GenerateIR_ret();
    koopa_raw_binary_op_t op;
    if (add_op == "+")
    {
        op = KOOPA_RBO_ADD;
    }
    if (add_op == "-")
    {
        op = KOOPA_RBO_SUB;
    }
    koopa_raw_value_t lhs = (koopa_raw_value_t)add_exp->GenerateIR_ret();
    koopa_raw_value_t rhs = (koopa_raw_value_t)mul_exp->GenerateIR_ret();
    koopa_raw_value_data_t *ret = generate_binary_inst(lhs, rhs, op);
    block_list.add_inst(ret);
    return ret;
}

void *MulExpAST::GenerateIR_ret() const
{
#ifdef DEBUG
    std::cout << "MulExp" << std::endl;
#endif
    if (type == 1)
        return unary_exp->GenerateIR_ret();
    koopa_raw_binary_op_t op;
    if (mul_op == "*")
    {
        op = KOOPA_RBO_MUL;
    }
    if (mul_op == "/")
    {
        op = KOOPA_RBO_DIV;
    }
    if (mul_op == "%")
    {
        op = KOOPA_RBO_MOD;
    }
    koopa_raw_value_t lhs = (koopa_raw_value_t)mul_exp->GenerateIR_ret();
    koopa_raw_value_t rhs = (koopa_raw_value_t)unary_exp->GenerateIR_ret();
    koopa_raw_value_data_t *ret = generate_binary_inst(lhs, rhs, op);
    block_list.add_inst(ret);
    return ret;
}

void *UnaryExpAST::GenerateIR_ret() const
{
#ifdef DEBUG
    std::cout << "UnaryExp" << std::endl;
#endif
    if (type == PRIMARY)
    {
        return primary_exp->GenerateIR_ret();
    }
    else if (type == UNARY)
    {
        if (unary_op == "+")
        {
            return unary_exp->GenerateIR_ret();
        }
        koopa_raw_binary_op_t op;
        if (unary_op == "-")
        {
            op = KOOPA_RBO_SUB;
        }
        if (unary_op == "!")
        {
            op = KOOPA_RBO_EQ;
        }
        koopa_raw_value_t lhs = (koopa_raw_value_t)generate_number(0);
        koopa_raw_value_t rhs = (koopa_raw_value_t)unary_exp->GenerateIR_ret();
        koopa_raw_value_data_t *ret = generate_binary_inst(lhs, rhs, op);
        block_list.add_inst(ret);
        return ret;
    }
    else if (type == FUNC)
    {
        koopa_raw_function_t func = symbol_table.get_value(ident).data.func_value;
        std::vector<const void *> args;
        for (auto arg = (*func_rparam_list).begin();
             arg != (*func_rparam_list).end(); arg++)
        {
            args.push_back((*arg)->GenerateIR_ret());
        }
        koopa_raw_value_data_t *ret = generate_call_inst(func, args);
        block_list.add_inst(ret);
        return ret;
    }
    assert(0);
    return nullptr;
}

void *PrimaryExpAST::GenerateIR_ret() const
{
#ifdef DEBUG
    std::cout << "PrimaryExp" << std::endl;
#endif
    if (type == 1)
        return exp->GenerateIR_ret();
    else if (type == 2)
        return lval->GenerateIR_ret();
    else
        return generate_number(number);
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
    if (type == PRIMARY)
        return primary_exp->CalculateValue();
    else if (type == UNARY)
    {
        if (unary_op == "+")
            return unary_exp->CalculateValue();
        if (unary_op == "-")
            return -unary_exp->CalculateValue();
        if (unary_op == "!")
            return !unary_exp->CalculateValue();
    }
    assert(0);
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
/******************************************GenerateGlobalValues********************************************/
/**********************************************************************************************************/

void DeclAST::GenerateGlobalValues(std::vector<const void *> &values) const
{
#ifdef DEBUG
    std::cout << "Decl" << std::endl;
#endif
    if (type == 1)
    {
        const_decl->GenerateIR_void();
    }
    else
    {
        var_decl->GenerateGlobalValues(values);
    }
    return;
}

void VarDeclAST::GenerateGlobalValues(std::vector<const void *> &values) const
{
#ifdef DEBUG
    std::cout << "VarDecl" << std::endl;
#endif
    // koopa_raw_type_t type = (koopa_raw_type_t)btype->GenerateIR();
    for (auto var_def = (*var_def_list).begin();
         var_def != (*var_def_list).end(); var_def++)
    {
        (*var_def)->GenerateGlobalValues(values);
    }
    return;
}

void VarDefAST::GenerateGlobalValues(std::vector<const void *> &values) const
{
#ifdef DEBUG
    std::cout << "VarDef" << std::endl;
#endif
    koopa_raw_value_t value;
    if (type == 2)
    {
        value = (koopa_raw_value_t)init_val->GenerateIR_ret();
    }
    else
    {
        value = (koopa_raw_value_t)generate_number(0);
    }
    koopa_raw_value_data_t *ret = generate_global_alloc(ident, value);

    symbol_table.add_symbol(ident, SymbolTable::Value(SymbolTable::Value::Var, (koopa_raw_value_t)ret));
    values.push_back(ret);
    return;
}

/**********************************************************************************************************/
/************************************************Utils*****************************************************/
/**********************************************************************************************************/

void CompUnitAST::load_lib_funcs(std::vector<const void *> &funcs) const
{
    koopa_raw_function_data_t *func;
    std::vector<const void *> params_ty;

    // int getint()
    func = generate_function_decl("getint", params_ty, generate_type(KOOPA_RTT_INT32));
    symbol_table.add_symbol("getint",
                            SymbolTable::Value(SymbolTable::Value::Func, (koopa_raw_function_t)func));
    funcs.push_back(func);

    // int getch()
    func = generate_function_decl("getch", params_ty, generate_type(KOOPA_RTT_INT32));
    symbol_table.add_symbol("getch",
                            SymbolTable::Value(SymbolTable::Value::Func, (koopa_raw_function_t)func));
    funcs.push_back(func);

    // int getarray(*int)
    params_ty.push_back(generate_type(KOOPA_RTT_POINTER, KOOPA_RTT_INT32));
    func = generate_function_decl("getarray", params_ty, generate_type(KOOPA_RTT_INT32));
    symbol_table.add_symbol("getarray",
                            SymbolTable::Value(SymbolTable::Value::Func, (koopa_raw_function_t)func));
    funcs.push_back(func);

    // void putint(int)
    params_ty.clear();
    params_ty.push_back(generate_type(KOOPA_RTT_INT32));
    func = generate_function_decl("putint", params_ty, generate_type(KOOPA_RTT_UNIT));
    symbol_table.add_symbol("putint",
                            SymbolTable::Value(SymbolTable::Value::Func, (koopa_raw_function_t)func));
    funcs.push_back(func);

    // void putch(int)
    params_ty.clear();
    params_ty.push_back(generate_type(KOOPA_RTT_INT32));
    func = generate_function_decl("putch", params_ty, generate_type(KOOPA_RTT_UNIT));
    symbol_table.add_symbol("putch",
                            SymbolTable::Value(SymbolTable::Value::Func, (koopa_raw_function_t)func));
    funcs.push_back(func);

    // void putarray(int, *int)
    params_ty.clear();
    params_ty.push_back(generate_type(KOOPA_RTT_INT32));
    params_ty.push_back(generate_type(KOOPA_RTT_POINTER, KOOPA_RTT_INT32));
    func = generate_function_decl("putarray", params_ty, generate_type(KOOPA_RTT_UNIT));
    symbol_table.add_symbol("putarray",
                            SymbolTable::Value(SymbolTable::Value::Func, (koopa_raw_function_t)func));
    funcs.push_back(func);

    // void starttime()
    params_ty.clear();
    func = generate_function_decl("starttime", params_ty, generate_type(KOOPA_RTT_UNIT));
    symbol_table.add_symbol("starttime",
                            SymbolTable::Value(SymbolTable::Value::Func, (koopa_raw_function_t)func));
    funcs.push_back(func);

    // void stoptime()
    params_ty.clear();
    func = generate_function_decl("stoptime", params_ty, generate_type(KOOPA_RTT_UNIT));
    symbol_table.add_symbol("stoptime",
                            SymbolTable::Value(SymbolTable::Value::Func, (koopa_raw_function_t)func));
    funcs.push_back(func);
}

std::string LValAST::GetIdent() const
{
#ifdef DEBUG
    std::cout << "LVal::GetIdent" << std::endl;
#endif
    return ident;
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

koopa_raw_value_data_t *generate_number(int32_t number)
{
    koopa_raw_value_data_t *ret = new koopa_raw_value_data();
    ret->ty = generate_type(KOOPA_RTT_INT32);
    ret->name = nullptr;
    ret->used_by = generate_slice(KOOPA_RSIK_VALUE);
    ret->kind.tag = KOOPA_RVT_INTEGER;
    ret->kind.data.integer.value = number;
    return ret;
}

koopa_raw_value_data_t *generate_func_arg_ref(koopa_raw_type_t ty, std::string ident)
{
    koopa_raw_value_data_t *ret = new koopa_raw_value_data();
    ret->ty = ty;
    char *name = new char[ident.size() + 2];
    name[ident.size() + 1] = '\0';
    ret->name = strcpy(name, ("@" + ident).c_str());
    ret->used_by = generate_slice(KOOPA_RSIK_VALUE);
    ret->kind.tag = KOOPA_RVT_FUNC_ARG_REF;
    ret->kind.data.func_arg_ref.index = -1;
    return ret;
}

koopa_raw_function_data_t *generate_function_decl(std::string ident, std::vector<const void *> &params_ty, const struct koopa_raw_type_kind *func_type)
{
    koopa_raw_function_data_t *ret = new koopa_raw_function_data_t();
    // init ty
    koopa_raw_type_kind_t *func_ty = new koopa_raw_type_kind_t();
    func_ty->tag = KOOPA_RTT_FUNCTION;
    if (params_ty.size() == 0)
    {
        func_ty->data.function.params = generate_slice(KOOPA_RSIK_TYPE);
    }
    else
    {
        func_ty->data.function.params = generate_slice(params_ty, KOOPA_RSIK_TYPE);
    }
    ret->params = generate_slice(KOOPA_RSIK_VALUE);
    func_ty->data.function.ret = func_type;
    ret->ty = func_ty;
    char *name = new char[ident.size() + 2];
    name[ident.size() + 1] = '\0';
    ret->name = strcpy(name, ("@" + ident).c_str());
    ret->bbs = generate_slice(KOOPA_RSIK_BASIC_BLOCK);
    return ret;
}

koopa_raw_function_data_t *generate_function(std::string ident, std::vector<const void *> &params, koopa_raw_type_t func_type)
{
    koopa_raw_function_data_t *ret = new koopa_raw_function_data_t();
    // init ty
    koopa_raw_type_kind_t *func_ty = new koopa_raw_type_kind_t();
    func_ty->tag = KOOPA_RTT_FUNCTION;
    if (params.size() == 0)
    {
        func_ty->data.function.params = generate_slice(KOOPA_RSIK_TYPE);
        ret->params = generate_slice(KOOPA_RSIK_VALUE);
    }
    else
    {
        std::vector<const void *> params_ty;
        for (size_t i = 0; i < params.size(); i++)
        {
            params_ty.push_back(((koopa_raw_value_data_t *)params[i])->ty);
        }
        func_ty->data.function.params = generate_slice(params_ty, KOOPA_RSIK_TYPE);
        ret->params = generate_slice(params, KOOPA_RSIK_VALUE);
    }
    func_ty->data.function.ret = func_type;
    ret->ty = func_ty;
    char *name = new char[ident.size() + 2];
    name[ident.size() + 1] = '\0';
    ret->name = strcpy(name, ("@" + ident).c_str());
    return ret;
}

koopa_raw_basic_block_data_t *generate_block(const char *name)
{
    koopa_raw_basic_block_data_t *ret = new koopa_raw_basic_block_data_t();
    ret->name = name;
    ret->params = generate_slice(KOOPA_RSIK_VALUE);
    ret->used_by = generate_slice(KOOPA_RSIK_VALUE);
    return ret;
}

koopa_raw_value_data_t *generate_global_alloc(std::string ident, koopa_raw_value_t value)
{
    koopa_raw_value_data_t *ret = new koopa_raw_value_data();
    ret->ty = generate_type(KOOPA_RTT_POINTER, KOOPA_RTT_INT32);
    char *name = new char[ident.size() + 2];
    name[ident.size() + 1] = '\0';
    ret->name = strcpy(name, ("@" + ident).c_str());
    ret->used_by = generate_slice(KOOPA_RSIK_VALUE);
    ret->kind.tag = KOOPA_RVT_GLOBAL_ALLOC;
    ret->kind.data.global_alloc.init = value;
    return ret;
}

koopa_raw_value_data_t *generate_alloc_inst(std::string ident, koopa_raw_type_tag_t tag)
{
    koopa_raw_value_data_t *ret = new koopa_raw_value_data();
    ret->ty = generate_type(KOOPA_RTT_POINTER, tag);
    char *name = new char[ident.size() + 2];
    name[ident.size() + 1] = '\0';
    ret->name = strcpy(name, ("@" + ident).c_str());
    ret->used_by = generate_slice(KOOPA_RSIK_VALUE);
    ret->kind.tag = KOOPA_RVT_ALLOC;
    return ret;
}

koopa_raw_value_data_t *generate_store_inst(koopa_raw_value_t dest, koopa_raw_value_t value)
{
    koopa_raw_value_data_t *ret = new koopa_raw_value_data();
    ret->ty = generate_type(KOOPA_RTT_UNIT);
    ret->name = nullptr;
    ret->used_by = generate_slice(KOOPA_RSIK_VALUE);
    ret->kind.tag = KOOPA_RVT_STORE;
    auto &store = ret->kind.data.store;
    store.dest = dest;
    store.value = value;
    return ret;
}

koopa_raw_value_data_t *generate_load_inst(koopa_raw_value_t src)
{
    koopa_raw_value_data_t *ret = new koopa_raw_value_data();
    ret->ty = generate_type(KOOPA_RTT_INT32);
    ret->name = nullptr;
    ret->used_by = generate_slice(KOOPA_RSIK_VALUE);
    ret->kind.tag = KOOPA_RVT_LOAD;
    ret->kind.data.load.src = src;
    return ret;
}

koopa_raw_value_data_t *generate_binary_inst(koopa_raw_value_t lhs, koopa_raw_value_t rhs, koopa_raw_binary_op_t op)
{
    koopa_raw_value_data_t *ret = new koopa_raw_value_data();
    ret->ty = generate_type(KOOPA_RTT_INT32);
    ret->name = nullptr;
    ret->used_by = generate_slice(KOOPA_RSIK_VALUE);
    ret->kind.tag = KOOPA_RVT_BINARY;
    auto &binary = ret->kind.data.binary;
    binary.op = op;
    binary.lhs = lhs;
    binary.rhs = rhs;
    return ret;
}

koopa_raw_value_data_t *generate_return_inst(koopa_raw_value_t value)
{
    koopa_raw_value_data_t *ret = new koopa_raw_value_data();
    ret->ty = generate_type(KOOPA_RTT_UNIT);
    ret->name = nullptr;
    ret->used_by = generate_slice(KOOPA_RSIK_VALUE);
    ret->kind.tag = KOOPA_RVT_RETURN;
    ret->kind.data.ret.value = value;
    return ret;
}

koopa_raw_value_data_t *generate_jump_inst(koopa_raw_basic_block_data_t *dest)
{
    koopa_raw_value_data_t *ret = new koopa_raw_value_data();
    ret->ty = generate_type(KOOPA_RTT_UNIT);
    ret->name = nullptr;
    ret->used_by = generate_slice(KOOPA_RSIK_VALUE);
    ret->kind.tag = KOOPA_RVT_JUMP;
    ret->kind.data.jump.args = generate_slice(KOOPA_RSIK_VALUE);
    ret->kind.data.jump.target = dest;
    return ret;
}

koopa_raw_value_data_t *generate_branch_inst(
    koopa_raw_value_t cond, koopa_raw_basic_block_data_t *true_bb, koopa_raw_basic_block_data_t *false_bb)
{
    koopa_raw_value_data_t *ret = new koopa_raw_value_data();
    ret->ty = generate_type(KOOPA_RTT_UNIT);
    ret->name = nullptr;
    ret->used_by = generate_slice(KOOPA_RSIK_VALUE);
    ret->kind.tag = KOOPA_RVT_BRANCH;
    ret->kind.data.branch.cond = cond;
    ret->kind.data.branch.true_bb = true_bb;
    ret->kind.data.branch.false_bb = false_bb;
    ret->kind.data.branch.true_args = generate_slice(KOOPA_RSIK_VALUE);
    ret->kind.data.branch.false_args = generate_slice(KOOPA_RSIK_VALUE);
    return ret;
}

koopa_raw_value_data_t *generate_call_inst(koopa_raw_function_t func, std::vector<const void *> &args)
{
    koopa_raw_value_data_t *ret = new koopa_raw_value_data();
    ret->ty = func->ty->data.function.ret;
    ret->name = nullptr;
    ret->used_by = generate_slice(KOOPA_RSIK_VALUE);
    ret->kind.tag = KOOPA_RVT_CALL;
    ret->kind.data.call.callee = func;
    if (args.size() == 0)
        ret->kind.data.call.args = generate_slice(KOOPA_RSIK_VALUE);
    else
        ret->kind.data.call.args = generate_slice(args, KOOPA_RSIK_VALUE);
    return ret;
}

/*********************************************************************************************************/
/************************************************Dump*****************************************************/
/*********************************************************************************************************/

void CompUnitAST::Dump() const
{
    std::cout << "CompUnit{";
    for (auto def = (*def_list).begin();
         def != (*def_list).end(); def++)
    {
        std::cout << std::endl
                  << "{";
        (*def)->Dump();
        std::cout << "}";
    }
    std::cout << "}";
}

void DefAST::Dump() const
{
    std::cout << "Def{";
    if (type == DECL)
        decl->Dump();
    else
        func_def->Dump();
    std::cout << "}";
}

void FuncDefAST::Dump() const
{
    std::cout << "FuncDef{";
    func_type->Dump();
    std::cout << " " << ident << " (";
    if (func_fparam_list->size() > 0)
    {
        (*(*func_fparam_list).begin())->Dump();
        for (auto fparam = (*func_fparam_list).begin() + 1;
             fparam != (*func_fparam_list).end(); fparam++)
        {
            std::cout << ",";
            (*fparam)->Dump();
        }
    }
    std::cout << ")";
    block->Dump();
    std::cout << "}";
}

void FuncFParamAST::Dump() const
{
    std::cout << "FuncFParam{";
    btype->Dump();
    std::cout << " " << ident << " ";
    std::cout << "}";
}

void TypeAST::Dump() const
{
    std::cout << "Type{";
    if (type == INT)
    {
        std::cout << " int ";
    }
    else if (type == VOID)
    {
        std::cout << " void ";
    }
    std::cout << "}";
}

void BlockAST::Dump() const
{
    std::cout << std::endl
              << "Block{ {";
    for (auto block_item = (*block_item_list).begin();
         block_item != (*block_item_list).end(); block_item++)
    {
        std::cout << std::endl;
        (*block_item)->Dump();
    }
    std::cout << "} }";
}

void BlockItemAST::Dump() const
{
    std::cout << "BlockItem{";
    if (type == 1)
        decl->Dump();
    else
        stmt->Dump();
    std::cout << "}";
}

void DeclAST::Dump() const
{
    std::cout << std::endl
              << "Decl{";
    if (type == 1)
        const_decl->Dump();
    else
        var_decl->Dump();
    std::cout << "}";
}

void ConstDeclAST::Dump() const
{
    std::cout << "ConstDecl{";
    std::cout << "const ";
    btype->Dump();
    (*(*const_def_list).begin())->Dump();
    for (auto const_def = (*const_def_list).begin() + 1;
         const_def != (*const_def_list).end(); const_def++)
    {
        std::cout << ",";
        (*const_def)->Dump();
    }
    std::cout << ";}";
}

void ConstDefAST::Dump() const
{
    std::cout << "ConstDef{";
    std::cout << " " << ident << " =";
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
    std::cout << "VarDecl{";
    btype->Dump();
    (*(*var_def_list).begin())->Dump();
    for (auto var_def = (*var_def_list).begin() + 1;
         var_def != (*var_def_list).end(); var_def++)
    {
        std::cout << ",";
        (*var_def)->Dump();
    }
    std::cout << ";}";
}

void VarDefAST::Dump() const
{
    std::cout << "VarDef{";
    std::cout << " " << ident << " ";
    if (type == 2)
    {
        std::cout << " = ";
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
    std::cout << std::endl
              << "Stmt{";
    if (type == ASSIGN)
    {
        lval->Dump();
        std::cout << "=";
        exp->Dump();
        std::cout << ";";
    }
    else if (type == EXP)
    {
        if (exp != nullptr)
            exp->Dump();
        std::cout << ";";
    }
    else if (type == BLOCK)
    {
        block->Dump();
    }
    else if (type == RETURN)
    {
        std::cout << " return ";
        if (exp != nullptr)
            exp->Dump();
        std::cout << ";";
    }
    else if (type == IF_ELSE)
    {
        exp->Dump();
        if (stmt != nullptr)
        {
            std::cout << std::endl
                      << "else";
            stmt->Dump();
        }
    }
    else if (type == WHILE)
    {
        exp->Dump();
    }
    std::cout << "}";
}

void IfExpAST::Dump() const
{
    std::cout << "IfExp{";
    std::cout << "if(";
    exp->Dump();
    std::cout << ")";
    stmt->Dump();
    std::cout << "}";
}

void WhileExpAST::Dump() const
{
    std::cout << "WhileExp{";
    if (type == WHILE)
    {
        std::cout << "while(";
        exp->Dump();
        std::cout << ")";
        stmt->Dump();
    }
    else if (type == CONTINUE)
    {
        std::cout << "continue;";
    }
    else if (type == BREAK)
    {
        std::cout << "break;";
    }
    std::cout << "}";
}

void LValAST::Dump() const
{
    std::cout << "LVal{";
    std::cout << " " << ident << " ";
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
        std::cout << " " << eq_op << " ";
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
        std::cout << " " << rel_op << " ";
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
        std::cout << " " << add_op << " ";
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
        std::cout << " " << mul_op << " ";
        unary_exp->Dump();
    }
    std::cout << "}";
}

void UnaryExpAST::Dump() const
{
    std::cout << "UnaryExp{";
    if (type == PRIMARY)
        primary_exp->Dump();
    else if (type == UNARY)
    {
        std::cout << " " << unary_op << " ";
        unary_exp->Dump();
    }
    else if (type == FUNC)
    {
        std::cout << " " << ident << " " << "(";
        if (func_rparam_list->size() != 0)
        {
            (*(*func_rparam_list).begin())->Dump();
            for (auto rparam = (*func_rparam_list).begin() + 1;
                 rparam != (*func_rparam_list).end(); rparam++)
            {
                std::cout << ",";
                (*rparam)->Dump();
            }
        }
        std::cout << ")";
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
        std::cout << " " << number << " ";
    std::cout << "}";
}
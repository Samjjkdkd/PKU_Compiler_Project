#pragma once
#include <vector>
#include <string>
#include <iostream>
#include <memory>
#include <assert.h>
#include <fstream>
#include <cstring>
#include <unordered_map>
#include "koopa.h"

#define DEBUG
// CompUnit      ::= Def {Def};
// Def           ::= Decl | FuncDef;

// FuncDef       ::= FuncType IDENT "(" [FuncFParams] ")" Block;

// FuncType      ::= "void" | "int";
// FuncFParams   ::= FuncFParam {"," FuncFParam};
// FuncFParam    ::= BType IDENT;

// Block         ::= "{" {BlockItem} "}";
// BlockItem     ::= Decl | Stmt;

// Decl          ::= ConstDecl | VarDecl;

// ConstDecl     ::= "const" BType ConstDef {"," ConstDef} ";";
// BType         ::= "int";
// ConstDef      ::= IDENT "=" ConstInitVal;
// ConstInitVal  ::= ConstExp;
// ConstExp      ::= Exp;

// VarDecl       ::= BType VarDef {"," VarDef} ";";
// VarDef        ::= IDENT | IDENT "=" InitVal;
// InitVal       ::= Exp;

// Stmt          :: = LVal "=" Exp ";"
//                    | [Exp] ";"
//                    | Block
//                    | IfExp ["else" Stmt]
//                    | WhileExp
//                    | "return" [Exp] ";";

// IfExp         ::= "if" "(" Exp ")" Stmt;
// WhileExp      ::= "while" "(" Exp ")" Stmt;
//                   | "continue" ";";
//                   | "break" ";";

// LVal        ::= IDENT;

// Exp         ::= LOrExp;
// LOrExp      ::= LAndExp | LOrExp "||" LAndExp;
// LAndExp     ::= EqExp | LAndExp "&&" EqExp;
// EqExp       ::= RelExp | EqExp EqOp RelExp;
// EqOp        ::= "==" | "!=";
// RelExp      ::= AddExp | RelExp RelOp AddExp;
// RelOp       ::= "<" | "<=" | ">" | ">=";
// AddExp      ::= MulExp | AddExp AddOp MulExp;
// AddOp       ::= "+" | "-";
// MulExp      ::= UnaryExp | MulExp MulOp UnaryExp;
// MulOp       ::= "*" | "/" | "%";
// UnaryExp    ::= PrimaryExp
//                 | UnaryOp UnaryExp
//                 | IDENT "(" [FuncRParams] ")";
// UnaryOp     ::= "+" | "-" | "!";

// PrimaryExp  ::= "(" Exp ")"  | LVal | Number;
// FuncRParams ::= Exp {"," Exp};
// Number      ::= INT_CONST;

/****************************************************************************************************************/
/************************************************SymbolTable*****************************************************/
/****************************************************************************************************************/

class SymbolTable
{
public:
    class Value
    {
    public:
        enum ValueType
        {
            Var,
            Const,
            Func
        } type;
        union Data
        {
            int const_value;
            koopa_raw_value_t var_value;
            koopa_raw_function_t func_value;
        } data;
        Value() = default;
        Value(ValueType type, int value) : type(type)
        {
            assert(type == Const);
            data.const_value = value;
        };
        Value(ValueType type, koopa_raw_value_t value) : type(type)
        {
            assert(type == Var);
            data.var_value = value;
        };
        Value(ValueType type, koopa_raw_function_t value) : type(type)
        {
            assert(type == Func);
            data.func_value = value;
        };
    };

    void add_symbol(std::string name, SymbolTable::Value value);
    Value get_value(std::string name);
    void add_table();
    void del_table();

private:
    std::vector<std::unordered_map<std::string, SymbolTable::Value>> symbol_table_stack;
};
static SymbolTable symbol_table;

/******************************************************************************************************************/
/************************************************BlockList*****************************************************/
/******************************************************************************************************************/

class BlockList
{
private:
    std::vector<const void *> block_list;
    std::vector<const void *> tmp_inst_buf;

public:
    void init();
    void add_block(koopa_raw_basic_block_data_t *block);
    void add_inst(const void *inst);
    void push_tmp_inst();
    bool check_return();
    void rearrange_block_list();
    std::vector<const void *> get_block_list();
};
static BlockList block_list;

/******************************************************************************************************************/
/************************************************LoopList**********************************************************/
/******************************************************************************************************************/

class LoopStack
{
private:
    class LoopBlocks
    {
    public:
        koopa_raw_basic_block_data_t *cond_block;
        koopa_raw_basic_block_data_t *end_block;
        LoopBlocks(koopa_raw_basic_block_data_t *cond_block, koopa_raw_basic_block_data_t *end_block)
        {
            this->cond_block = cond_block;
            this->end_block = end_block;
        }
    };
    std::vector<LoopBlocks> loop_stack;

public:
    void add_loop(koopa_raw_basic_block_data_t *cond, koopa_raw_basic_block_data_t *end);
    koopa_raw_basic_block_data_t *get_cond_block();
    koopa_raw_basic_block_data_t *get_end_block();
    void del_loop();
    bool is_inside_loop();
};

static LoopStack loop_stack;
/********************************************************************************************************/
/************************************************AST*****************************************************/
/********************************************************************************************************/

class BaseAST
{
public:
    virtual ~BaseAST() = default;
    virtual void Dump() const = 0;
    virtual std::string GetIdent() const { return ""; };
    virtual std::int32_t CalculateValue() const { return 0; };
    virtual void GenerateGlobalValues(std::vector<const void *> &vec) const { return; };
    virtual void *GenerateIR_ret() const { return nullptr; };
    virtual void GenerateIR_void() const { return; };
    virtual void GenerateIR_void(koopa_raw_type_tag_t tag) const { return; };
    virtual void GenerateIR_void(std::vector<const void *> &funcs, std::vector<const void *> &values) const { return; };
    virtual void GenerateIR_void(std::vector<const void *> &funcs) const { return; };
};

// CompUnit  ::= Def {Def};
class CompUnitAST : public BaseAST
{
public:
    std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>> def_list;

    void Dump() const override;
    void *GenerateIR_ret() const override;
    void load_lib_funcs(std::vector<const void *> &funcs) const;
};

// Def           ::= Decl | FuncDef;
class DefAST : public BaseAST
{
public:
    enum
    {
        DECL,
        FUNC_DEF
    } type;
    std::unique_ptr<BaseAST> func_def;
    std::unique_ptr<BaseAST> decl;

    void Dump() const override;
    void GenerateIR_void(std::vector<const void *> &funcs, std::vector<const void *> &values) const override;
};

// FuncDef       ::= FuncType IDENT "(" [FuncFParams] ")" Block;
class FuncDefAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> func_type;
    std::string ident;
    std::unique_ptr<BaseAST> block;
    std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>> func_fparam_list;

    void Dump() const override;
    void GenerateIR_void(std::vector<const void *> &vec) const override;
};

// FuncType  ::= "int" | "void";
class TypeAST : public BaseAST
{
public:
    enum
    {
        VOID,
        INT
    } type;

    void Dump() const override;
    void *GenerateIR_ret() const override;
};

// FuncFParams   ::= FuncFParam {"," FuncFParam};
// FuncFParam    ::= BType IDENT;
class FuncFParamAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> btype;
    std::string ident;

    void Dump() const override;
    void *GenerateIR_ret() const override;
};

// Block       ::= "{" {BlockItem} "}";
class BlockAST : public BaseAST
{
public:
    std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>> block_item_list;

    void Dump() const override;
    void GenerateIR_void() const override;
};

// BlockItem   ::= Decl | Stmt;
class BlockItemAST : public BaseAST
{
public:
    std::int32_t type;
    std::unique_ptr<BaseAST> decl;
    std::unique_ptr<BaseAST> stmt;

    void Dump() const override;
    void GenerateIR_void() const override;
};

// Decl          ::= ConstDecl | VarDecl;
class DeclAST : public BaseAST
{
public:
    std::int32_t type;
    std::unique_ptr<BaseAST> const_decl;
    std::unique_ptr<BaseAST> var_decl;

    void Dump() const override;
    void GenerateIR_void() const override;
    void GenerateGlobalValues(std::vector<const void *> &values) const override;
};

// ConstDecl     ::= "const" BType ConstDef {"," ConstDef} ";";
class ConstDeclAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> btype;
    std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>> const_def_list;

    void Dump() const override;
    void GenerateIR_void() const override;
};

// BType         ::= "int";

// ConstDef      ::= IDENT "=" ConstInitVal;
class ConstDefAST : public BaseAST
{
public:
    std::string ident;
    std::unique_ptr<BaseAST> const_init_val;

    void Dump() const override;
    void GenerateIR_void() const override;
};

// ConstInitVal  ::= ConstExp;
class ConstInitValAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> const_exp;

    void Dump() const override;
    std::int32_t CalculateValue() const override;
};

// ConstExp    ::= Exp;
class ConstExpAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> exp;

    void Dump() const override;
    std::int32_t CalculateValue() const override;
};

// VarDecl       ::= BType VarDef {"," VarDef} ";";
class VarDeclAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> btype;
    std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>> var_def_list;

    void Dump() const override;
    void GenerateIR_void() const override;
    void GenerateGlobalValues(std::vector<const void *> &values) const override;
};

// VarDef        ::= IDENT | IDENT "=" InitVal;
class VarDefAST : public BaseAST
{
public:
    std::int32_t type;
    std::string ident;
    std::unique_ptr<BaseAST> init_val;

    void Dump() const override;
    void GenerateIR_void(koopa_raw_type_tag_t tag) const override;
    void GenerateGlobalValues(std::vector<const void *> &values) const override;
};

// InitVal       ::= Exp;
class InitValAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> exp;

    void Dump() const override;
    void *GenerateIR_ret() const override;
};

// Stmt          :: = LVal "=" Exp ";"
//                    | [Exp] ";"
//                    | Block
//                    | IfExp ["else" Stmt]
//                    | WhileExp
//                    | "return"[Exp] ";";
class StmtAST : public BaseAST
{
public:
    enum
    {
        IF_ELSE,
        WHILE,
        ASSIGN,
        EXP,
        BLOCK,
        RETURN
    } type;
    std::unique_ptr<BaseAST> lval;
    std::unique_ptr<BaseAST> exp;
    std::unique_ptr<BaseAST> block;
    std::unique_ptr<BaseAST> stmt;

    void Dump() const override;
    void GenerateIR_void() const override;
};

// IfExp         ::= "if" "(" Exp ")" Stmt;
class IfExpAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> exp;
    std::unique_ptr<BaseAST> stmt;

    void Dump() const override;
    void *GenerateIR_ret() const override;
};

// WhileExp      ::= "while" "(" Exp ")" Stmt;
//                   | "continue" ";";
//                   | "break" ";";
class WhileExpAST : public BaseAST
{
public:
    enum
    {
        WHILE,
        CONTINUE,
        BREAK
    } type;
    std::unique_ptr<BaseAST> exp;
    std::unique_ptr<BaseAST> stmt;

    void Dump() const override;
    void GenerateIR_void() const override;
};

// LVal        ::= IDENT;
class LValAST : public BaseAST
{
public:
    std::string ident;

    void Dump() const override;
    std::string GetIdent() const override;
    void *GenerateIR_ret() const override;
    std::int32_t CalculateValue() const override;
};

// Exp         ::= LOrExp;
class ExpAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> lor_exp;

    void Dump() const override;
    void *GenerateIR_ret() const override;
    std::int32_t CalculateValue() const override;
};

// LOrExp      ::= LAndExp | LOrExp "||" LAndExp;
class LOrExpAST : public BaseAST
{
public:
    std::int32_t type;
    std::unique_ptr<BaseAST> lor_exp;
    std::unique_ptr<BaseAST> land_exp;

    void Dump() const override;
    void *GenerateIR_ret() const override;
    std::int32_t CalculateValue() const override;
};

// LAndExp     ::= EqExp | LAndExp "&&" EqExp;
class LAndExpAST : public BaseAST
{
public:
    std::int32_t type;
    std::unique_ptr<BaseAST> land_exp;
    std::unique_ptr<BaseAST> eq_exp;

    void Dump() const override;
    void *GenerateIR_ret() const override;
    std::int32_t CalculateValue() const override;
};

// EqExp       ::= RelExp | EqExp EqOp RelExp;
// EqOp        ::= "==" | "!=";
class EqExpAST : public BaseAST
{
public:
    std::int32_t type;
    std::string eq_op;
    std::unique_ptr<BaseAST> eq_exp;
    std::unique_ptr<BaseAST> rel_exp;

    void Dump() const override;
    void *GenerateIR_ret() const override;
    std::int32_t CalculateValue() const override;
};

// RelExp      ::= AddExp | RelExp RelOp AddExp;
// RelOp       ::= "<" | "<=" | ">" | ">=";
class RelExpAST : public BaseAST
{
public:
    std::int32_t type;
    std::string rel_op;
    std::unique_ptr<BaseAST> rel_exp;
    std::unique_ptr<BaseAST> add_exp;

    void Dump() const override;
    void *GenerateIR_ret() const override;
    std::int32_t CalculateValue() const override;
};

// AddExp      ::= MulExp | AddExp AddOp MulExp;
// AddOp :: = "+" | "-";
class AddExpAST : public BaseAST
{
public:
    std::int32_t type;
    std::string add_op;
    std::unique_ptr<BaseAST> add_exp;
    std::unique_ptr<BaseAST> mul_exp;

    void Dump() const override;
    void *GenerateIR_ret() const override;
    std::int32_t CalculateValue() const override;
};

// MulExp      ::= UnaryExp | MulExp MulOp UnaryExp;
// MulOp :: = "*" | "/" | "%";
class MulExpAST : public BaseAST
{
public:
    std::int32_t type;
    std::string mul_op;
    std::unique_ptr<BaseAST> mul_exp;
    std::unique_ptr<BaseAST> unary_exp;

    void Dump() const override;
    void *GenerateIR_ret() const override;
    std::int32_t CalculateValue() const override;
};

// UnaryExp    ::= PrimaryExp | UnaryOp UnaryExp;
//                  | IDENT "(" [FuncRParams] ")";
// UnaryOp :: = "+" | "-" | "!";
class UnaryExpAST : public BaseAST
{
public:
    enum
    {
        PRIMARY,
        UNARY,
        FUNC
    } type;

    std::string unary_op;
    std::unique_ptr<BaseAST> primary_exp;
    std::unique_ptr<BaseAST> unary_exp;
    std::string ident;
    std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>> func_rparam_list;

    void Dump() const override;
    void *GenerateIR_ret() const override;
    std::int32_t CalculateValue() const override;
};

// PrimaryExp  ::= "(" Exp ")" | LVal | Number;
class PrimaryExpAST : public BaseAST
{
public:
    std::int32_t type;
    std::unique_ptr<BaseAST> exp;
    std::unique_ptr<BaseAST> lval;
    std::int32_t number;

    void Dump() const override;
    void *GenerateIR_ret() const override;
    std::int32_t CalculateValue() const override;
};

// Number      ::= INT_CONST;

/**********************************************************************************************************/
/************************************************Utils*****************************************************/
/**********************************************************************************************************/

koopa_raw_slice_t generate_slice(koopa_raw_slice_item_kind_t kind = KOOPA_RSIK_UNKNOWN);
koopa_raw_slice_t generate_slice(std::vector<const void *> &vec,
                                 koopa_raw_slice_item_kind_t kind = KOOPA_RSIK_UNKNOWN);
koopa_raw_slice_t generate_slice(const void *data,
                                 koopa_raw_slice_item_kind_t kind = KOOPA_RSIK_UNKNOWN);
koopa_raw_type_t generate_type(koopa_raw_type_tag_t tag);
koopa_raw_type_t generate_type(koopa_raw_type_tag_t tag, koopa_raw_type_tag_t base);
koopa_raw_value_data_t *generate_number(int32_t number);
koopa_raw_value_data_t *generate_func_arg_ref(koopa_raw_type_t ty, std::string ident);
koopa_raw_function_data_t *generate_function_decl(std::string ident, std::vector<const void *> &params_ty, koopa_raw_type_t func_type);
koopa_raw_function_data_t *generate_function(std::string ident, std::vector<const void *> &params, koopa_raw_type_t func_type);
koopa_raw_basic_block_data_t *generate_block(const char *name);
koopa_raw_value_data_t *generate_global_alloc(std::string ident, koopa_raw_value_t value);
koopa_raw_value_data_t *generate_alloc_inst(std::string ident, koopa_raw_type_tag_t tag);
koopa_raw_value_data_t *generate_store_inst(koopa_raw_value_t dest, koopa_raw_value_t value);
koopa_raw_value_data_t *generate_load_inst(koopa_raw_value_t src);
koopa_raw_value_data_t *generate_binary_inst(koopa_raw_value_t lhs, koopa_raw_value_t rhs, koopa_raw_binary_op_t op);
koopa_raw_value_data_t *generate_return_inst(koopa_raw_value_t value);
koopa_raw_value_data_t *generate_jump_inst(koopa_raw_basic_block_data_t *target);
koopa_raw_value_data_t *generate_branch_inst(koopa_raw_value_t cond, koopa_raw_basic_block_data_t *true_bb, koopa_raw_basic_block_data_t *false_bb);
koopa_raw_value_data_t *generate_call_inst(koopa_raw_function_t func, std::vector<const void *> &args);

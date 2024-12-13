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

// #define DEBUG
// CompUnit      ::= FuncDef;

// FuncDef       ::= FuncType IDENT "(" ")" Block;

// FuncType      ::= "int";

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
//                    | "return" [Exp] ";";

// IfExp         ::= "if" "(" Exp ")" Stmt;

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
// UnaryExp    ::= PrimaryExp | UnaryOp UnaryExp;
// UnaryOp     ::= "+" | "-" | "!";

// PrimaryExp  ::= "(" Exp ")"  | LVal | Number;

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
            Const
        } type;
        union Data
        {
            int const_value;
            koopa_raw_value_t var_value;
        } data;
        Value() = default;
        Value(ValueType type, int value) : type(type)
        {
            data.const_value = value;
        };
        Value(ValueType type, koopa_raw_value_t value) : type(type)
        {
            data.var_value = value;
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
    std::vector<const void *> *block_list;
    std::vector<const void *> tmp_inst_buf;

public:
    void init(std::vector<const void *> *blocks);
    void add_block(koopa_raw_basic_block_data_t *block);
    void add_inst(const void *inst);
    void finish_block();
};
static BlockList block_list;

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
    virtual void *GenerateIR() const { return nullptr; };
};

// CompUnit  ::= FuncDef
class CompUnitAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> func_def;

    void Dump() const override;
    void *GenerateIR() const override;
};

// FuncDef   ::= FuncType IDENT "(" ")" Block;
class FuncDefAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> func_type;
    std::string ident;
    std::unique_ptr<BaseAST> block;

    void Dump() const override;
    void *GenerateIR() const override;
};

// FuncType  ::= "int";
class FuncTypeAST : public BaseAST
{
public:
    std::string func_type = "int";

    void Dump() const override;
    void *GenerateIR() const override;
};

// Block       ::= "{" {BlockItem} "}";
class BlockAST : public BaseAST
{
public:
    std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>> block_item_list;

    void Dump() const override;
    void *GenerateIR() const override;
};

// BlockItem   ::= Decl | Stmt;
class BlockItemAST : public BaseAST
{
public:
    std::int32_t type;
    std::unique_ptr<BaseAST> decl;
    std::unique_ptr<BaseAST> stmt;

    void Dump() const override;
    void *GenerateIR() const override;
};

// Decl          ::= ConstDecl | VarDecl;
class DeclAST : public BaseAST
{
public:
    std::int32_t type;
    std::unique_ptr<BaseAST> const_decl;
    std::unique_ptr<BaseAST> var_decl;

    void Dump() const override;
    void *GenerateIR() const override;
};

// ConstDecl     ::= "const" BType ConstDef {"," ConstDef} ";";
class ConstDeclAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> btype;
    std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>> const_def_list;

    void Dump() const override;
    void *GenerateIR() const override;
};

// BType         ::= "int";
class BTypeAST : public BaseAST
{
public:
    std::string btype = "int";

    void Dump() const override;
    void *GenerateIR() const override;
};

// ConstDef      ::= IDENT "=" ConstInitVal;
class ConstDefAST : public BaseAST
{
public:
    std::string ident;
    std::unique_ptr<BaseAST> const_init_val;

    void Dump() const override;
    void *GenerateIR() const override;
};

// ConstInitVal  ::= ConstExp;
class ConstInitValAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> const_exp;

    void Dump() const override;
    void *GenerateIR() const override;
    std::int32_t CalculateValue() const override;
};

// ConstExp    ::= Exp;
class ConstExpAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> exp;

    void Dump() const override;
    void *GenerateIR() const override;
    std::int32_t CalculateValue() const override;
};

// VarDecl       ::= BType VarDef {"," VarDef} ";";
class VarDeclAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> btype;
    std::unique_ptr<std::vector<std::unique_ptr<BaseAST>>> var_def_list;

    void Dump() const override;
    void *GenerateIR() const override;
};

// VarDef        ::= IDENT | IDENT "=" InitVal;
class VarDefAST : public BaseAST
{
public:
    std::int32_t type;
    std::string ident;
    std::unique_ptr<BaseAST> init_val;

    void Dump() const override;
    void *GenerateIR() const override;
};

// InitVal       ::= Exp;
class InitValAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> exp;

    void Dump() const override;
    void *GenerateIR() const override;
};

// Stmt          :: = LVal "=" Exp ";"
//                    | [Exp] ";"
//                    | Block
//                    | "if" "(" Exp ")" Stmt ["else" Stmt]
//                    | "return"[Exp] ";";
class StmtAST : public BaseAST
{
public:
    enum
    {
        IF_ELSE,
        ASSIGN,
        EXP,
        BLOCK,
        RETURN
    } type;
    std::unique_ptr<BaseAST> lval;
    std::unique_ptr<BaseAST> exp;
    std::unique_ptr<BaseAST> block;
    std::unique_ptr<BaseAST> if_exp;
    std::unique_ptr<BaseAST> else_stmt;

    void Dump() const override;
    void *GenerateIR() const override;
};

class IfExpAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> exp;
    std::unique_ptr<BaseAST> stmt;

    void Dump() const override;
    void *GenerateIR() const override;
};

// LVal        ::= IDENT;
class LValAST : public BaseAST
{
public:
    std::string ident;

    void Dump() const override;
    std::string GetIdent() const override;
    void *GenerateIR() const override;
    std::int32_t CalculateValue() const override;
};

// Exp         ::= LOrExp;
class ExpAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> lor_exp;

    void Dump() const override;
    void *GenerateIR() const override;
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
    void *GenerateIR() const override;
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
    void *GenerateIR() const override;
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
    void *GenerateIR() const override;
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
    void *GenerateIR() const override;
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
    void *GenerateIR() const override;
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
    void *GenerateIR() const override;
    std::int32_t CalculateValue() const override;
};

// UnaryExp    ::= PrimaryExp | UnaryOp UnaryExp;
// UnaryOp :: = "+" | "-" | "!";
class UnaryExpAST : public BaseAST
{
public:
    std::int32_t type;
    std::string unary_op;
    std::unique_ptr<BaseAST> primary_exp;
    std::unique_ptr<BaseAST> unary_exp;

    void Dump() const override;
    void *GenerateIR() const override;
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
    void *GenerateIR() const override;
    std::int32_t CalculateValue() const override;
};

// Number      ::= INT_CONST;

/**********************************************************************************************************/
/************************************************Utils*****************************************************/
/**********************************************************************************************************/

void *generate_bool(koopa_raw_value_t exp);
koopa_raw_slice_t generate_slice(koopa_raw_slice_item_kind_t kind = KOOPA_RSIK_UNKNOWN);
koopa_raw_slice_t generate_slice(std::vector<const void *> &vec,
                                 koopa_raw_slice_item_kind_t kind = KOOPA_RSIK_UNKNOWN);
koopa_raw_slice_t generate_slice(const void *data,
                                 koopa_raw_slice_item_kind_t kind = KOOPA_RSIK_UNKNOWN);
koopa_raw_type_t generate_type(koopa_raw_type_tag_t tag);
koopa_raw_type_t generate_type(koopa_raw_type_tag_t tag, koopa_raw_type_tag_t base);
koopa_raw_value_data *generate_number(int32_t number);
koopa_raw_basic_block_data_t *generate_block(const char *name);
koopa_raw_value_data_t *generate_jump_inst(koopa_raw_basic_block_data_t *target);
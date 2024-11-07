#pragma once
#include <vector>
#include <string>
#include <iostream>
#include <memory>
#include <assert.h>
#include <fstream>
#include <cstring>
#include "koopa.h"
// CompUnit      ::= FuncDef;

// Decl          ::= ConstDecl | VarDecl;
// ConstDecl     ::= "const" BType ConstDef {"," ConstDef} ";";
// BType         ::= "int";
// ConstDef      ::= IDENT "=" ConstInitVal;
// ConstInitVal  ::= ConstExp;

// VarDecl       ::= BType VarDef {"," VarDef} ";";
// VarDef        ::= IDENT | IDENT "=" InitVal;
// InitVal       ::= Exp;

// FuncDef     ::= FuncType IDENT "(" ")" Block;
// FuncType    ::= "int";

// Block       ::= "{" {BlockItem} "}";
// BlockItem   ::= Decl | Stmt;
// Stmt        ::= LVal "=" Exp ";"| "return" Exp ";";

// Exp         ::= LOrExp;
// LVal        ::= IDENT;
// PrimaryExp  ::= "(" Exp ")"  | LVal | Number;
// Number      ::= INT_CONST;
// UnaryExp    ::= PrimaryExp | UnaryOp UnaryExp;
// UnaryOp     ::= "+" | "-" | "!";
// MulExp      ::= UnaryExp | MulExp MulOp UnaryExp;
// MulOp       ::= "*" | "/" | "%";
// AddExp      ::= MulExp | AddExp AddOp MulExp;
// AddOp       ::= "+" | "-";
// RelExp      ::= AddExp | RelExp RelOp AddExp;
// RelOp       ::= "<" | "<=" | ">" | ">=";
// EqExp       ::= RelExp | EqExp EqOp RelExp;
// EqOp        ::= "==" | "!=";
// LAndExp     ::= EqExp | LAndExp "&&" EqExp;
// LOrExp      ::= LAndExp | LOrExp "||" LAndExp;
// ConstExp    ::= Exp;
// 所有 AST 的基类
class BaseAST
{
public:
    virtual ~BaseAST() = default;
    virtual void *GenerateIR() const { return nullptr; };
    virtual void *GenerateIR(std::vector<const void *> &inst_buf) const { return nullptr; };
    virtual void *GenerateIR(koopa_raw_slice_t parent,
                             std::vector<const void *> &inst_buf) const { return nullptr; };
};

// CompUnit  ::= FuncDef
class CompUnitAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> func_def;

    void *GenerateIR() const override;
};

// FuncDef   ::= FuncType IDENT "(" ")" Block;
class FuncDefAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> func_type;
    std::string ident;
    std::unique_ptr<BaseAST> block;

    void *GenerateIR() const override;
};

// FuncType  ::= "int";
class FuncTypeAST : public BaseAST
{
public:
    std::string func_type = "int";

    void *GenerateIR() const override;
};

// Block     ::= "{" Stmt "}";
class BlockAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> stmt;

    void *GenerateIR() const override;
};

// Stmt        ::= "return" Exp ";";
class StmtAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> exp;

    void *GenerateIR(std::vector<const void *> &inst_buf) const override;
};

// Exp         ::= LOrExp;
class ExpAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> lor_exp;

    void *GenerateIR(koopa_raw_slice_t parent,
                     std::vector<const void *> &inst_buf) const override;
};

// PrimaryExp  ::= "(" Exp ")" | Number;
class PrimaryExpAST : public BaseAST
{
public:
    std::int32_t type;
    std::unique_ptr<BaseAST> exp;
    std::int32_t number;

    void *GenerateIR(koopa_raw_slice_t parent,
                     std::vector<const void *> &inst_buf) const override;
};

// Number      ::= INT_CONST;

// UnaryExp    ::= PrimaryExp | UnaryOp UnaryExp;
class UnaryExpAST : public BaseAST
{
public:
    std::int32_t type;
    std::string unary_op;
    std::unique_ptr<BaseAST> primary_exp;
    std::unique_ptr<BaseAST> unary_exp;

    void *GenerateIR(koopa_raw_slice_t parent,
                     std::vector<const void *> &inst_buf) const override;
};
// UnaryOp :: = "+" | "-" | "!";
// MulExp      ::= UnaryExp | MulExp MulOp UnaryExp;
class MulExpAST : public BaseAST
{
public:
    std::int32_t type;
    std::string mul_op;
    std::unique_ptr<BaseAST> mul_exp;
    std::unique_ptr<BaseAST> unary_exp;

    void *GenerateIR(koopa_raw_slice_t parent,
                     std::vector<const void *> &inst_buf) const override;
};
// MulOp :: = "*" | "/" | "%";
// AddExp      ::= MulExp | AddExp AddOp MulExp;
class AddExpAST : public BaseAST
{
public:
    std::int32_t type;
    std::string add_op;
    std::unique_ptr<BaseAST> add_exp;
    std::unique_ptr<BaseAST> mul_exp;

    void *GenerateIR(koopa_raw_slice_t parent,
                     std::vector<const void *> &inst_buf) const override;
};
// AddOp :: = "+" | "-";
// RelExp      ::= AddExp | RelExp RelOp AddExp;
class RelExpAST : public BaseAST
{
public:
    std::int32_t type;
    std::string rel_op;
    std::unique_ptr<BaseAST> rel_exp;
    std::unique_ptr<BaseAST> add_exp;

    void *GenerateIR(koopa_raw_slice_t parent,
                     std::vector<const void *> &inst_buf) const override;
};
// RelOp       ::= "<" | "<=" | ">" | ">=";
// EqExp       ::= RelExp | EqExp EqOp RelExp;
class EqExpAST : public BaseAST
{
public:
    std::int32_t type;
    std::string eq_op;
    std::unique_ptr<BaseAST> eq_exp;
    std::unique_ptr<BaseAST> rel_exp;

    void *GenerateIR(koopa_raw_slice_t parent,
                     std::vector<const void *> &inst_buf) const override;
};
// EqOp        ::= "==" | "!=";
// LAndExp     ::= EqExp | LAndExp "&&" EqExp;
class LAndExpAST : public BaseAST
{
public:
    std::int32_t type;
    std::unique_ptr<BaseAST> land_exp;
    std::unique_ptr<BaseAST> eq_exp;

    void *GenerateIR(koopa_raw_slice_t parent,
                     std::vector<const void *> &inst_buf) const override;
};
// LOrExp      ::= LAndExp | LOrExp "||" LAndExp;
class LOrExpAST : public BaseAST
{
public:
    std::int32_t type;
    std::unique_ptr<BaseAST> lor_exp;
    std::unique_ptr<BaseAST> land_exp;

    void *GenerateIR(koopa_raw_slice_t parent,
                     std::vector<const void *> &inst_buf) const override;
};

void *make_bool(koopa_raw_slice_t parent, std::vector<const void *> &inst_buf,
                koopa_raw_value_t exp);

koopa_raw_slice_t generate_slice(koopa_raw_slice_item_kind_t kind = KOOPA_RSIK_UNKNOWN);
koopa_raw_slice_t generate_slice(std::vector<const void *> &vec,
                                 koopa_raw_slice_item_kind_t kind = KOOPA_RSIK_UNKNOWN);
koopa_raw_slice_t generate_slice(const void *data,
                                 koopa_raw_slice_item_kind_t kind = KOOPA_RSIK_UNKNOWN);
koopa_raw_type_t generate_type(koopa_raw_type_tag_t tag);
koopa_raw_value_data *generate_number(koopa_raw_slice_t parent, int32_t number);
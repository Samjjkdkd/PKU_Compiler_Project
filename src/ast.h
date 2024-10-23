#pragma once
#include <iostream>
#include <memory>
#include "utils.h"
// CompUnit    ::= FuncDef;
// FuncDef     ::= FuncType IDENT "(" ")" Block;
// FuncType    ::= "int";
// Block       ::= "{" Stmt "}";
// Stmt        ::= "return" Exp ";";
// Exp         ::= LOrExp;
// PrimaryExp  ::= "(" Exp ")" | Number;
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
// 所有 AST 的基类
class BaseAST
{
public:
    virtual ~BaseAST() = default;
    // virtual void Dump() const = 0;
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
    // void Dump() const override;
    void *GenerateIR() const override;
};

// FuncDef   ::= FuncType IDENT "(" ")" Block;
class FuncDefAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> func_type;
    std::string ident;
    std::unique_ptr<BaseAST> block;
    // void Dump() const override;
    void *GenerateIR() const override;
};

// FuncType  ::= "int";
class FuncTypeAST : public BaseAST
{
public:
    std::string func_type = "int";
    // void Dump() const override;
    void *GenerateIR() const override;
};

// Block     ::= "{" Stmt "}";
class BlockAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> stmt;
    // void Dump() const override;
    void *GenerateIR() const override;
};

// Stmt        ::= "return" Exp ";";
class StmtAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> exp;
    // void Dump() const override;
    void *GenerateIR(std::vector<const void *> &inst_buf) const override;
};

// Exp         ::= LOrExp;
class ExpAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> lor_exp;
    // void Dump() const override;
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
    // void Dump() const override;
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
    // void Dump() const override;
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
    // void Dump() const override;
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
    // void Dump() const override;
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
    // void Dump() const override;
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
    // void Dump() const override;
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
    // void Dump() const override;
    void *make_bool(koopa_raw_slice_t parent, std::vector<const void *> &inst_buf,
                    koopa_raw_value_t exp) const;
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
    // void Dump() const override;
    void *make_bool(koopa_raw_slice_t parent, std::vector<const void *> &inst_buf,
                    koopa_raw_value_t exp) const;
    void *GenerateIR(koopa_raw_slice_t parent,
                     std::vector<const void *> &inst_buf) const override;
};
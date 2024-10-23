#pragma once
#include <iostream>
#include <memory>
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
    virtual void GenerateIR() const = 0;
};

// CompUnit  ::= FuncDef
class CompUnitAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> func_def;
    // void Dump() const override;
    void GenerateIR() const override;
};

// FuncDef   ::= FuncType IDENT "(" ")" Block;
class FuncDefAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> func_type;
    std::string ident;
    std::unique_ptr<BaseAST> block;
    // void Dump() const override;
    void GenerateIR() const override;
};

// FuncType  ::= "int";
class FuncTypeAST : public BaseAST
{
public:
    std::string intstr;
    // void Dump() const override;
    void GenerateIR() const override;
};

// Block     ::= "{" Stmt "}";
class BlockAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> stmt;
    // void Dump() const override;
    void GenerateIR() const override;
};

// Stmt        ::= "return" Exp ";";
class StmtAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> exp;
    // void Dump() const override;
    void GenerateIR() const override;
};

// Exp         ::= LOrExp;
class ExpAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> lorexp;
    // void Dump() const override;
    void GenerateIR() const override;
};

// PrimaryExp  ::= "(" Exp ")" | Number;
class PrimaryExpAST : public BaseAST
{
public:
    std::int32_t type;
    std::unique_ptr<BaseAST> exp;
    std::int32_t number;
    // void Dump() const override;
    void GenerateIR() const override;
};

// Number      ::= INT_CONST;

// UnaryExp    ::= PrimaryExp | UnaryOp UnaryExp;
class UnaryExpAST : public BaseAST
{
public:
    std::int32_t type;
    std::string unaryop;
    std::unique_ptr<BaseAST> primaryexp;
    std::unique_ptr<BaseAST> unaryexp;
    // void Dump() const override;
    void GenerateIR() const override;
};
// UnaryOp :: = "+" | "-" | "!";
// MulExp      ::= UnaryExp | MulExp MulOp UnaryExp;
class MulExpAST : public BaseAST
{
public:
    std::int32_t type;
    std::string mulop;
    std::unique_ptr<BaseAST> mulexp;
    std::unique_ptr<BaseAST> unaryexp;
    // void Dump() const override;
    void GenerateIR() const override;
};
// MulOp :: = "*" | "/" | "%";
// AddExp      ::= MulExp | AddExp AddOp MulExp;
class AddExpAST : public BaseAST
{
public:
    std::int32_t type;
    std::string addop;
    std::unique_ptr<BaseAST> addexp;
    std::unique_ptr<BaseAST> mulexp;
    // void Dump() const override;
    void GenerateIR() const override;
};
// AddOp :: = "+" | "-";
// RelExp      ::= AddExp | RelExp RelOp AddExp;
class RelExpAST : public BaseAST
{
public:
    std::int32_t type;
    std::string relop;
    std::unique_ptr<BaseAST> relexp;
    std::unique_ptr<BaseAST> addexp;
    // void Dump() const override;
    void GenerateIR() const override;
};
// RelOp       ::= "<" | "<=" | ">" | ">=";
// EqExp       ::= RelExp | EqExp EqOp RelExp;
class EqExpAST : public BaseAST
{
public:
    std::int32_t type;
    std::string eqop;
    std::unique_ptr<BaseAST> eqexp;
    std::unique_ptr<BaseAST> relexp;
    // void Dump() const override;
    void GenerateIR() const override;
};
// EqOp        ::= "==" | "!=";
// LAndExp     ::= EqExp | LAndExp "&&" EqExp;
class LAndExpAST : public BaseAST
{
public:
    std::int32_t type;
    std::unique_ptr<BaseAST> landexp;
    std::unique_ptr<BaseAST> eqexp;
    // void Dump() const override;
    void GenerateIR() const override;
};
// LOrExp      ::= LAndExp | LOrExp "||" LAndExp;
class LOrExpAST : public BaseAST
{
public:
    std::int32_t type;
    std::unique_ptr<BaseAST> lorexp;
    std::unique_ptr<BaseAST> landexp;
    // void Dump() const override;
    void GenerateIR() const override;
};
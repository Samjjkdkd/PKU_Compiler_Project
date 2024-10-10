#pragma once
#include <iostream>
#include <memory>
// 所有 AST 的基类
class BaseAST
{
public:
    virtual ~BaseAST() = default;

    virtual void Dump() const = 0;
};

// CompUnit 是 BaseAST
class CompUnitAST : public BaseAST
{
public:
    // 用智能指针管理对象
    std::unique_ptr<BaseAST> func_def;

    void Dump() const override
    {
        // std::cout << "CompUnitAST { ";
        func_def->Dump();
        // std::cout << " }";
    }
};

// FuncDef 也是 BaseAST
class FuncDefAST : public BaseAST
{
public:
    std::unique_ptr<BaseAST> func_type;
    std::string ident;
    std::unique_ptr<BaseAST> block;

    void Dump() const override
    {
        // std::cout << "FuncDefAST { ";
        std::cout << "fun ";
        std::cout << "@main";
        func_type->Dump();
        std::cout << "(): i32 ";
        // std::cout << ", " << ident << ", ";
        std::cout << "{" << std::endl;
        std::cout << "%" << "entry:" << std::endl;
        std::cout << "  ret ";
        block->Dump();
        std::cout << std::endl;
        std::cout << "}";
        // std::cout << " }";
    }
};

class FuncTypeAST : public BaseAST
{
public:
    // 用智能指针管理对象
    std::string intstr;

    void Dump() const override
    {
        // std::cout << "FuncTypeAST { ";
        // std::cout << intstr;
        // std::cout << " }";
    }
};

class BlockAST : public BaseAST
{
public:
    // 用智能指针管理对象
    std::unique_ptr<BaseAST> stmt;

    void Dump() const override
    {
        // std::cout << "BlockAST { ";
        stmt->Dump();
        // std::cout << " }";
    }
};

class StmtAST : public BaseAST
{
public:
    // 用智能指针管理对象
    std::string numberstr;

    void Dump() const override
    {
        // std::cout << "StmtAST { ";
        std::cout << numberstr;
        // std::cout << " }";
    }
};

#pragma once
#include <iostream>
#include <memory>
// 所有 AST 的基类
class BaseAST
{
public:
    virtual ~BaseAST() = default;

    virtual void Dump() const = 0;
    virtual void GenerateIR() const = 0;
};

// CompUnit 是 BaseAST
class CompUnitAST : public BaseAST
{
public:
    // 用智能指针管理对象
    std::unique_ptr<BaseAST> func_def;

    void Dump() const override
    {
        std::cout << "CompUnitAST { ";
        func_def->Dump();
        std::cout << " }";
    }
    void GenerateIR() const override
    {
        func_def->GenerateIR();
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
        std::cout << "FuncDefAST { ";
        func_type->Dump();
        std::cout << ", " << ident << ", ";
        block->Dump();
        std::cout << " }";
    }
    void GenerateIR() const override
    {
        std::cout << "fun ";
        std::cout << "@main";
        func_type->GenerateIR();
        std::cout << "(): i32 ";
        std::cout << "{" << std::endl;
        std::cout << "%" << "entry:" << std::endl;
        std::cout << "  ret ";
        block->GenerateIR();
        std::cout << std::endl;
        std::cout << "}";
    }
};

class FuncTypeAST : public BaseAST
{
public:
    // 用智能指针管理对象
    std::string intstr;

    void Dump() const override
    {
        std::cout << "FuncTypeAST { ";
        std::cout << intstr;
        std::cout << " }";
    }
    void GenerateIR() const override
    {
    }
};

class BlockAST : public BaseAST
{
public:
    // 用智能指针管理对象
    std::unique_ptr<BaseAST> stmt;

    void Dump() const override
    {
        std::cout << "BlockAST { ";
        stmt->Dump();
        std::cout << " }";
    }
    void GenerateIR() const override
    {
        stmt->GenerateIR();
    }
};

class StmtAST : public BaseAST
{
public:
    // 用智能指针管理对象
    std::string numberstr;

    void Dump() const override
    {
        std::cout << "StmtAST { ";
        std::cout << numberstr;
        std::cout << " }";
    }
    void GenerateIR() const override
    {
        std::cout << numberstr;
    }
};

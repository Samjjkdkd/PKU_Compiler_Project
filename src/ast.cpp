#include <iostream>
#include <memory>
#include "ast.hpp"

static int tempsymbol = 0;
void CompUnitAST::GenerateIR() const
{
    func_def->GenerateIR();
}

void FuncDefAST::GenerateIR() const
{
    std::cout << "fun @" << ident << "(): ";
    func_type->GenerateIR();
    std::cout << " {" << std::endl;
    block->GenerateIR();
    std::cout << std::endl
              << "}" << std::endl;
}

void FuncTypeAST::GenerateIR() const
{
    std::cout << "i32";
}

void BlockAST::GenerateIR() const
{
    std::cout << "\%entry:" << std::endl;
    stmt->GenerateIR();
}

void StmtAST::GenerateIR() const
{
    exp->GenerateIR();
    std::cout << "  ret %" << tempsymbol - 1;
}

void ExpAST::GenerateIR() const
{
    lorexp->GenerateIR();
}

void PrimaryExpAST::GenerateIR() const
{
    if (type == 1)
    {
        exp->GenerateIR();
    }
    else if (type == 2)
    {
        std::cout << "  %" << tempsymbol << " = add 0, ";
        tempsymbol++;
        std::cout << number;
        std::cout << std::endl;
    }
}

void UnaryExpAST::GenerateIR() const
{
    if (type == 1)
    {
        primaryexp->GenerateIR();
    }
    else if (type == 2)
    {
        unaryexp->GenerateIR();
        if (unaryop == "+")
        {
            return;
        }
        std::cout << "  %" << tempsymbol << " = ";
        if (unaryop == "-")
        {
            std::cout << "sub";
        }
        else if (unaryop == "!")
        {
            std::cout << "eq";
        }
        std::cout << " 0, %" << tempsymbol - 1 << std::endl;
        tempsymbol++;
    }
}

void MulExpAST::GenerateIR() const
{
    if (type == 1)
    {
        unaryexp->GenerateIR();
    }
    else if (type == 2)
    {
        mulexp->GenerateIR();
        int first = tempsymbol - 1;
        unaryexp->GenerateIR();
        int second = tempsymbol - 1;
        std::cout << "  %" << tempsymbol << " = ";
        if (mulop == "*")
        {
            std::cout << "mul";
        }
        else if (mulop == "/")
        {
            std::cout << "div";
        }
        else if (mulop == "%")
        {
            std::cout << "mod";
        }
        std::cout << " %" << first;
        std::cout << ", %" << second << std::endl;
        tempsymbol++;
    }
}

void AddExpAST::GenerateIR() const
{
    if (type == 1)
    {
        mulexp->GenerateIR();
    }
    else if (type == 2)
    {
        addexp->GenerateIR();
        int first = tempsymbol - 1;
        mulexp->GenerateIR();
        int second = tempsymbol - 1;
        std::cout << "  %" << tempsymbol << " = ";
        if (addop == "+")
        {
            std::cout << "add";
        }
        else if (addop == "-")
        {
            std::cout << "sub";
        }
        std::cout << " %" << first;
        std::cout << ", %" << second << std::endl;
        tempsymbol++;
    }
}

void RelExpAST::GenerateIR() const
{
    if (type == 1)
    {
        addexp->GenerateIR();
    }
    else if (type == 2)
    {
        relexp->GenerateIR();
        int first = tempsymbol - 1;
        addexp->GenerateIR();
        int second = tempsymbol - 1;
        std::cout << "  %" << tempsymbol << " = ";
        if (relop == "<")
        {
            std::cout << "lt";
        }
        else if (relop == ">")
        {
            std::cout << "gt";
        }
        else if (relop == "<=")
        {
            std::cout << "le";
        }
        else if (relop == ">=")
        {
            std::cout << "ge";
        }
        std::cout << " %" << first;
        std::cout << ", %" << second << std::endl;
        tempsymbol++;
    }
}

void EqExpAST::GenerateIR() const
{
    if (type == 1)
    {
        relexp->GenerateIR();
    }
    else if (type == 2)
    {
        eqexp->GenerateIR();
        int first = tempsymbol - 1;
        relexp->GenerateIR();
        int second = tempsymbol - 1;
        std::cout << "  %" << tempsymbol << " = ";
        if (eqop == "==")
        {
            std::cout << "eq";
        }
        else if (eqop == "!=")
        {
            std::cout << "ne";
        }
        std::cout << " %" << first;
        std::cout << ", %" << second << std::endl;
        tempsymbol++;
    }
}

void LAndExpAST::GenerateIR() const
{
    if (type == 1)
    {
        eqexp->GenerateIR();
    }
    else if (type == 2)
    {
        // A!=0 & B!=0
        landexp->GenerateIR();
        int first = tempsymbol - 1;
        eqexp->GenerateIR();
        int second = tempsymbol - 1;
        std::cout << "  %" << tempsymbol << " = ne ";
        std::cout << "%" << first << ", 0" << std::endl;
        tempsymbol++;
        first = tempsymbol - 1;
        std::cout << "  %" << tempsymbol << " = ne ";
        std::cout << "%" << second << ", 0" << std::endl;
        tempsymbol++;
        second = tempsymbol - 1;
        std::cout << "  %" << tempsymbol << " = and ";
        std::cout << "%" << first << ", %" << second << std::endl;
        tempsymbol++;
    }
}

void LOrExpAST::GenerateIR() const
{
    if (type == 1)
    {
        landexp->GenerateIR();
    }
    else if (type == 2)
    {
        // A!=0 | B!=0
        lorexp->GenerateIR();
        int first = tempsymbol - 1;
        landexp->GenerateIR();
        int second = tempsymbol - 1;
        std::cout << "  %" << tempsymbol << " = ne ";
        std::cout << "%" << first << ", 0" << std::endl;
        tempsymbol++;
        first = tempsymbol - 1;
        std::cout << "  %" << tempsymbol << " = ne ";
        std::cout << "%" << second << ", 0" << std::endl;
        tempsymbol++;
        second = tempsymbol - 1;
        std::cout << "  %" << tempsymbol << " = or ";
        std::cout << "%" << first << ", %" << second << std::endl;
        tempsymbol++;
    }
}

// Dump
/*
void CompUnitAST::Dump() const
{
    std::cout << "CompUnit" << "{" << std::endl;
    func_def->Dump();
    std::cout << "}";
}

void FuncDefAST::Dump() const
{
    std::cout << "FuncDef" << "{" << std::endl;
    func_type->Dump();
    std::cout << " " << ident << "()" << std::endl;
    block->Dump();
    std::cout << "}";
}

void FuncTypeAST::Dump() const
{
    std::cout << "FuncType" << "{" << std::endl;
    std::cout << "int" << " ";
    std::cout << "}";
}

void BlockAST::Dump() const
{
    std::cout << "Block" << "{" << std::endl;
    stmt->Dump();
    std::cout << "}";
}

void StmtAST::Dump() const
{
    std::cout << "Stmt" << "{" << std::endl;
    exp->Dump();
    std::cout << "}";
}

void ExpAST::Dump() const
{
    std::cout << "Exp" << "{" << std::endl;
    unaryexp->Dump();
    std::cout << "}";
}

void PrimaryExpAST::Dump() const
{
    std::cout << "PrimaryExp" << "{" << std::endl;
    exp1_number2->Dump();
    std::cout << "}";
}

void NumberAST::Dump() const
{
    std::cout << "Number" << "{" << std::endl;
    std::cout << int_const << " ";
    std::cout << "}";
}

void UnaryExpAST::Dump() const
{
    std::cout << "UnaryExp" << "{" << std::endl;
    if (type == 1)
    {
        primaryexp1_unaryexp2->Dump();
    }
    else if (type == 2)
    {
        std::cout << *unaryop << " ";
        primaryexp1_unaryexp2->Dump();
    }
    std::cout << "}";
}
*/
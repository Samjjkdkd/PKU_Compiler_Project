%code requires {
  #include <memory>
  #include <string>
  #include "ast.h"
}

%{

#include <iostream>
#include <memory>
#include <string>
#include "ast.h"

// 声明 lexer 函数和错误处理函数
int yylex();
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s);

using namespace std;

%}

// 定义 parser 函数和错误处理函数的附加参数
%parse-param { std::unique_ptr<BaseAST> &ast }

// yylval 的定义, 我们把它定义成了一个联合体 (union)
// 因为 token 的值有的是字符串指针, 有的是整数
// 之前我们在 lexer 中用到的 str_val 和 int_val 就是在这里被定义的
// 至于为什么要用字符串指针而不直接用 string 或者 unique_ptr<string>?
// 请自行 STFW 在 union 里写一个带析构函数的类会出现什么情况

%union {
  std::string *str_val;
  int int_val;
  BaseAST *ast_val;
  std::vector<std::unique_ptr<BaseAST> > *vec_val;
}

// lexer 返回的所有 token 种类的声明
// 注意 IDENT 和 INT_CONST 会返回 token 的值, 分别对应 str_val 和 int_val
%token INT RETURN CONST IF ELSE WHILE CONTINUE BREAK
%token <str_val> LE GE EQ NE LAND LOR
%token <str_val> LT GT MINOR PLUS MUL DIV MOD NOT
%token <str_val> IDENT
%token <int_val> INT_CONST

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

// 非终结符的类型定义
%type <ast_val> FuncDef FuncType Block Stmt 
%type <ast_val> Exp PrimaryExp UnaryExp MulExp AddExp
%type <ast_val> RelExp EqExp LAndExp LOrExp
%type <ast_val> Decl ConstDecl BType ConstDef ConstInitVal 
%type <vec_val> ConstDefList BlockItemList VarDefList DefList
%type <ast_val> BlockItem LVal ConstExp
%type <ast_val> VarDecl VarDef InitVal
%type <ast_val> IfExp WhileExp Def

%type <str_val> UnaryOp MulOp AddOp RelOp EqOp
%type <int_val> Number

%%

// 开始符, CompUnit ::= FuncDef, 大括号后声明了解析完成后 parser 要做的事情
// 之前我们定义了 FuncDef 会返回一个 str_val, 也就是字符串指针
// 而 parser 一旦解析完 CompUnit, 就说明所有的 token 都被解析了, 即解析结束了
// 此时我们应该把 FuncDef 返回的结果收集起来, 作为 AST 传给调用 parser 的函数
// $1 指代规则里第一个符号的返回值, 也就是 FuncDef 的返回值
CompUnit
  : DefList {
    auto comp_unit = make_unique<CompUnitAST>();
    comp_unit->def_list = unique_ptr<vector<unique_ptr<BaseAST> >>($1);
    ast = move(comp_unit);
  }
  ;

DefList
  : Def {
    auto ast = new vector<unique_ptr<BaseAST> >();
    ast->push_back(unique_ptr<BaseAST>($1));
    $$ = ast;
  }
  | DefList Def {
    auto ast = static_cast<vector<unique_ptr<BaseAST> >*>($1);
    ast->push_back(unique_ptr<BaseAST>($2));
    $$ = ast;
  }
  ;

Def
  : FuncDef {
    auto ast = new DefAST();
    ast->func_def = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

// FuncDef ::= FuncType IDENT '(' ')' Block;
// 我们这里可以直接写 '(' 和 ')', 因为之前在 lexer 里已经处理了单个字符的情况
// 解析完成后, 把这些符号的结果收集起来, 然后拼成一个新的字符串, 作为结果返回
// $$ 表示非终结符的返回值, 我们可以通过给这个符号赋值的方法来返回结果
// 你可能会问, FuncType, IDENT 之类的结果已经是字符串指针了
// 为什么还要用 unique_ptr 接住它们, 然后再解引用, 把它们拼成另一个字符串指针呢
// 因为所有的字符串指针都是我们 new 出来的, new 出来的内存一定要 delete
// 否则会发生内存泄漏, 而 unique_ptr 这种智能指针可以自动帮我们 delete
// 虽然此处你看不出用 unique_ptr 和手动 delete 的区别, 但当我们定义了 AST 之后
// 这种写法会省下很多内存管理的负担

FuncDef
  : FuncType IDENT '(' ')' Block {
    auto ast = new FuncDefAST();
    ast->func_type = unique_ptr<BaseAST>($1);
    ast->ident = *unique_ptr<string>($2);
    ast->block = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  ;

FuncType
  : INT {
    auto ast = new FuncTypeAST();
    $$ = ast;
  }
  ;

Block
  : '{' BlockItemList '}' {
    auto ast = new BlockAST();
    ast->block_item_list = unique_ptr<vector<unique_ptr<BaseAST> >>($2);
    $$ = ast;
  }
  | '{' '}' {
    auto ast = new BlockAST();
    ast->block_item_list = nullptr;
    $$ = ast;
  }
  ;

BlockItemList
  : BlockItem {
    auto ast = new vector<unique_ptr<BaseAST> >();
    ast->push_back(unique_ptr<BaseAST>($1));
    $$ = ast;
  }
  | BlockItemList BlockItem {
    auto ast = static_cast<vector<unique_ptr<BaseAST> >*>($1);
    ast->push_back(unique_ptr<BaseAST>($2));
    $$ = ast;
  }
  ;

BlockItem
  : Decl {
    auto ast = new BlockItemAST();
    ast->type = 1;
    ast->decl = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | Stmt {
    auto ast = new BlockItemAST();
    ast->type = 2;
    ast->stmt = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

Decl
  : ConstDecl {
    auto ast = new DeclAST();
    ast->type = 1;
    ast->const_decl = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | VarDecl {
    auto ast = new DeclAST();
    ast->type = 2;
    ast->var_decl = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

ConstDecl
  : CONST BType ConstDefList ';' { 
    auto ast = new ConstDeclAST();
    ast->btype = unique_ptr<BaseAST>($2);
    ast->const_def_list = unique_ptr<vector<unique_ptr<BaseAST> >>($3);
    $$ = ast;
  }
  ;

BType
  : INT {
    auto ast = new BTypeAST();
    $$ = ast;
  }
  ;

ConstDefList
  : ConstDef {
    auto ast = new vector<unique_ptr<BaseAST> >();
    ast->push_back(unique_ptr<BaseAST>($1));
    $$ = ast;
  }
  | ConstDefList ',' ConstDef {
    auto ast = static_cast<vector<unique_ptr<BaseAST> >*>($1);
    ast->push_back(unique_ptr<BaseAST>($3));
    $$ = ast;
  }
  ;

ConstDef
  : IDENT '=' ConstInitVal {
    auto ast = new ConstDefAST();
    ast->ident = *unique_ptr<string>($1);
    ast->const_init_val = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

ConstInitVal
  : ConstExp {
    auto ast = new ConstInitValAST();
    ast->const_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

ConstExp
  : Exp {
    auto ast = new ConstExpAST();
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

VarDecl
  : BType VarDefList ';' {
    auto ast = new VarDeclAST();
    ast->btype = unique_ptr<BaseAST>($1);
    ast->var_def_list = unique_ptr<vector<unique_ptr<BaseAST> >>($2);
    $$ = ast;
  }
  ;

VarDefList
  : VarDef {
    auto ast = new vector<unique_ptr<BaseAST> >();
    ast->push_back(unique_ptr<BaseAST>($1));
    $$ = ast;
  }
  | VarDefList ',' VarDef {
    auto ast = static_cast<vector<unique_ptr<BaseAST> >*>($1);
    ast->push_back(unique_ptr<BaseAST>($3));
    $$ = ast;
  }
  ;

VarDef
  : IDENT{
    auto ast = new VarDefAST();
    ast->type = 1;
    ast->ident = *unique_ptr<string>($1);
    $$ = ast;
  }
  | IDENT '=' InitVal {
    auto ast = new VarDefAST();
    ast->type = 2;
    ast->ident = *unique_ptr<string>($1);
    ast->init_val = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

InitVal
  : Exp {
    auto ast = new InitValAST();
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

Stmt
  : LVal '=' Exp ';' {
    auto ast = new StmtAST();
    ast->type = StmtAST::ASSIGN;
    ast->lval = unique_ptr<BaseAST>($1);
    ast->exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | Exp ';' {
    auto ast = new StmtAST();
    ast->type = StmtAST::EXP;
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | ';' {
    auto ast = new StmtAST();
    ast->type = StmtAST::EXP;
    ast->exp = nullptr;
    $$ = ast;
  }
  | Block {
    auto ast = new StmtAST();
    ast->type = StmtAST::BLOCK;
    ast->block = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | IfExp ELSE Stmt %prec ELSE{
    auto ast = new StmtAST();
    ast->type = StmtAST::IF_ELSE;
    ast->exp = unique_ptr<BaseAST>($1);
    ast->stmt = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | IfExp %prec LOWER_THAN_ELSE{
    auto ast = new StmtAST();
    ast->type = StmtAST::IF_ELSE;
    ast->exp = unique_ptr<BaseAST>($1);
    ast->stmt = nullptr;
    $$ = ast;
  }
  | WhileExp {
    auto ast = new StmtAST();
    ast->type = StmtAST::WHILE;
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | RETURN Exp ';' {
    auto ast = new StmtAST();
    ast->type = StmtAST::RETURN;
    ast->exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | RETURN ';' {
    auto ast = new StmtAST();
    ast->type = StmtAST::RETURN;
    ast->exp = nullptr;
    $$ = ast;
  }
  ;

IfExp
  : IF '(' Exp ')' Stmt{
    auto ast = new IfExpAST();
    ast->exp = unique_ptr<BaseAST>($3);
    ast->stmt = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  ;

WhileExp
  : WHILE '(' Exp ')' Stmt{
    auto ast = new WhileExpAST();
    ast->type = WhileExpAST::WHILE;
    ast->exp = unique_ptr<BaseAST>($3);
    ast->stmt = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  | CONTINUE ';' {
    auto ast = new WhileExpAST();
    ast->type = WhileExpAST::CONTINUE;
    ast->exp = nullptr;
    ast->stmt = nullptr;
    $$ = ast;
  }
  | BREAK ';' {
    auto ast = new WhileExpAST();
    ast->type = WhileExpAST::BREAK;
    ast->exp = nullptr;
    ast->stmt = nullptr;
    $$ = ast;
  }
  ;

LVal
  : IDENT {
    auto ast = new LValAST();
    ast->ident = *unique_ptr<string>($1);
    $$ = ast;
  }
  ;

Exp
  : LOrExp {
    auto ast = new ExpAST();
    ast->lor_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

LOrExp
  : LAndExp {
    auto ast = new LOrExpAST();
    ast->type = 1;
    ast->land_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | LOrExp LOR LAndExp {
    auto ast = new LOrExpAST();
    ast->type = 2;
    ast->lor_exp = unique_ptr<BaseAST>($1);
    ast->land_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

LAndExp
  : EqExp {
    auto ast = new LAndExpAST();
    ast->type = 1;
    ast->eq_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | LAndExp LAND EqExp {
    auto ast = new LAndExpAST();
    ast->type = 2;
    ast->land_exp = unique_ptr<BaseAST>($1);
    ast->eq_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

EqExp
  : RelExp {
    auto ast = new EqExpAST();
    ast->type = 1;
    ast->rel_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | EqExp EqOp RelExp {
    auto ast = new EqExpAST();
    ast->type = 2;
    ast->eq_op = *unique_ptr<string>($2);
    ast->eq_exp = unique_ptr<BaseAST>($1);
    ast->rel_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

EqOp
  : EQ 
  | NE {
    $$ = $1;
  }
  ;

RelExp
  : AddExp {
    auto ast = new RelExpAST();
    ast->type = 1;
    ast->add_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | RelExp RelOp AddExp {
    auto ast = new RelExpAST();
    ast->type = 2;
    ast->rel_op = *unique_ptr<string>($2);
    ast->rel_exp = unique_ptr<BaseAST>($1);
    ast->add_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

RelOp
  : LT
  | GT
  | LE
  | GE {
    $$ = $1;
  }
  ;

AddExp
  : MulExp {
    auto ast = new AddExpAST();
    ast->type = 1;
    ast->mul_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | AddExp AddOp MulExp {
    auto ast = new AddExpAST();
    ast->type = 2;
    ast->add_op = *unique_ptr<string>($2);
    ast->add_exp = unique_ptr<BaseAST>($1);
    ast->mul_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

AddOp
  : PLUS
  | MINOR {
    $$ = $1;
  }
  ;

MulExp
  : UnaryExp {
    auto ast = new MulExpAST();
    ast->unary_exp = unique_ptr<BaseAST>($1);
    ast->type = 1;
    $$ = ast;
  }
  | MulExp MulOp UnaryExp {
    auto ast = new MulExpAST();
    ast->type = 2;
    ast->mul_op = *unique_ptr<string>($2);
    ast->mul_exp = unique_ptr<BaseAST>($1);
    ast->unary_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

MulOp
  : MUL
  | DIV
  | MOD {
    $$ = $1;
  }
  ;

UnaryExp
  : PrimaryExp {
    auto ast = new UnaryExpAST();
    ast->type = 1;
    ast->primary_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | UnaryOp UnaryExp {
    auto ast = new UnaryExpAST();
    ast->type = 2;
    ast->unary_op = *unique_ptr<string>($1);
    ast->unary_exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  ;

UnaryOp
  : PLUS 
  | MINOR 
  | NOT {
    $$ = $1;
  }
  ;

PrimaryExp
  : '(' Exp ')' {
    auto ast = new PrimaryExpAST();
    ast->type = 1;
    ast->exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | LVal {
    auto ast = new PrimaryExpAST();
    ast->type = 2;
    ast->lval = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | Number {
    auto ast = new PrimaryExpAST();
    ast->type = 3;
    ast->number = $1;
    $$ = ast;
  }
  ;

Number
  : INT_CONST {
    $$ = $1;
  }
  ;

%%

// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
void yyerror(unique_ptr<BaseAST> &ast, const char *s) {
  cerr << "error: " << s << endl;
}

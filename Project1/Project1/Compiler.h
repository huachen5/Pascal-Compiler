#pragma once
#include <string>
#include <vector>
#include <unordered_map>

// 词法 Token 定义 
enum LexTokenType {
    // 基础关键字
    TK_PROGRAM, TK_VAR, TK_INTEGER, TK_REAL, TK_CHAR, TK_BEGIN, TK_END,
    TK_PROCEDURE, TK_IF, TK_THEN, TK_ELSE,
    // 新增：复杂数据类型与控制流关键字
    TK_CONST, TK_TYPE, TK_RECORD, TK_ARRAY, TK_OF, TK_FUNCTION,
    TK_STRING, TK_BOOLEAN, TK_TRUE, TK_FALSE, TK_WHILE, TK_DO,

    // 标识符与常数
    TK_ID, TK_NUM, TK_STR, 

    // 界符与运算符
    TK_COMMA, TK_COLON, TK_SEMI, TK_ASSIGN, TK_MUL, TK_DIV, TK_PLUS, TK_MINUS, TK_DOT, TK_LPAREN, TK_RPAREN,
    TK_EQUAL, TK_LESS, TK_GREATER, TK_LE, TK_GE, TK_NEQ,
    TK_LBRACKET, TK_RBRACKET, TK_DOTDOT, // 新增：[, ], ..

    // 其他
    TK_EOF, TK_ERROR
};

struct LexToken {
    LexTokenType type;
    std::string lexeme;
    int line;
    char tableType;
    int tableIndex;
};

//  后端语义数据结构
struct SymbolItem {
    std::string name;
    std::string type;
    std::string cat;//变量v,常量c，临时变量t，函数f，引用参数ref，过程p
    int level;//作用域层级
    int addr;//存储地址（偏移量）
    bool isActive;//是否仍在作业区内（if等语句）
    std::string strVal;
};

struct Quad {
    std::string op;
    std::string arg1;
    std::string arg2;
    std::string result;
};

class Scanner {
private:
    std::string source;//原代码字符串
    int pos;
    int line;
    std::unordered_map<std::string, LexTokenType> keywords;
    std::unordered_map<std::string, int> keywordIndices;

    std::vector<std::string> symbolTable;//标识符表
    std::vector<std::string> constantTable;//常量表

    char peek();
    char advance();
    bool isAtEnd();
    void skipWhitespace();

    int getSymbolIndex(const std::string& name);
    int getConstantIndex(const std::string& val);

public:
    Scanner(const std::string& src);
    std::vector<LexToken> tokenize();//生成token序列
    static std::wstring formatTokensForDisplay(const std::vector<LexToken>& tokens);

    const std::vector<std::string>& getSymbolTable() const { return symbolTable; }
    const std::vector<std::string>& getConstantTable() const { return constantTable; }
};
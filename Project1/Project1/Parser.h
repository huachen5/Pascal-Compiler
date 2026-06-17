#pragma once
#include "Compiler.h"
#include <vector>
#include <string>

class Parser {
private:
    std::vector<LexToken> tokens;
    int currentPos;
    std::wstring errorMessage;
    bool hasError;

    std::vector<Quad> quadList;
    std::vector<SymbolItem> semanticTable;//符号表
    int tempVarCounter;
    int currentOffset;
    int currentLevel;

    std::string newTemp(const std::string& type);//生成新临时变量

    //========四元式生成函数=========
    void emit(const std::string& op, const std::string& arg1, const std::string& arg2, const std::string& result);
    void enter(const std::string& name, const std::string& type, const std::string& cat, int offset,const std::string& strVal = "");//加入符号表
    SymbolItem* lookup(const std::string& name);//查找符号表（从后向前）

    LexToken peek();
    LexToken advance();
    bool match(LexTokenType expectedType);//匹配当前token为指定类型
    bool isRelationOperator(LexTokenType type);//判定是否为关系运算符
    void reportError(const std::wstring& expectedMsg);

    // 高级文法支持
    void parse_Program();
    void parse_Block();
    void parse_Const();
    void parse_TypeBlock();
    void parse_Variable();
    void parse_ProcFuncDeclaration();
    void parse_IdSequence(std::vector<std::string>& idList);
    void parse_Type(std::string& outType, int& outSize);

    // 语句与控制流
    void parse_Statement();
    void parse_ComSentence();
    void parse_IfSentence();
    void parse_WhileSentence();
    void parse_AssignOrCall(const std::string& targetVar);

    // 表达式与内存访问
    std::string parse_VariableAccess(); // 处理 a[i].name 这种复杂左值
    std::string parse_Expression();
    std::string parse_Term();
    std::string parse_Factor();

public:
    Parser(const std::vector<LexToken>& tokenList);
    bool parse();
    std::wstring getErrorMessage() const;
    const std::vector<Quad>& getQuadList() const { return quadList; }
    const std::vector<SymbolItem>& getSemanticTable() const { return semanticTable; }
};
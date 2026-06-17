#include "Parser.h"

Parser::Parser(const std::vector<LexToken>& tokenList)
    : tokens(tokenList), currentPos(0), hasError(false), tempVarCounter(1), currentOffset(0), currentLevel(0) {
}

std::wstring Parser::getErrorMessage() const {
    return errorMessage;
}

LexToken Parser::peek() { 
    if (currentPos >= tokens.size()) 
        return { TK_EOF, "EOF", -1, 'e', 0 }; 
    return tokens[currentPos]; 
}
LexToken Parser::advance() { 
    LexToken c = peek(); 
    if (c.type != TK_EOF) 
        currentPos++; 
    return c; 
}
void Parser::reportError(const std::wstring& expectedMsg) {
    if (hasError) return;
    hasError = true;

    // 1. 将 peek() 返回的临时对象存入局部变量
    LexToken currentToken = peek();

    // 2. 从同一个对象的 lexeme 中获取 begin 和 end
    std::wstring wLexeme(currentToken.lexeme.begin(), currentToken.lexeme.end());

    errorMessage = L"语法错误 (第 " + std::to_wstring(currentToken.line) + L" 行): \n期待 " + expectedMsg + L", 但遇到了 '" + wLexeme + L"'";
}
bool Parser::match(LexTokenType expectedType) {
    if (hasError) return false;
    if (peek().type == expectedType) { advance(); return true; }
    reportError(L"特定的符号/关键字"); return false;
}
bool Parser::isRelationOperator(LexTokenType type) {
    return type == TK_EQUAL || type == TK_LESS || type == TK_GREATER || type == TK_LE || type == TK_GE || type == TK_NEQ;
}

//======新的临时变量创建函数====
std::string Parser::newTemp(const std::string& type) {
    std::string tName = "t" + std::to_string(tempVarCounter++);
    enter(tName, type, "v", currentOffset); currentOffset += 4; return tName;
}

//======四元式生成函数======
void Parser::emit(const std::string& op, const std::string& arg1, const std::string& arg2, const std::string& res) {
    quadList.push_back({ op, arg1, arg2, res });
}
void Parser::enter(const std::string& name, const std::string& type, const std::string& cat, int offset, const std::string& strVal) {
    semanticTable.push_back({ name, type, cat, currentLevel, offset, true, strVal });
}
SymbolItem* Parser::lookup(const std::string& name) {
    for (int i = semanticTable.size() - 1; i >= 0; --i) {
        if (semanticTable[i].isActive && semanticTable[i].name == name) return &semanticTable[i];
    }
    return nullptr;
}

bool Parser::parse() {
    parse_Program();
    if (!hasError && peek().type != TK_EOF) 
    { 
        hasError = true; 
        errorMessage = L"语法错误: 代码结尾有多余内容。"; 
    }
    return !hasError;
}

//入口，识别program
void Parser::parse_Program() {
    if (hasError) return;
    match(TK_PROGRAM);

    std::string progName = peek().lexeme;
    match(TK_ID);

    // 程序名 P 位于第0层
    enter(progName, "program", "p", 0);

    if (peek().type == TK_SEMI) advance();

    emit("program", progName, "_", "_");
    parse_Block();                      //递归调用子程序
    match(TK_DOT);
    emit("end", progName, "_", "_");
}

void Parser::parse_Block() {
    if (hasError) return;

    while (!hasError) {
        LexTokenType t = peek().type;
        if (t == TK_CONST) parse_Const();
        else if (t == TK_TYPE) parse_TypeBlock();
        else if (t == TK_VAR) parse_Variable();
        else if (t == TK_PROCEDURE || t == TK_FUNCTION) parse_ProcFuncDeclaration();
        else break;
    }
    parse_ComSentence();
}

void Parser::parse_Const() {
    match(TK_CONST);
    while (peek().type == TK_ID && !hasError) {
        std::string cname = peek().lexeme;
        advance();
        if (peek().type == TK_EQUAL || peek().type == TK_ASSIGN)
            advance();

        std::string cvalStr = peek().lexeme;
        int cval = 0;
        try { cval = std::stoi(cvalStr); }
        catch (...) {}

        advance();
        std::string constType = "i"; // 默认当作整数 "i"
        if (!cvalStr.empty() && cvalStr.front() == '\'') {
            constType = "c";         // 如果有单引号，说明是字符，改成 "c"
        }
        else if (cvalStr.find('.') != std::string::npos) {
            constType = "r";         // 如果包含小数点，存为实数 "r"
        }
        enter(cname, constType, "c", cval, cvalStr); // 用新算出来的 constType 填进表里
        match(TK_SEMI);
    }
}

void Parser::parse_TypeBlock() {
    match(TK_TYPE);
    while (peek().type == TK_ID && !hasError) {
        std::string typeName = peek().lexeme;
        advance();
        match(TK_EQUAL);

        int typeSize = 0;
        if (peek().type == TK_RECORD) {
            advance();
            int offset = 0;
            while (peek().type != TK_END && !hasError) {
                std::vector<std::string> fieldIds;
                parse_IdSequence(fieldIds);
                match(TK_COLON);
                std::string fieldType;
                int fieldSize = 0;
                parse_Type(fieldType, fieldSize);
                match(TK_SEMI);

                for (const auto& id : fieldIds) {
                    enter(typeName + "." + id, fieldType, "d", fieldSize);
                    offset += fieldSize;
                }
            }
            match(TK_END);
            typeSize = offset;
            enter(typeName, "record", "type", typeSize);
        }
        else if (peek().type == TK_ARRAY) {
            advance();
            match(TK_LBRACKET);

            bool negLow = false;
            if (peek().type == TK_MINUS) { negLow = true; advance(); }
            std::string lowBoundStr = peek().lexeme;
            if (peek().type == TK_NUM || peek().type == TK_ID) advance();

            match(TK_DOTDOT);

            bool negHigh = false;
            if (peek().type == TK_MINUS) { negHigh = true; advance(); }
            std::string highBoundStr = peek().lexeme;
            if (peek().type == TK_NUM || peek().type == TK_ID) advance();

            match(TK_RBRACKET);
            match(TK_OF);

            std::string elemType;
            int elemSize = 0;
            parse_Type(elemType, elemSize);

            auto getBoundValue = [&](const std::string& boundStr, bool isNeg) -> int {
                int val = 0;
                try { val = std::stoi(boundStr); }
                catch (...) {
                    SymbolItem* sym = lookup(boundStr);
                    if (sym && sym->cat == "c") val = sym->addr;
                }
                return isNeg ? -val : val;
                };

            int l = getBoundValue(lowBoundStr, negLow);
            int h = getBoundValue(highBoundStr, negHigh);
            int count = h - l + 1;
            if (count <= 0) count = 1;
            typeSize = count * elemSize;

            enter(typeName, "array of " + elemType, "type", typeSize);
        }
        else {
            std::string baseType;
            parse_Type(baseType, typeSize);
            enter(typeName, baseType, "type", typeSize);
        }
        match(TK_SEMI);
    }
}

void Parser::parse_Variable() {
    match(TK_VAR);
    while (peek().type == TK_ID && !hasError) {
        std::vector<std::string> ids;
        parse_IdSequence(ids);
        match(TK_COLON);
        std::string typeName;
        int typeSize;
        parse_Type(typeName, typeSize);
        match(TK_SEMI);
        for (const auto& id : ids) {
            enter(id, typeName, "v", currentOffset);
            currentOffset += typeSize;
        }
    }
}

void Parser::parse_IdSequence(std::vector<std::string>& idList) {
    idList.push_back(peek().lexeme);
    match(TK_ID);
    while (peek().type == TK_COMMA) {
        advance();
        idList.push_back(peek().lexeme);
        match(TK_ID);
    }
}

void Parser::parse_Type(std::string& outType, int& outSize) {
    if (peek().type == TK_ID) {
        outType = peek().lexeme;
        SymbolItem* sym = lookup(outType);
        if (sym && sym->cat == "type") {
            outSize = sym->addr;
        }
        else {
            outSize = 4;
        }
        advance();
        return;
    }
    LexTokenType t = peek().type;
    if (t == TK_INTEGER) {
        outType = "i"; outSize = 4; advance();
    }
    else if (t == TK_REAL) {
        outType = "r"; outSize = 8; advance();
    }
    else if (t == TK_CHAR) {
        outType = "c"; outSize = 1; advance();
    }
    else if (t == TK_STRING) {
        outType = "string";
        outSize = 20;
        advance();
        if (peek().type == TK_LBRACKET) {
            advance();
            if (peek().type == TK_NUM) {
                try { outSize = std::stoi(peek().lexeme); }
                catch (...) {}
                advance();
            }
            match(TK_RBRACKET);
        }
    }
    else if (t == TK_BOOLEAN) {
        outType = "b"; outSize = 1; advance();
    }
    else reportError(L"类型定义");
}

void Parser::parse_ProcFuncDeclaration() {
    if (hasError) return;
    bool isFunc = (peek().type == TK_FUNCTION);
    advance();   // 吃掉 procedure / function
    std::string name = peek().lexeme;
    match(TK_ID);

    // 进入新作用域
    currentLevel++;
    int oldOffset = currentOffset;
    currentOffset = 0;
    int scopeStartIndex = semanticTable.size();

    // 先登记函数/过程名（占位）
    enter(name, isFunc ? "func" : "void", "f", 0);  // addr 暂为0
    int funcIndex = semanticTable.size() - 1;       // 记录位置

    int paramCount = 0;

    // 解析参数列表
    if (peek().type == TK_LPAREN) {
        advance();
        while (!hasError && peek().type != TK_RPAREN) {
            bool isVar = false;
            if (peek().type == TK_VAR) {
                isVar = true;
                advance();
            }

            std::vector<std::string> pIds;
            parse_IdSequence(pIds);
            match(TK_COLON);
            std::string pType;
            int pSize;
            parse_Type(pType, pSize);
            for (const auto& id : pIds) {
                enter(id, pType, isVar ? "vn" : "vf", currentOffset);
                currentOffset += pSize;
                paramCount++;
            }
            if (peek().type == TK_SEMI)
                advance();
            else
                break;
        }
        match(TK_RPAREN);
    }

    // 解析函数返回值类型
    std::string retType = "void";
    if (isFunc) {
        match(TK_COLON);
        int retSize;
        parse_Type(retType, retSize);
    }

    // 更新已登记的函数/过程的类型和参数个数
    semanticTable[funcIndex].type = retType;
    semanticTable[funcIndex].addr = paramCount;   // 存储参数个数

    match(TK_SEMI);

    emit(isFunc ? "function" : "procedure", name, "_", "_");
    parse_Block();
    emit("endproc", name, "_", "_");

    if (peek().type == TK_SEMI)
        advance();

    // 退出作用域：将本层所有符号（包括函数名本身）标记为不可见
    for (size_t i = scopeStartIndex; i < semanticTable.size(); i++)
        semanticTable[i].isActive = false;

    currentLevel--;
    currentOffset = oldOffset;
}

void Parser::parse_Statement() {
    if (hasError) return;
    if (peek().type == TK_IF) parse_IfSentence();
    else if (peek().type == TK_WHILE) parse_WhileSentence();
    else if (peek().type == TK_BEGIN) parse_ComSentence();
    else if (peek().type == TK_ID) {
        std::string target = parse_VariableAccess();
        parse_AssignOrCall(target);
    }
}

void Parser::parse_ComSentence() {
    if (hasError) return;
    match(TK_BEGIN);
    parse_Statement();
    while (peek().type == TK_SEMI) {
        advance();
        if (peek().type == TK_END) break;
        parse_Statement();
    }
    match(TK_END);
}

std::string Parser::parse_VariableAccess() {
    std::string res = peek().lexeme;
    match(TK_ID);
    while (peek().type == TK_LBRACKET || peek().type == TK_DOT) {
        if (peek().type == TK_LBRACKET) {
            res += "["; advance();
            res += parse_Expression();
            match(TK_RBRACKET);
            res += "]";
        }
        else if (peek().type == TK_DOT) {
            res += ".";
            advance();
            res += peek().lexeme;
            match(TK_ID);
        }
    }
    return res;
}

//赋值四元式生成函数
void Parser::parse_AssignOrCall(const std::string& target) {
    if (peek().type == TK_ASSIGN || peek().type == TK_EQUAL) {
        advance();
        std::string expr = parse_Expression();
        emit(":=", expr, "_", target);
    }
    else if (peek().type == TK_LPAREN) {
        advance();
        while (!hasError && peek().type != TK_RPAREN) {
            std::string arg = parse_Expression();
            emit("param", arg, "_", "_");
            if (peek().type == TK_COMMA) advance();
            else break;
        }
        match(TK_RPAREN);
        emit("call", target, "_", "_");
    }
}

void Parser::parse_IfSentence() {
    match(TK_IF);

    // 1. 解析条件表达式
    std::string arg1 = parse_Expression();
    std::string op = "=="; // 默认操作符
    std::string arg2 = "true";
    if (isRelationOperator(peek().type)) {
        op = peek().lexeme;
        advance();
        arg2 = parse_Expression();
    }
    match(TK_THEN);

    // 2. 生成关系运算，存入临时变量 t
    std::string t = newTemp("b"); // b代表布尔型临时变量
    emit(op, arg1, arg2, t);      // 例如: ( <, a, 500, t1 )

    // 3. 生成结构化 if 四元式
    emit("if", t, "_", "_");      // 例如: ( if, t1, _, _ )

    // 4. 解析 then 语句块
    parse_Statement();

    // 5. 处理 else 结构
    if (peek().type == TK_ELSE) {
        emit("el", "_", "_", "_"); // 遇到 else: ( el, _, _, _ )
        match(TK_ELSE);
        parse_Statement();
    }

    // 6. 结束标记
    emit("ie", "_", "_", "_");     // if 结束: ( ie, _, _, _ )
}

void Parser::parse_WhileSentence() {
    match(TK_WHILE);

    // 1. 标记 while 循环开始
    emit("wh", "_", "_", "_");     // ( wh, _, _, _ )

    // 2. 解析条件表达式
    std::string arg1 = parse_Expression();
    std::string op = "==";
    std::string arg2 = "true";
    if (isRelationOperator(peek().type)) {
        op = peek().lexeme;
        advance();
        arg2 = parse_Expression();
    }
    match(TK_DO);

    // 3. 生成关系运算，存入临时变量 t
    std::string t = newTemp("b");
    emit(op, arg1, arg2, t);

    // 4. 生成 do 四元式，带上条件判断结果
    emit("do", t, "_", "_");       // ( do, t1, _, _ )

    // 5. 解析循环体
    parse_Statement();

    // 6. 标记 while 循环结束
    emit("we", "_", "_", "_");     // ( we, _, _, _ )
}

//运算四元式生成函数
std::string Parser::parse_Expression() {
    std::string left = parse_Term();
    while (peek().type == TK_PLUS || peek().type == TK_MINUS) {
        std::string op = peek().lexeme;
        advance();
        std::string right = parse_Term();
        std::string result = newTemp("i");
        emit(op, left, right, result);
        left = result;
    }
    return left;
}

std::string Parser::parse_Term() {
    std::string left = parse_Factor();
    while (peek().type == TK_MUL || peek().type == TK_DIV) {
        std::string op = peek().lexeme;
        advance();
        std::string right = parse_Factor();
        std::string result = newTemp("i");
        emit(op, left, right, result);
        left = result;
    }
    return left;
}

std::string Parser::parse_Factor() {
    LexToken c = peek();
    if (c.type == TK_ID) {
        std::string name = parse_VariableAccess();
        if (peek().type == TK_LPAREN) {
            advance();
            while (!hasError && peek().type != TK_RPAREN) {
                std::string arg = parse_Expression();
                emit("param", arg, "_", "_");
                if (peek().type == TK_COMMA) advance();
                else break;
            }
            match(TK_RPAREN);
            std::string res = newTemp("i");
            emit("call", name, "_", res);
            return res;
        }
        return name;
    }
    else if (c.type == TK_NUM || c.type == TK_STR) {
        advance(); return c.lexeme;
    }
    else if (c.type == TK_TRUE || c.type == TK_FALSE) {
        advance(); return c.lexeme;
    }
    else if (c.type == TK_LPAREN) {
        advance();
        std::string res = parse_Expression();
        match(TK_RPAREN);
        return res;
    }
    reportError(L"标识符、常数或表达式");
    return "";
}
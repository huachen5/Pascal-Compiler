#include "Compiler.h"
#include <cctype>
#include <sstream>

Scanner::Scanner(const std::string& src) : source(src), pos(0), line(1) {
    keywords["program"] = TK_PROGRAM;      keywordIndices["program"] = 1;
    keywords["var"] = TK_VAR;              keywordIndices["var"] = 2;
    keywords["integer"] = TK_INTEGER;      keywordIndices["integer"] = 3;
    keywords["real"] = TK_REAL;            keywordIndices["real"] = 4;
    keywords["char"] = TK_CHAR;            keywordIndices["char"] = 5;
    keywords["begin"] = TK_BEGIN;          keywordIndices["begin"] = 6;
    keywords["end"] = TK_END;              keywordIndices["end"] = 7;
    keywords["procedure"] = TK_PROCEDURE;  keywordIndices["procedure"] = 8;
    keywords["if"] = TK_IF;                keywordIndices["if"] = 9;
    keywords["then"] = TK_THEN;            keywordIndices["then"] = 10;
    keywords["else"] = TK_ELSE;            keywordIndices["else"] = 11;
    keywords["const"] = TK_CONST;          keywordIndices["const"] = 12;
    keywords["type"] = TK_TYPE;            keywordIndices["type"] = 13;
    keywords["record"] = TK_RECORD;        keywordIndices["record"] = 14;
    keywords["array"] = TK_ARRAY;          keywordIndices["array"] = 15;
    keywords["of"] = TK_OF;                keywordIndices["of"] = 16;
    keywords["function"] = TK_FUNCTION;    keywordIndices["function"] = 17;
    keywords["string"] = TK_STRING;        keywordIndices["string"] = 18;
    keywords["boolean"] = TK_BOOLEAN;      keywordIndices["boolean"] = 19;
    keywords["true"] = TK_TRUE;            keywordIndices["true"] = 20;
    keywords["false"] = TK_FALSE;          keywordIndices["false"] = 21;
    keywords["while"] = TK_WHILE;          keywordIndices["while"] = 22;
    keywords["do"] = TK_DO;                keywordIndices["do"] = 23;
}

char Scanner::peek() { if (isAtEnd()) return '\0'; return source[pos]; }
char Scanner::advance() { return source[pos++]; }
bool Scanner::isAtEnd() { return pos >= source.length(); }

void Scanner::skipWhitespace() {//处理空格等符号
    while (!isAtEnd()) {
        char c = peek();
        if (c == ' ' || c == '\r' || c == '\t') advance();
        else if (c == '\n') { line++; advance(); }
        else if (c == '{') {
            while (!isAtEnd() && peek() != '}') {
                if (peek() == '\n') line++;
                advance();
            }
            if (!isAtEnd()) advance();
        }
        else break;
    }
}

int Scanner::getSymbolIndex(const std::string& name) {
    auto it = std::find(symbolTable.begin(), symbolTable.end(), name);
    if (it != symbolTable.end()) return std::distance(symbolTable.begin(), it) + 1;
    symbolTable.push_back(name);
    return symbolTable.size();
}

int Scanner::getConstantIndex(const std::string& val) {
    auto it = std::find(constantTable.begin(), constantTable.end(), val);
    if (it != constantTable.end()) return std::distance(constantTable.begin(), it) + 1;
    constantTable.push_back(val); return constantTable.size();
}

std::vector<LexToken> Scanner::tokenize() {
    std::vector<LexToken> tokens;
    while (!isAtEnd()) {
        skipWhitespace();
        if (isAtEnd()) break;
        char c = advance();

        if (isalpha(c) || c == '_') {//判定标识符或关键字
            std::string lexeme; lexeme += c;
            while (isalnum(peek()) || peek() == '_') lexeme += advance();
            if (keywords.count(lexeme)) tokens.push_back({ keywords[lexeme], lexeme, line, 'k', keywordIndices[lexeme] });
            else { int i_idx = getSymbolIndex(lexeme); tokens.push_back({ TK_ID, lexeme, line, 'i', i_idx }); }
        }
        else if (isdigit(c)) {//判定常数
            std::string lexeme; lexeme += c;
            while (isdigit(peek())) lexeme += advance();
            if (peek() == '.' && (pos + 1) < source.length() && source[pos + 1] != '.') {
                lexeme += advance(); // 吞掉这个小数点 '.'
                while (isdigit(peek())) lexeme += advance(); // 继续吞掉小数部分
            }
            int c_idx = getConstantIndex(lexeme);
            tokens.push_back({ TK_NUM, lexeme, line, 'c', c_idx });
        }
        else if (c == '\'') {//判断是否是字符串常量，存放在C表中
            std::string strVal = "";
            while (!isAtEnd() && peek() != '\'') strVal += advance();
            if (!isAtEnd()) advance(); // 吞掉右侧单引号
            int c_idx = getConstantIndex("'" + strVal + "'");
            tokens.push_back({ TK_STR, "'" + strVal + "'", line, 'c', c_idx });
        }
        else {
            switch (c) {
            case ',': tokens.push_back({ TK_COMMA, ",", line, 'p', 1 }); break;
            case ';': tokens.push_back({ TK_SEMI, ";", line, 'p', 3 }); break;
            case '*': tokens.push_back({ TK_MUL, "*", line, 'p', 5 }); break;
            case '/': tokens.push_back({ TK_DIV, "/", line, 'p', 6 }); break;
            case '+': tokens.push_back({ TK_PLUS, "+", line, 'p', 7 }); break;
            case '-': tokens.push_back({ TK_MINUS, "-", line, 'p', 8 }); break;
            case '(': tokens.push_back({ TK_LPAREN, "(", line, 'p', 10 }); break;
            case ')': tokens.push_back({ TK_RPAREN, ")", line, 'p', 11 }); break;
            case '[': tokens.push_back({ TK_LBRACKET, "[", line, 'p', 18 }); break;
            case ']': tokens.push_back({ TK_RBRACKET, "]", line, 'p', 19 }); break;
            case '=': tokens.push_back({ TK_EQUAL, "=", line, 'p', 12 }); break;
            case '.':
                if (peek() == '.') { advance(); tokens.push_back({ TK_DOTDOT, "..", line, 'p', 20 }); }
                else tokens.push_back({ TK_DOT, ".", line, 'p', 9 }); break;
            case ':':
                if (peek() == '=') { advance(); tokens.push_back({ TK_ASSIGN, ":=", line, 'p', 4 }); }
                else tokens.push_back({ TK_COLON, ":", line, 'p', 2 }); break;
            case '<':
                if (peek() == '=') { advance(); tokens.push_back({ TK_LE, "<=", line, 'p', 13 }); }
                else if (peek() == '>') { advance(); tokens.push_back({ TK_NEQ, "<>", line, 'p', 14 }); }
                else tokens.push_back({ TK_LESS, "<", line, 'p', 15 }); break;
            case '>':
                if (peek() == '=') { advance(); tokens.push_back({ TK_GE, ">=", line, 'p', 16 }); }
                else tokens.push_back({ TK_GREATER, ">", line, 'p', 17 }); break;
            default: tokens.push_back({ TK_ERROR, std::string(1, c), line, 'e', 0 }); break;
            }
        }
    }
    return tokens;
}

//词法分析页面输出
std::wstring Scanner::formatTokensForDisplay(const std::vector<LexToken>& tokens) {
    std::wstring result = L"词法分析报告 (二元组对照)：\n\n";
    int currentLine = -1;
    std::wstring lineStr = L"";
    int tokenCountInRow = 0;
    for (const auto& tk : tokens) {
        if (tk.type == TK_EOF) continue;
        if (tk.line != currentLine) {//行号切换逻辑
            if (currentLine != -1)
                result += lineStr + L"\n";
            currentLine = tk.line;
            tokenCountInRow = 0;
            lineStr = L"Line ";
            if (tk.line < 10)
                lineStr += L" ";
            lineStr += std::to_wstring(tk.line) + L":  ";
        }
        if (tokenCountInRow >= 5) {
            result += lineStr + L"\n"; lineStr = L"          "; tokenCountInRow = 0;
        }
        std::wstring tokenStr;
        if (tk.type == TK_ERROR) {
            std::wstring wLexeme(tk.lexeme.begin(), tk.lexeme.end());
            tokenStr = L"【非法: '" + wLexeme + L"'】";
        }
        else {
            std::wstring typeChar(1, tk.tableType);
            std::wstring wLexeme(tk.lexeme.begin(), tk.lexeme.end());
            tokenStr = L"(" + typeChar + L"," + std::to_wstring(tk.tableIndex) + L")[" + wLexeme + L"]";
        }
        while (tokenStr.length() < 20) tokenStr += L" "; // 加宽对齐以适应字符串
        lineStr += tokenStr; tokenCountInRow++;
    }
    if (!lineStr.empty()) result += lineStr + L"\n";
    return result;
}
#pragma once
#include "Compiler.h"
#include <vector>
#include <string>
#include <stack>
#include <set>
#include <sstream>

class TargetGenerator {
public:
    TargetGenerator(const std::vector<Quad>& quads);
    // 直接返回宽字符串，完美适配 EasyX
    std::wstring generate();

private:
    std::vector<Quad> quadList;
    int labelCounter;

    std::stack<std::wstring> ifElseLabelStack;
    std::stack<std::wstring> ifEndLabelStack;
    std::stack<std::wstring> whileStartLabelStack;
    std::stack<std::wstring> whileEndLabelStack;

    std::wstring newLabel(const std::wstring& prefix);
    bool isNumber(const std::string& str);
    bool isVariable(const std::string& str);

    std::set<std::string> extractVariables();
};
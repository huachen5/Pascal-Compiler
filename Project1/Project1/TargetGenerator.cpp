#include "TargetGenerator.h"

TargetGenerator::TargetGenerator(const std::vector<Quad>& quads)
    : quadList(quads), labelCounter(0) {}

bool TargetGenerator::isNumber(const std::string& str) {
    if (str.empty() || str == "_") return false;
    for (char c : str) {
        if (!isdigit(c) && c != '.' && c != '-') return false;
    }
    return true;
}

bool TargetGenerator::isVariable(const std::string& str) {
    return !str.empty() && str != "_" && !isNumber(str);
}

std::wstring TargetGenerator::newLabel(const std::wstring& prefix) {
    return prefix + L"_" + std::to_wstring(labelCounter++);
}

std::set<std::string> TargetGenerator::extractVariables() {
    std::set<std::string> vars;
    for (const auto& q : quadList) {
        if (isVariable(q.arg1)) vars.insert(q.arg1);
        if (isVariable(q.arg2)) vars.insert(q.arg2);
        if (isVariable(q.result)) vars.insert(q.result);
    }
    return vars;
}

std::wstring TargetGenerator::generate() {
    std::wstringstream asmCode;

    asmCode << L"DATA SEGMENT\n";
    std::set<std::string> vars = extractVariables();    //扫描四元式
    for (const auto& v : vars) {
        std::wstring wv(v.begin(), v.end());
        asmCode << L"    " << wv << L" DW 0\n";
    }
    asmCode << L"DATA ENDS\n\n";

    asmCode << L"CODE SEGMENT\n";
    asmCode << L"    ASSUME CS:CODE, DS:DATA\n";
    asmCode << L"START:\n";
    asmCode << L"    MOV AX, DATA\n";
    asmCode << L"    MOV DS, AX\n\n";

    for (const auto& q : quadList) {
        std::wstring wOp(q.op.begin(), q.op.end());
        std::wstring wArg1(q.arg1.begin(), q.arg1.end());
        std::wstring wArg2(q.arg2.begin(), q.arg2.end());
        std::wstring wRes(q.result.begin(), q.result.end());

        asmCode << L"    ; (" << wOp << L", " << wArg1 << L", " << wArg2 << L", " << wRes << L")\n";

        if (q.op == "+") {
            asmCode << L"    MOV AX, " << wArg1 << L"\n";
            asmCode << L"    MOV BX, " << wArg2 << L"\n";
            asmCode << L"    ADD AX, BX\n";
            asmCode << L"    MOV " << wRes << L", AX\n";
        }
        else if (q.op == "-") {
            asmCode << L"    MOV AX, " << wArg1 << L"\n";
            asmCode << L"    MOV BX, " << wArg2 << L"\n";
            asmCode << L"    SUB AX, BX\n";
            asmCode << L"    MOV " << wRes << L", AX\n";
        }
        else if (q.op == "*") {
            asmCode << L"    MOV AX, " << wArg1 << L"\n";
            asmCode << L"    MOV BX, " << wArg2 << L"\n";
            asmCode << L"    IMUL BX\n";
            asmCode << L"    MOV " << wRes << L", AX\n";
        }
        else if (q.op == "/") {
            asmCode << L"    MOV AX, " << wArg1 << L"\n";
            asmCode << L"    CWD\n";
            asmCode << L"    MOV BX, " << wArg2 << L"\n";
            asmCode << L"    IDIV BX\n";
            asmCode << L"    MOV " << wRes << L", AX\n";
        }
        else if (q.op == ":=") {
            asmCode << L"    MOV AX, " << wArg1 << L"\n";
            asmCode << L"    MOV " << wRes << L", AX\n";
        }
        else if (q.op == "==" || q.op == "<" || q.op == ">" || q.op == "<=" || q.op == ">=" || q.op == "<>") {
            std::wstring lTrue = newLabel(L"L_CMP_TRUE");
            std::wstring lEnd = newLabel(L"L_CMP_END");

            asmCode << L"    MOV AX, " << wArg1 << L"\n";
            asmCode << L"    MOV BX, " << wArg2 << L"\n";
            asmCode << L"    CMP AX, BX\n";

            if (q.op == "==") asmCode << L"    JE " << lTrue << L"\n";
            else if (q.op == "<") asmCode << L"    JL " << lTrue << L"\n";
            else if (q.op == ">") asmCode << L"    JG " << lTrue << L"\n";
            else if (q.op == "<=") asmCode << L"    JLE " << lTrue << L"\n";
            else if (q.op == ">=") asmCode << L"    JGE " << lTrue << L"\n";
            else if (q.op == "<>") asmCode << L"    JNE " << lTrue << L"\n";

            asmCode << L"    MOV " << wRes << L", 0\n";
            asmCode << L"    JMP " << lEnd << L"\n";
            asmCode << lTrue << L":\n";
            asmCode << L"    MOV " << wRes << L", 1\n";
            asmCode << lEnd << L":\n";
        }
        else if (q.op == "if") {
            std::wstring elseLabel = newLabel(L"L_ELSE");
            std::wstring endLabel = newLabel(L"L_IF_END");
            ifElseLabelStack.push(elseLabel);
            ifEndLabelStack.push(endLabel);

            asmCode << L"    MOV AX, " << wArg1 << L"\n";
            asmCode << L"    CMP AX, 0\n";
            asmCode << L"    JE " << elseLabel << L"\t\t ; Jump to else/end if false\n";
        }
        else if (q.op == "el") {
            std::wstring endLabel = ifEndLabelStack.top();
            std::wstring elseLabel = ifElseLabelStack.top();
            ifElseLabelStack.pop();

            asmCode << L"    JMP " << endLabel << L"\n";
            asmCode << elseLabel << L":\n";
        }
        else if (q.op == "ie") {
            if (!ifElseLabelStack.empty()) {
                asmCode << ifElseLabelStack.top() << L":\n";
                ifElseLabelStack.pop();
            }
            asmCode << ifEndLabelStack.top() << L":\n";
            ifEndLabelStack.pop();
        }
        else if (q.op == "wh") {
            std::wstring startLabel = newLabel(L"L_WHILE_START");
            std::wstring endLabel = newLabel(L"L_WHILE_END");
            whileStartLabelStack.push(startLabel);
            whileEndLabelStack.push(endLabel);

            asmCode << startLabel << L":\n";
        }
        else if (q.op == "do") {
            asmCode << L"    MOV AX, " << wArg1 << L"\n";
            asmCode << L"    CMP AX, 0\n";
            asmCode << L"    JE " << whileEndLabelStack.top() << L"\t ; Jump to end if false\n";
        }
        else if (q.op == "we") {
            asmCode << L"    JMP " << whileStartLabelStack.top() << L"\n";
            asmCode << whileEndLabelStack.top() << L":\n";
            whileStartLabelStack.pop();
            whileEndLabelStack.pop();
        }
        else if (q.op == "end") {
            asmCode << L"    MOV AH, 4CH\n";
            asmCode << L"    INT 21H\t\t\t\n";
        }
        asmCode << L"\n";
    }

    asmCode << L"CODE ENDS\n";
    asmCode << L"    END START\n";

    return asmCode.str();
}
#include "Optimizer.h"
#include <algorithm>
#include <set>

bool Optimizer::isNumber(const std::string& str) {
    if (str.empty() || str == "_") return false;
    for (char c : str) {
        if (!isdigit(c) && c != '.' && c != '-')
            return false;
    }
    return true;
}

// --- 辅助函数，判断是否为内部临时变量 (如 t1, t2, t3...) ---
static bool isTempVar(const std::string& name) {
    if (name.empty()) return false;
    if (name[0] == 't' && name.length() > 1) {
        for (size_t i = 1; i < name.length(); ++i) {
            if (!isdigit(name[i])) return false;
        }
        return true;
    }
    return false;
}

// 辅助函数：对单个基础块进行 DAG 优化 
static std::vector<Quad> optimizeBasicBlock(const std::vector<Quad>& blockQuads) {
    if (blockQuads.empty()) return {};

    std::vector<DAGNode> nodes;
    std::unordered_map<std::string, int> varToNode;
    int nextId = 0;

    auto getOrCreateLeaf = [&](const std::string& val) -> int {
        if (val == "_") return -1;
        if (varToNode.count(val))
            return varToNode[val];
        DAGNode newNode = { nextId++, "", -1, -1, val, {} };
        if (!Optimizer::isNumber(val))
            newNode.vars.push_back(val);
        nodes.push_back(newNode);
        if (!Optimizer::isNumber(val))
            varToNode[val] = newNode.id;
        return newNode.id;
        };

    // 构建 DAG 图
    for (const auto& q : blockQuads) {
        if (q.op == ":=") {
            int leftId = getOrCreateLeaf(q.arg1);
            varToNode[q.result] = leftId;
            auto& vList = nodes[leftId].vars;
            if (std::find(vList.begin(), vList.end(), q.result) == vList.end()) {
                vList.push_back(q.result);
            }
        }
        else {
            int leftId = getOrCreateLeaf(q.arg1);
            int rightId = getOrCreateLeaf(q.arg2);

            // 先判断是不是算术运算符，只有加减乘除才能触发常数折叠
            bool isArithmetic = (q.op == "+" || q.op == "-" || q.op == "*" || q.op == "/");

            if (isArithmetic && Optimizer::isNumber(nodes[leftId].value) && Optimizer::isNumber(nodes[rightId].value)) {
                int lVal = std::stoi(nodes[leftId].value);
                int rVal = std::stoi(nodes[rightId].value);
                int resVal = 0;

                if (q.op == "+") resVal = lVal + rVal;
                else if (q.op == "-") resVal = lVal - rVal;
                else if (q.op == "*") resVal = lVal * rVal;
                else if (q.op == "/") resVal = lVal / rVal;

                int constId = getOrCreateLeaf(std::to_string(resVal));
                varToNode[q.result] = constId;
                nodes[constId].vars.push_back(q.result);
                continue;
            }

            // 如果不是常数算术运算（> < ==），就建新节点
            int foundNodeId = -1;
            for (const auto& node : nodes) {
                if (node.op == q.op && node.left == leftId && node.right == rightId) {
                    foundNodeId = node.id;
                    break;
                }
            }

            if (foundNodeId != -1) {
                varToNode[q.result] = foundNodeId;
                nodes[foundNodeId].vars.push_back(q.result);
            }
            else {
                DAGNode newNode = { nextId++, q.op, leftId, rightId, "", {q.result} };
                nodes.push_back(newNode);
                varToNode[q.result] = newNode.id;
            }
        }
    }

    // 导出 DAG 图 
    std::vector<Quad> optimized;
    auto getBestArgName = [&](int nodeId) -> std::string {
        if (nodeId == -1) return "_";

        // 优先寻找挂载在这个节点上的“真实变量名”
        for (const auto& v : nodes[nodeId].vars) {
            if (!isTempVar(v)) return v;                 // 只要有真变量，不管是不是常数，一律返回变量名
        }

        // 如没有真实变量，返回数值本身
        if (Optimizer::isNumber(nodes[nodeId].value)) return nodes[nodeId].value;

        //返回临时变量
        return nodes[nodeId].vars.empty() ? nodes[nodeId].value : nodes[nodeId].vars[0];
        };

    for (const auto& node : nodes) {
        if (node.vars.empty() && node.op == "") continue; // 跳过无用的纯常数叶子节点

        std::string mainVar = "";

        // 尝试找一个非临时变量作为主变量（比如 a, b, c）
        auto it = std::find_if(node.vars.begin(), node.vars.end(), [](const std::string& v) { return !isTempVar(v); });

        if (it != node.vars.end()) {
            mainVar = *it;
        }
        else if (!node.vars.empty()) {
            // 把第一个临时变量作为 mainVar 导出来，以防它被后续的 if/while 用到
            mainVar = node.vars[0];
        }
        else {
            continue;
        }

        if (node.op != "") {
            std::string arg1Str = getBestArgName(node.left);
            std::string arg2Str = getBestArgName(node.right);
            optimized.push_back({ node.op, arg1Str, arg2Str, mainVar });
        }
        else if (Optimizer::isNumber(node.value) && it != node.vars.end()) {
            // 只有当有真实变量存在时，才导出 a := 常数 的指令
            optimized.push_back({ ":=", node.value, "_", mainVar });
        }

        // 只给真实的变量赋值，不给临时变量产生多余的赋值指令
        for (const auto& v : node.vars) {
            if (v != mainVar && !isTempVar(v)) {
                optimized.push_back({ ":=", mainVar, "_", v });
            }
        }
    }
    return optimized;
}


//支持结构化控制流的 DAG 优化 ---
std::vector<Quad> Optimizer::optimizeDAG(const std::vector<Quad>& originalQuads) {
    if (originalQuads.empty()) return {};

    std::set<int> leaders;
    leaders.insert(0);

    for (size_t i = 0; i < originalQuads.size(); ++i) {
        const auto& q = originalQuads[i];

        // 识别结构化控制流标记，基本块的边界
        if (q.op == "if" || q.op == "el" || q.op == "ie" ||
            q.op == "wh" || q.op == "do" || q.op == "we" ||
            q.op == "function" || q.op == "procedure" ||
            q.op == "endproc" || q.op == "program" || q.op == "end") {
            leaders.insert(i);
            if (i + 1 < originalQuads.size()) {
                leaders.insert(i + 1);
            }
        }
    }

    std::vector<Quad> result;
    std::vector<Quad> currentBlock;

    auto flushBlock = [&]() {
        if (currentBlock.empty()) return;
        std::string firstOp = currentBlock[0].op;
        bool canOptimize = (firstOp == ":=" || firstOp == "+" || firstOp == "-" || firstOp == "*" || firstOp == "/");

        if (canOptimize) {
            auto optimizedBlock = optimizeBasicBlock(currentBlock);
            result.insert(result.end(), optimizedBlock.begin(), optimizedBlock.end());
        }
        else {
            for (const auto& q : currentBlock) result.push_back(q);
        }
        currentBlock.clear();
        };

    for (size_t i = 0; i < originalQuads.size(); ++i) {
        if (leaders.count(i) > 0) {
            flushBlock();
        }

        const auto& q = originalQuads[i];
        bool isControlFlow = (q.op == "if" || q.op == "el" || q.op == "ie" ||
            q.op == "wh" || q.op == "do" || q.op == "we" ||
            q.op == "param" || q.op == "call" ||
            q.op == "function" || q.op == "procedure" ||
            q.op == "endproc" || q.op == "program" || q.op == "end");

        if (isControlFlow) {
            flushBlock();
            currentBlock.push_back(q);
            flushBlock();
        }
        else {
            currentBlock.push_back(q);
        }
    }
    flushBlock();

    return result;
}
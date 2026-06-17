#pragma once
#pragma once
#include "Compiler.h"
#include <vector>
#include <string>
#include <unordered_map>

// DAG 节点结构
struct DAGNode {
    int id;
    std::string op;        // 运算符 (如果是叶子节点，这里为空)
    int left;              // 左子节点 ID (-1 表示无)
    int right;             // 右子节点 ID (-1 表示无)
    std::string value;     // 节点的值（常数数值，或初始标识符名）
    std::vector<std::string> vars; // 挂载在该节点上的所有变量名
};

class Optimizer {
public:
    // 核心优化接口：传入原始四元式，返回优化后的四元式
    static std::vector<Quad> optimizeDAG(const std::vector<Quad>& originalQuads);
    // 辅助函数：判断字符串是否为全数字（常数）
    static bool isNumber(const std::string& str);

private:

};
#define NOMINMAX  
#define _CRT_SECURE_NO_WARNINGS
#include <graphics.h>
#include <string>
#include <vector>
#include <windows.h>
#include <commdlg.h>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <map>
#include "Compiler.h" 
#include "Parser.h"
#include "Optimizer.h"
#include "TargetGenerator.h"

using namespace std;

enum Scene { MAIN_MENU, SHOW_TOKEN, SHOW_DICT, SHOW_SYMBOL, SHOW_QUAD, SHOW_ASM,SHOW_ACT_RECORD};
Scene currentScene = MAIN_MENU;

struct Button { int x, y, width, height; wstring text; COLORREF bgColor; };

wstring SelectTextFile() {
    OPENFILENAMEW ofn; wchar_t szFile[260] = { 0 };
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = GetHWnd(); ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = L"Pascal Files\0*.txt;*.pas\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    if (GetOpenFileNameW(&ofn) == TRUE)
        return wstring(ofn.lpstrFile);
    return L"";
}

string ReadFileToStr(const wstring& filepath) {
    ifstream file(filepath);
    if (!file.is_open())
        return "";
    stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void DrawButton(const Button& btn) {
    setfillcolor(btn.bgColor);
    setlinecolor(RGB(150, 150, 150));
    fillrectangle(btn.x, btn.y, btn.x + btn.width, btn.y + btn.height);
    settextcolor(BLACK); settextstyle(22, 0, L"微软雅黑");
    setbkmode(TRANSPARENT);
    int tx = btn.x + (btn.width - textwidth(btn.text.c_str())) / 2;
    int ty = btn.y + (btn.height - textheight(btn.text.c_str())) / 2;
    outtextxy(tx, ty, btn.text.c_str());
}

// 绘制垂直滚动条
void DrawScrollBar(int startX, int startY, int totalHeight, int totalItems, int visibleItems, int scrollOffset) {
    // 如果总数据不足一页，不需要滚动条
    if (totalItems <= visibleItems) return;

    // 1. 绘制滚动条背景槽 (浅灰色)
    setfillcolor(RGB(235, 235, 235));
    solidrectangle(startX, startY, startX + 12, startY + totalHeight);

    // 2. 计算滑块的高度 (按比例缩放，最小保证 20 像素以便于观察)
    int thumbHeight = (std::max)(20, totalHeight * visibleItems / totalItems);

    // 3. 计算滑块的 Y 轴位置
    int maxOffset = totalItems - visibleItems;
    if (scrollOffset > maxOffset) scrollOffset = maxOffset; // 容错约束
    int thumbY = startY + (totalHeight - thumbHeight) * scrollOffset / maxOffset;

    // 4. 绘制滑块 (深灰色)
    setfillcolor(RGB(170, 170, 170));
    solidrectangle(startX, thumbY, startX + 12, thumbY + thumbHeight);
}

bool IsButtonClicked(const Button& btn, int mx, int my) { return (mx >= btn.x && mx <= btn.x + btn.width && my >= btn.y && my <= btn.y + btn.height); }

void DrawTextPage(const wstring& title, const wstring& content, Button& btnBack, int scrollOffset) {
    cleardevice();
    settextcolor(BLUE);
    settextstyle(30, 0, L"微软雅黑");
    outtextxy(50, 20, title.c_str());
    settextcolor(RGB(120, 120, 120));
    settextstyle(16, 0, L"微软雅黑");
    setfillcolor(WHITE);
    solidrectangle(50, 70, 950, 500);
    settextcolor(BLACK);
    settextstyle(18, 0, L"Consolas");
    wstringstream ss(content);
    wstring wline; int y = 80;
    int currentLine = 0;
    while (getline(ss, wline)) {
        if (currentLine >= scrollOffset) {
            if (y < 480) {
                outtextxy(60, y, wline.c_str());
                y += 22;
            }
            else
                break;
        }
        currentLine++;
    }

    int totalLines = std::count(content.begin(), content.end(), L'\n') + 1;
    int visibleRows = 18; 
    DrawScrollBar(970, 80, 400, totalLines, visibleRows, scrollOffset);
    DrawButton(btnBack);
}

void DrawDictTablePage(const vector<string>& cTable, Button& btnBack, int scrollOffset) {
    cleardevice();
    settextcolor(BLUE);
    settextstyle(30, 0, L"微软雅黑");
    outtextxy(50, 20, L"词法内部字典");
    setfillcolor(WHITE);
    solidrectangle(50, 70, 950, 500);
    setlinecolor(BLACK);
    settextcolor(BLACK);
    settextstyle(18, 0, L"Consolas");
    int rowH = 30, y = 90, curLine = 0;
    wstring kData[] = { L"program", L"var", L"integer", L"real", L"char", L"begin", L"end", L"procedure", L"if", L"then", L"else", L"const", L"type", L"record", L"array", L"of", L"function", L"string", L"boolean", L"true", L"false", L"while", L"do" };
    wstring pData[] = { L",", L":", L";", L":=", L"*", L"/", L"+", L"-", L"(", L")", L"[", L"]", L"=", L"<=", L"<>", L"<", L">=", L">", L".", L".." };
    int maxRows = (std::max)({ 23, 20, (int)cTable.size() });
    int startX_K = 80, colW_K[] = { 40, 120 }, startX_P = 380, colW_P[] = { 40, 100 }, startX_C = 660, colW_C[] = { 40, 200 };

    auto drawHeader = [&](int startX, const wstring& title, int w1, int w2) {
        rectangle(startX, y, startX + w1, y + rowH);
        outtextxy(startX + 10, y + 5, L"ID");
        rectangle(startX + w1, y, startX + w1 + w2, y + rowH);
        outtextxy(startX + w1 + 10, y + 5, title.c_str());
        };

    if (curLine >= scrollOffset && y <= 480 - rowH) {
        drawHeader(startX_K, L"关键字 (K)", colW_K[0], colW_K[1]);
        drawHeader(startX_P, L"界符 (P)", colW_P[0], colW_P[1]);
        drawHeader(startX_C, L"常数/字面量(C)", colW_C[0], colW_C[1]);
        y += rowH;
    }
    curLine++;
    for (int i = 0; i < maxRows; i++) {
        if (curLine >= scrollOffset && y <= 480 - rowH) {
            if (i < 23) {
                rectangle(startX_K, y, startX_K + colW_K[0], y + rowH);
                outtextxy(startX_K + 10, y + 5, to_wstring(i + 1).c_str());
                rectangle(startX_K + colW_K[0], y, startX_K + colW_K[0] + colW_K[1], y + rowH);
                outtextxy(startX_K + colW_K[0] + 10, y + 5, kData[i].c_str());
            }
            if (i < 20) {
                rectangle(startX_P, y, startX_P + colW_P[0], y + rowH);
                outtextxy(startX_P + 10, y + 5, to_wstring(i + 1).c_str());
                rectangle(startX_P + colW_P[0], y, startX_P + colW_P[0] + colW_P[1], y + rowH);
                outtextxy(startX_P + colW_P[0] + 10, y + 5, pData[i].c_str());
            }
            if (i < cTable.size()) {
                rectangle(startX_C, y, startX_C + colW_C[0], y + rowH);
                outtextxy(startX_C + 10, y + 5, to_wstring(i + 1).c_str());
                rectangle(startX_C + colW_C[0], y, startX_C + colW_C[0] + colW_C[1], y + rowH);
                wstring cVal(cTable[i].begin(), cTable[i].end());
                outtextxy(startX_C + colW_C[0] + 10, y + 5, cVal.c_str());
            }
            y += rowH;
        }
        curLine++;
    }
    int visibleRows = 13;
    DrawScrollBar(970, 90, 390, maxRows, visibleRows, scrollOffset);
    DrawButton(btnBack);
}

void DrawSymbolTablePage(const vector<SymbolItem>& symTable, Button& btnBack, int scrollOffset) {
    cleardevice();
    settextcolor(BLUE);
    settextstyle(30, 0, L"微软雅黑");
    outtextxy(50, 20, L"符号表");
    setfillcolor(WHITE);
    solidrectangle(50, 70, 950, 500);
    setlinecolor(BLACK);
    settextcolor(BLACK);
    settextstyle(18, 0, L"Consolas");

    int rowH = 30, y = 90, curLine = 0;
    int colW[] = { 100, 120, 80, 160 };
    wstring headers[] = { L"NAME", L"TYPE", L"CAT", L"ADDR(层,偏)/INFO" };

    for (int c = 0; c < 4; c++) {
        int w = textwidth(headers[c].c_str()) + 30;
        if (w > colW[c]) colW[c] = w;
    }

    // 让 "p" 和 "f" 采用相同的格式化逻辑计算列宽
    for (const auto& item : symTable) {
        wstring addrDisplay;
        if (item.cat == "type") addrDisplay = L"Size: " + std::to_wstring(item.addr);
        //else if (item.cat == "field") addrDisplay = L"Offset: " + std::to_wstring(item.addr);
        else if (item.cat == "d") addrDisplay = L"Size: " + std::to_wstring(item.addr);
        else if (item.cat == "f" || item.cat == "p") addrDisplay = L"层数: " + std::to_wstring(item.level) + L", 参数: " + std::to_wstring(item.addr);
        else if (item.cat == "c") addrDisplay = L"Value: " + std::wstring(item.strVal.begin(),item.strVal.end());
        else addrDisplay = L"(" + std::to_wstring(item.level) + L", " + std::to_wstring(item.addr) + L")";

        wstring rowData[] = {
            wstring(item.name.begin(), item.name.end()),
            wstring(item.type.begin(), item.type.end()),
            wstring(item.cat.begin(), item.cat.end()),
            addrDisplay
        };
        for (int c = 0; c < 4; c++) {
            int w = textwidth(rowData[c].c_str()) + 30;
            if (w > colW[c]) colW[c] = w;
        }
    }

    int totalW = colW[0] + colW[1] + colW[2] + colW[3];
    int startX = 50 + (900 - totalW) / 2;
    if (startX < 60) startX = 60;

    if (curLine >= scrollOffset && y <= 480 - rowH) {
        int curX = startX;
        for (int c = 0; c < 4; c++) {
            rectangle(curX, y, curX + colW[c], y + rowH);
            outtextxy(curX + 10, y + 5, headers[c].c_str());
            curX += colW[c];
        }
        y += rowH;
    }
    curLine++;

    for (size_t i = 0; i < symTable.size(); i++) {
        if (curLine >= scrollOffset && y <= 480 - rowH) {
            int curX = startX; const auto& item = symTable[i];

            // 实际绘制：让 "p" 和 "f" 采用相同的渲染文本
            wstring addrDisplay;
            if (item.cat == "type") addrDisplay = L"Size: " + std::to_wstring(item.addr);
            else if (item.cat == "d") addrDisplay = L"Size: " + std::to_wstring(item.addr);
            else if (item.cat == "f" || item.cat == "p") addrDisplay = L"层数: " + std::to_wstring(item.level) + L", 参数: " + std::to_wstring(item.addr);
            else if (item.cat == "c") addrDisplay = L"Value: " + std::wstring(item.strVal.begin(),item.strVal.end());
            else addrDisplay = L"(" + std::to_wstring(item.level) + L", " + std::to_wstring(item.addr) + L")";

            wstring rowData[] = {
                wstring(item.name.begin(), item.name.end()),
                wstring(item.type.begin(), item.type.end()),
                wstring(item.cat.begin(), item.cat.end()),
                addrDisplay
            };
            for (int c = 0; c < 4; c++) {
                rectangle(curX, y, curX + colW[c], y + rowH);
                outtextxy(curX + 10, y + 5, rowData[c].c_str());
                curX += colW[c];
            }
            y += rowH;
        }
        curLine++;
    }
    int visibleRows = 13;
    DrawScrollBar(970, 90, 390, symTable.size(), visibleRows, scrollOffset);
    DrawButton(btnBack);
}

void DrawQuadTablePage(const vector<Quad>& originalQuads, const vector<Quad>& optimizedQuads, bool isOptimizedView, Button& btnBack, int scrollOffset) {
    cleardevice();
    settextcolor(BLUE);
    settextstyle(30, 0, L"微软雅黑");
    outtextxy(50, 20, L"四元式");
    Button btnOrig = { 550, 25, 100, 35, L"原始", isOptimizedView ? RGB(220, 220, 220) : RGB(150, 220, 255) };
    Button btnOpt = { 660, 25, 100, 35, L"优化", isOptimizedView ? RGB(150, 220, 255) : RGB(220, 220, 220) };
    DrawButton(btnOrig); DrawButton(btnOpt);
    const vector<Quad>& currentData = isOptimizedView ? optimizedQuads : originalQuads;
    setfillcolor(WHITE);
    solidrectangle(50, 70, 950, 500);
    setlinecolor(BLACK);
    settextcolor(BLACK);
    settextstyle(18, 0, L"Consolas");
    int rowH = 30, startX = 140, y = 90, curLine = 0;
    int colW[] = { 80, 600 };
    wstring headers[] = { L"序号", L"四元式 ( OP, ARG1, ARG2, RESULT )" };

    if (curLine >= scrollOffset && y <= 480 - rowH) {
        int curX = startX;
        for (int i = 0; i < 2; i++) {
            rectangle(curX, y, curX + colW[i], y + rowH);
            outtextxy(curX + 10, y + 5, headers[i].c_str());
            curX += colW[i];
        }
        y += rowH;
    }
    curLine++;

    std::vector<int> quadBlockIdx(currentData.size(), 0);
    int cBlock = 0;
    bool expectNewBlock = true; // 初始状态期待一个新基本块

    for (size_t i = 0; i < currentData.size(); ++i) {
        const auto& q = currentData[i];

        bool isTargetLabel = (q.op == "wh" || q.op == "function" || q.op == "procedure");

        bool isTransferOrHalt = (q.op == "if" || q.op == "el" || q.op == "ie" ||
            q.op == "do" || q.op == "we" || q.op == "endproc" ||
            q.op == "program" || q.op == "end" || q.op == "call");

        // 如果遇到 TargetLabel 并且此时没在开启新块，强制开启
        if (isTargetLabel && !expectNewBlock) {
            cBlock++;
            expectNewBlock = true;
        }

        // 把当前语句安顿进当前基本块
        quadBlockIdx[i] = cBlock;
        expectNewBlock = false;

        // 如果这是一条转移/停止语句，当前块在“吃掉”它之后立刻宣告结束！
        if (isTransferOrHalt) {
            cBlock++;
            expectNewBlock = true;
        }
    }

    // 2. 准备的块背景色
    COLORREF blockColors[] = {
        RGB(245, 247, 250),  // 浅蓝灰
        RGB(242, 249, 242),  // 浅绿
        RGB(254, 248, 242),  // 浅橙
        RGB(247, 243, 253),  // 浅紫
        RGB(253, 253, 242)   // 浅黄
    };
    int numColors = sizeof(blockColors) / sizeof(blockColors[0]);

    auto padStr = [](const wstring& s, size_t width) { wstring res = s; while (res.length() < width) res += L' '; return res; };

    for (size_t i = 0; i < currentData.size(); i++) {
        if (curLine >= scrollOffset && y <= 480 - rowH) {
            int curX = startX;
            const auto& q = currentData[i];

            // 判定是否是控制流，只用于改变文字颜色，不再破坏背景
            bool isControlFlow = (q.op == "if" || q.op == "el" || q.op == "ie" ||
                q.op == "wh" || q.op == "do" || q.op == "we" ||
                q.op == "function" || q.op == "procedure" ||
                q.op == "endproc" || q.op == "program" || q.op == "end" || q.op == "call");

            // 统一填充该基本块专属的背景色
            COLORREF bgCol = blockColors[quadBlockIdx[i] % numColors];
            setfillcolor(bgCol);
            solidrectangle(curX, y, curX + colW[0] + colW[1], y + rowH);

            // 绘制网格边框
            setlinecolor(RGB(215, 215, 215));
            rectangle(curX, y, curX + colW[0], y + rowH);
            rectangle(curX + colW[0], y, curX + colW[0] + colW[1], y + rowH);

            // 强化基本块之间的截断分界线 (深灰色线段)
            if (i > 0 && quadBlockIdx[i] != quadBlockIdx[i - 1]) {
                setlinecolor(RGB(120, 120, 120));
                line(curX, y, curX + colW[0] + colW[1], y);
            }

            // 转换字符串
            wstring wOp(q.op.begin(), q.op.end());
            wstring wArg1(q.arg1.begin(), q.arg1.end());
            wstring wArg2(q.arg2.begin(), q.arg2.end());
            wstring wRes(q.result.begin(), q.result.end());
            wstring quadStr = L"( " + padStr(wOp + L",", 10) + padStr(wArg1 + L",", 18) + padStr(wArg2 + L",", 18) + wRes + L" )";

            // 渲染左侧序号
            settextcolor(BLACK);
            settextstyle(18, 0, L"Consolas");
            outtextxy(curX + 10, y + 5, to_wstring(i + 1).c_str());

            // 渲染右侧四元式主体
            if (isControlFlow) {
                settextcolor(RGB(200, 30, 30)); // 转移指令的背景和其他语句融合，但文字依然标红以示醒目
            }
            else if (isOptimizedView) {
                settextcolor(RGB(0, 110, 0));   // 优化的常规语句标绿
            }
            else {
                settextcolor(BLACK);            // 原始常规语句黑色
            }
            outtextxy(curX + colW[0] + 10, y + 5, quadStr.c_str());

            y += rowH;
        }
        curLine++;
    }
    int visibleRows = 13;
    DrawScrollBar(970, 90, 390, currentData.size(), visibleRows, scrollOffset);
    DrawButton(btnBack);
}
void DrawActivationRecordPage(const vector<SymbolItem>& symTable, Button& btnBack, int scrollOffset) {
    cleardevice();

    // 设置文字背景透明，防止重叠
    setbkmode(TRANSPARENT);

    // 标题区
    settextcolor(BLUE);
    settextstyle(30, 0, L"微软雅黑");
    outtextxy(50, 20, L"活动记录映像");

    // 白色画布背景
    setfillcolor(WHITE);
    solidrectangle(50, 70, 950, 500);
    settextcolor(RGB(100, 100, 100));
    settextstyle(18, 0, L"微软雅黑");
  

    // 1. 动态数据分组：支持无限深度的嵌套层级 
    std::map<int, vector<SymbolItem>> levelMap;
    for (const auto& item : symTable) {
        // 只提取变量、值参、引用参数
        if (item.cat == "v" || item.cat == "vn" || item.cat == "vf") {
            levelMap[item.level].push_back(item);
        }
    }

    // 对每个层级内部按偏移量降序排序
    auto cmp = [](const SymbolItem& a, const SymbolItem& b) { return a.addr > b.addr; };
    for (auto& pair : levelMap) {
        std::sort(pair.second.begin(), pair.second.end(), cmp);
    }

    // 2. 核心重构：将运行栈在屏幕正中央居中对齐
    int stackX = 320, stackW = 360, rowH = 40;
    int y = 130 - scrollOffset * 40;
    int totalRows = 0;

    auto drawCell = [&](const wstring& text, COLORREF bg, const wstring& addr = L"") {
        if (y + rowH > 110 && y < 490) {
            setfillcolor(bg);
            solidrectangle(stackX, (std::max)(y, 110), stackX + stackW, (std::min)(y + rowH, 490));
            setlinecolor(RGB(150, 150, 150));
            rectangle(stackX, (std::max)(y, 110), stackX + stackW, (std::min)(y + rowH, 490));

            if (y >= 110 && y + rowH <= 490) {
                if (!addr.empty()) {
                    settextcolor(RGB(200, 50, 50));
                    settextstyle(18, 0, L"Consolas");
                    outtextxy(stackX - 50, y + 10, addr.c_str());
                }
                settextcolor(BLACK);
                settextstyle(18, 0, L"微软雅黑");
                int tx = stackX + (stackW - textwidth(text.c_str())) / 2;
                outtextxy(tx, y + 10, text.c_str());
            }
        }
        y += rowH;
        totalRows++;
        };

    // 3. 动态遍历渲染所有层级 (Level 0 -> Level N)
    COLORREF headerColors[] = { RGB(200, 230, 255), RGB(255, 230, 200), RGB(200, 255, 200), RGB(255, 200, 255) };
    COLORREF cellColors[] = { RGB(240, 248, 255), RGB(255, 250, 240), RGB(240, 255, 240), RGB(255, 240, 255) };

    for (const auto& pair : levelMap) {
        int currentLvl = pair.first;
        const auto& items = pair.second;

        // 获取对应的颜色
        COLORREF headBg = headerColors[currentLvl % 4];
        COLORREF itemBg = cellColors[currentLvl % 4];

        // 动态生成标题
        wstring title = L"【Level " + to_wstring(currentLvl) + L" ";
        if (currentLvl == 0) title += L"全局主程序域】";
        else title += L"局部嵌套域】";

        // 绘制标题和该层的所有变量
        drawCell(title, headBg);
        for (const auto& item : items) {
            wstring name(item.name.begin(), item.name.end());
            drawCell(name, itemBg, to_wstring(item.addr));
        }

        y += 15;
    }

    // 动态绑定鼠标拖拽滚动条
    DrawScrollBar(970, 110, 380, totalRows + 1, 9, scrollOffset);
    DrawButton(btnBack);
    // 动态绑定鼠标拖拽滚动条
    DrawScrollBar(970, 110, 380, totalRows + 1, 9, scrollOffset);
    DrawButton(btnBack);
}

int main() {
    initgraph(1000, 600); setbkcolor(RGB(240, 240, 240));
    HWND hwnd = GetHWnd();
    SetWindowTextW(hwnd, L"Pascal 编译器可视化分析工具"); 
    Button btnSelect = { 700, 60, 160, 45, L"选择源文件", RGB(200, 225, 255) }; Button btnCompile = { 700, 115, 160, 45, L"一键编译", RGB(150, 255, 150) }; Button btnBack = { 420, 520, 160, 45, L"返回主页", RGB(255, 200, 200) };
    int startX = 110, startY = 200, btnW = 220, btnH = 60, gapX = 60, gapY = 50;
    vector<Button> funcButtons = {
        {startX, startY, btnW, btnH, L"1. Token 序列", RGB(220, 220, 220)},
        {startX + btnW + gapX, startY, btnW, btnH, L"2. K/P/C 字典表", RGB(220, 220, 220)},
        {startX + 2 * (btnW + gapX), startY, btnW, btnH, L"3. 符号表", RGB(220, 220, 220)},

        {startX, startY + btnH + gapY, btnW, btnH, L"4. 四元式 / DAG", RGB(220, 220, 220)},
        {startX + btnW + gapX, startY + btnH + gapY, btnW, btnH, L"5. 目标代码", RGB(220, 220, 220)},
        {startX + 2 * (btnW + gapX), startY + btnH + gapY, btnW, btnH, L"6. 运行时活动记录", RGB(220, 220, 220)}
    };
    wstring currentFilePath = L"", lexicalDisplayContent = L""; 
    vector<SymbolItem> globalSymbols; 
    vector<string> globalConstants; 
    vector<Quad> globalQuads, globalOptimizedQuads;
    wstring asmDisplayContent = L"";
    bool showOptimized = false, hasTokens = false, isParseSuccess = false; 
    int scrollOffset = 0;

    // ======== 用于滚动条拖拽的状态变量 ========
    bool isDragging = false;
    int dragStartY = 0;
    int dragStartOffset = 0;

    ExMessage msg; BeginBatchDraw();
    while (true) {
        if (currentScene == MAIN_MENU) {
            cleardevice();
            setfillcolor(WHITE);
            fillrectangle(120, 60, 670, 105);
            settextcolor(BLACK);
            settextstyle(20, 0, L"微软雅黑");
            wstring pathDisplay = currentFilePath.empty() ? L"未选择文件" : currentFilePath;
            outtextxy(130, 75, pathDisplay.c_str());
            DrawButton(btnSelect);
            DrawButton(btnCompile);
            for (const auto& btn : funcButtons)
                DrawButton(btn);
        }
        else if (currentScene == SHOW_TOKEN) DrawTextPage(L"词法分析：Token 原文对照", lexicalDisplayContent, btnBack, scrollOffset);
        else if (currentScene == SHOW_DICT) DrawDictTablePage(globalConstants, btnBack, scrollOffset);
        else if (currentScene == SHOW_SYMBOL) DrawSymbolTablePage(globalSymbols, btnBack, scrollOffset);
        else if (currentScene == SHOW_QUAD) DrawQuadTablePage(globalQuads, globalOptimizedQuads, showOptimized, btnBack, scrollOffset);
        else if (currentScene == SHOW_ASM) DrawTextPage(L"目标代码生成：8086 汇编语言", asmDisplayContent, btnBack, scrollOffset);
        else if (currentScene == SHOW_ACT_RECORD) DrawActivationRecordPage(globalSymbols, btnBack, scrollOffset);

        // ======== 动态计算当前页面的滚动条几何参数 ========
        int sbX = 970, sbY = 90, sbH = 390, totalItems = 0, visibleItems = 13;
        if (currentScene == SHOW_TOKEN || currentScene == SHOW_ASM) {
            sbY = 80; sbH = 400; visibleItems = 18;
            totalItems = std::count((currentScene == SHOW_TOKEN ? lexicalDisplayContent : asmDisplayContent).begin(),
                (currentScene == SHOW_TOKEN ? lexicalDisplayContent : asmDisplayContent).end(), L'\n') + 1;
        }
        else if (currentScene == SHOW_DICT) {
            totalItems = (std::max)({ 23, 20, (int)globalConstants.size() });
        }
        else if (currentScene == SHOW_SYMBOL) {
            totalItems = globalSymbols.size();
        }
        else if (currentScene == SHOW_QUAD) {
            totalItems = showOptimized ? globalOptimizedQuads.size() : globalQuads.size();
        }
        else if (currentScene == SHOW_ACT_RECORD) {
            sbY = 110; sbH = 380; visibleItems = 9;
            for (const auto& item : globalSymbols)
                if (item.cat == "v" || item.cat == "vn" || item.cat == "vf") totalItems++;
        }

        int maxOffset = (std::max)(0, totalItems - visibleItems);
        int thumbHeight = totalItems > 0 ? (std::max)(20, sbH * visibleItems / (std::max)(totalItems, 1)) : 20;
        int thumbY = sbY + (maxOffset > 0 ? (sbH - thumbHeight) * scrollOffset / maxOffset : 0);

        if (peekmessage(&msg, EM_MOUSE | EM_KEY)) {
            // 1. 键盘按键逻辑 
            if (msg.message == WM_KEYDOWN && currentScene != MAIN_MENU) {
                if (msg.vkcode == VK_UP) scrollOffset = (std::max)(0, scrollOffset - 1);
                else if (msg.vkcode == VK_DOWN) scrollOffset += 1;
                else if (msg.vkcode == VK_PRIOR) scrollOffset = (std::max)(0, scrollOffset - 10);
                else if (msg.vkcode == VK_NEXT) scrollOffset += 10;
            }
            // 2. 鼠标滚轮逻辑
            else if (msg.message == WM_MOUSEWHEEL && currentScene != MAIN_MENU) {
                if (msg.wheel > 0) scrollOffset = (std::max)(0, scrollOffset - 3);
                else if (msg.wheel < 0) scrollOffset += 3;
            }
            // 3. 鼠标左键按下
            else if (msg.message == WM_LBUTTONDOWN) {
                // 【判断是否点中了滚动条的滑块】
                if (currentScene != MAIN_MENU && totalItems > visibleItems &&
                    msg.x >= sbX && msg.x <= sbX + 12 && msg.y >= thumbY && msg.y <= thumbY + thumbHeight) {
                    isDragging = true;
                    dragStartY = msg.y;
                    dragStartOffset = scrollOffset;
                }
                else {
                    // 【原有的按钮点击逻辑】
                    if (currentScene == MAIN_MENU) {
                        if (IsButtonClicked(btnSelect, msg.x, msg.y)) {
                            wstring selectedPath = SelectTextFile();
                            if (!selectedPath.empty()) {
                                currentFilePath = selectedPath; hasTokens = false; isParseSuccess = false;
                                globalOptimizedQuads.clear(); showOptimized = false;
                            }
                        }
                        if (IsButtonClicked(btnCompile, msg.x, msg.y)) {
                            if (currentFilePath.empty()) MessageBoxW(GetHWnd(), L"请先选择源文件！", L"警告", MB_OK | MB_ICONWARNING);
                            else {
                                string sourceCode = ReadFileToStr(currentFilePath);
                                Scanner scanner(sourceCode); auto compiledTokens = scanner.tokenize(); hasTokens = true;
                                lexicalDisplayContent = Scanner::formatTokensForDisplay(compiledTokens); globalConstants = scanner.getConstantTable();
                                Parser parser(compiledTokens); isParseSuccess = parser.parse();
                                if (isParseSuccess) {
                                    globalQuads = parser.getQuadList(); globalSymbols = parser.getSemanticTable();
                                    globalOptimizedQuads.clear(); showOptimized = false;
                                    TargetGenerator gen(globalQuads); asmDisplayContent = gen.generate();
                                    MessageBoxW(GetHWnd(), L"编译成功！", L"提示", MB_OK | MB_ICONINFORMATION);
                                }
                                else { wstring wErrMsg = parser.getErrorMessage(); MessageBoxW(GetHWnd(), wErrMsg.c_str(), L"语法错误", MB_OK | MB_ICONERROR); }
                            }
                        }
                        for (int i = 0; i < funcButtons.size(); i++) {
                            if (IsButtonClicked(funcButtons[i], msg.x, msg.y)) {
                                scrollOffset = 0;
                                if (i == 0 || i == 1) {
                                    if (!hasTokens) { MessageBoxW(GetHWnd(), L"请先点击编译", L"提示", MB_OK); continue; }
                                    if (i == 0) currentScene = SHOW_TOKEN; else currentScene = SHOW_DICT;
                                }
                                else if (i >= 2 && i <= 5) {
                                    if (!isParseSuccess) { MessageBoxW(GetHWnd(), L"语法错误或未编译，无法查看", L"拦截", MB_OK); continue; }
                                    if (i == 2) currentScene = SHOW_SYMBOL;
                                    else if (i == 3) currentScene = SHOW_QUAD;
                                    else if (i == 4) currentScene = SHOW_ASM;
                                    else if (i == 5) currentScene = SHOW_ACT_RECORD;
                                }
                            }
                        }
                    }
                    else if (currentScene == SHOW_QUAD) {
                        if (msg.x >= 550 && msg.x <= 650 && msg.y >= 25 && msg.y <= 60) showOptimized = false;
                        else if (msg.x >= 660 && msg.x <= 760 && msg.y >= 25 && msg.y <= 60) { if (globalOptimizedQuads.empty()) globalOptimizedQuads = Optimizer::optimizeDAG(globalQuads); showOptimized = true; }
                        if (IsButtonClicked(btnBack, msg.x, msg.y)) { currentScene = MAIN_MENU; scrollOffset = 0; }
                    }
                    else if (IsButtonClicked(btnBack, msg.x, msg.y)) { currentScene = MAIN_MENU; scrollOffset = 0; }
                }
            }
            // 4. 【鼠标拖拽移动逻辑】
            else if (msg.message == WM_MOUSEMOVE) {
                if (isDragging && maxOffset > 0) {
                    int deltaY = msg.y - dragStartY;            // 鼠标移动的像素距离
                    int trackSpace = sbH - thumbHeight;         // 滚动条滑槽的总可滑动空间
                    if (trackSpace > 0) {
                        int deltaOffset = deltaY * maxOffset / trackSpace; 
                        scrollOffset = dragStartOffset + deltaOffset;
                        // 容错：防止拖出边界
                        if (scrollOffset < 0) scrollOffset = 0;
                        if (scrollOffset > maxOffset) scrollOffset = maxOffset;
                    }
                }
            }
            // 5. 【鼠标左键松开，结束拖拽】
            else if (msg.message == WM_LBUTTONUP) {
                isDragging = false;
            }
        }
        FlushBatchDraw();
        Sleep(10);
    } return 0;
}
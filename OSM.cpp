#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#ifdef _WIN32
#include <windows.h>
#endif
#include <locale>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <queue>
#include <map>
#include <unordered_map>
#include <cmath>
#include <iomanip>
#include <algorithm>
#include <chrono>
#include <thread>
#include <string>
#include <limits.h>

using namespace std;

// PCB类：进程控制块
class PCB {
public:
    string pName;               // 进程名称
    string pRemark;             // 程序备注
    string pStatus;             // 进程状态
    int createTime;             // 创建时间
    int runTime;                // 运行时间
    int grade;                  // 优先级
    int startTime;              // 开始时间
    int completeTime;           // 完成时间
    int turnoverTime;           // 周转时间
    double weightedTurnoverTime;// 带权周转时间
    int originalRunTime;        // 原始运行时间

    PCB(string name, int create, int runtime, int priority, string remark)
        : pName(name), createTime(create), runTime(runtime), grade(priority),
        pRemark(remark), pStatus("null"), startTime(-1), completeTime(0),
        turnoverTime(0), weightedTurnoverTime(0.0), originalRunTime(runtime) {}

    void updateStatus(const string& newStatus) {
        pStatus = newStatus;
        cout << "Process " << pName << " status updated to " << pStatus << endl;
    }
};

// 结构体定义，用于存储每个程序的信息
struct Program {
    string name;                // 程序名称
    vector<int> jump_addresses; // 跳转地址序列
    vector<int> page_sequence;  // 页面访问序列
    int maxPages;               // 每个进程的最大页面数
};

// 结构体定义，用于存储运行步骤
struct RunSteps {
    map<string, int> runTimes;                // 程序的运行时间
    map<string, vector<int>> pageSequences;    // 程序的页面访问序列
};

// 全局进程列表
vector<PCB> processList;

// 辅助函数：去除字符串首尾的空白字符
string trim(const string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == string::npos)
        return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

    // FIFOPageManager 类：处理 FIFO 页面替换算法
    class FIFOPageManager {
    public:
        double pageSize;                         // 页面大小（KB）
        int maxPages;                            // 物理页框数
        queue<int> fifoPages;                    // FIFO 队列
        vector<string> log;                      // 页面操作日志
        int pageFaults = 0;                      // 缺页次数
        int pageHits = 0;                        // 命中次数

        FIFOPageManager(double size, int max) : pageSize(size), maxPages(max) {}

        // FIFO 替换策略
        bool fifoReplace(int page) {
            // 检查页面是否在内存中
            vector<int> fifoVec = queueToVector(fifoPages);
            if (find(fifoVec.begin(), fifoVec.end(), page) != fifoVec.end()) {
                pageHits++;
                log.push_back("FIFO: 页面 " + to_string(page) + " 已在内存中 (命中)。");
                return false; // 页面命中，无需缺页
            }

            pageFaults++;
            if (fifoPages.size() >= maxPages) {
                if (fifoPages.empty()) {
                    cerr << "错误：FIFO 队列为空，无法移除页面。" << endl;
                    log.push_back("FIFO: 错误 - FIFO 队列为空，无法移除页面。");
                    return false; // 发生错误时返回 false
                }
                int removed = fifoPages.front();
                fifoPages.pop();
                log.push_back("FIFO: 页面 " + to_string(removed) + " 被移除。");
            }

            fifoPages.push(page);
            log.push_back("FIFO: 页面 " + to_string(page) + " 被添加。");

            return true; // 发生缺页
        }

        // 打印缺页和命中摘要
        void printSummary() const {
            cout << "FIFO 算法总结:" << endl;
            cout << "缺页次数: " << pageFaults << endl;
            cout << "命中次数: " << pageHits << endl;
            if (pageHits + pageFaults > 0) {
                cout << "命中率: " << fixed << setprecision(2)
                    << ((static_cast<double>(pageHits) / (pageHits + pageFaults)) * 100) << "%" << endl;
            }
        }

        // 打印物理块状态
        void printPhysicalBlockState() const {
            cout << "当前物理块状态 (FIFO 队列): [ ";
            queue<int> tempQueue = fifoPages;
            vector<int> fifoVec;
            while (!tempQueue.empty()) {
                fifoVec.push_back(tempQueue.front());
                tempQueue.pop();
            }
            for (int p : fifoVec) {
                cout << p << " ";
            }
            for (int i = fifoVec.size(); i < maxPages; ++i) {
                cout << "- ";
            }
            cout << "]" << endl;
        }

        // 打印操作日志
        void printLog() const {
            cout << "FIFO 页面操作日志:" << endl;
            for (const auto& entry : log) {
                cout << entry << endl;
            }
        }

    private:
        // 将队列转换为向量
        vector<int> queueToVector(queue<int> q) const {
            vector<int> result;
            while (!q.empty()) {
                result.push_back(q.front());
                q.pop();
            }
            return result;
        }
    };

    // LRUPageManager 类：处理 LRU 页面替换算法
    class LRUPageManager {
    public:
        double pageSize;                         // 页面大小（KB）
        int maxPages;                            // 物理页框数
        unordered_map<int, int> lruPages;       // LRU映射：页面号 -> 最近访问时间
        vector<string> log;                      // 页面操作日志
        int pageFaults = 0;                      // 缺页次数
        int pageHits = 0;                        // 命中次数

        LRUPageManager(double size, int max) : pageSize(size), maxPages(max) {}

        // LRU 替换策略
        bool lruReplace(int page, int currentTime) {
            if (lruPages.find(page) != lruPages.end()) {
                pageHits++;
                lruPages[page] = currentTime; // 更新最近访问时间
                log.push_back("LRU: 页面 " + to_string(page) + " 已在内存中 (命中)。");
                return false; // 页面命中，无需缺页
            }

            pageFaults++;
            if (lruPages.size() >= maxPages) {
                int lruPage = getLRUPage();
                if (lruPage == -1) {
                    cerr << "错误：未找到最久未使用的页面以移除。" << endl;
                    log.push_back("LRU: 错误 - 未找到最久未使用的页面以移除。");
                    return false; // 发生错误时返回 false
                }
                lruPages.erase(lruPage); // 移除最久未使用的页面
                log.push_back("LRU: 页面 " + to_string(lruPage) + " 被移除。");
            }

            lruPages[page] = currentTime; // 添加新页面，并记录访问时间
            log.push_back("LRU: 页面 " + to_string(page) + " 被添加。");

            return true; // 发生缺页
        }

        // 打印缺页和命中摘要
        void printSummary() const {
            cout << "LRU 算法总结:" << endl;
            cout << "缺页次数: " << pageFaults << endl;
            cout << "命中次数: " << pageHits << endl;
            if (pageHits + pageFaults > 0) {
                cout << "命中率: " << fixed << setprecision(2)
                    << ((static_cast<double>(pageHits) / (pageHits + pageFaults)) * 100) << "%" << endl;
            }
        }

        // 打印物理块状态
        void printPhysicalBlockState() const {
            cout << "当前物理块状态 (LRU 页面): [ ";
            // 将 LRU 页面的页面号按最近访问时间排序（从最久未使用到最近使用）
            vector<pair<int, int>> pages(lruPages.begin(), lruPages.end());
            sort(pages.begin(), pages.end(), [](const pair<int, int>& a, const pair<int, int>& b) {
                return a.second < b.second; // 从最久未使用到最近使用
                });

            vector<int> sortedPages;
            for (const auto& p : pages) {
                sortedPages.push_back(p.first);
            }

            for (int p : sortedPages) {
                cout << p << " ";
            }
            for (int i = sortedPages.size(); i < maxPages; ++i) {
                cout << "- ";
            }
            cout << "]" << endl;
        }

        // 打印操作日志
        void printLog() const {
            cout << "LRU 页面操作日志:" << endl;
            for (const auto& entry : log) {
                cout << entry << endl;
            }
        }

    private:
        // 获取最久未使用的页面
        int getLRUPage() const {
            int lruPage = -1;
            int minTime = INT32_MAX;
            for (const auto& entry : lruPages) {
                if (entry.second < minTime) {
                    minTime = entry.second;
                    lruPage = entry.first;
                }
            }
            return lruPage;
        }
    };

// 函数声明
RunSteps loadRunSteps();
map<string, map<string, double>> loadPrograms();

void loadProcesses(const map<string, int>& runTimes);
void fcfsScheduling();
void rrScheduling();
void simulatePageReplacement(const vector<Program>& programs, int pageSize, int timeSlice);

// 解析 run.txt 文件，生成程序列表（页面访问序列）
vector<Program> parseRunFile(const string& filename, int pageSize) {
    ifstream infile(filename);
    if (!infile.is_open()) {
        cerr << "错误：无法打开文件 " << filename << endl;
        exit(1);
    }

    vector<Program> programs;
    string line;
    Program currentProgram;

    while (getline(infile, line)) {
        line = trim(line);
        if (line.empty()) continue; // 跳过空行

        // 检查是否为程序名称行
        if (line.find("ProgramName") == 0 || line.find("程序") == 0) {
            // 如果当前有正在处理的程序，先保存
            if (!currentProgram.name.empty()) {
                // 将跳转地址转换为页面号
                for (int addr : currentProgram.jump_addresses) {
                    currentProgram.page_sequence.push_back(addr / pageSize);
                }
                programs.push_back(currentProgram);
                currentProgram = Program(); // 重置为下一个程序
            }

            // 提取程序名称
            stringstream ss(line);
            string tag, prog_name;
            ss >> tag >> prog_name;
            currentProgram.name = prog_name;
        }
        else {
            // 处理事件行
            stringstream ss(line);
            int timestamp;
            string event;
            int value;
            ss >> timestamp >> event;

            if (event == "Jump" || event == "跳转") {
                ss >> value;
                currentProgram.jump_addresses.push_back(value);
            }
            // 忽略 Start 和 End 事件
        }
    }

    // 保存最后一个程序
    if (!currentProgram.name.empty()) {
        for (int addr : currentProgram.jump_addresses) {
            currentProgram.page_sequence.push_back(addr / pageSize);
        }
        programs.push_back(currentProgram);
    }

    infile.close();
    return programs;
}

// 加载运行步骤，从 run.txt 中读取
RunSteps loadRunSteps() {
    RunSteps runSteps;
    ifstream file("run.txt");
    if (!file.is_open()) {
        cerr << "Error: Unable to open run.txt" << endl;
        return runSteps;
    }

    string line;
    string currentProgram;
    while (getline(file, line)) {
        line = trim(line);
        if (line.empty()) continue; // 跳过空行

        // 检查是否为程序名称行（"ProgramName" 或 "程序"）
        if (line.find("ProgramName") == 0 || line.find("程序") == 0) {
            // 提取程序名称
            size_t pos_space = line.find_first_of(" \t"); // 查找第一个空格或制表符
            if (pos_space != string::npos) {
                string afterKeyword = trim(line.substr(pos_space + 1));
                currentProgram = afterKeyword;
                cout << "Found Program Name: [" << currentProgram << "]" << endl; // Debug info
            }
            else {
                cerr << "Warning: Unable to extract program name: " << line << endl;
            }
        }
        else {
            // 确保存在当前程序名
            if (currentProgram.empty()) {
                cerr << "Warning: Found run step before program name: " << line << endl;
                continue;
            }

            // 解析运行步骤行（格式："时间 操作 参数" 或 "时间 操作"）
            istringstream iss(line);
            int time;
            string operation, param;

            // 如果无法解析到时间和操作，跳过
            if (!(iss >> time >> operation)) {
                cerr << "Warning: Failed to parse run step line: " << line << endl;
                continue;
            }

            // 检测是否有第三列参数
            if (!(iss >> param)) {
                param = ""; // 如果没有参数，设置为空字符串
            }

            // 如果操作为 "结束" 或 "End"，将时间作为最终运行时间
            if (operation == "结束" || operation == "End") {
                runSteps.runTimes[currentProgram] = max(runSteps.runTimes[currentProgram], time);
                cout << "Set final run time for [" << currentProgram << "] to " << time << " ms" << endl;
                continue;
            }

            // 如果操作为 "Jump" 或 "跳转"，将参数作为跳转地址
            if (operation == "Jump" || operation == "跳转") {
                if (!param.empty()) {
                    try {
                        int address = stoi(param);
                        runSteps.pageSequences[currentProgram].push_back(address);
                        cout << "Added Jump Address " << address << " to [" << currentProgram << "]" << endl;
                    }
                    catch (const invalid_argument&) {
                        cerr << "Warning: Invalid jump address: " << param << " in line: " << line << endl;
                    }
                }
                else {
                    cerr << "Warning: Missing jump address in line: " << line << endl;
                }
            }
            else {
                // 其他操作可以根据需要扩展
                cout << "Info: Unhandled operation [" << operation << "] in line: " << line << endl;
            }

            // 更新当前程序的运行时间，取时间的最大值
            runSteps.runTimes[currentProgram] = max(runSteps.runTimes[currentProgram], time);
            cout << "Updated run time [" << currentProgram << "]: " << time << " ms" << endl; // Debug info
        }
    }

    file.close();

    // 输出加载的运行时间
    cout << "\nLoaded Run Times:" << endl;
    for (const auto& [program, time] : runSteps.runTimes) {
        cout << "Program: [" << program << "], Run Time: " << time << " ms" << endl; // Debug info
    }

    // 输出加载的页面访问序列
    cout << "Loaded Page Sequences:" << endl;
    for (const auto& [program, pages] : runSteps.pageSequences) {
        cout << "Program: [" << program << "], Pages: ";
        for (int page : pages) {
            cout << page << " ";
        }
        cout << endl;
    }

    return runSteps;
}

// 加载程序详细信息，从 program.txt 中读取
map<string, map<string, double>> loadPrograms() {
    ifstream file("program.txt");
    if (!file.is_open()) {
        cerr << "Error: Unable to open program.txt" << endl;
        return {};
    }

    map<string, map<string, double>> programs;
    string line, currentProgram;
    bool isFirstLine = true; // 标记是否为第一行

    while (getline(file, line)) {
        if (isFirstLine) {
            // 检测并移除 BOM
            if (line.size() >= 3 &&
                static_cast<unsigned char>(line[0]) == 0xEF &&
                static_cast<unsigned char>(line[1]) == 0xBB &&
                static_cast<unsigned char>(line[2]) == 0xBF) {
                line = line.substr(3);
            }
            isFirstLine = false;
        }

        // 打印读取的行及其长度以进行调试
        cout << "Reading line: [" << line << "], Length: " << line.length() << endl;

        line = trim(line);
        if (line.empty()) continue; // 跳过空行

        // Check if it's a FileName line
        if (line.find("FileName") == 0) {
            // Extract program name
            size_t pos_space = line.find_first_of(" \t", 8); // "FileName" 是8个字符
            if (pos_space != string::npos) {
                string afterKeyword = trim(line.substr(pos_space + 1));
                currentProgram = afterKeyword;
                cout << "Found Program: [" << currentProgram << "]" << endl; // Debug info
            }
            else {
                cerr << "Warning: Unable to extract file name: " << line << endl;
            }
        }
        else {
            // 确保存在当前程序名
            if (currentProgram.empty()) {
                cerr << "Warning: Found function definition before program name: " << line << endl;
                continue;
            }

            // Parse function line
            // Example line: "Main 0.6"
            istringstream iss(line);
            string funcName;
            double size;
            if (!(iss >> funcName >> size)) {
                cerr << "Warning: Failed to parse function line: " << line << endl;
                continue;
            }

            programs[currentProgram][funcName] = size;
            cout << "Added Function [" << funcName << "] Size " << size << " KB to Program [" << currentProgram << "]" << endl; // Debug info
        }
    }

    file.close();
    return programs;
}
// 加载进程信息，从 process.txt 中读取
void loadProcesses(const map<string, int>& runTimes) {
    cout << "\nLoaded Run Times:" << endl;
    for (const auto& [program, time] : runTimes) {
        cout << "Program: [" << program << "], Run Time: " << time << "ms" << endl; // Debug info
    }

    ifstream file("process.txt");
    if (!file.is_open()) {
        cerr << "Error: Unable to open process.txt" << endl;
        return;
    }
    string line;
    bool isFirstLine = true; // 标记是否为第一行

    while (getline(file, line)) {
        if (isFirstLine) {
            // 检测并移除 BOM
            if (line.size() >= 3 &&
                static_cast<unsigned char>(line[0]) == 0xEF &&
                static_cast<unsigned char>(line[1]) == 0xBB &&
                static_cast<unsigned char>(line[2]) == 0xBF) {
                line = line.substr(3);
            }
            isFirstLine = false;
        }

        // 打印读取的行及其长度以进行调试
        cout << "Reading line: [" << line << "], Length: " << line.length() << endl;

        line = trim(line);
        if (line.empty()) continue; // 跳过空行

        // 使用 istringstream 读取
        istringstream iss(line);
        string pName, pRemark;
        int createTime, grade;
        if (!(iss >> pName >> createTime >> grade >> pRemark)) {
            cerr << "Warning: Failed to parse process line: " << line << endl;
            continue;
        }

        cout << "Processing Process: [" << pName << "], Program Remark: [" << pRemark << "]" << endl; // Debug info

        if (runTimes.find(pRemark) != runTimes.end()) {
            processList.emplace_back(pName, createTime, runTimes.at(pRemark), grade, pRemark);
            cout << "Added Process: " << pName << " Run Time: " << runTimes.at(pRemark) << endl; // Debug info
        }
        else {
            cerr << "Warning: Program [" << pRemark << "] not found in runTimes. Skipping Process [" << pName << "]." << endl;
        }
    }
    file.close();
}



// FCFS 调度算法（改进：动态记录每个时间片的状态）
void fcfsScheduling() {
    ofstream resultFile("result.txt");
    if (!resultFile.is_open()) {
        cerr << "Error: Unable to open result.txt" << endl;
        return;
    }

    // 按创建时间排序
    sort(processList.begin(), processList.end(), [](PCB& a, PCB& b) {
        return a.createTime < b.createTime;
        });

    int currentTime = 0; // 当前时间

    // 输出表头
    resultFile << "时间片\t运行的进程\t";
    for (const auto& process : processList) {
        resultFile << "进程" << process.pName << "状态\t";
    }
    resultFile << "\n";

    // 调度过程
    for (auto& process : processList) {
        if (currentTime < process.createTime) {
            currentTime = process.createTime; // 等待进程到达
        }
        process.startTime = currentTime; // 记录开始时间
        process.completeTime = currentTime + process.runTime; // 记录完成时间

        // 每个时间片动态更新状态
        for (int t = 0; t < process.runTime; ++t) {
            resultFile << currentTime + t << "\t" << process.pName << "\t"; // 当前时间片和运行的进程

            // 遍历所有进程，更新状态
            for (auto& p : processList) {
                if (p.pName == process.pName) {
                    resultFile << "run\t"; // 当前运行的进程
                }
                else if (p.completeTime <= currentTime + t) {
                    resultFile << "complete\t"; // 已完成的进程
                }
                else if (p.createTime > currentTime + t) {
                    resultFile << "null\t"; // 尚未到达的进程
                }
                else {
                    resultFile << "ready\t"; // 已到达但未运行的进程
                }
            }
            resultFile << "\n";
        }

        // 更新时间到进程完成时
        currentTime += process.runTime;

        // 计算并记录统计信息
        process.turnoverTime = process.completeTime - process.createTime; // 周转时间
        process.weightedTurnoverTime = static_cast<double>(process.turnoverTime) / process.runTime; // 带权周转时间
    }

    // 输出每个进程的最终统计信息
    resultFile << "\n最终统计信息：\n";
    for (const auto& process : processList) {
        resultFile << "进程名称: " << process.pName
            << ", 开始时间: " << process.startTime
            << ", 完成时间: " << process.completeTime
            << ", 周转时间: " << process.turnoverTime
            << ", 带权周转时间: " << fixed << setprecision(2)
            << process.weightedTurnoverTime << "\n";
    }

    resultFile.close();
    cout << "FCFS 调度完成，结果已保存到 result.txt" << endl;
}

// 时间片轮转调度（RR）
void rrScheduling() {
    ofstream resultFile("result.txt", ios::app); // append to result.txt
    if (!resultFile.is_open()) {
        cerr << "错误：无法打开 result.txt 进行结果保存" << endl;
        return;
    }

    queue<PCB*> processQueue;
    // 按创建时间排序
    sort(processList.begin(), processList.end(), [](const PCB& a, const PCB& b) {
        return a.createTime < b.createTime;
        });

    int currentTime = 0;
    size_t index = 0; // Index for tracking processes to enqueue
    unordered_map<string, int> remainingTimeMap;

    // Initialize remaining times
    for (auto& process : processList) {
        remainingTimeMap[process.pName] = process.runTime;
    }

    // 输出表头
    resultFile << "时间片\t运行的进程\t";
    for (const auto& process : processList) {
        resultFile << "进程" << process.pName << "状态\t";
    }
    resultFile << "\n";

    // Main scheduling loop
    while (!processQueue.empty() || index < processList.size()) {
        // 如果队列为空，跳到下一个进程的创建时间
        if (processQueue.empty() && index < processList.size() && processList[index].createTime > currentTime) {
            currentTime = processList[index].createTime;
        }

        // 将当前时间的进程加入队列
        while (index < processList.size() && processList[index].createTime <= currentTime) {
            PCB* newProcess = &processList[index];
            processQueue.push(newProcess);
            cout << "进程: " << newProcess->pName << " 在 " << currentTime << " ms 时被加入队列" << endl;
            index++;
        }

        // 如果队列仍为空，则当前时间加1继续
        if (processQueue.empty()) {
            currentTime++;
            continue;
        }

        // 从队列中取出一个进程
        PCB* currentProcess = processQueue.front();
        processQueue.pop();

        // 如果是第一次运行，设置开始时间
        if (currentProcess->startTime == -1) {
            currentProcess->startTime = currentTime;
        }

        // 执行时间片
        int execTime = 1; // 每次执行1个时间片单位
        if (remainingTimeMap[currentProcess->pName] < execTime) {
            execTime = remainingTimeMap[currentProcess->pName];
        }
        remainingTimeMap[currentProcess->pName] -= execTime;
        currentTime += execTime;

        // 输出当前时间片的状态
        resultFile << currentTime - execTime << "\t" << currentProcess->pName << "\t";

        for (const auto& process : processList) {
            if (process.pName == currentProcess->pName && remainingTimeMap[process.pName] > 0) {
                resultFile << "run\t"; // 当前运行的进程
            }
            else if (remainingTimeMap[process.pName] == 0) {
                resultFile << "complete\t"; // 已完成的进程
            }
            else if (process.createTime > currentTime - execTime) {
                resultFile << "null\t"; // 尚未到达的进程
            }
            else {
                resultFile << "ready\t"; // 已到达但未运行的进程
            }
        }
        resultFile << "\n";

        // 如果进程未完成，重新加入队列
        if (remainingTimeMap[currentProcess->pName] > 0) {
            processQueue.push(currentProcess);
        }
        else {
            // 完成的进程
            currentProcess->completeTime = currentTime;
            currentProcess->turnoverTime = currentProcess->completeTime - currentProcess->createTime;
            currentProcess->weightedTurnoverTime =
                static_cast<double>(currentProcess->turnoverTime) / currentProcess->originalRunTime;

            resultFile << "进程 " << currentProcess->pName << " 在 " << currentTime << " ms 完成 "
                << "| 周转时间： " << currentProcess->turnoverTime
                << " ms | 带权周转时间： " << fixed << setprecision(2)
                << currentProcess->weightedTurnoverTime << endl;
        }
    }

    // 输出最终统计信息
    resultFile << "\n最终统计信息：\n";
    for (const auto& process : processList) {
        resultFile << "进程名称: " << process.pName
            << ", 开始时间: " << process.startTime
            << ", 完成时间: " << process.completeTime
            << ", 周转时间: " << process.turnoverTime
            << ", 带权周转时间: " << fixed << setprecision(2)
            << process.weightedTurnoverTime << "\n";
    }

    resultFile.close();
    cout << "RR 调度完成，结果已保存到 result.txt" << endl;
}

// 模拟页面置换算法
void simulatePageReplacement(const vector<Program>& programs, int maxPages, int pageSize, int timeSlice) {
    // 获取用户选择的页面替换算法
    cout << "请选择页面替换算法：" << endl;
    cout << "1. FIFO" << endl;
    cout << "2. LRU" << endl;
    cout << "请输入选项 (1 或 2): ";
    int choice;
    while (!(cin >> choice) || (choice != 1 && choice != 2)) {
        cout << "输入无效，请输入1或2: ";
        cin.clear();
        cin.ignore(10000, '\n');
    }
    string algorithm = (choice == 1) ? "FIFO" : "LRU";
    cout << "选择的页面替换算法为: " << algorithm << endl;

    // 创建 PageManager 实例
    FIFOPageManager* fifoManager = nullptr;
    LRUPageManager* lruManager = nullptr;

    if (choice == 1) {
        fifoManager = new FIFOPageManager(pageSize, maxPages);
    }
    else {
        lruManager = new LRUPageManager(pageSize, maxPages);
    }

    // 输出物理块状态和缺页日志
    cout << "\n--------------------页面调度日志--------------------\n";
    cout << "程序 | 访问页面 | 缺页 | 物理块状态" << endl;
    cout << "--------------------------------------------\n";

    // 模拟时间片调度
    // 创建一个 vector 来跟踪每个程序的当前访问索引
    vector<size_t> programIndices(programs.size(), 0);
    bool allFinished = false;
    int globalTime = 0; // 用于 LRU

    while (!allFinished) {
        allFinished = true;
        for (size_t i = 0; i < programs.size(); ++i) {
            // 检查当前程序是否还有页面需要访问
            if (programIndices[i] < programs[i].page_sequence.size()) {
                allFinished = false;
                // 处理一个时间片的页面访问
                int pagesToProcess = min(timeSlice, static_cast<int>(programs[i].page_sequence.size() - programIndices[i]));
                for (int p = 0; p < pagesToProcess; ++p) {
                    int page = programs[i].page_sequence[programIndices[i]];
                    bool pageFault = false;

                    // 使用选择的页面替换算法
                    if (choice == 1) { // FIFO
                        pageFault = fifoManager->fifoReplace(page);
                    }
                    else { // LRU
                        globalTime++;
                        pageFault = lruManager->lruReplace(page, globalTime);
                    }

                    // 打印访问页面、缺页情况
                    cout << programs[i].name << " | 页面 " << page << " | ";
                    if (pageFault) {
                        cout << "是 | ";
                    }
                    else {
                        cout << "否 | ";
                    }

                    // 打印物理块状态
                    if (choice == 1) { // FIFO
                        fifoManager->printPhysicalBlockState();
                    }
                    else { // LRU
                        lruManager->printPhysicalBlockState();
                    }
                }
                programIndices[i] += pagesToProcess;
            }
        }
    }

    // 输出页面置换总结报告
    cout << "\n--------------------分页调度总结报告--------------------\n";
    if (choice == 1 && fifoManager != nullptr) {
        fifoManager->printSummary();
        fifoManager->printLog();
    }
    else if (choice == 2 && lruManager != nullptr) {
        lruManager->printSummary();
        lruManager->printLog();
    }
    cout << "------------------------------------------------------------\n";

    // 释放动态分配的内存
    if (fifoManager != nullptr) {
        delete fifoManager;
    }
    if (lruManager != nullptr) {
        delete lruManager;
    }
}
// 主函数
int main() {
    // 设置控制台代码页为65001（UTF-8）
#ifdef _WIN32
    system("chcp 65001");
#endif

    // 设置区域设置为用户默认
    setlocale(LC_ALL, "");

    // 关闭同步IO，提高性能
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    cout.tie(nullptr);

    // 加载运行步骤
    RunSteps runSteps = loadRunSteps();

    // 加载进程信息
    loadProcesses(runSteps.runTimes);

    // 加载程序详细信息
    map<string, map<string, double>> programs = loadPrograms();

    while (true) {
        cout << "\n请选择功能：" << endl;
        cout << "1. 显示进程信息" << endl;
        cout << "2. 显示程序详细信息" << endl;
        cout << "3. 先来先服务调度（FCFS）" << endl;
        cout << "4. 时间片轮转调度（RR）" << endl;
        cout << "5. 分页调度" << endl;
        cout << "6. 退出程序" << endl;
        int choice;
        cin >> choice;

        switch (choice) {
        case 1:
            if (processList.empty()) {
                cout << "未加载任何进程信息。" << endl;
            }
            else {
                for (const auto& process : processList) {
                    cout << "进程: " << process.pName << ", 创建时间: " << process.createTime
                        << ", 运行时间: " << process.runTime << ", 优先级: " << process.grade
                        << ", 程序备注: " << process.pRemark << endl;
                }
            }
            break;
        case 2:
            if (programs.empty()) {
                cout << "未加载任何程序详细信息。" << endl;
            }
            else {
                for (const auto& [program, functions] : programs) {
                    cout << "程序: " << program << endl;
                    for (const auto& [func, size] : functions) {
                        cout << "  功能: " << func << ", 大小: " << size << " KB" << endl;
                    }
                }
            }
            break;
        case 3:
            fcfsScheduling();
            cout << "先来先服务调度（FCFS）完成。结果已保存到 result.txt" << endl;
            break;
        case 4:
            rrScheduling();
            cout << "时间片轮转调度（RR）完成。结果已保存到 result.txt" << endl;
            break;
        case 5: {
            // 用户输入页面大小、每个进程的最大页面数和时间片长度
            cout << "请输入页面大小（KB）: ";
            int pageSize;
            while (!(cin >> pageSize) || pageSize <= 0) {
                cout << "输入无效，页面大小必须为正整数，请重新输入: ";
                cin.clear();
                cin.ignore(10000, '\n');
            }
            cout << "页面大小设置为: " << pageSize << " KB" << endl;

            cout << "请输入每个进程的最大页面数: ";
            int maxPages;
            while (!(cin >> maxPages) || maxPages <= 0) {
                cout << "输入无效，最大页面数必须为正整数，请重新输入: ";
                cin.clear();
                cin.ignore(10000, '\n');
            }
            cout << "每个进程的最大页面数设置为: " << maxPages << endl;

            cout << "请输入时间片长度（即每个进程在一个轮转中可以访问的页面数）: ";
            int timeSlice;
            while (!(cin >> timeSlice) || timeSlice <= 0) {
                cout << "输入无效，时间片长度必须为正整数，请重新输入: ";
                cin.clear();
                cin.ignore(10000, '\n');
            }
            cout << "时间片长度设置为: " << timeSlice << endl;

            // 解析 run.txt 文件
            string runFileName = "run.txt"; // 确保 run.txt 在同一目录下
            vector<Program> programs = parseRunFile(runFileName, pageSize);

            if (programs.empty()) {
                cout << "在 " << runFileName << " 中未找到任何程序。" << endl;
                return 0;
            }

            // 模拟页面置换
            simulatePageReplacement(programs, maxPages, pageSize, timeSlice);

            cout << "\n*************************** 模拟完成 ***************************" << endl;
            break;
        }

        case 6:
            cout << "正在退出程序..." << endl;
            return 0;
        default:
            cout << "无效的选择，请重试。" << endl;
        }
    }

    return 0;
}
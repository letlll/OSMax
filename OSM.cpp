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
#include <algorithm> // 引入 std::min 和 std::max

// PCB类：进程控制块
class PCB {
public:
    std::string pName;               // 进程名称
    std::string pRemark;             // 程序名称
    std::string pStatus;             // 进程状态
    int createTime;                  // 创建时间
    int runTime;                     // 运行时间
    int grade;                       // 优先级
    int startTime;                   // 开始时间
    int completeTime;                // 完成时间
    int turnoverTime;                // 周转时间
    double weightedTurnoverTime;     // 带权周转时间
    int originalRunTime;             // 原始运行时间

    PCB(std::string name, int create, int runtime, int priority, std::string remark)
        : pName(name), createTime(create), runTime(runtime), grade(priority),
        pRemark(remark), pStatus("null"), startTime(-1), completeTime(0),
        turnoverTime(0), weightedTurnoverTime(0.0), originalRunTime(runtime) {}

    void updateStatus(const std::string& newStatus) {
        pStatus = newStatus;
        std::cout << "Process " << pName << " status updated to " << pStatus << std::endl;
    }


};

// PageManager类：处理FIFO和LRU页面替换
class PageManager {
public:
    double pageSize;
    int maxPages;
    std::queue<int> fifoPages;                     // FIFO队列
    std::unordered_map<int, int> lruPages;        // LRU映射：页面及其最近访问时间
    std::vector<std::string> log;                  // 页面操作日志
    int pageFaults = 0;
    int pageHits = 0;

    PageManager(double size, int max) : pageSize(size), maxPages(max) {}

    // FIFO替换策略
    bool fifoReplace(int page) {
        // 检查页面是否在内存中
        std::vector<int> fifoVec = queueToVector(fifoPages);
        if (std::find(fifoVec.begin(), fifoVec.end(), page) != fifoVec.end()) {
            pageHits++;
            log.push_back("FIFO: Page " + std::to_string(page) + " already in memory (hit).");
            return false;  // 页面命中时返回 false，表示没有缺页
        }

        pageFaults++;
        if (fifoPages.size() >= maxPages) {
            if (fifoPages.empty()) {
                std::cerr << "Error: FIFO queue is empty, cannot remove page." << std::endl;
                log.push_back("FIFO: Error - FIFO queue is empty, cannot remove page.");
                return false; // 发生错误时返回 false
            }
            int removed = fifoPages.front();
            fifoPages.pop();
            log.push_back("FIFO: Page " + std::to_string(removed) + " removed.");
        }

        fifoPages.push(page);
        log.push_back("FIFO: Page " + std::to_string(page) + " added.");

        return true;  // 页面替换时返回 true，表示发生了缺页
    }

    // LRU替换策略
    bool lruReplace(int page, int currentTime) {
        if (lruPages.find(page) != lruPages.end()) {
            pageHits++;
            lruPages[page] = currentTime;  // 更新最近访问时间
            log.push_back("LRU: Page " + std::to_string(page) + " already in memory (hit).");
            return false;  // 页面命中时返回 false，表示没有缺页
        }

        pageFaults++;
        if (lruPages.size() >= maxPages) {
            int lruPage = getLRUPage();
            if (lruPage == -1) {
                std::cerr << "Error: No LRU page found to remove." << std::endl;
                log.push_back("LRU: Error - No LRU page found to remove.");
                return false; // 发生错误时返回 false
            }
            lruPages.erase(lruPage);  // 删除最久未使用的页面
            log.push_back("LRU: Page " + std::to_string(lruPage) + " removed.");
        }

        lruPages[page] = currentTime;  // 添加新页面，并更新访问时间
        log.push_back("LRU: Page " + std::to_string(page) + " added.");

        return true;  // 页面替换时返回 true，表示发生了缺页
    }



    void printSummary() {
        std::cout << "Page Faults: " << pageFaults << std::endl;
        std::cout << "Page Hits: " << pageHits << std::endl;
        if (pageHits + pageFaults > 0) {
            std::cout << "Hit Rate: " << std::fixed << std::setprecision(2) << ((static_cast<double>(pageHits) / (pageHits + pageFaults)) * 100) << "%" << std::endl;
        }
    }

private:
    // 将队列转换为向量
    std::vector<int> queueToVector(std::queue<int> q) {
        std::vector<int> result;
        while (!q.empty()) {
            result.push_back(q.front());
            q.pop();
        }
        return result;
    }

    // 获取最久未使用的页面
    int getLRUPage() {
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

// 全局变量
std::vector<PCB> processList;
double pageSize = 4.0; // 默认页面大小（KB）
int timeSlice = 2;      // 默认时间片（ms）

// 函数声明
void loadProcesses(std::map<std::string, int>& runTimes);
std::map<std::string, int> loadRunSteps();
std::map<std::string, std::map<std::string, double>> loadPrograms();
void fcfsScheduling();
void rrScheduling();
void simulateCPU(std::map<std::string, int>& runTimes);
void pageScheduling(std::map<std::string, std::map<std::string, double>>& programs);
void printPhysicalBlockState(PageManager& pageManager);

// 辅助函数：去除字符串首尾的空白字符
std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
        return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

// 加载运行步骤，从run.txt中读取
std::map<std::string, int> loadRunSteps() {
    std::ifstream file("run.txt");
    if (!file.is_open()) {
        std::cerr << "Error: Unable to open run.txt" << std::endl;
        return {};
    }

    std::map<std::string, int> runTimes;
    std::string line, currentProgram;
    bool isFirstLine = true; // 标记是否为第一行

    while (std::getline(file, line)) {
        if (isFirstLine) {
            // 检测并移除BOM
            if (line.size() >= 3 &&
                static_cast<unsigned char>(line[0]) == 0xEF &&
                static_cast<unsigned char>(line[1]) == 0xBB &&
                static_cast<unsigned char>(line[2]) == 0xBF) {
                line = line.substr(3);
            }
            isFirstLine = false;
        }

        // 打印读取的行及其长度以进行调试
        std::cout << "Reading line: [" << line << "], Length: " << line.length() << std::endl;

        line = trim(line);
        if (line.empty()) continue; // Skip empty lines

        // 检查是否为程序名行（"ProgramName" 或 "程序"）
        if (line.find("ProgramName") == 0 || line.find("程序") == 0) {
            // 提取程序名
            size_t pos_space = line.find_first_of(" \t"); // 查找第一个空格或制表符
            if (pos_space != std::string::npos) {
                std::string afterKeyword = line.substr(pos_space + 1);
                afterKeyword = trim(afterKeyword);
                currentProgram = afterKeyword;
                std::cout << "Found Program Name: [" << currentProgram << "]" << std::endl; // Debug info
            }
            else {
                std::cerr << "Warning: Unable to extract program name: " << line << std::endl;
            }
        }
        else {
            // 确保存在当前程序名
            if (currentProgram.empty()) {
                std::cerr << "Warning: Found run step before program name: " << line << std::endl;
                continue;
            }

            // 解析运行步骤行（格式："时间 操作 参数" 或 "时间 操作"）
            std::istringstream iss(line);
            int time;
            std::string operation, param;

            // 如果无法解析到时间和操作，跳过
            if (!(iss >> time >> operation)) {
                std::cerr << "Warning: Failed to parse run step line: " << line << std::endl;
                continue;
            }

            // 检测是否有第三列参数
            if (!(iss >> param)) {
                param = ""; // 如果没有参数，设置为空字符串
            }

            // 如果操作为 "结束" 或 "End"，将时间作为最终运行时间
            if (operation == "结束" || operation == "End") {
                runTimes[currentProgram] = std::max(runTimes[currentProgram], time);
                std::cout << "Set final run time for [" << currentProgram << "] to " << time << " ms" << std::endl;
                continue;
            }

            // 更新当前程序的运行时间，取时间的最大值
            runTimes[currentProgram] = std::max(runTimes[currentProgram], time);
            std::cout << "Updated run time [" << currentProgram << "]: " << time << std::endl; // Debug info
        }
    }

    file.close();

    // 输出加载的运行时间
    std::cout << "Loaded Run Times:" << std::endl;
    for (const auto& [program, time] : runTimes) {
        std::cout << "Program: [" << program << "], Run Time: " << time << " ms" << std::endl; // Debug info
    }

    return runTimes;
}

// 加载程序详细信息，从program.txt中读取
std::map<std::string, std::map<std::string, double>> loadPrograms() {
    std::ifstream file("program.txt");
    if (!file.is_open()) {
        std::cerr << "Error: Unable to open program.txt" << std::endl;
        return {};
    }

    std::map<std::string, std::map<std::string, double>> programs;
    std::string line, currentProgram;
    bool isFirstLine = true; // 标记是否为第一行

    while (std::getline(file, line)) {
        if (isFirstLine) {
            // 检测并移除BOM
            if (line.size() >= 3 &&
                static_cast<unsigned char>(line[0]) == 0xEF &&
                static_cast<unsigned char>(line[1]) == 0xBB &&
                static_cast<unsigned char>(line[2]) == 0xBF) {
                line = line.substr(3);
            }
            isFirstLine = false;
        }

        // 打印读取的行及其长度以进行调试
        std::cout << "Reading line: [" << line << "], Length: " << line.length() << std::endl;

        line = trim(line);
        if (line.empty()) continue; // Skip empty lines

        // Check if it's a FileName line
        if (line.find("FileName") == 0) {
            // Extract program name
            size_t pos_space = line.find_first_of(" \t", 8); // "FileName" is 8 characters
            if (pos_space != std::string::npos) {
                std::string afterKeyword = line.substr(pos_space + 1);
                afterKeyword = trim(afterKeyword);
                currentProgram = afterKeyword;
                std::cout << "Found Program: [" << currentProgram << "]" << std::endl; // Debug info
            }
            else {
                std::cerr << "Warning: Unable to extract file name: " << line << std::endl;
            }
        }
        else {
            // Ensure there is a current program name
            if (currentProgram.empty()) {
                std::cerr << "Warning: Found function definition before program name: " << line << std::endl;
                continue;
            }

            // Parse function line
            // Example line: "Main	0.6"
            std::istringstream iss(line);
            std::string funcName;
            double size;
            if (!(iss >> funcName >> size)) {
                std::cerr << "Warning: Failed to parse function line: " << line << std::endl;
                continue;
            }

            programs[currentProgram][funcName] = size;
            std::cout << "Added Function [" << funcName << "] Size " << size << " KB to Program [" << currentProgram << "]" << std::endl; // Debug info
        }
    }

    file.close();
    return programs;
}

// 加载进程信息，从process.txt中读取
void loadProcesses(std::map<std::string, int>& runTimes) {
    std::cout << "Loaded Run Times:" << std::endl;
    for (const auto& [program, time] : runTimes) {
        std::cout << "Program: [" << program << "], Run Time: " << time << "ms" << std::endl; // Debug info
    }

    std::ifstream file("process.txt");
    if (!file.is_open()) {
        std::cerr << "Error: Unable to open process.txt" << std::endl;
        return;
    }
    std::string line;
    bool isFirstLine = true; // 标记是否为第一行

    while (std::getline(file, line)) {
        if (isFirstLine) {
            // 检测并移除BOM
            if (line.size() >= 3 &&
                static_cast<unsigned char>(line[0]) == 0xEF &&
                static_cast<unsigned char>(line[1]) == 0xBB &&
                static_cast<unsigned char>(line[2]) == 0xBF) {
                line = line.substr(3);
            }
            isFirstLine = false;
        }

        // 打印读取的行及其长度以进行调试
        std::cout << "Reading line: [" << line << "], Length: " << line.length() << std::endl;

        line = trim(line);
        if (line.empty()) continue; // Skip empty lines

        // 使用istringstream读取
        std::istringstream iss(line);
        std::string pName, pRemark;
        int createTime, grade;
        if (!(iss >> pName >> createTime >> grade >> pRemark)) {
            std::cerr << "Warning: Failed to parse process line: " << line << std::endl;
            continue;
        }

        std::cout << "Processing Process: [" << pName << "], Program Remark: [" << pRemark << "]" << std::endl; // Debug info

        if (runTimes.find(pRemark) != runTimes.end()) {
            processList.emplace_back(pName, createTime, runTimes[pRemark], grade, pRemark);
            std::cout << "Added Process: " << pName << " Run Time: " << runTimes[pRemark] << std::endl; // Debug info
        }
        else {
            std::cerr << "Warning: Program [" << pRemark << "] not found in runTimes. Skipping Process [" << pName << "]." << std::endl;
        }
    }
    file.close();
}

// FCFS 调度算法（改进：动态记录每个时间片的状态）
void fcfsScheduling() {
    std::ofstream resultFile("result.txt");
    if (!resultFile.is_open()) {
        std::cerr << "Error: Unable to open result.txt" << std::endl;
        return;
    }

    // 按创建时间排序
    std::sort(processList.begin(), processList.end(), [](PCB& a, PCB& b) {
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
            << ", 带权周转时间: " << std::fixed << std::setprecision(2)
            << process.weightedTurnoverTime << "\n";
    }

    resultFile.close();
    std::cout << "FCFS 调度完成，结果已保存到 result.txt" << std::endl;
}





// 时间片轮转调度（RR）
void rrScheduling() {
    std::ofstream resultFile("result.txt", std::ios::app);
    if (!resultFile.is_open()) {
        std::cerr << "错误：无法打开 result.txt 进行结果保存" << std::endl;
        return;
    }

    std::queue<PCB*> processQueue;
    std::sort(processList.begin(), processList.end(), [](const PCB& a, const PCB& b) {
        return a.createTime < b.createTime;
        });

    int currentTime = 0;
    size_t index = 0; // Index for tracking processes to enqueue
    std::unordered_map<std::string, int> remainingTimeMap;

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
            resultFile << " 进程: " << newProcess->pName << " 在 " << currentTime << " ms 时被加入队列" << std::endl;
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
        int execTime = std::min(1, remainingTimeMap[currentProcess->pName]);
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
                << " ms | 带权周转时间： " << std::fixed << std::setprecision(2)
                << currentProcess->weightedTurnoverTime << std::endl;
        }
    }

    // 输出最终统计信息
    resultFile << "\n最终统计信息：\n";
    for (const auto& process : processList) {
        resultFile << "进程名称: " << process.pName
            << ", 开始时间: " << process.startTime
            << ", 完成时间: " << process.completeTime
            << ", 周转时间: " << process.turnoverTime
            << ", 带权周转时间: " << std::fixed << std::setprecision(2)
            << process.weightedTurnoverTime << "\n";
    }

    resultFile.close();
    std::cout << "RR 调度完成，结果已保存到 result.txt" << std::endl;
}



void simulateCPU() {
    std::ifstream file("run.txt");
    if (!file.is_open()) {
        std::cerr << "Error: Unable to open run.txt" << std::endl;
        return;
    }
    std::cout << "模拟CPU使用情况..." << std::endl;

    std::string line, currentProgram;
    std::map<int, std::vector<std::string>> cpuLog;  // 时间点 -> 操作列表
    std::map<std::string, std::string> processStatus; // 进程名 -> 当前状态
    std::map<std::string, int> processStartTimes;     // 进程名 -> 启动时间
    bool isFirstLine = true; // 标记是否为第一行

    // 读取所有文件内容并处理
    while (std::getline(file, line)) {
        if (isFirstLine) {
            // 检测并移除BOM
            if (line.size() >= 3 &&
                static_cast<unsigned char>(line[0]) == 0xEF &&
                static_cast<unsigned char>(line[1]) == 0xBB &&
                static_cast<unsigned char>(line[2]) == 0xBF) {
                line = line.substr(3);
            }
            isFirstLine = false;
        }

        line = trim(line);
        if (line.empty()) continue; // 跳过空行

        // 检查是否是 ProgramName 行
        if (line.find("ProgramName") == 0) {
            size_t pos_space = line.find_first_of(" \t", 11); // "ProgramName"是11个字符
            if (pos_space != std::string::npos) {
                currentProgram = trim(line.substr(pos_space + 1));
                std::cout << "Simulating Program: [" << currentProgram << "]" << std::endl;

                // 初始化进程状态为 "Ready"
                processStatus[currentProgram] = "Ready";
                processStartTimes[currentProgram] = 0; // 假设开始时间是0
            }
            else {
                std::cerr << "Warning: Unable to extract program name: " << line << std::endl;
            }
        }
        else {
            // 解析运行步骤行
            std::istringstream iss(line);
            int time;
            std::string operation, param;
            if (iss >> time >> operation) {
                std::string event = currentProgram + " " + operation;
                if (operation == "End") {
                    cpuLog[time].push_back(event);  // 记录"End"操作
                    processStatus[currentProgram] = "Terminated";  // 更新进程状态
                }
                else if (iss >> param) {
                    cpuLog[time].push_back(event + " " + param);
                }
                else {
                    cpuLog[time].push_back(event);
                }
            }
        }
    }
    file.close();

    // 获取最大时间点
    int maxTime = cpuLog.rbegin()->first;

    // 每1毫秒递增并显示状态
    for (int t = 0; t <= maxTime; ++t) {
        // 输出当前时刻的进程状态
        std::cout << "时间: " << t << " ms, 当前进程状态:" << std::endl;

        // 输出当前时间点的操作
        if (cpuLog.find(t) != cpuLog.end()) {
            for (const auto& operation : cpuLog[t]) {
                std::cout << "    操作: " << operation << std::endl;
            }
        }

        // 更新进程状态
        for (auto& [process, status] : processStatus) {
            // 如果进程是运行中，且时间片结束，将状态设置为 ready
            if (status == "Running") {
                status = "Ready"; // 运行结束后，设置为就绪
            }
            // 如果进程已终止，则不更新
            else if (status == "Terminated") {
                continue;
            }

            // 打印每个进程的当前状态
            std::cout << "    进程: " << process << ", 状态: " << status << std::endl;
        }

        // 模拟每1毫秒的递增
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    std::cout << "CPU仿真完成!" << std::endl;
}




// 模拟分页调度
void pageScheduling(std::map<std::string, std::map<std::string, double>>& programs) {
    std::map<std::string, int> pageRequirements;

    // 计算每个程序的页面需求，不根据实际进程大小动态改变页面数
    for (const auto& [program, functions] : programs) {
        double totalSize = 0.0;
        for (const auto& [func, size] : functions) {
            totalSize += size; // 累积程序的总内存需求
        }

        // 根据总内存需求计算所需的页面数
        pageRequirements[program] = static_cast<int>(std::ceil(totalSize / pageSize));
        std::cout << "程序 " << program << " 所需页面数: " << pageRequirements[program] << std::endl;
    }

    // 获取用户输入的最大页面数
    std::cout << "\n请输入每个进程的最大页面数: ";
    int maxPages;
    while (!(std::cin >> maxPages) || maxPages <= 0) {
        std::cout << "输入无效，最大页面数必须为正整数，请重新输入: ";
        std::cin.clear();
        std::cin.ignore(10000, '\n');
    }
    std::cout << "每个进程的最大页面数设置为: " << maxPages << std::endl;

    // 获取用户选择的页面替换算法
    std::cout << "请选择页面替换算法：" << std::endl;
    std::cout << "1. FIFO" << std::endl;
    std::cout << "2. LRU" << std::endl;
    std::cout << "请输入选项 (1 或 2): ";
    int choice;
    while (!(std::cin >> choice) || (choice != 1 && choice != 2)) {
        std::cout << "输入无效，请输入1或2: ";
        std::cin.clear();
        std::cin.ignore(10000, '\n');
    }
    std::cout << "选择的页面替换算法为: " << (choice == 1 ? "FIFO" : "LRU") << std::endl;

    // 创建PageManager实例
    PageManager pageManager(pageSize, maxPages);
    int currentTime = 0;

    // 输出物理块状态和缺页日志
    std::cout << "\n--------------------页面调度日志--------------------\n";
    std::cout << "访问页面 | 物理块状态        | 缺页\n";
    std::cout << "--------------------------------------------\n";

    for (const auto& [program, pages] : pageRequirements) {
        std::cout << "正在处理程序: " << program << " | 需要页面数: " << pages << std::endl;
        for (int page = 0; page < pages; ++page) {
            bool pageFault = false;

            std::cout << std::setw(9) << page << " | ";

            // 使用选择的页面替换算法
            if (choice == 1) {
                pageFault = pageManager.fifoReplace(page);
            }
            else {
                pageFault = pageManager.lruReplace(page, currentTime);
            }

            // 打印物理块状态
            printPhysicalBlockState(pageManager);

            // 打印缺页状态
            if (pageFault) {
                std::cout << "是" << std::endl;
            }
            else {
                std::cout << "否" << std::endl;
            }

            currentTime++;
        }
    }

    // 输出页面置换总结报告
    std::cout << "\n--------------------分页调度总结报告--------------------\n";
    pageManager.printSummary();
}

// 打印物理块的状态
void printPhysicalBlockState(PageManager& pageManager) {
    std::queue<int> fifoPages = pageManager.fifoPages;
    std::unordered_map<int, int> lruPages = pageManager.lruPages;

    // 打印FIFO物理块状态
    std::cout << "[ ";
    if (!fifoPages.empty()) {
        std::vector<int> state;
        while (!fifoPages.empty()) {
            state.push_back(fifoPages.front());
            fifoPages.pop();
        }
        for (int page : state) {
            std::cout << page << " ";
        }
    }
    else {
        std::cout << "- ";
    }

    std::cout << "] | ";

    // 打印LRU物理块状态
    if (!lruPages.empty()) {
        for (const auto& entry : lruPages) {
            std::cout << entry.first << " ";
        }
    }
    else {
        std::cout << "- ";
    }

    std::cout << "| ";
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
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    std::cout.tie(nullptr);

    // 加载运行步骤
    std::map<std::string, int> runTimes = loadRunSteps();

    // 加载进程信息
    loadProcesses(runTimes);

    // 加载程序详细信息
    std::map<std::string, std::map<std::string, double>> programs = loadPrograms();

    while (true) {
        std::cout << "\n请选择功能：" << std::endl;
        std::cout << "1. 显示进程信息" << std::endl;
        std::cout << "2. 显示程序详细信息" << std::endl;
        std::cout << "3. 先来先服务调度（FCFS）" << std::endl;
        std::cout << "4. 时间片轮转调度（RR）" << std::endl;
        std::cout << "5. 分页调度" << std::endl;
        std::cout << "6. 退出程序" << std::endl;
        int choice;
        std::cin >> choice;

        switch (choice) {
        case 1:
            if (processList.empty()) {
                std::cout << "未加载任何进程信息。" << std::endl;
            }
            else {
                for (const auto& process : processList) {
                    std::cout << "进程: " << process.pName << ", 创建时间: " << process.createTime
                        << ", 运行时间: " << process.runTime << ", 优先级: " << process.grade
                        << ", 程序备注: " << process.pRemark << std::endl;
                }
            }
            break;
        case 2:
            if (programs.empty()) {
                std::cout << "未加载任何程序详细信息。" << std::endl;
            }
            else {
                for (const auto& [program, functions] : programs) {
                    std::cout << "程序: " << program << std::endl;
                    for (const auto& [func, size] : functions) {
                        std::cout << "  功能: " << func << ", 大小: " << size << " KB" << std::endl;
                    }
                }
            }
            break;
        case 3:
            fcfsScheduling();
            std::cout << "先来先服务调度（FCFS）完成。结果已保存到 result.txt" << std::endl;
            break;
        case 4:
            rrScheduling();
            std::cout << "时间片轮转调度（RR）完成。结果已保存到 result.txt" << std::endl;
            break;
        case 5: {
            // 分页调度功能
            std::cout << "请选择操作：" << std::endl;
            std::cout << "1. 设置页面大小和时间片长度" << std::endl;
            std::cout << "2. 执行分页调度" << std::endl;
            std::cout << "请输入选项 (1 或 2): ";
            int option;
            while (!(std::cin >> option) || (option != 1 && option != 2)) {
                std::cout << "输入无效，请输入1或2: ";
                std::cin.clear();
                std::cin.ignore(10000, '\n');
            }

            if (option == 1) {
                // 设置新的页面大小
                std::cout << "请输入新的页面大小 (单位: KB): ";
                while (!(std::cin >> pageSize) || pageSize <= 0.0) {
                    std::cout << "输入无效，页面大小必须为正数，请重新输入: ";
                    std::cin.clear();
                    std::cin.ignore(10000, '\n');
                }

                // 设置新的时间片长度
                std::cout << "请输入新的时间片长度 (单位: ms): ";
                while (!(std::cin >> timeSlice) || timeSlice <= 0) {
                    std::cout << "输入无效，时间片长度必须为正整数，请重新输入: ";
                    std::cin.clear();
                    std::cin.ignore(10000, '\n');
                }

                std::cout << "页面大小已设置为: " << pageSize << " KB | 时间片长度已设置为: " << timeSlice << " ms" << std::endl;
            }
            else if (option == 2) {
                // 执行分页调度
                pageScheduling(programs);
            }
            break;
        }


        case 6:
            std::cout << "正在退出程序..." << std::endl;
            return 0;
        default:
            std::cout << "无效的选择，请重试。" << std::endl;
        }
    }

    return 0;
}

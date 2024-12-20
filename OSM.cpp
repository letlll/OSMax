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
#include <algorithm> // ���� std::min �� std::max

// PCB�ࣺ���̿��ƿ�
class PCB {
public:
    std::string pName;               // ��������
    std::string pRemark;             // ��������
    std::string pStatus;             // ����״̬
    int createTime;                  // ����ʱ��
    int runTime;                     // ����ʱ��
    int grade;                       // ���ȼ�
    int startTime;                   // ��ʼʱ��
    int completeTime;                // ���ʱ��
    int turnoverTime;                // ��תʱ��
    double weightedTurnoverTime;     // ��Ȩ��תʱ��
    int originalRunTime;             // ԭʼ����ʱ��

    PCB(std::string name, int create, int runtime, int priority, std::string remark)
        : pName(name), createTime(create), runTime(runtime), grade(priority),
        pRemark(remark), pStatus("Waiting"), startTime(-1), completeTime(0),
        turnoverTime(0), weightedTurnoverTime(0.0), originalRunTime(runtime) {}
};

// PageManager�ࣺ����FIFO��LRUҳ���滻
class PageManager {
public:
    double pageSize;
    int maxPages;
    std::queue<int> fifoPages;                     // FIFO����
    std::unordered_map<int, int> lruPages;        // LRUӳ�䣺ҳ�漰���������ʱ��
    std::vector<std::string> log;                  // ҳ�������־
    int pageFaults = 0;
    int pageHits = 0;

    PageManager(double size, int max) : pageSize(size), maxPages(max) {}

    // FIFO�滻����
    void fifoReplace(int page) {
        // ���ҳ���Ƿ����ڴ���
        std::vector<int> fifoVec = queueToVector(fifoPages);
        if (std::find(fifoVec.begin(), fifoVec.end(), page) != fifoVec.end()) {
            pageHits++;
            log.push_back("FIFO: Page " + std::to_string(page) + " already in memory (hit).");
            return;
        }
        pageFaults++;
        if (fifoPages.size() >= maxPages) {
            if (fifoPages.empty()) {
                std::cerr << "Error: FIFO queue is empty, cannot remove page." << std::endl;
                log.push_back("FIFO: Error - FIFO queue is empty, cannot remove page.");
                return;
            }
            int removed = fifoPages.front();
            fifoPages.pop();
            log.push_back("FIFO: Page " + std::to_string(removed) + " removed.");
        }
        fifoPages.push(page);
        log.push_back("FIFO: Page " + std::to_string(page) + " added.");
    }



    // LRU�滻����
    void lruReplace(int page, int currentTime) {
        if (lruPages.find(page) != lruPages.end()) {
            pageHits++;
            lruPages[page] = currentTime;
            log.push_back("LRU: Page " + std::to_string(page) + " already in memory (hit).");
            return;
        }
        pageFaults++;
        if (lruPages.size() >= maxPages) {
            int lruPage = getLRUPage();
            if (lruPage == -1) {
                std::cerr << "Error: No LRU page found to remove." << std::endl;
                log.push_back("LRU: Error - No LRU page found to remove.");
                return;
            }
            lruPages.erase(lruPage);
            log.push_back("LRU: Page " + std::to_string(lruPage) + " removed.");
        }
        lruPages[page] = currentTime;
        log.push_back("LRU: Page " + std::to_string(page) + " added.");
    }


    void printSummary() {
        std::cout << "Page Faults: " << pageFaults << std::endl;
        std::cout << "Page Hits: " << pageHits << std::endl;
        if (pageHits + pageFaults > 0) {
            std::cout << "Hit Rate: " << std::fixed << std::setprecision(2) << ((static_cast<double>(pageHits) / (pageHits + pageFaults)) * 100) << "%" << std::endl;
        }
    }

private:
    // ������ת��Ϊ����
    std::vector<int> queueToVector(std::queue<int> q) {
        std::vector<int> result;
        while (!q.empty()) {
            result.push_back(q.front());
            q.pop();
        }
        return result;
    }

    // ��ȡ���δʹ�õ�ҳ��
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

// ȫ�ֱ���
std::vector<PCB> processList;
double pageSize = 4.0; // Ĭ��ҳ���С��KB��
int timeSlice = 2;      // Ĭ��ʱ��Ƭ��ms��

// ��������
void loadProcesses(std::map<std::string, int>& runTimes);
std::map<std::string, int> loadRunSteps();
std::map<std::string, std::map<std::string, double>> loadPrograms();
void fcfsScheduling();
void rrScheduling();
void simulateCPU(std::map<std::string, int>& runTimes);
void pageScheduling(std::map<std::string, std::map<std::string, double>>& programs);

// ����������ȥ���ַ�����β�Ŀհ��ַ�
std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
        return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

// �������в��裬��run.txt�ж�ȡ
std::map<std::string, int> loadRunSteps() {
    std::ifstream file("run.txt");
    if (!file.is_open()) {
        std::cerr << "Error: Unable to open run.txt" << std::endl;
        return {};
    }
    std::map<std::string, int> runTimes;
    std::string line, currentProgram;
    bool isFirstLine = true; // ����Ƿ�Ϊ��һ��

    while (std::getline(file, line)) {
        if (isFirstLine) {
            // ��Ⲣ�Ƴ�BOM
            if (line.size() >= 3 &&
                static_cast<unsigned char>(line[0]) == 0xEF &&
                static_cast<unsigned char>(line[1]) == 0xBB &&
                static_cast<unsigned char>(line[2]) == 0xBF) {
                line = line.substr(3);
            }
            isFirstLine = false;
        }

        // ��ӡ��ȡ���м��䳤���Խ��е���
        std::cout << "Reading line: [" << line << "], Length: " << line.length() << std::endl;

        line = trim(line);
        if (line.empty()) continue; // Skip empty lines

        // Check if it's a ProgramName line
        if (line.find("ProgramName") == 0) {
            // Extract program name
            size_t pos_space = line.find_first_of(" \t", 11); // "ProgramName" is 11 characters
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
            // Ensure there is a current program name
            if (currentProgram.empty()) {
                std::cerr << "Warning: Found run step before program name: " << line << std::endl;
                continue;
            }

            // Parse run step line
            // Example line: "5	Jump	1021"
            std::istringstream iss(line);
            int time;
            std::string operation, param;
            if (!(iss >> time >> operation >> param)) {
                std::cerr << "Warning: Failed to parse run step line: " << line << std::endl;
                continue;
            }

            // Update runTimes with the maximum run time for the current program
            if (runTimes.find(currentProgram) == runTimes.end() || runTimes[currentProgram] < time) {
                runTimes[currentProgram] = time; // Explicitly cast to int
                std::cout << "Updated run time [" << currentProgram << "]: " << time << std::endl; // Debug info
            }
        }
    }

    file.close();

    // Output loaded run times
    std::cout << "Loaded Run Times:" << std::endl;
    for (const auto& [program, time] : runTimes) {
        std::cout << "Program: [" << program << "], Run Time: " << time << "ms" << std::endl; // Debug info
    }

    return runTimes;
}

// ���س�����ϸ��Ϣ����program.txt�ж�ȡ
std::map<std::string, std::map<std::string, double>> loadPrograms() {
    std::ifstream file("program.txt");
    if (!file.is_open()) {
        std::cerr << "Error: Unable to open program.txt" << std::endl;
        return {};
    }

    std::map<std::string, std::map<std::string, double>> programs;
    std::string line, currentProgram;
    bool isFirstLine = true; // ����Ƿ�Ϊ��һ��

    while (std::getline(file, line)) {
        if (isFirstLine) {
            // ��Ⲣ�Ƴ�BOM
            if (line.size() >= 3 &&
                static_cast<unsigned char>(line[0]) == 0xEF &&
                static_cast<unsigned char>(line[1]) == 0xBB &&
                static_cast<unsigned char>(line[2]) == 0xBF) {
                line = line.substr(3);
            }
            isFirstLine = false;
        }

        // ��ӡ��ȡ���м��䳤���Խ��е���
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

// ���ؽ�����Ϣ����process.txt�ж�ȡ
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
    bool isFirstLine = true; // ����Ƿ�Ϊ��һ��

    while (std::getline(file, line)) {
        if (isFirstLine) {
            // ��Ⲣ�Ƴ�BOM
            if (line.size() >= 3 &&
                static_cast<unsigned char>(line[0]) == 0xEF &&
                static_cast<unsigned char>(line[1]) == 0xBB &&
                static_cast<unsigned char>(line[2]) == 0xBF) {
                line = line.substr(3);
            }
            isFirstLine = false;
        }

        // ��ӡ��ȡ���м��䳤���Խ��е���
        std::cout << "Reading line: [" << line << "], Length: " << line.length() << std::endl;

        line = trim(line);
        if (line.empty()) continue; // Skip empty lines

        // ʹ��istringstream��ȡ
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

// �����ȷ�����ȣ�FCFS��
void fcfsScheduling() {
    std::ofstream resultFile("result.txt");
    if (!resultFile.is_open()) {
        std::cerr << "Error: Unable to open result.txt" << std::endl;
        return;
    }
    // ������ʱ������
    std::sort(processList.begin(), processList.end(), [](PCB& a, PCB& b) {
        return a.createTime < b.createTime;
        });
    int currentTime = 0;
    for (auto& process : processList) {
        if (currentTime < process.createTime) {
            currentTime = process.createTime;
        }
        process.startTime = currentTime;
        process.completeTime = currentTime + process.runTime;
        process.turnoverTime = process.completeTime - process.createTime;
        process.weightedTurnoverTime = static_cast<double>(process.turnoverTime) / process.runTime;
        currentTime += process.runTime;

        resultFile << "��������: " << process.pName << ", ��ʼʱ��: " << process.startTime
            << ", ���ʱ��: " << process.completeTime << ", ��תʱ��: " << process.turnoverTime
            << ", ��Ȩ��תʱ��: " << std::fixed << std::setprecision(2) << process.weightedTurnoverTime << std::endl;
    }
    resultFile.close();
}

// ʱ��Ƭ��ת���ȣ�RR��
// Assume PCB class and other necessary components are defined as previously

void rrScheduling() {
    // Open 'result.txt' in truncate mode to clear previous results
    std::ofstream resultFile("result.txt", std::ios::trunc);
    if (!resultFile.is_open()) {
        std::cerr << "Error: Unable to open result.txt" << std::endl;
        return;
    }
    resultFile.close(); // Close immediately; will reopen in append mode later

    // Create a queue to hold pointers to PCB objects waiting for scheduling
    std::queue<PCB*> processQueue;

    // Sort the processList by creation time (ascending)
    std::sort(processList.begin(), processList.end(), [](const PCB& a, const PCB& b) {
        return a.createTime < b.createTime;
        });

    int currentTime = 0;
    size_t index = 0; // Index to track the next process to enqueue
    std::unordered_map<std::string, int> remainingTimeMap; // Map to track remaining time per process

    // Initialize remainingTimeMap with each process's runTime
    for (auto& process : processList) {
        remainingTimeMap[process.pName] = process.runTime;
    }

    // Main scheduling loop
    while (!processQueue.empty() || index < processList.size()) {
        // Advance currentTime to the next process's createTime if queue is empty
        if (processQueue.empty() && index < processList.size() && processList[index].createTime > currentTime) {
            currentTime = processList[index].createTime;
        }

        // Enqueue all processes that have arrived by currentTime
        while (index < processList.size() && processList[index].createTime <= currentTime) {
            PCB* newProcess = &processList[index];
            processQueue.push(newProcess);
            std::cout << "Enqueued Process: " << newProcess->pName << " at time " << currentTime << " ms" << std::endl;
            index++;
        }

        // If the queue is still empty after enqueuing, increment currentTime and continue
        if (processQueue.empty()) {
            currentTime++;
            continue;
        }

        // Dequeue the first process in the queue
        PCB* currentProcess = processQueue.front();
        processQueue.pop();

        // If the process is running for the first time, set its startTime
        if (currentProcess->startTime == -1) {
            currentProcess->startTime = currentTime;
            std::cout << "Process " << currentProcess->pName << " started at " << currentTime << " ms" << std::endl;
        }

        // Determine the execution time for this time slice
        int execTime = std::min(timeSlice, remainingTimeMap[currentProcess->pName]);
        std::cout << "Executing Process: " << currentProcess->pName
            << " | Execution Time: " << execTime << " ms" << std::endl;

        // Update times
        remainingTimeMap[currentProcess->pName] -= execTime;
        currentTime += execTime;

        // Debug: Current queue status after execution
        std::cout << "Current Time: " << currentTime << " ms | Remaining Time for "
            << currentProcess->pName << ": " << remainingTimeMap[currentProcess->pName] << " ms" << std::endl;

        // If the process still has remaining time, re-enqueue it
        if (remainingTimeMap[currentProcess->pName] > 0) {
            processQueue.push(currentProcess);
            std::cout << "Process " << currentProcess->pName << " re-enqueued at " << currentTime << " ms" << std::endl;
        }
        else {
            // Process has completed execution
            currentProcess->completeTime = currentTime;
            currentProcess->turnoverTime = currentProcess->completeTime - currentProcess->createTime;
            currentProcess->weightedTurnoverTime = static_cast<double>(currentProcess->turnoverTime) / currentProcess->originalRunTime;

            // Remove from remainingTimeMap
            remainingTimeMap.erase(currentProcess->pName);

            std::cout << "Process " << currentProcess->pName << " completed at " << currentTime << " ms"
                << " | Turnover Time: " << currentProcess->turnoverTime
                << " ms | Weighted Turnover Time: " << std::fixed << std::setprecision(2)
                << currentProcess->weightedTurnoverTime << std::endl;
        }

        // Optional: Print current queue status
        std::queue<PCB*> tempQueue = processQueue;
        std::cout << "Current Queue: ";
        while (!tempQueue.empty()) {
            PCB* p = tempQueue.front();
            tempQueue.pop();
            std::cout << p->pName << " ";
        }
        std::cout << std::endl;
    }

    // Append the scheduling results to 'result.txt'
    resultFile.open("result.txt", std::ios::app);
    if (!resultFile.is_open()) {
        std::cerr << "Error: Unable to open result.txt for appending" << std::endl;
        return;
    }

    for (const auto& process : processList) {
        resultFile << "��������: " << process.pName
            << ", ��ʼʱ��: " << process.startTime
            << ", ���ʱ��: " << process.completeTime
            << ", ��תʱ��: " << process.turnoverTime
            << ", ��Ȩ��תʱ��: " << std::fixed << std::setprecision(2)
            << process.weightedTurnoverTime << std::endl;
    }

    resultFile.close();
    std::cout << "RR Scheduling Complete. Results saved to result.txt" << std::endl;
}

// ģ��CPUʹ�����
void simulateCPU(std::map<std::string, int>& runTimes) {
    std::ifstream file("run.txt");
    if (!file.is_open()) {
        std::cerr << "Error: Unable to open run.txt" << std::endl;
        return;
    }
    std::cout << "Simulating CPU Usage..." << std::endl;

    std::string line, currentProgram;
    std::map<int, std::string> cpuLog; // ʱ��� -> ��������
    bool isFirstLine = true; // ����Ƿ�Ϊ��һ��

    while (std::getline(file, line)) {
        if (isFirstLine) {
            // ��Ⲣ�Ƴ�BOM
            if (line.size() >= 3 &&
                static_cast<unsigned char>(line[0]) == 0xEF &&
                static_cast<unsigned char>(line[1]) == 0xBB &&
                static_cast<unsigned char>(line[2]) == 0xBF) {
                line = line.substr(3);
            }
            isFirstLine = false;
        }

        // ��ӡ��ȡ���м��䳤���Խ��е���
        std::cout << "Reading line: [" << line << "], Length: " << line.length() << std::endl;

        line = trim(line);
        if (line.empty()) continue; // Skip empty lines

        // Check if it's a ProgramName line
        if (line.find("ProgramName") == 0) {
            size_t pos_space = line.find_first_of(" \t", 11); // "ProgramName" is 11 characters
            if (pos_space != std::string::npos) {
                std::string afterKeyword = line.substr(pos_space + 1);
                afterKeyword = trim(afterKeyword);
                currentProgram = afterKeyword;
                std::cout << "Simulating Program: [" << currentProgram << "]" << std::endl; // Debug info
            }
            else {
                std::cerr << "Warning: Unable to extract program name: " << line << std::endl;
            }
        }
        else {
            // Parse run step line
            // Example line: "5	Jump	1021"
            std::istringstream iss(line);
            int time;
            std::string operation, param;
            if (!(iss >> time >> operation >> param)) {
                std::cerr << "Warning: Failed to parse run step line: " << line << std::endl;
                continue;
            }

            cpuLog[time] = currentProgram + " " + operation + " " + param; // Format log
        }
    }
    file.close();

    // Output operations in chronological order
    for (const auto& [time, logStr] : cpuLog) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Simulate time interval
        std::cout << "Time: " << time << " ms, Operation: " << logStr << std::endl;
    }

    std::cout << "CPU Simulation Complete!" << std::endl;
}

// ��ҳ����
// ��ҳ����
void pageScheduling(std::map<std::string, std::map<std::string, double>>& programs) {
    std::map<std::string, int> pageRequirements;
    // ����ÿ�����������ҳ����
    for (const auto& [program, functions] : programs) {
        double totalSize = 0.0;
        for (const auto& [func, size] : functions) {
            totalSize += size;
        }
        pageRequirements[program] = static_cast<int>(std::ceil(totalSize / pageSize));
        std::cout << "������� " << program << " ����ҳ����: " << pageRequirements[program] << std::endl; // Debug
    }

    // ��ʾÿ�������ҳ������
    std::cout << "����ҳ������:" << std::endl;
    for (const auto& [program, pages] : pageRequirements) {
        std::cout << "����: " << program << " | ����ҳ����: " << pages << std::endl;
    }

    // ��ȡ�û���������ҳ����
    std::cout << "������ÿ�����̵����ҳ����: ";
    int maxPages;
    while (!(std::cin >> maxPages) || maxPages <= 0) {
        std::cout << "������Ч�����ҳ��������Ϊ������������������: ";
        std::cin.clear();
        std::cin.ignore(10000, '\n');
    }
    std::cout << "ÿ�����̵����ҳ��������Ϊ: " << maxPages << std::endl; // Debug

    // ��ȡ�û�ѡ���ҳ���滻�㷨
    std::cout << "��ѡ��ҳ���滻�㷨��" << std::endl;
    std::cout << "1. FIFO" << std::endl;
    std::cout << "2. LRU" << std::endl;
    std::cout << "������ѡ�� (1 �� 2): ";
    int choice;
    while (!(std::cin >> choice) || (choice != 1 && choice != 2)) {
        std::cout << "������Ч��������1��2: ";
        std::cin.clear();
        std::cin.ignore(10000, '\n');
    }
    std::cout << "ѡ���ҳ���滻�㷨Ϊ: " << (choice == 1 ? "FIFO" : "LRU") << std::endl; // Debug

    // ����PageManagerʵ��
    PageManager pageManager(pageSize, maxPages);
    int currentTime = 0;

    // ����ÿ������������ҳ����
    for (const auto& [program, pages] : pageRequirements) {
        std::cout << "���ڴ������: " << program << " | ��Ҫҳ����: " << pages << std::endl;
        for (int page = 0; page < pages; ++page) {
            if (choice == 1) {
                // ʹ��FIFO�㷨
                pageManager.fifoReplace(page);
                std::cout << "Ӧ��FIFO�㷨����ҳ��: " << page << std::endl; // Debug
            }
            else {
                // ʹ��LRU�㷨
                pageManager.lruReplace(page, currentTime);
                std::cout << "Ӧ��LRU�㷨����ҳ��: " << page << " | ��ǰʱ��: " << currentTime << std::endl; // Debug
            }
            currentTime++;
        }
    }

    // ���ҳ���û���־
    std::cout << "\nҳ���û���־:" << std::endl;
    for (const auto& logEntry : pageManager.log) {
        std::cout << logEntry << std::endl;
    }

    // �����ҳ�����ܽᱨ��
    std::cout << "\n��ҳ�����ܽᱨ��:" << std::endl;
    std::cout << "ҳ�����д���: " << pageManager.pageHits << std::endl;
    std::cout << "ҳ���û����� (ҳ�����): " << pageManager.pageFaults << std::endl;
    if (pageManager.pageHits + pageManager.pageFaults > 0) {
        double hitRate = (static_cast<double>(pageManager.pageHits) / (pageManager.pageHits + pageManager.pageFaults)) * 100.0;
        std::cout << "ҳ��������: " << std::fixed << std::setprecision(2) << hitRate << "%" << std::endl;
    }
    else {
        std::cout << "ҳ��������: 0.00%" << std::endl;
    }
}


// ������
int main() {
    // ���ÿ���̨����ҳΪ65001��UTF-8��
#ifdef _WIN32
    system("chcp 65001");


#endif

    // ������������Ϊ�û�Ĭ��
    setlocale(LC_ALL, "");

    // �ر�ͬ��IO���������
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    std::cout.tie(nullptr);

    // �������в���
    std::map<std::string, int> runTimes = loadRunSteps();

    // ���ؽ�����Ϣ
    loadProcesses(runTimes);

    // ���س�����ϸ��Ϣ
    std::map<std::string, std::map<std::string, double>> programs = loadPrograms();

    while (true) {
        std::cout << "\n��ѡ���ܣ�" << std::endl;
        std::cout << "1. ��ʾ������Ϣ" << std::endl;
        std::cout << "2. ��ʾ������ϸ��Ϣ" << std::endl;
        std::cout << "3. ��ʾ�������в���" << std::endl;
        std::cout << "4. �����ȷ�����ȣ�FCFS��" << std::endl;
        std::cout << "5. ʱ��Ƭ��ת���ȣ�RR��" << std::endl;
        std::cout << "6. ��ҳ����" << std::endl;
        std::cout << "7. ģ��CPUռ�����" << std::endl;
        std::cout << "8. �˳�����" << std::endl;
        int choice;
        std::cin >> choice;

        switch (choice) {
        case 1:
            if (processList.empty()) {
                std::cout << "δ�����κν�����Ϣ��" << std::endl;
            }
            else {
                for (const auto& process : processList) {
                    std::cout << "����: " << process.pName << ", ����ʱ��: " << process.createTime
                        << ", ����ʱ��: " << process.runTime << ", ���ȼ�: " << process.grade
                        << ", ����ע: " << process.pRemark << std::endl;
                }
            }
            break;
        case 2:
            if (programs.empty()) {
                std::cout << "δ�����κγ�����ϸ��Ϣ��" << std::endl;
            }
            else {
                for (const auto& [program, functions] : programs) {
                    std::cout << "����: " << program << std::endl;
                    for (const auto& [func, size] : functions) {
                        std::cout << "  ����: " << func << ", ��С: " << size << " KB" << std::endl;
                    }
                }
            }
            break;
        case 3:
            if (runTimes.empty()) {
                std::cout << "δ�����κγ������в��衣" << std::endl;
            }
            else {
                for (const auto& [program, time] : runTimes) {
                    std::cout << "����: " << program << ", ����ʱ��: " << time << "ms" << std::endl;
                }
            }
            break;
        case 4:
            fcfsScheduling();
            std::cout << "�����ȷ�����ȣ�FCFS����ɡ�����ѱ��浽 result.txt" << std::endl;
            break;
        case 5:
            rrScheduling();
            std::cout << "ʱ��Ƭ��ת���ȣ�RR����ɡ�����ѱ��浽 result.txt" << std::endl;
            break;
        case 6: {
            // ��ҳ���ȹ���
            std::cout << "��ѡ�������" << std::endl;
            std::cout << "1. ����ҳ���С��ʱ��Ƭ����" << std::endl;
            std::cout << "2. ִ�з�ҳ����" << std::endl;
            std::cout << "������ѡ�� (1 �� 2): ";
            int option;
            while (!(std::cin >> option) || (option != 1 && option != 2)) {
                std::cout << "������Ч��������1��2: ";
                std::cin.clear();
                std::cin.ignore(10000, '\n');
            }

            if (option == 1) {
                // �����µ�ҳ���С
                std::cout << "�������µ�ҳ���С (��λ: KB): ";
                while (!(std::cin >> pageSize) || pageSize <= 0.0) {
                    std::cout << "������Ч��ҳ���С����Ϊ����������������: ";
                    std::cin.clear();
                    std::cin.ignore(10000, '\n');
                }

                // �����µ�ʱ��Ƭ����
                std::cout << "�������µ�ʱ��Ƭ���� (��λ: ms): ";
                while (!(std::cin >> timeSlice) || timeSlice <= 0) {
                    std::cout << "������Ч��ʱ��Ƭ���ȱ���Ϊ������������������: ";
                    std::cin.clear();
                    std::cin.ignore(10000, '\n');
                }

                std::cout << "ҳ���С������Ϊ: " << pageSize << " KB | ʱ��Ƭ����������Ϊ: " << timeSlice << " ms" << std::endl;
            }
            else if (option == 2) {
                // ִ�з�ҳ����
                pageScheduling(programs);
            }
            break;
        }

        case 7:
            simulateCPU(runTimes);
            break;
        case 8:
            std::cout << "�����˳�����..." << std::endl;
            return 0;
        default:
            std::cout << "��Ч��ѡ�������ԡ�" << std::endl;
        }
    }

    return 0;
}

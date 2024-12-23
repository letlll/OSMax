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
        pRemark(remark), pStatus("null"), startTime(-1), completeTime(0),
        turnoverTime(0), weightedTurnoverTime(0.0), originalRunTime(runtime) {}

    void updateStatus(const std::string& newStatus) {
        pStatus = newStatus;
        std::cout << "Process " << pName << " status updated to " << pStatus << std::endl;
    }


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
    bool fifoReplace(int page) {
        // ���ҳ���Ƿ����ڴ���
        std::vector<int> fifoVec = queueToVector(fifoPages);
        if (std::find(fifoVec.begin(), fifoVec.end(), page) != fifoVec.end()) {
            pageHits++;
            log.push_back("FIFO: Page " + std::to_string(page) + " already in memory (hit).");
            return false;  // ҳ������ʱ���� false����ʾû��ȱҳ
        }

        pageFaults++;
        if (fifoPages.size() >= maxPages) {
            if (fifoPages.empty()) {
                std::cerr << "Error: FIFO queue is empty, cannot remove page." << std::endl;
                log.push_back("FIFO: Error - FIFO queue is empty, cannot remove page.");
                return false; // ��������ʱ���� false
            }
            int removed = fifoPages.front();
            fifoPages.pop();
            log.push_back("FIFO: Page " + std::to_string(removed) + " removed.");
        }

        fifoPages.push(page);
        log.push_back("FIFO: Page " + std::to_string(page) + " added.");

        return true;  // ҳ���滻ʱ���� true����ʾ������ȱҳ
    }

    // LRU�滻����
    bool lruReplace(int page, int currentTime) {
        if (lruPages.find(page) != lruPages.end()) {
            pageHits++;
            lruPages[page] = currentTime;  // �����������ʱ��
            log.push_back("LRU: Page " + std::to_string(page) + " already in memory (hit).");
            return false;  // ҳ������ʱ���� false����ʾû��ȱҳ
        }

        pageFaults++;
        if (lruPages.size() >= maxPages) {
            int lruPage = getLRUPage();
            if (lruPage == -1) {
                std::cerr << "Error: No LRU page found to remove." << std::endl;
                log.push_back("LRU: Error - No LRU page found to remove.");
                return false; // ��������ʱ���� false
            }
            lruPages.erase(lruPage);  // ɾ�����δʹ�õ�ҳ��
            log.push_back("LRU: Page " + std::to_string(lruPage) + " removed.");
        }

        lruPages[page] = currentTime;  // �����ҳ�棬�����·���ʱ��
        log.push_back("LRU: Page " + std::to_string(page) + " added.");

        return true;  // ҳ���滻ʱ���� true����ʾ������ȱҳ
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
void printPhysicalBlockState(PageManager& pageManager);

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

        // ����Ƿ�Ϊ�������У�"ProgramName" �� "����"��
        if (line.find("ProgramName") == 0 || line.find("����") == 0) {
            // ��ȡ������
            size_t pos_space = line.find_first_of(" \t"); // ���ҵ�һ���ո���Ʊ��
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
            // ȷ�����ڵ�ǰ������
            if (currentProgram.empty()) {
                std::cerr << "Warning: Found run step before program name: " << line << std::endl;
                continue;
            }

            // �������в����У���ʽ��"ʱ�� ���� ����" �� "ʱ�� ����"��
            std::istringstream iss(line);
            int time;
            std::string operation, param;

            // ����޷�������ʱ��Ͳ���������
            if (!(iss >> time >> operation)) {
                std::cerr << "Warning: Failed to parse run step line: " << line << std::endl;
                continue;
            }

            // ����Ƿ��е����в���
            if (!(iss >> param)) {
                param = ""; // ���û�в���������Ϊ���ַ���
            }

            // �������Ϊ "����" �� "End"����ʱ����Ϊ��������ʱ��
            if (operation == "����" || operation == "End") {
                runTimes[currentProgram] = std::max(runTimes[currentProgram], time);
                std::cout << "Set final run time for [" << currentProgram << "] to " << time << " ms" << std::endl;
                continue;
            }

            // ���µ�ǰ���������ʱ�䣬ȡʱ������ֵ
            runTimes[currentProgram] = std::max(runTimes[currentProgram], time);
            std::cout << "Updated run time [" << currentProgram << "]: " << time << std::endl; // Debug info
        }
    }

    file.close();

    // ������ص�����ʱ��
    std::cout << "Loaded Run Times:" << std::endl;
    for (const auto& [program, time] : runTimes) {
        std::cout << "Program: [" << program << "], Run Time: " << time << " ms" << std::endl; // Debug info
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

// FCFS �����㷨���Ľ�����̬��¼ÿ��ʱ��Ƭ��״̬��
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

    int currentTime = 0; // ��ǰʱ��

    // �����ͷ
    resultFile << "ʱ��Ƭ\t���еĽ���\t";
    for (const auto& process : processList) {
        resultFile << "����" << process.pName << "״̬\t";
    }
    resultFile << "\n";

    // ���ȹ���
    for (auto& process : processList) {
        if (currentTime < process.createTime) {
            currentTime = process.createTime; // �ȴ����̵���
        }
        process.startTime = currentTime; // ��¼��ʼʱ��
        process.completeTime = currentTime + process.runTime; // ��¼���ʱ��

        // ÿ��ʱ��Ƭ��̬����״̬
        for (int t = 0; t < process.runTime; ++t) {
            resultFile << currentTime + t << "\t" << process.pName << "\t"; // ��ǰʱ��Ƭ�����еĽ���

            // �������н��̣�����״̬
            for (auto& p : processList) {
                if (p.pName == process.pName) {
                    resultFile << "run\t"; // ��ǰ���еĽ���
                }
                else if (p.completeTime <= currentTime + t) {
                    resultFile << "complete\t"; // ����ɵĽ���
                }
                else if (p.createTime > currentTime + t) {
                    resultFile << "null\t"; // ��δ����Ľ���
                }
                else {
                    resultFile << "ready\t"; // �ѵ��ﵫδ���еĽ���
                }
            }
            resultFile << "\n";
        }

        // ����ʱ�䵽�������ʱ
        currentTime += process.runTime;

        // ���㲢��¼ͳ����Ϣ
        process.turnoverTime = process.completeTime - process.createTime; // ��תʱ��
        process.weightedTurnoverTime = static_cast<double>(process.turnoverTime) / process.runTime; // ��Ȩ��תʱ��
    }

    // ���ÿ�����̵�����ͳ����Ϣ
    resultFile << "\n����ͳ����Ϣ��\n";
    for (const auto& process : processList) {
        resultFile << "��������: " << process.pName
            << ", ��ʼʱ��: " << process.startTime
            << ", ���ʱ��: " << process.completeTime
            << ", ��תʱ��: " << process.turnoverTime
            << ", ��Ȩ��תʱ��: " << std::fixed << std::setprecision(2)
            << process.weightedTurnoverTime << "\n";
    }

    resultFile.close();
    std::cout << "FCFS ������ɣ�����ѱ��浽 result.txt" << std::endl;
}





// ʱ��Ƭ��ת���ȣ�RR��
void rrScheduling() {
    std::ofstream resultFile("result.txt", std::ios::app);
    if (!resultFile.is_open()) {
        std::cerr << "�����޷��� result.txt ���н������" << std::endl;
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

    // �����ͷ
    resultFile << "ʱ��Ƭ\t���еĽ���\t";
    for (const auto& process : processList) {
        resultFile << "����" << process.pName << "״̬\t";
    }
    resultFile << "\n";

    // Main scheduling loop
    while (!processQueue.empty() || index < processList.size()) {
        // �������Ϊ�գ�������һ�����̵Ĵ���ʱ��
        if (processQueue.empty() && index < processList.size() && processList[index].createTime > currentTime) {
            currentTime = processList[index].createTime;
        }

        // ����ǰʱ��Ľ��̼������
        while (index < processList.size() && processList[index].createTime <= currentTime) {
            PCB* newProcess = &processList[index];
            processQueue.push(newProcess);
            resultFile << " ����: " << newProcess->pName << " �� " << currentTime << " ms ʱ���������" << std::endl;
            index++;
        }

        // ���������Ϊ�գ���ǰʱ���1����
        if (processQueue.empty()) {
            currentTime++;
            continue;
        }

        // �Ӷ�����ȡ��һ������
        PCB* currentProcess = processQueue.front();
        processQueue.pop();

        // ����ǵ�һ�����У����ÿ�ʼʱ��
        if (currentProcess->startTime == -1) {
            currentProcess->startTime = currentTime;
        }

        // ִ��ʱ��Ƭ
        int execTime = std::min(1, remainingTimeMap[currentProcess->pName]);
        remainingTimeMap[currentProcess->pName] -= execTime;
        currentTime += execTime;

        // �����ǰʱ��Ƭ��״̬
        resultFile << currentTime - execTime << "\t" << currentProcess->pName << "\t";

        for (const auto& process : processList) {
            if (process.pName == currentProcess->pName && remainingTimeMap[process.pName] > 0) {
                resultFile << "run\t"; // ��ǰ���еĽ���
            }
            else if (remainingTimeMap[process.pName] == 0) {
                resultFile << "complete\t"; // ����ɵĽ���
            }
            else if (process.createTime > currentTime - execTime) {
                resultFile << "null\t"; // ��δ����Ľ���
            }
            else {
                resultFile << "ready\t"; // �ѵ��ﵫδ���еĽ���
            }
        }
        resultFile << "\n";

        // �������δ��ɣ����¼������
        if (remainingTimeMap[currentProcess->pName] > 0) {
            processQueue.push(currentProcess);
        }
        else {
            // ��ɵĽ���
            currentProcess->completeTime = currentTime;
            currentProcess->turnoverTime = currentProcess->completeTime - currentProcess->createTime;
            currentProcess->weightedTurnoverTime =
                static_cast<double>(currentProcess->turnoverTime) / currentProcess->originalRunTime;

            resultFile << "���� " << currentProcess->pName << " �� " << currentTime << " ms ��� "
                << "| ��תʱ�䣺 " << currentProcess->turnoverTime
                << " ms | ��Ȩ��תʱ�䣺 " << std::fixed << std::setprecision(2)
                << currentProcess->weightedTurnoverTime << std::endl;
        }
    }

    // �������ͳ����Ϣ
    resultFile << "\n����ͳ����Ϣ��\n";
    for (const auto& process : processList) {
        resultFile << "��������: " << process.pName
            << ", ��ʼʱ��: " << process.startTime
            << ", ���ʱ��: " << process.completeTime
            << ", ��תʱ��: " << process.turnoverTime
            << ", ��Ȩ��תʱ��: " << std::fixed << std::setprecision(2)
            << process.weightedTurnoverTime << "\n";
    }

    resultFile.close();
    std::cout << "RR ������ɣ�����ѱ��浽 result.txt" << std::endl;
}



void simulateCPU() {
    std::ifstream file("run.txt");
    if (!file.is_open()) {
        std::cerr << "Error: Unable to open run.txt" << std::endl;
        return;
    }
    std::cout << "ģ��CPUʹ�����..." << std::endl;

    std::string line, currentProgram;
    std::map<int, std::vector<std::string>> cpuLog;  // ʱ��� -> �����б�
    std::map<std::string, std::string> processStatus; // ������ -> ��ǰ״̬
    std::map<std::string, int> processStartTimes;     // ������ -> ����ʱ��
    bool isFirstLine = true; // ����Ƿ�Ϊ��һ��

    // ��ȡ�����ļ����ݲ�����
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

        line = trim(line);
        if (line.empty()) continue; // ��������

        // ����Ƿ��� ProgramName ��
        if (line.find("ProgramName") == 0) {
            size_t pos_space = line.find_first_of(" \t", 11); // "ProgramName"��11���ַ�
            if (pos_space != std::string::npos) {
                currentProgram = trim(line.substr(pos_space + 1));
                std::cout << "Simulating Program: [" << currentProgram << "]" << std::endl;

                // ��ʼ������״̬Ϊ "Ready"
                processStatus[currentProgram] = "Ready";
                processStartTimes[currentProgram] = 0; // ���迪ʼʱ����0
            }
            else {
                std::cerr << "Warning: Unable to extract program name: " << line << std::endl;
            }
        }
        else {
            // �������в�����
            std::istringstream iss(line);
            int time;
            std::string operation, param;
            if (iss >> time >> operation) {
                std::string event = currentProgram + " " + operation;
                if (operation == "End") {
                    cpuLog[time].push_back(event);  // ��¼"End"����
                    processStatus[currentProgram] = "Terminated";  // ���½���״̬
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

    // ��ȡ���ʱ���
    int maxTime = cpuLog.rbegin()->first;

    // ÿ1�����������ʾ״̬
    for (int t = 0; t <= maxTime; ++t) {
        // �����ǰʱ�̵Ľ���״̬
        std::cout << "ʱ��: " << t << " ms, ��ǰ����״̬:" << std::endl;

        // �����ǰʱ���Ĳ���
        if (cpuLog.find(t) != cpuLog.end()) {
            for (const auto& operation : cpuLog[t]) {
                std::cout << "    ����: " << operation << std::endl;
            }
        }

        // ���½���״̬
        for (auto& [process, status] : processStatus) {
            // ��������������У���ʱ��Ƭ��������״̬����Ϊ ready
            if (status == "Running") {
                status = "Ready"; // ���н���������Ϊ����
            }
            // �����������ֹ���򲻸���
            else if (status == "Terminated") {
                continue;
            }

            // ��ӡÿ�����̵ĵ�ǰ״̬
            std::cout << "    ����: " << process << ", ״̬: " << status << std::endl;
        }

        // ģ��ÿ1����ĵ���
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    std::cout << "CPU�������!" << std::endl;
}




// ģ���ҳ����
void pageScheduling(std::map<std::string, std::map<std::string, double>>& programs) {
    std::map<std::string, int> pageRequirements;

    // ����ÿ�������ҳ�����󣬲�����ʵ�ʽ��̴�С��̬�ı�ҳ����
    for (const auto& [program, functions] : programs) {
        double totalSize = 0.0;
        for (const auto& [func, size] : functions) {
            totalSize += size; // �ۻ���������ڴ�����
        }

        // �������ڴ�������������ҳ����
        pageRequirements[program] = static_cast<int>(std::ceil(totalSize / pageSize));
        std::cout << "���� " << program << " ����ҳ����: " << pageRequirements[program] << std::endl;
    }

    // ��ȡ�û���������ҳ����
    std::cout << "\n������ÿ�����̵����ҳ����: ";
    int maxPages;
    while (!(std::cin >> maxPages) || maxPages <= 0) {
        std::cout << "������Ч�����ҳ��������Ϊ������������������: ";
        std::cin.clear();
        std::cin.ignore(10000, '\n');
    }
    std::cout << "ÿ�����̵����ҳ��������Ϊ: " << maxPages << std::endl;

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
    std::cout << "ѡ���ҳ���滻�㷨Ϊ: " << (choice == 1 ? "FIFO" : "LRU") << std::endl;

    // ����PageManagerʵ��
    PageManager pageManager(pageSize, maxPages);
    int currentTime = 0;

    // ��������״̬��ȱҳ��־
    std::cout << "\n--------------------ҳ�������־--------------------\n";
    std::cout << "����ҳ�� | �����״̬        | ȱҳ\n";
    std::cout << "--------------------------------------------\n";

    for (const auto& [program, pages] : pageRequirements) {
        std::cout << "���ڴ������: " << program << " | ��Ҫҳ����: " << pages << std::endl;
        for (int page = 0; page < pages; ++page) {
            bool pageFault = false;

            std::cout << std::setw(9) << page << " | ";

            // ʹ��ѡ���ҳ���滻�㷨
            if (choice == 1) {
                pageFault = pageManager.fifoReplace(page);
            }
            else {
                pageFault = pageManager.lruReplace(page, currentTime);
            }

            // ��ӡ�����״̬
            printPhysicalBlockState(pageManager);

            // ��ӡȱҳ״̬
            if (pageFault) {
                std::cout << "��" << std::endl;
            }
            else {
                std::cout << "��" << std::endl;
            }

            currentTime++;
        }
    }

    // ���ҳ���û��ܽᱨ��
    std::cout << "\n--------------------��ҳ�����ܽᱨ��--------------------\n";
    pageManager.printSummary();
}

// ��ӡ������״̬
void printPhysicalBlockState(PageManager& pageManager) {
    std::queue<int> fifoPages = pageManager.fifoPages;
    std::unordered_map<int, int> lruPages = pageManager.lruPages;

    // ��ӡFIFO�����״̬
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

    // ��ӡLRU�����״̬
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
        std::cout << "3. �����ȷ�����ȣ�FCFS��" << std::endl;
        std::cout << "4. ʱ��Ƭ��ת���ȣ�RR��" << std::endl;
        std::cout << "5. ��ҳ����" << std::endl;
        std::cout << "6. �˳�����" << std::endl;
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
            fcfsScheduling();
            std::cout << "�����ȷ�����ȣ�FCFS����ɡ�����ѱ��浽 result.txt" << std::endl;
            break;
        case 4:
            rrScheduling();
            std::cout << "ʱ��Ƭ��ת���ȣ�RR����ɡ�����ѱ��浽 result.txt" << std::endl;
            break;
        case 5: {
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


        case 6:
            std::cout << "�����˳�����..." << std::endl;
            return 0;
        default:
            std::cout << "��Ч��ѡ�������ԡ�" << std::endl;
        }
    }

    return 0;
}

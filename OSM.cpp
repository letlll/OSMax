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

// PCB�ࣺ���̿��ƿ�
class PCB {
public:
    string pName;               // ��������
    string pRemark;             // ����ע
    string pStatus;             // ����״̬
    int createTime;             // ����ʱ��
    int runTime;                // ����ʱ��
    int grade;                  // ���ȼ�
    int startTime;              // ��ʼʱ��
    int completeTime;           // ���ʱ��
    int turnoverTime;           // ��תʱ��
    double weightedTurnoverTime;// ��Ȩ��תʱ��
    int originalRunTime;        // ԭʼ����ʱ��

    PCB(string name, int create, int runtime, int priority, string remark)
        : pName(name), createTime(create), runTime(runtime), grade(priority),
        pRemark(remark), pStatus("null"), startTime(-1), completeTime(0),
        turnoverTime(0), weightedTurnoverTime(0.0), originalRunTime(runtime) {}

    void updateStatus(const string& newStatus) {
        pStatus = newStatus;
        cout << "Process " << pName << " status updated to " << pStatus << endl;
    }
};

// �ṹ�嶨�壬���ڴ洢ÿ���������Ϣ
struct Program {
    string name;                // ��������
    vector<int> jump_addresses; // ��ת��ַ����
    vector<int> page_sequence;  // ҳ���������
    int maxPages;               // ÿ�����̵����ҳ����
};

// �ṹ�嶨�壬���ڴ洢���в���
struct RunSteps {
    map<string, int> runTimes;                // ���������ʱ��
    map<string, vector<int>> pageSequences;    // �����ҳ���������
};

// ȫ�ֽ����б�
vector<PCB> processList;

// ����������ȥ���ַ�����β�Ŀհ��ַ�
string trim(const string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == string::npos)
        return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

    // FIFOPageManager �ࣺ���� FIFO ҳ���滻�㷨
    class FIFOPageManager {
    public:
        double pageSize;                         // ҳ���С��KB��
        int maxPages;                            // ����ҳ����
        queue<int> fifoPages;                    // FIFO ����
        vector<string> log;                      // ҳ�������־
        int pageFaults = 0;                      // ȱҳ����
        int pageHits = 0;                        // ���д���

        FIFOPageManager(double size, int max) : pageSize(size), maxPages(max) {}

        // FIFO �滻����
        bool fifoReplace(int page) {
            // ���ҳ���Ƿ����ڴ���
            vector<int> fifoVec = queueToVector(fifoPages);
            if (find(fifoVec.begin(), fifoVec.end(), page) != fifoVec.end()) {
                pageHits++;
                log.push_back("FIFO: ҳ�� " + to_string(page) + " �����ڴ��� (����)��");
                return false; // ҳ�����У�����ȱҳ
            }

            pageFaults++;
            if (fifoPages.size() >= maxPages) {
                if (fifoPages.empty()) {
                    cerr << "����FIFO ����Ϊ�գ��޷��Ƴ�ҳ�档" << endl;
                    log.push_back("FIFO: ���� - FIFO ����Ϊ�գ��޷��Ƴ�ҳ�档");
                    return false; // ��������ʱ���� false
                }
                int removed = fifoPages.front();
                fifoPages.pop();
                log.push_back("FIFO: ҳ�� " + to_string(removed) + " ���Ƴ���");
            }

            fifoPages.push(page);
            log.push_back("FIFO: ҳ�� " + to_string(page) + " ����ӡ�");

            return true; // ����ȱҳ
        }

        // ��ӡȱҳ������ժҪ
        void printSummary() const {
            cout << "FIFO �㷨�ܽ�:" << endl;
            cout << "ȱҳ����: " << pageFaults << endl;
            cout << "���д���: " << pageHits << endl;
            if (pageHits + pageFaults > 0) {
                cout << "������: " << fixed << setprecision(2)
                    << ((static_cast<double>(pageHits) / (pageHits + pageFaults)) * 100) << "%" << endl;
            }
        }

        // ��ӡ�����״̬
        void printPhysicalBlockState() const {
            cout << "��ǰ�����״̬ (FIFO ����): [ ";
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

        // ��ӡ������־
        void printLog() const {
            cout << "FIFO ҳ�������־:" << endl;
            for (const auto& entry : log) {
                cout << entry << endl;
            }
        }

    private:
        // ������ת��Ϊ����
        vector<int> queueToVector(queue<int> q) const {
            vector<int> result;
            while (!q.empty()) {
                result.push_back(q.front());
                q.pop();
            }
            return result;
        }
    };

    // LRUPageManager �ࣺ���� LRU ҳ���滻�㷨
    class LRUPageManager {
    public:
        double pageSize;                         // ҳ���С��KB��
        int maxPages;                            // ����ҳ����
        unordered_map<int, int> lruPages;       // LRUӳ�䣺ҳ��� -> �������ʱ��
        vector<string> log;                      // ҳ�������־
        int pageFaults = 0;                      // ȱҳ����
        int pageHits = 0;                        // ���д���

        LRUPageManager(double size, int max) : pageSize(size), maxPages(max) {}

        // LRU �滻����
        bool lruReplace(int page, int currentTime) {
            if (lruPages.find(page) != lruPages.end()) {
                pageHits++;
                lruPages[page] = currentTime; // �����������ʱ��
                log.push_back("LRU: ҳ�� " + to_string(page) + " �����ڴ��� (����)��");
                return false; // ҳ�����У�����ȱҳ
            }

            pageFaults++;
            if (lruPages.size() >= maxPages) {
                int lruPage = getLRUPage();
                if (lruPage == -1) {
                    cerr << "����δ�ҵ����δʹ�õ�ҳ�����Ƴ���" << endl;
                    log.push_back("LRU: ���� - δ�ҵ����δʹ�õ�ҳ�����Ƴ���");
                    return false; // ��������ʱ���� false
                }
                lruPages.erase(lruPage); // �Ƴ����δʹ�õ�ҳ��
                log.push_back("LRU: ҳ�� " + to_string(lruPage) + " ���Ƴ���");
            }

            lruPages[page] = currentTime; // �����ҳ�棬����¼����ʱ��
            log.push_back("LRU: ҳ�� " + to_string(page) + " ����ӡ�");

            return true; // ����ȱҳ
        }

        // ��ӡȱҳ������ժҪ
        void printSummary() const {
            cout << "LRU �㷨�ܽ�:" << endl;
            cout << "ȱҳ����: " << pageFaults << endl;
            cout << "���д���: " << pageHits << endl;
            if (pageHits + pageFaults > 0) {
                cout << "������: " << fixed << setprecision(2)
                    << ((static_cast<double>(pageHits) / (pageHits + pageFaults)) * 100) << "%" << endl;
            }
        }

        // ��ӡ�����״̬
        void printPhysicalBlockState() const {
            cout << "��ǰ�����״̬ (LRU ҳ��): [ ";
            // �� LRU ҳ���ҳ��Ű��������ʱ�����򣨴����δʹ�õ����ʹ�ã�
            vector<pair<int, int>> pages(lruPages.begin(), lruPages.end());
            sort(pages.begin(), pages.end(), [](const pair<int, int>& a, const pair<int, int>& b) {
                return a.second < b.second; // �����δʹ�õ����ʹ��
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

        // ��ӡ������־
        void printLog() const {
            cout << "LRU ҳ�������־:" << endl;
            for (const auto& entry : log) {
                cout << entry << endl;
            }
        }

    private:
        // ��ȡ���δʹ�õ�ҳ��
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

// ��������
RunSteps loadRunSteps();
map<string, map<string, double>> loadPrograms();

void loadProcesses(const map<string, int>& runTimes);
void fcfsScheduling();
void rrScheduling();
void simulatePageReplacement(const vector<Program>& programs, int pageSize, int timeSlice);

// ���� run.txt �ļ������ɳ����б�ҳ��������У�
vector<Program> parseRunFile(const string& filename, int pageSize) {
    ifstream infile(filename);
    if (!infile.is_open()) {
        cerr << "�����޷����ļ� " << filename << endl;
        exit(1);
    }

    vector<Program> programs;
    string line;
    Program currentProgram;

    while (getline(infile, line)) {
        line = trim(line);
        if (line.empty()) continue; // ��������

        // ����Ƿ�Ϊ����������
        if (line.find("ProgramName") == 0 || line.find("����") == 0) {
            // �����ǰ�����ڴ���ĳ����ȱ���
            if (!currentProgram.name.empty()) {
                // ����ת��ַת��Ϊҳ���
                for (int addr : currentProgram.jump_addresses) {
                    currentProgram.page_sequence.push_back(addr / pageSize);
                }
                programs.push_back(currentProgram);
                currentProgram = Program(); // ����Ϊ��һ������
            }

            // ��ȡ��������
            stringstream ss(line);
            string tag, prog_name;
            ss >> tag >> prog_name;
            currentProgram.name = prog_name;
        }
        else {
            // �����¼���
            stringstream ss(line);
            int timestamp;
            string event;
            int value;
            ss >> timestamp >> event;

            if (event == "Jump" || event == "��ת") {
                ss >> value;
                currentProgram.jump_addresses.push_back(value);
            }
            // ���� Start �� End �¼�
        }
    }

    // �������һ������
    if (!currentProgram.name.empty()) {
        for (int addr : currentProgram.jump_addresses) {
            currentProgram.page_sequence.push_back(addr / pageSize);
        }
        programs.push_back(currentProgram);
    }

    infile.close();
    return programs;
}

// �������в��裬�� run.txt �ж�ȡ
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
        if (line.empty()) continue; // ��������

        // ����Ƿ�Ϊ���������У�"ProgramName" �� "����"��
        if (line.find("ProgramName") == 0 || line.find("����") == 0) {
            // ��ȡ��������
            size_t pos_space = line.find_first_of(" \t"); // ���ҵ�һ���ո���Ʊ��
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
            // ȷ�����ڵ�ǰ������
            if (currentProgram.empty()) {
                cerr << "Warning: Found run step before program name: " << line << endl;
                continue;
            }

            // �������в����У���ʽ��"ʱ�� ���� ����" �� "ʱ�� ����"��
            istringstream iss(line);
            int time;
            string operation, param;

            // ����޷�������ʱ��Ͳ���������
            if (!(iss >> time >> operation)) {
                cerr << "Warning: Failed to parse run step line: " << line << endl;
                continue;
            }

            // ����Ƿ��е����в���
            if (!(iss >> param)) {
                param = ""; // ���û�в���������Ϊ���ַ���
            }

            // �������Ϊ "����" �� "End"����ʱ����Ϊ��������ʱ��
            if (operation == "����" || operation == "End") {
                runSteps.runTimes[currentProgram] = max(runSteps.runTimes[currentProgram], time);
                cout << "Set final run time for [" << currentProgram << "] to " << time << " ms" << endl;
                continue;
            }

            // �������Ϊ "Jump" �� "��ת"����������Ϊ��ת��ַ
            if (operation == "Jump" || operation == "��ת") {
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
                // �����������Ը�����Ҫ��չ
                cout << "Info: Unhandled operation [" << operation << "] in line: " << line << endl;
            }

            // ���µ�ǰ���������ʱ�䣬ȡʱ������ֵ
            runSteps.runTimes[currentProgram] = max(runSteps.runTimes[currentProgram], time);
            cout << "Updated run time [" << currentProgram << "]: " << time << " ms" << endl; // Debug info
        }
    }

    file.close();

    // ������ص�����ʱ��
    cout << "\nLoaded Run Times:" << endl;
    for (const auto& [program, time] : runSteps.runTimes) {
        cout << "Program: [" << program << "], Run Time: " << time << " ms" << endl; // Debug info
    }

    // ������ص�ҳ���������
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

// ���س�����ϸ��Ϣ���� program.txt �ж�ȡ
map<string, map<string, double>> loadPrograms() {
    ifstream file("program.txt");
    if (!file.is_open()) {
        cerr << "Error: Unable to open program.txt" << endl;
        return {};
    }

    map<string, map<string, double>> programs;
    string line, currentProgram;
    bool isFirstLine = true; // ����Ƿ�Ϊ��һ��

    while (getline(file, line)) {
        if (isFirstLine) {
            // ��Ⲣ�Ƴ� BOM
            if (line.size() >= 3 &&
                static_cast<unsigned char>(line[0]) == 0xEF &&
                static_cast<unsigned char>(line[1]) == 0xBB &&
                static_cast<unsigned char>(line[2]) == 0xBF) {
                line = line.substr(3);
            }
            isFirstLine = false;
        }

        // ��ӡ��ȡ���м��䳤���Խ��е���
        cout << "Reading line: [" << line << "], Length: " << line.length() << endl;

        line = trim(line);
        if (line.empty()) continue; // ��������

        // Check if it's a FileName line
        if (line.find("FileName") == 0) {
            // Extract program name
            size_t pos_space = line.find_first_of(" \t", 8); // "FileName" ��8���ַ�
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
            // ȷ�����ڵ�ǰ������
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
// ���ؽ�����Ϣ���� process.txt �ж�ȡ
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
    bool isFirstLine = true; // ����Ƿ�Ϊ��һ��

    while (getline(file, line)) {
        if (isFirstLine) {
            // ��Ⲣ�Ƴ� BOM
            if (line.size() >= 3 &&
                static_cast<unsigned char>(line[0]) == 0xEF &&
                static_cast<unsigned char>(line[1]) == 0xBB &&
                static_cast<unsigned char>(line[2]) == 0xBF) {
                line = line.substr(3);
            }
            isFirstLine = false;
        }

        // ��ӡ��ȡ���м��䳤���Խ��е���
        cout << "Reading line: [" << line << "], Length: " << line.length() << endl;

        line = trim(line);
        if (line.empty()) continue; // ��������

        // ʹ�� istringstream ��ȡ
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



// FCFS �����㷨���Ľ�����̬��¼ÿ��ʱ��Ƭ��״̬��
void fcfsScheduling() {
    ofstream resultFile("result.txt");
    if (!resultFile.is_open()) {
        cerr << "Error: Unable to open result.txt" << endl;
        return;
    }

    // ������ʱ������
    sort(processList.begin(), processList.end(), [](PCB& a, PCB& b) {
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
            << ", ��Ȩ��תʱ��: " << fixed << setprecision(2)
            << process.weightedTurnoverTime << "\n";
    }

    resultFile.close();
    cout << "FCFS ������ɣ�����ѱ��浽 result.txt" << endl;
}

// ʱ��Ƭ��ת���ȣ�RR��
void rrScheduling() {
    ofstream resultFile("result.txt", ios::app); // append to result.txt
    if (!resultFile.is_open()) {
        cerr << "�����޷��� result.txt ���н������" << endl;
        return;
    }

    queue<PCB*> processQueue;
    // ������ʱ������
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
            cout << "����: " << newProcess->pName << " �� " << currentTime << " ms ʱ���������" << endl;
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
        int execTime = 1; // ÿ��ִ��1��ʱ��Ƭ��λ
        if (remainingTimeMap[currentProcess->pName] < execTime) {
            execTime = remainingTimeMap[currentProcess->pName];
        }
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
                << " ms | ��Ȩ��תʱ�䣺 " << fixed << setprecision(2)
                << currentProcess->weightedTurnoverTime << endl;
        }
    }

    // �������ͳ����Ϣ
    resultFile << "\n����ͳ����Ϣ��\n";
    for (const auto& process : processList) {
        resultFile << "��������: " << process.pName
            << ", ��ʼʱ��: " << process.startTime
            << ", ���ʱ��: " << process.completeTime
            << ", ��תʱ��: " << process.turnoverTime
            << ", ��Ȩ��תʱ��: " << fixed << setprecision(2)
            << process.weightedTurnoverTime << "\n";
    }

    resultFile.close();
    cout << "RR ������ɣ�����ѱ��浽 result.txt" << endl;
}

// ģ��ҳ���û��㷨
void simulatePageReplacement(const vector<Program>& programs, int maxPages, int pageSize, int timeSlice) {
    // ��ȡ�û�ѡ���ҳ���滻�㷨
    cout << "��ѡ��ҳ���滻�㷨��" << endl;
    cout << "1. FIFO" << endl;
    cout << "2. LRU" << endl;
    cout << "������ѡ�� (1 �� 2): ";
    int choice;
    while (!(cin >> choice) || (choice != 1 && choice != 2)) {
        cout << "������Ч��������1��2: ";
        cin.clear();
        cin.ignore(10000, '\n');
    }
    string algorithm = (choice == 1) ? "FIFO" : "LRU";
    cout << "ѡ���ҳ���滻�㷨Ϊ: " << algorithm << endl;

    // ���� PageManager ʵ��
    FIFOPageManager* fifoManager = nullptr;
    LRUPageManager* lruManager = nullptr;

    if (choice == 1) {
        fifoManager = new FIFOPageManager(pageSize, maxPages);
    }
    else {
        lruManager = new LRUPageManager(pageSize, maxPages);
    }

    // ��������״̬��ȱҳ��־
    cout << "\n--------------------ҳ�������־--------------------\n";
    cout << "���� | ����ҳ�� | ȱҳ | �����״̬" << endl;
    cout << "--------------------------------------------\n";

    // ģ��ʱ��Ƭ����
    // ����һ�� vector ������ÿ������ĵ�ǰ��������
    vector<size_t> programIndices(programs.size(), 0);
    bool allFinished = false;
    int globalTime = 0; // ���� LRU

    while (!allFinished) {
        allFinished = true;
        for (size_t i = 0; i < programs.size(); ++i) {
            // ��鵱ǰ�����Ƿ���ҳ����Ҫ����
            if (programIndices[i] < programs[i].page_sequence.size()) {
                allFinished = false;
                // ����һ��ʱ��Ƭ��ҳ�����
                int pagesToProcess = min(timeSlice, static_cast<int>(programs[i].page_sequence.size() - programIndices[i]));
                for (int p = 0; p < pagesToProcess; ++p) {
                    int page = programs[i].page_sequence[programIndices[i]];
                    bool pageFault = false;

                    // ʹ��ѡ���ҳ���滻�㷨
                    if (choice == 1) { // FIFO
                        pageFault = fifoManager->fifoReplace(page);
                    }
                    else { // LRU
                        globalTime++;
                        pageFault = lruManager->lruReplace(page, globalTime);
                    }

                    // ��ӡ����ҳ�桢ȱҳ���
                    cout << programs[i].name << " | ҳ�� " << page << " | ";
                    if (pageFault) {
                        cout << "�� | ";
                    }
                    else {
                        cout << "�� | ";
                    }

                    // ��ӡ�����״̬
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

    // ���ҳ���û��ܽᱨ��
    cout << "\n--------------------��ҳ�����ܽᱨ��--------------------\n";
    if (choice == 1 && fifoManager != nullptr) {
        fifoManager->printSummary();
        fifoManager->printLog();
    }
    else if (choice == 2 && lruManager != nullptr) {
        lruManager->printSummary();
        lruManager->printLog();
    }
    cout << "------------------------------------------------------------\n";

    // �ͷŶ�̬������ڴ�
    if (fifoManager != nullptr) {
        delete fifoManager;
    }
    if (lruManager != nullptr) {
        delete lruManager;
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
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    cout.tie(nullptr);

    // �������в���
    RunSteps runSteps = loadRunSteps();

    // ���ؽ�����Ϣ
    loadProcesses(runSteps.runTimes);

    // ���س�����ϸ��Ϣ
    map<string, map<string, double>> programs = loadPrograms();

    while (true) {
        cout << "\n��ѡ���ܣ�" << endl;
        cout << "1. ��ʾ������Ϣ" << endl;
        cout << "2. ��ʾ������ϸ��Ϣ" << endl;
        cout << "3. �����ȷ�����ȣ�FCFS��" << endl;
        cout << "4. ʱ��Ƭ��ת���ȣ�RR��" << endl;
        cout << "5. ��ҳ����" << endl;
        cout << "6. �˳�����" << endl;
        int choice;
        cin >> choice;

        switch (choice) {
        case 1:
            if (processList.empty()) {
                cout << "δ�����κν�����Ϣ��" << endl;
            }
            else {
                for (const auto& process : processList) {
                    cout << "����: " << process.pName << ", ����ʱ��: " << process.createTime
                        << ", ����ʱ��: " << process.runTime << ", ���ȼ�: " << process.grade
                        << ", ����ע: " << process.pRemark << endl;
                }
            }
            break;
        case 2:
            if (programs.empty()) {
                cout << "δ�����κγ�����ϸ��Ϣ��" << endl;
            }
            else {
                for (const auto& [program, functions] : programs) {
                    cout << "����: " << program << endl;
                    for (const auto& [func, size] : functions) {
                        cout << "  ����: " << func << ", ��С: " << size << " KB" << endl;
                    }
                }
            }
            break;
        case 3:
            fcfsScheduling();
            cout << "�����ȷ�����ȣ�FCFS����ɡ�����ѱ��浽 result.txt" << endl;
            break;
        case 4:
            rrScheduling();
            cout << "ʱ��Ƭ��ת���ȣ�RR����ɡ�����ѱ��浽 result.txt" << endl;
            break;
        case 5: {
            // �û�����ҳ���С��ÿ�����̵����ҳ������ʱ��Ƭ����
            cout << "������ҳ���С��KB��: ";
            int pageSize;
            while (!(cin >> pageSize) || pageSize <= 0) {
                cout << "������Ч��ҳ���С����Ϊ������������������: ";
                cin.clear();
                cin.ignore(10000, '\n');
            }
            cout << "ҳ���С����Ϊ: " << pageSize << " KB" << endl;

            cout << "������ÿ�����̵����ҳ����: ";
            int maxPages;
            while (!(cin >> maxPages) || maxPages <= 0) {
                cout << "������Ч�����ҳ��������Ϊ������������������: ";
                cin.clear();
                cin.ignore(10000, '\n');
            }
            cout << "ÿ�����̵����ҳ��������Ϊ: " << maxPages << endl;

            cout << "������ʱ��Ƭ���ȣ���ÿ��������һ����ת�п��Է��ʵ�ҳ������: ";
            int timeSlice;
            while (!(cin >> timeSlice) || timeSlice <= 0) {
                cout << "������Ч��ʱ��Ƭ���ȱ���Ϊ������������������: ";
                cin.clear();
                cin.ignore(10000, '\n');
            }
            cout << "ʱ��Ƭ��������Ϊ: " << timeSlice << endl;

            // ���� run.txt �ļ�
            string runFileName = "run.txt"; // ȷ�� run.txt ��ͬһĿ¼��
            vector<Program> programs = parseRunFile(runFileName, pageSize);

            if (programs.empty()) {
                cout << "�� " << runFileName << " ��δ�ҵ��κγ���" << endl;
                return 0;
            }

            // ģ��ҳ���û�
            simulatePageReplacement(programs, maxPages, pageSize, timeSlice);

            cout << "\n*************************** ģ����� ***************************" << endl;
            break;
        }

        case 6:
            cout << "�����˳�����..." << endl;
            return 0;
        default:
            cout << "��Ч��ѡ�������ԡ�" << endl;
        }
    }

    return 0;
}
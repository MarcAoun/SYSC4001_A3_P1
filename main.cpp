#include "interrupts_Marc_Adam.hpp"
#include <iostream>
#include <vector>
#include <fstream>
#include <string>
using namespace std;

enum State { NEW, READY, RUNNING, WAITING, TERMINATED };

struct Partition {
    int number;
    int size;
    string code;
};

struct PCB {
    int pid;
    int memSize;
    int arrivalTime;
    int totalCpuTime;
    int remainingCpuTime;
    int ioFrequency;
    int ioDuration;
    int partitionNumber;
    State state;
    int priority;
};

vector<Partition> partitions;
vector<PCB> pcbTable;

void initPartitions() {
    partitions = {
        {1,40,"free"},
        {2,25,"free"},
        {3,15,"free"},
        {4,10,"free"},
        {5,8,"free"},
        {6,2,"free"}
    };
}

int findPartition(int memSize) {
    for (auto &p : partitions)
        if (p.code=="free" && p.size >= memSize)
            return p.number;
    return -1;
}

void loadInput(string filename) {
    ifstream file(filename);
    int pid,mem,arr,cpu,freq,dur;
    while (file >> pid >> mem >> arr >> cpu >> freq >> dur) {
        PCB p;
        p.pid = pid;
        p.memSize = mem;
        p.arrivalTime = arr;
        p.totalCpuTime = cpu;
        p.remainingCpuTime = cpu;
        p.ioFrequency = freq;
        p.ioDuration = dur;
        p.partitionNumber = -1;
        p.state = NEW;
        p.priority = 0;
        pcbTable.push_back(p);
    }
}

void admitProcesses() {
    for (auto &p : pcbTable) {
        int part = findPartition(p.memSize);
        if (part != -1) {
            p.partitionNumber = part;
            p.state = READY;
            partitions[part - 1].code = to_string(p.pid);
        }
    }
}

int main() {
    initPartitions();
    loadInput("input_files/test1.txt");
    admitProcesses();
    for (auto &p : pcbTable) {
        cout << p.pid << " " << p.partitionNumber << " " << p.state << endl;
    }
    return 0;
}

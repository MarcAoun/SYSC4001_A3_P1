#include "interrupts_student1_student2.hpp"
#include <numeric>     // for future use, if needed
#include <sstream>

// Simple helper: read a line and split by whitespace into tokens
std::vector<std::string> split_whitespace(const std::string &line) {
    std::vector<std::string> tokens;
    std::stringstream ss(line);
    std::string token;
    while (ss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

// -------------- Metrics support --------------

struct Metrics {
    double throughput;
    double avgWait;
    double avgTurnaround;
    double avgResponse;
};

// compute average gap between consecutive I/O operations for ONE process
double computeAvgResponseTimeForProcess(const PCB &p) {
    const auto &v = p.ioStartTimes;
    if (v.size() < 2) {
        // no or only one I/O => response time not really defined, treat as 0
        return 0.0;
    }

    double sumGaps = 0.0;
    for (size_t i = 1; i < v.size(); ++i) {
        sumGaps += (v[i] - v[i - 1]);   // time between two I/O operations
    }
    return sumGaps / static_cast<double>(v.size() - 1);
}

// procs = all processes that finished
Metrics computeMetrics(const std::vector<PCB> &procs, int simEndTime) {
    Metrics m{};

    if (procs.empty() || simEndTime <= 0) return m;

    const int N = static_cast<int>(procs.size());

    double totalWait  = 0.0;
    double totalTurn  = 0.0;
    double totalResp  = 0.0;

    for (const PCB &p : procs) {
        // change field names here if your PCB uses different ones
        int arrival  = p.arrival_time;
        int finish   = p.finishTime;
        int waitTime = p.totalWaitTime;

        double turnaround = static_cast<double>(finish - arrival);
        double resp       = computeAvgResponseTimeForProcess(p);

        totalWait += waitTime;
        totalTurn += turnaround;
        totalResp += resp;
    }

    // throughput = completed / total time
    m.throughput    = static_cast<double>(N) / static_cast<double>(simEndTime);
    m.avgWait       = totalWait / static_cast<double>(N);
    m.avgTurnaround = totalTurn / static_cast<double>(N);
    m.avgResponse   = totalResp / static_cast<double>(N);

    return m;
}

// nice CSV-style print so you can copy into Excel
void printMetricsCSV(const std::string &schedulerName,
                     const std::string &scenarioName,
                     const Metrics &m)
{
    std::cout << schedulerName << ","
              << scenarioName  << ","
              << m.throughput  << ","
              << m.avgWait     << ","
              << m.avgTurnaround << ","
              << m.avgResponse << std::endl;
}

// -------------- main simulation --------------

int main(int argc, char *argv[]) {
    // ------------- choose input file -----------------
    std::string input_path = "input_files/input.txt";   // default
    if (argc > 1) {
        input_path = argv[1];   // allow: ./bin/main input_files/whatever.txt
    }

    std::ifstream file(input_path);
    if (!file.is_open()) {
        std::cerr << "Error: could not open input file: " << input_path << std::endl;
        return 1;
    }

    // ------------- load processes into PCB table -------------
    std::vector<PCB> job_queue;
    std::string line;

    while (std::getline(file, line)) {
        if (line.empty()) {
            continue;
        }

        // tokens: PID mem arrival totalCPU ioFreq ioDur
        std::vector<std::string> tokens = split_whitespace(line);
        if (tokens.size() < 6) {
            std::cerr << "Warning: skipping malformed line: " << line << std::endl;
            continue;
        }

        PCB p = add_process(tokens);

        // make sure arrivalTime is set for metrics
        // if add_process already sets it, you can remove this
        p.arrivalTime = /* whatever your arrival value is, from tokens or p */ p.arrivalTime;
        p.finishTime = 0;
        p.totalWaitTime = 0;
        // p.ioStartTimes starts empty

        job_queue.push_back(p);
    }

    file.close();

    if (job_queue.empty()) {
        std::cerr << "No valid processes were loaded from " << input_path << std::endl;
        return 1;
    }

    std::cout << "Loaded " << job_queue.size() << " processes from "
              << input_path << std::endl;

    // ------------- assign memory to each process -------------
    for (auto &p : job_queue) {
        bool ok = assign_memory(p);
        if (!ok) {
            std::cout << "PID " << p.PID << " could not be assigned a partition (not enough memory)"
                      << std::endl;
        } else {
            std::cout << "PID " << p.PID << " assigned to partition "
                      << p.partition_number << std::endl;
        }
    }

    // show current PCB table
    std::cout << std::endl << "Current PCB table:" << std::endl;
    std::cout << print_PCB(job_queue) << std::endl;

    // ------------- minimal execution log (your existing example) -------------
    std::string exec_log;
    exec_log += print_exec_header();

    unsigned int current_time = 0;
    PCB &first = job_queue.front();

    states old_state = NEW;
    states new_state = READY;

    first.state = new_state;
    sync_queue(job_queue, first);

    exec_log += print_exec_status(current_time, first.PID, old_state, new_state);
    exec_log += print_exec_footer();

    // write to output_files/execution.txt
    write_output(exec_log, "output_files/execution.txt");

    std::cout << "Simulation finished. Execution log written to "
              << "output_files/execution.txt" << std::endl;

    // ------------- metrics at end of simulation -------------
    // for now, this example just uses current_time = 0 and treats all as finished
    // when you have a real scheduler loop, set:
    //   current_time = final clock value
    //   set p.finishTime when each process actually completes

    int simEndTime = static_cast<int>(current_time);   // change when you have a real clock
    for (auto &p : job_queue) {
        if (p.finishTime == 0) {
            p.finishTime = simEndTime;  // temporary, so code compiles and metrics are defined
        }
    }

    std::vector<PCB> finishedProcs = job_queue;

    std::string schedulerName = "FCFS";          // change depending on which scheduler
    std::string scenarioName  = input_path;      // input file as scenario name

    Metrics m = computeMetrics(finishedProcs, simEndTime);
    printMetricsCSV(schedulerName, scenarioName, m);

    return 0;
}

/**
 * External Priority + Round Robin (100 ms)
 * Uses PID as a simple priority: smaller PID = higher priority.
 */

#include "interrupts_student1_student2.hpp"

// helper: pick highest-priority (smallest PID) from ready_queue
static int index_of_highest_priority(const std::vector<PCB> &ready_queue) {
    if (ready_queue.empty()) return -1;
    int best = 0;
    for (int i = 1; i < (int)ready_queue.size(); ++i) {
        if (ready_queue[i].PID < ready_queue[best].PID)
            best = i;
    }
    return best;
}

std::tuple<std::string /* add std::string for bonus mark if needed */>
run_simulation(std::vector<PCB> list_processes) {

    std::vector<PCB> ready_queue;
    std::vector<PCB> job_list = list_processes;

    unsigned int current_time = 0;
    PCB running;
    idle_CPU(running);

    unsigned int quantum_used = 0;

    std::string execution_status;
    execution_status = print_exec_header();

    while (!all_process_terminated(job_list)) {

        // ---- 1) add newly-arrived processes to READY ----
        for (auto &process : job_list) {
            if (process.state == NOT_ASSIGNED &&
                process.arrival_time <= current_time) {

                if (assign_memory(process)) {
                    states old_state = NEW;
                    process.state = READY;
                    ready_queue.push_back(process);
                    sync_queue(job_list, process);

                    execution_status +=
                        print_exec_status(current_time,
                                          process.PID,
                                          old_state,
                                          READY);
                }
            }
        }

        // ---- 2) possible preemption due to higher priority arrival ----
        if (running.state == RUNNING && !ready_queue.empty()) {
            int hi = index_of_highest_priority(ready_queue);
            if (hi >= 0 && ready_queue[hi].PID < running.PID) {
                // preempt running
                execution_status +=
                    print_exec_status(current_time,
                                      running.PID,
                                      RUNNING,
                                      READY);

                running.state = READY;
                sync_queue(job_list, running);
                ready_queue.push_back(running);
                idle_CPU(running);
            }
        }

        // ---- 3) if CPU idle, choose highest-priority READY, RR fashion ----
        if (running.state != RUNNING) {
            int hi = index_of_highest_priority(ready_queue);
            if (hi >= 0) {
                running = ready_queue[hi];
                ready_queue.erase(ready_queue.begin() + hi);

                running.state = RUNNING;
                if (running.start_time == -1)
                    running.start_time = current_time;

                sync_queue(job_list, running);
                quantum_used = 0;

                execution_status +=
                    print_exec_status(current_time,
                                      running.PID,
                                      READY,
                                      RUNNING);
            }
        }

        // ---- 4) advance CPU 1 ms ----
        if (running.state == RUNNING) {
            if (running.remaining_time > 0)
                running.remaining_time--;

            quantum_used++;
            sync_queue(job_list, running);

            current_time++;

            // finished?
            if (running.remaining_time == 0) {
                execution_status +=
                    print_exec_status(current_time,
                                      running.PID,
                                      RUNNING,
                                      TERMINATED);

                terminate_process(running, job_list);
                idle_CPU(running);
            }
            // time slice over?
            else if (quantum_used >= 100) {
                execution_status +=
                    print_exec_status(current_time,
                                      running.PID,
                                      RUNNING,
                                      READY);

                running.state = READY;
                sync_queue(job_list, running);
                ready_queue.push_back(running);
                idle_CPU(running);
            }
        } else {
            current_time++;
        }
    }

    execution_status += print_exec_footer();
    return std::make_tuple(execution_status);
}

// ------------------ main ------------------
int main(int argc, char **argv) {

    if (argc != 2) {
        std::cout << "ERROR!\nExpected 1 argument, received " << argc - 1 << std::endl;
        std::cout << "To run the program, do: ./EP_RR.exe <your_input_file.txt>" << std::endl;
        return -1;
    }

    auto file_name = argv[1];
    std::ifstream input_file(file_name);

    if (!input_file.is_open()) {
        std::cerr << "Error: Unable to open file: " << file_name << std::endl;
        return -1;
    }

    std::string line;
    std::vector<PCB> list_process;
    while (std::getline(input_file, line)) {
        auto input_tokens = split_delim(line, ", ");
        auto new_process  = add_process(input_tokens);
        list_process.push_back(new_process);
    }
    input_file.close();

    auto [exec] = run_simulation(list_process);

    write_output(exec, "output_files/EP_RR_execution.txt.txt");

    return 0;
}

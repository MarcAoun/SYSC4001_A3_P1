/**
 * @file interrupts_student1_student2_EP.cpp
 * External Priority (no preemption) scheduler for SYSC4001 A3 P1
 */

#include "interrupts_student1_student2.hpp"

// ------------------ helper: FCFS order (by arrival time) ------------------
void FCFS(std::vector<PCB> &ready_queue) {
    std::sort(
        ready_queue.begin(),
        ready_queue.end(),
        [](const PCB &first, const PCB &second) {
            return first.arrival_time > second.arrival_time;
        }
    );
}

// ------------------ main simulation ------------------
std::tuple<std::string /* add std::string for bonus mark if needed */>
run_simulation(std::vector<PCB> list_processes) {

    std::vector<PCB> ready_queue;   // ready queue
    std::vector<PCB> job_list;      // master list we track states in

    // copy initial processes into job_list
    job_list = list_processes;

    unsigned int current_time = 0;
    PCB running;
    idle_CPU(running);              // sets running to NOT_ASSIGNED

    std::string execution_status;
    execution_status = print_exec_header();

    // run until every process in job_list is TERMINATED
    while (!all_process_terminated(job_list)) {

        // ---- 1) move newly-arrived jobs into READY ----
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

        // ---- 2) choose a process to run (External Priority / FCFS) ----
        if (running.state != RUNNING) {
            if (!ready_queue.empty()) {
                // FCFS on arrival time (earliest first)
                FCFS(ready_queue);

                // run_process sets running = back of ready_queue,
                // sets state RUNNING, and syncs job_list
                run_process(running, job_list, ready_queue, current_time);

                execution_status +=
                    print_exec_status(current_time,
                                      running.PID,
                                      READY,
                                      RUNNING);
            }
        }

        // ---- 3) advance CPU one ms ----
        if (running.state == RUNNING) {
            // execute 1 ms of CPU time
            if (running.remaining_time > 0) {
                running.remaining_time--;
            }

            // update master list
            sync_queue(job_list, running);

            current_time++;

            // if finished, terminate
            if (running.remaining_time == 0) {
                execution_status +=
                    print_exec_status(current_time,
                                      running.PID,
                                      RUNNING,
                                      TERMINATED);

                terminate_process(running, job_list); // sets TERMINATED + frees mem
                idle_CPU(running);                    // CPU becomes idle
            }
        } else {
            // CPU idle this tick
            current_time++;
        }
    }

    execution_status += print_exec_footer();
    return std::make_tuple(execution_status);
}

// ------------------ main: provided style ------------------
int main(int argc, char **argv) {

    //Get the input file from the user
    if (argc != 2) {
        std::cout << "ERROR!\nExpected 1 argument, received " << argc - 1 << std::endl;
        std::cout << "To run the program, do: ./EP.exe <your_input_file.txt>" << std::endl;
        return -1;
    }

    //Open the input file
    auto file_name = argv[1];
    std::ifstream input_file;
    input_file.open(file_name);

    //Ensure that the file actually opens
    if (!input_file.is_open()) {
        std::cerr << "Error: Unable to open file: " << file_name << std::endl;
        return -1;
    }

    //Parse the entire input file and populate a vector of PCBs.
    //To do so, the add_process() helper function is used (see include file).
    std::string line;
    std::vector<PCB> list_process;
    while (std::getline(input_file, line)) {
        auto input_tokens = split_delim(line, ", ");
        auto new_process = add_process(input_tokens);
        list_process.push_back(new_process);
    }
    input_file.close();

    //With the list of processes, run the simulation
    auto [exec] = run_simulation(list_process);

    // write into the output_files directory
    write_output(exec, "output_files/EP_execution.txt");

    return 0;
}

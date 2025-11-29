/**
 * @file interrupts.cpp
 * @author Sasisekhar Govind
 * @brief template main.cpp file for Assignment 3 Part 1 of SYSC4001
 * 
 */

#include<interrupts_RileyFoxton_EstebanHeidrich.hpp>

void FCFS(std::vector<PCB> &ready_queue) {
    std::sort( 
                ready_queue.begin(),
                ready_queue.end(),
                []( const PCB &first, const PCB &second ){
                    return (first.arrival_time > second.arrival_time); 
                } 
            );
}

std::tuple<std::string /* add std::string for bonus mark */ > run_simulation(std::vector<PCB> list_processes) {

    std::vector<PCB> ready_queue;   //The ready queue of processes
    std::vector<PCB> wait_queue;    //The wait queue of processes
    std::vector<PCB> job_list;      //A list to keep track of all the processes. This is similar
                                    //to the "Process, Arrival time, Burst time" table that you
                                    //see in questions. You don't need to use it, I put it here
                                    //to make the code easier :).

    unsigned int current_time = 0;
    PCB running;

    std::vector<int> time_last_IO;
    std::vector<int> time_in_IO;
    unsigned int running_since_IO = 0;
    unsigned int start_slice = current_time;
    int sliceLeft = SLICE;
    bool switchProcess = false;

    //Initialize an empty running process
    idle_CPU(running);

    std::string execution_status;

    //make the output table (the header row)
    execution_status = print_exec_header();

    //Loop while till there are no ready or waiting processes.
    //This is the main reason I have job_list, you don't have to use it.
    while(!all_process_terminated(job_list) || job_list.empty()) {

        //Inside this loop, there are three things you must do:
        // 1) Populate the ready queue with processes as they arrive
        // 2) Manage the wait queue
        // 3) Schedule processes from the ready queue

        //Population of ready queue is given to you as an example.
        //Go through the list of proceeses
        for(auto &process : list_processes) {
            if(process.arrival_time == current_time) {//check if the AT = current time
                //if so, assign memory and put the process into the ready queue
                assign_memory(process);

                process.state = READY;  //Set the process state to READY
                ready_queue.push_back(process); //Add the process to the ready queue
                /////
                //Process has never had an IO
                time_last_IO.push_back(current_time);
                /////
                job_list.push_back(process); //Add it to the list of processes

                execution_status += print_exec_status(current_time, process.PID, NEW, READY);

            }
        }

        ///////////////////////MANAGE WAIT QUEUE/////////////////////////
        //This mainly involves keeping track of how long a process must remain in the ready queue
        for(int i = 0; i<wait_queue.size();i++){
            if(wait_queue.at(i).io_duration <= time_in_IO.at(i) && wait_queue.at(i).io_duration >= 0){
                ready_queue.push_back(wait_queue.at(i));
                time_last_IO.push_back(current_time);

                execution_status += print_exec_status(current_time, wait_queue.at(i).PID, WAITING, READY);
                wait_queue.at(i).state = READY;

                wait_queue.erase(wait_queue.begin()+i);
                time_in_IO.erase(time_in_IO.begin()+i);
            }
            std::cout<<"In waiting\n";
        }
        /////////////////////////////////////////////////////////////////

        //////////////////////////SCHEDULER//////////////////////////////
        if(switchProcess || (ready_queue.size()>0 && running.PID<0)){
            if(ready_queue.size()>0){
                running = ready_queue.front();
                running_since_IO = 0;
                sliceLeft = SLICE;
                if(running.start_time<0){
                    running.start_time = current_time;
                }
                execution_status += print_exec_status(current_time, running.PID, READY, RUNNING);

                ready_queue.erase(ready_queue.begin());
                time_last_IO.erase(time_last_IO.begin());
                std::cout<<std::to_string(current_time)+"\n";
            }
        }
        switchProcess = false;
        current_time ++;
        for(int i = 0; i<time_in_IO.size();i++){
            time_in_IO.at(i)++;
        }
        if(running.partition_number>-1){
            running.remaining_time--;
            sliceLeft--;
            running_since_IO++;
        }
        if(running.remaining_time<=0 && running.partition_number > -1){
            execution_status += print_exec_status(current_time, running.PID, RUNNING, TERMINATED);
            running.state = TERMINATED;

            terminate_process(running, job_list);

            switchProcess = true;
            std::cout<<"terminated\n";
        }
        else if(sliceLeft<=0 && running.partition_number > -1){
            execution_status += print_exec_status(current_time, running.PID, RUNNING, READY);
            running.state = READY;

            ready_queue.push_back(running);
            time_last_IO.push_back(running_since_IO);

            switchProcess = true;
            std::cout<<"slice up\n";
        }
        else if(running_since_IO>=running.io_freq && running.io_freq > 0 && running.io_duration > 0){
            execution_status += print_exec_status(current_time, running.PID, RUNNING, WAITING);
            running.state = WAITING;

            wait_queue.push_back(running);
            time_in_IO.push_back(0);

            idle_CPU(running);
            
            switchProcess = true;
            std::cout<<"IO burst\n";
        }
        /////////////////////////////////////////////////////////////////
    }
    
    //Close the output table
    execution_status += print_exec_footer();

    return std::make_tuple(execution_status);
}


int main(int argc, char** argv) {

    //Get the input file from the user
    if(argc != 2) {
        std::cout << "ERROR!\nExpected 1 argument, received " << argc - 1 << std::endl;
        std::cout << "To run the program, do: ./interrutps <your_input_file.txt>" << std::endl;
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
    while(std::getline(input_file, line)) {
        auto input_tokens = split_delim(line, ", ");
        auto new_process = add_process(input_tokens);
        list_process.push_back(new_process);
    }
    input_file.close();

    //With the list of processes, run the simulation
    auto [exec] = run_simulation(list_process);

    write_output(exec, "execution.txt");

    return 0;
}
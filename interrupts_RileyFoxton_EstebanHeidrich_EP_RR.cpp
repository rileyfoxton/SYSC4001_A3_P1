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
    int track = 0;
    bool inserted = false;

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
                process.time_in_ready=0;
                //This block related to the proper insertion of the newly arrived process
                //if the ready queue is empty then add it to the front
                if(ready_queue.size()==0){
                    ready_queue.push_back(process); 
                    time_last_IO.push_back(0);
                }
                else{
                    track = 0;
                    inserted = false;
                    //if the ready queue is not empty then insert it in the first place where the priority (definined by the PID)
                    //is greater than the PID before it
                    while(track<ready_queue.size()){
                        if(process.PID>ready_queue.at(track).PID){
                            ready_queue.insert(ready_queue.begin()+track, process);
                            time_last_IO.insert(time_last_IO.begin()+track, 0);
                            track = ready_queue.size()+1;
                            inserted = true;
                        }
                        track++;
                    }
                    //if nothing was inserted then it is the highest priority and should go at the end
                    if(!inserted){
                        ready_queue.insert(ready_queue.end(), process);
                        time_last_IO.insert(time_last_IO.end(), 0);
                    }
                }
                job_list.push_back(process); //Add it to the list of processes

                execution_status += print_exec_status(current_time, process.PID, NEW, READY);
                //cout to help calculate metrics
                std::cout<<"ID: "+ std::to_string(process.PID)+ ". Arrived: "+ std::to_string(current_time)<< std::endl;

            }
        }

        ///////////////////////MANAGE WAIT QUEUE/////////////////////////
        //This mainly involves keeping track of how long a process must remain in the ready 
        for(int i = 0; i<wait_queue.size();i++){
            //if the process at i has been in the wait queue equal to or longer than the io duration and the io duration is a positive number
            if(wait_queue.at(i).io_duration <= time_in_IO.at(i) && wait_queue.at(i).io_duration >= 0){
                //if the ready queue is empty insert it at the front
                if(ready_queue.size()==0){
                    ready_queue.push_back(wait_queue.at(i)); //Add the process to the ready queue
                    time_last_IO.push_back(0);
                }
                else{
                    track = 0;
                    inserted = false;

                    //if the ready queue is not empty then insert it in the first place where the priority (definined by the PID)
                    while(track<ready_queue.size()){
                        //is greater than the PID before it
                        if(wait_queue.at(i).PID>ready_queue.at(track).PID){
                            ready_queue.insert(ready_queue.begin()+track, wait_queue.at(i));
                            time_last_IO.insert(time_last_IO.begin()+track, 0);
                            track = ready_queue.size()+1;
                            inserted = true;
                        }
                        track++;
                    }
                    //if nothing was inserted then it is the highest priority and should go at the end
                    if(!inserted){
                        ready_queue.insert(ready_queue.end(), wait_queue.at(i));
                        time_last_IO.insert(time_last_IO.end(), 0);
                    }
                }
                //clear the process from the wait queue and adjust state
                execution_status += print_exec_status(current_time, wait_queue.at(i).PID, WAITING, READY);
                wait_queue.at(i).state = READY;

                wait_queue.erase(wait_queue.begin()+i);
                time_in_IO.erase(time_in_IO.begin()+i);
            }
        }
        /////////////////////////////////////////////////////////////////

        //////////////////////////SCHEDULER//////////////////////////////
        
        if(ready_queue.back().PID<running.PID){
            track = 0;
            inserted = false;
            while(track<ready_queue.size()){
                //insert where the priority is less than the process in front of it
                if(running.PID>ready_queue.at(track).PID){
                    ready_queue.insert(ready_queue.begin()+track, running);
                    time_last_IO.insert(time_last_IO.begin()+track, running_since_IO);
                    track = ready_queue.size()+1;
                    inserted = true;
                }
                track++;
            }
            //if it has not been inserted yet put it at the front
            if(!inserted){
                ready_queue.insert(ready_queue.end(), running);
                time_last_IO.insert(time_last_IO.end(), running_since_IO);
            }
            switchProcess = true;
        }
        //if the flag for changing is active or there is no process running but the ready queue has an item
        if((switchProcess && ready_queue.size()>0) || (ready_queue.size()>0 && running.PID<0)){
            
            //change the running process and reset the slice time
            running = ready_queue.back();
            running_since_IO = time_last_IO.back();
            sliceLeft = SLICE;
            
            //if the process hasnt been run before set its start time
            if(running.start_time<0){
                running.start_time = current_time;
                //cout to help calculate metrics
                std::cout<<"ID: "+ std::to_string(running.PID)+ ". Started: "+ std::to_string(current_time)<< std::endl;
            }
           
            //update table and remove item from the ready queue
            execution_status += print_exec_status(current_time, running.PID, READY, RUNNING);
            ready_queue.erase(ready_queue.end());
            time_last_IO.erase(time_last_IO.end());
    
        }
        //disable the change process flag and increment the time
        switchProcess = false;
        current_time ++;
        //increment the wait time for the processes waiting for I/O
        for(int i = 0; i<time_in_IO.size();i++){
            time_in_IO.at(i)++;
        }
        //increment the time processes have been in the ready queue
        for(int i = 0; i<ready_queue.size();i++){
            ready_queue.at(i).time_in_ready+=1;
        }
        //if there is a process runnning decrement the slice time and the time remaining
        //increase the counter to the next I/O
        if(running.partition_number>-1){
            running.remaining_time--;
            sliceLeft--;
            running_since_IO++;
        }
        //if the process is done executing
        if(running.remaining_time<=0 && running.partition_number > -1){
            //update the table and state
            execution_status += print_exec_status(current_time, running.PID, RUNNING, TERMINATED);
            running.state = TERMINATED;

            //cout to help calculate metrics
            std::cout<<"ID: "+ std::to_string(running.PID)+ ". Ended: "+ std::to_string(current_time)<< std::endl;
            std::cout<<"ID: "+ std::to_string(running.PID)+ ". Time Ready: "+ std::to_string(running.time_in_ready)<< std::endl;

            terminate_process(running, job_list);
            idle_CPU(running);
            switchProcess = true;
        }
        //if the process is being kicked out due to the slice time being up
        else if(sliceLeft<=0 && running.partition_number > -1){
            //update the table and state
            execution_status += print_exec_status(current_time, running.PID, RUNNING, READY);
            running.state = READY;

            //find the proper place to insert the process in the ready queue
            track = 0;
            inserted = false;
            while(track<ready_queue.size()){
                //insert where the priority is less than the process in front of it
                if(running.PID>ready_queue.at(track).PID){
                    ready_queue.insert(ready_queue.begin()+track, running);
                    time_last_IO.insert(time_last_IO.begin()+track, running_since_IO);
                    track = ready_queue.size()+1;
                    inserted = true;
                }
                track++;
            }
            //if it has not been inserted yet put it at the front
            if(!inserted){
                ready_queue.insert(ready_queue.end(), running);
                time_last_IO.insert(time_last_IO.end(), running_since_IO);
            }

            switchProcess = true;
        }
        //update the running process at this point
        if((switchProcess && ready_queue.size()>0) || (ready_queue.size()>0 && running.PID<0)){
            //update running to the next highest priority process
            running = ready_queue.back();
            running_since_IO = time_last_IO.back();
            sliceLeft = SLICE;
            //if this process has not run before set its start time
            if(running.start_time<0){
                running.start_time = current_time;
                //cout to help calculate metrics
                std::cout<<"ID: "+ std::to_string(running.PID)+ ". Started: "+ std::to_string(current_time)<< std::endl;
            }
            execution_status += print_exec_status(current_time, running.PID, READY, RUNNING);
            //remove process from the ready queue
            ready_queue.erase(ready_queue.end());
            time_last_IO.erase(time_last_IO.end());        
            switchProcess = false;
        }
        //check if the running process need to be moved to the wait queue
        if(running_since_IO>=running.io_freq && running.io_freq > 0 && running.io_duration > 0){
            //update the table
            execution_status += print_exec_status(current_time, running.PID, RUNNING, WAITING);
            running.state = WAITING;
            //insert in the wait queue
            wait_queue.push_back(running);
            time_in_IO.push_back(0);

            idle_CPU(running);
            
            switchProcess = true;
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
#include <chrono>
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <mutex>
#include <queue>
#include <random>

using namespace std;
using namespace std::chrono;
using namespace std::this_thread;

mutex mtx;

bool round_completion = false;      // Used if n rounds are completed.
bool *round_start;                  // Used for every round.
long *outgoing_msg;                 // This array stores outgoing messages.
long *outgoing_int_msg;             // This array is used to intermediate processing of values.
long *incoming_msg;                 // This array stores incoming messages.
queue<long> *q;                     // This array of queues stores messages from neighbors.
int go_ahead_signal = 0;
int num_msgs        = 0;
int num_threads;
int round_no;




bool start_next_round() { 
    return num_threads == go_ahead_signal;
}

void floodmax(long threadid, int thread_num) {
    long tid    = threadid;
    long max_id = tid;
    int t;

    std::random_device rd;                                  //Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd());                                 //Standard mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<> distrib(1, 12);

    while (!round_completion) {
        while (!round_start[thread_num]) {
            sleep_for(milliseconds(1));
        }

        round_start[thread_num] = false;
        
        if (round_no != 0) {
            t = distrib(gen);
            sleep_for(milliseconds(t));

            mtx.lock();
            long incoming = q[thread_num].front();
            mtx.unlock();

            if (incoming > tid) {
                max_id = incoming;
                for (int j = 0; j < num_threads; j++) {
                    if (thread_num != j) {
                        mtx.lock();
                        q[thread_num].push(incoming);
                        num_msgs++;
                        mtx.unlock();
                    }
                }
            }
            else if (incoming < tid) {
                outgoing_int_msg[thread_num] = 0;
            }
            else {}
            mtx.lock();
            q[thread_num].pop();
            mtx.unlock();
            go_ahead_signal++;                          // This notifies that this particular round is complete.
        }
        else {
            for (int j = 0; j < num_threads; j++) {
                if (thread_num != j) {
                    mtx.lock();
                    q[j].push(tid);
                    num_msgs++;
                    mtx.unlock();
                }
            }
            ++go_ahead_signal;                          // This notifies that this particular round is complete.
        }   
    }
    mtx.lock();
    cout << "I am Thread ID " << tid << " and my leader is " << max_id << endl;
    mtx.unlock();
}



int main(int argc, char* argv[]) {
    int rc;
    long i = 0;
    int data;
    
    
    ifstream infile;
    infile.open("input.dat");                            // This file contains the input data.
    infile >> data;                                      // This initial data contains the number of threads.
    num_threads = data;

    int pid_list[num_threads] = {};                      // Contains ID's of the processes.
    int neighbors[num_threads][num_threads];             // Connectivity matrix of edges between nodes.
    int data_collector[num_threads*num_threads] = {};    // Temporary array to collect data from file.


    while (!infile.eof()) {                              // Create a number vector of size n and add numbers to it.
        infile >> data;                                  // This collects all remaining data: process id's and connectivity.
        data_collector[i++] = data;
    }
    infile.close();

    int c = num_threads;
    for (int i = 0; i < num_threads+1; i++) {            // Here we sort out id's from the connectivity data.
        for (int j = 0; j < num_threads; j++) {
            if (i == 0) {
                pid_list[j] = data_collector[j];
            }
            else {
                neighbors[i-1][j] = data_collector[c];
                c++;
            }
        }
    }

    outgoing_msg     = new long[num_threads];
    outgoing_int_msg = new long[num_threads];
    incoming_msg     = new long[num_threads];
    round_start      = new bool[num_threads];
    q                = new queue<long>[num_threads];
    
    for (int j = 0; j < num_threads; j++) {
        round_start[j] = false;
    }
    
    thread threads[num_threads];
    for (int k = 0; k < num_threads; k++) {                         // Run the threads for num_threads number of rounds.
        cout << "Master Thread: Round " << k << endl;
        round_no = k;

        if (!k) {
            for (i = 0; i < num_threads; i++) {                     // Create threads in round 0.
                q[i].push(pid_list[i]);
                threads[i] = thread(floodmax, pid_list[i], i);
            }
        }

        for (int j = 0; j < num_threads; j++) {
            round_start[j] = true;
        }

        while (!start_next_round()) {
            sleep_for(milliseconds(10));                            // Wait until we get a go ahead from all threads.
        }

        go_ahead_signal = 0;                                        // Set to 0 so all threads can notify of completion.
        
        for (int j = 0; j < num_threads; j++) {
            outgoing_msg[j] = outgoing_int_msg[j]; 
        }

        for (int j = 0; j < num_threads; j++) {
            incoming_msg[j] = outgoing_msg[(j+1) % num_threads];    // Assign the outgoing of round n-1 to incoming of round n.
        }
    }

    for (int j = 0; j < num_threads; j++) {
        round_start[j] = true;
    }
    
    round_completion   = true;
    
    for (i = 0; i < num_threads; i++) {
        threads[i].join();
    }

    cout << endl << "Total messages sent = " << num_msgs << endl;

   return 0;
}
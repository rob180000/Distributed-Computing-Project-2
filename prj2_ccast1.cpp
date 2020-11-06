// Ryan Bagby
// Lakshmi Priyanka Selvaraj


#include <chrono>
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <mutex>
#include <queue>
#include <random>
#include <set>

using namespace std;
using namespace std::chrono;
using namespace std::this_thread;

mutex mtx;

queue<long> *q_lcr;
queue<pair<long,long>> *q;           // This array of queues stores messages from neighbors. 
set<int> *neighbors;                // Set of edges between nodes.



int go_ahead_signal   = 0;
int num_msgs          = 0;
int num_threads;
int round_no;
int delay;
int *num_nghbrs;


std::random_device rd;                                  // Obtains a seed for the random number engine.
std::mt19937 gen(rd());                                 // Standard mersenne_twister_engine seeded with rd().
std::uniform_int_distribution<> distrib(1, 12);



void asynch_lcr(long threadid, int thread_num) {
    long max      = threadid;
    bool push_max = true;
    int rounds    = num_threads;
    int op_t_id   = (thread_num+1) % num_threads;
    long incoming;

    
    while (rounds > 0) {
        delay = distrib(gen);                                   // Generates a random number from the range 1-12
        sleep_for(nanoseconds(delay));                          // Random number delays pushing value to output queue.
        if (push_max) {
            q_lcr[op_t_id].push(max);                           // If a new max is found, thread outputs the new maximum.
            num_msgs++;
        }

        else {
            q_lcr[op_t_id].push(0);
            num_msgs++;
        }
        push_max = false;
        
        while(q_lcr[thread_num].size() == 0) {
            sleep_for(milliseconds(10));
        }

        for (int h = 0; h < q_lcr[thread_num].size(); h++) {                // Retrieve elements from the input queue.
            incoming = q_lcr[thread_num].front();
            
            if (incoming > threadid) {
                max      = incoming;                                    // New maximum found from the input queue.
                push_max = true;
            }
            q_lcr[thread_num].pop();
        }
        rounds--;                                                       // Each thread runs n rounds asynchronously. 
    }
    mtx.lock();
    cout << "My ID is " << threadid << " and my leader is " << max << endl ;    
    mtx.unlock();
}


void asynch_floodmax(long thread_id, int thread_num, bool is_root) {
    pair<long,long> incoming;
    long msg_id        = thread_id;
    long max_id        = thread_id;
    long parentid      = -1;
    long msg;
    int num_nghbr_msgs = 0;
    bool reported      = false;
    bool received      = false;
    if (is_root) {
        parentid   = thread_id;
        for (int j = 0; j < num_threads; j++) {
            if (thread_num != j && neighbors[thread_num].find(j) != neighbors[thread_num].end()) {
                mtx.lock();
                q[j].push(make_pair(max_id, thread_id));
                num_msgs++;
                mtx.unlock();
            }
        }
        reported = true;
    }
    while (true) {

        while (!q[thread_num].empty()) {

            mtx.lock();
            incoming = q[thread_num].front();
            msg_id   = get<1>(incoming);
            msg      = get<0>(incoming);
            mtx.unlock();

            if (msg == -1) {                    // Break out of loops and terminate.
                break;
            }
            if (parentid == -1) {               // Set parent as first message it receives.
                parentid = msg_id;
            }
        }
    }
    mtx.lock();
    cout << "I am Thread ID " << thread_id << " and my leader is " << max_id << endl;
    mtx.unlock();
}


int main(int argc, char* argv[]) {
    long i = 0;
    int data;
    int c;
    bool lcr = false;


    ifstream infile;
    infile.open("connectivity.txt");                            // This file contains the input data.
    infile >> data;                                             // This initial data contains the number of threads.
    num_threads = data;

    int thread_ids[num_threads] = {};                           // Contains ID's of the processes.
    int data_collector[num_threads*(num_threads+1)] = {};       // Temporary array to collect data from file.
    neighbors = new set<int>[num_threads];                      // Set of edges between nodes.

    while (!infile.eof()) {                                     // This collects all remaining data: process id's and connectivity.
        infile >> data;
        data_collector[i++] = data;
    }
    infile.close();
    num_nghbrs = new int[num_threads];
    for (i = 0; i < num_threads; i++) {
        num_nghbrs[i] = 0; 
    }

    c = 0;
    for (int k = 0; k < num_threads+1; k++) {            // Sort out id's from the connectivity data.
        for (int j = 0; j < num_threads; j++) {
            if (k == 0) {
                thread_ids[j] = data_collector[j];
            }
            else {
                if (data_collector[c] == 1) {
                    neighbors[k-1].insert(j);
                    num_nghbrs[k-1]++;
                }
            }
            c++;
        }
    }
    q           = new queue<pair<long,int>>[num_threads];
    q_lcr       = new queue<long>[num_threads];
    
    thread threads[num_threads];
    if (argv[1] == "lcr") {
        for (i = 0; i < num_threads; i++) {                     // Create threads.
            threads[i] = thread(asynch_lcr, thread_ids[i], i);
        }
    }
    else {
        for (i = 0; i < num_threads; i++) {                     // Create threads.
            if (!i) {
                threads[i] = thread(asynch_floodmax, thread_ids[i], i, true);
            }
            else {
                threads[i] = thread(asynch_floodmax, thread_ids[i], i, false);
            }
        }
    }
    for (i = 0; i < num_threads; i++) {
        threads[i].join();
    }
    cout << endl << "Total messages sent = " << num_msgs << endl;

   return 0;
}
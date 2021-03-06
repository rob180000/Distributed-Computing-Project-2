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
queue<pair<long,int>> *q;           // This array of queues stores messages from neighbors. 
set<int> *neighbors;                // Set of edges between nodes.

int go_ahead_signal   = 0;
int num_msgs          = 0;
int num_threads;
int round_no;
int delay;
int *num_nghbrs;
int root_num = -1;


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


void asynch_floodmax(long thread_id, int thread_num) {
    pair<long,int> incoming;
    long msg_max_id    = thread_id;
    long max_id        = thread_id;
    int parent_num;
    int num_nghbr_msgs = 0;
    int num_children;
    int child_num;
    int msg_num;
    string status;
    queue<int> children;
    bool root    = false;
    bool leaf    = true;



    delay = distrib(gen);                       // Random delay to simulate asynchronous behavior.
    sleep_for(nanoseconds(delay));

    // Floodmax.
    for (int j = 0; j < num_threads; j++) {     // Flood neighbors with thread id.
        if (thread_num != j && neighbors[thread_num].find(j) != neighbors[thread_num].end() && neighbors[thread_num].size() > 1) {
            mtx.lock();
            q[j].push(make_pair(max_id, thread_num));
            num_msgs++;
            mtx.unlock();
        }
    }

    while (q[thread_num].empty()) {                                  // Wait for messages to arrive.
        sleep_for(nanoseconds(5));
    }

    mtx.lock();
    if (neighbors[thread_num].size() > 1 && root_num == -1) {
        root_num = thread_num;
        root     = true;
        leaf     = false;
    } 
    mtx.unlock();

    mtx.lock();
    incoming   = q[thread_num].front();
    mtx.unlock();

    msg_num    = get<1>(incoming);
    msg_max_id = get<0>(incoming);

    if (!root) {
        parent_num = msg_num;                                    // Set parent as sender of first message received.
        mtx.lock();
        q[parent_num].push(make_pair(-1, thread_num));           // Send confirmation to parent that it is a parent.
        num_msgs++;
        mtx.unlock();
    }
    else {
        for (int j = 0; j < num_threads; j++) {
            if (thread_num != j && neighbors[thread_num].find(j) != neighbors[thread_num].end()) {
                mtx.lock();
                q[j].push(make_pair(-2, thread_num));           // Send neighbor message that it is root (parent).
                num_msgs++;
                mtx.unlock();
            }
        }
    }

    if (msg_max_id > max_id) {
        max_id = msg_max_id;
    }

    mtx.lock();
    q[thread_num].pop();
    mtx.unlock();
    
    while (!q[thread_num].empty()) {
        mtx.lock();
        incoming   = q[thread_num].front();
        mtx.unlock();

        msg_num    = get<1>(incoming);
        msg_max_id = get<0>(incoming);

        if (msg_max_id == -1 && msg_num != parent_num) {
            children.push(msg_num);
            leaf = false;
        }
        else if (msg_max_id == -2) {
            parent_num = msg_num;
        }

        if (msg_max_id > max_id) {
            max_id = msg_max_id;
        }

        mtx.lock();
        q[thread_num].pop();
        mtx.unlock();
    }

    if (neighbors[thread_num].size() > 1) {
        leaf = false;
    }

    // Convergecast.
    // Start from leaf nodes and propagate current max id's up to root.
    if (leaf) {                                                         // Leaf nodes are those with no children.
        mtx.lock();
        q[parent_num].push(make_pair(max_id, thread_num));              // Send max to parent.
        num_msgs++;
        mtx.unlock();
    }

    if (!leaf) {
        sleep_for(nanoseconds(5));

        mtx.lock();
        incoming   = q[thread_num].front();
        mtx.unlock();

        msg_num    = get<1>(incoming);
        msg_max_id = get<0>(incoming);

        if (msg_max_id > max_id) {
            max_id = msg_max_id;
        }

        mtx.lock();
        q[parent_num].push(make_pair(max_id, thread_num));          // Send max to parent.
        num_msgs++;
        q[thread_num].pop();
        mtx.unlock();
    }

    // Now propagate the overall max down from root to leaf nodes and terminate.
    if (root) {
        sleep_for(nanoseconds(5));
        //cout << "Thread num " << thread_num << " as ROOT starting BroadCast" << endl;
        while (q[thread_num].empty()) {                     // Wait for messages from children.
            sleep_for(milliseconds(5));
        }

        while (!q[thread_num].empty()) {
            mtx.lock();
            incoming   = q[thread_num].front();
            mtx.unlock();

            msg_num    = get<1>(incoming);
            msg_max_id = get<0>(incoming);

            if (msg_max_id > max_id) {
                max_id = msg_max_id;
            }

            q[thread_num].pop();
        }

        // Send messages to children.
        while (!children.empty()) {
            child_num = children.front();                   

            mtx.lock();
            q[child_num].push(make_pair(max_id, thread_num));
            num_msgs++;
            q[thread_num].pop();
            mtx.unlock();

            children.pop();                                 // Iterate through children by popping children queue.
        }
    }
    // Keep sending messages down towards leaf nodes.
    // Non-leaf nodes.
    else if (!leaf && !root) {
        sleep_for(nanoseconds(5));

        mtx.lock();
        incoming   = q[thread_num].front();
        mtx.unlock();

        msg_num    = get<1>(incoming);
        msg_max_id = get<0>(incoming);

        if (msg_max_id > max_id) {
            max_id = msg_max_id;
        }

        while (!children.empty()) {
            child_num = children.front();

            mtx.lock();
            q[parent_num].push(make_pair(max_id, thread_num));       // Send max to children.
            num_msgs++;
            q[thread_num].pop();
            mtx.unlock();

            children.pop();
        }
    }
    // Leaf nodes.
    else {

        sleep_for(nanoseconds(5));
        msg_max_id = get<0>(q[thread_num].front());
        if (msg_max_id > max_id) {
            max_id = msg_max_id;
        }
    }

    if (thread_id == max_id) {
        status = "Leader";
    }
    else {
        status = "Non-Leader";
    }                     
    
    mtx.lock();
    cout << "I am Thread ID " << thread_id << " and I am a " << status << endl;
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
            threads[i] = thread(asynch_floodmax, thread_ids[i], i);
        }
    }
    for (i = 0; i < num_threads; i++) {
        threads[i].join();
    }
    cout << endl << "Total messages sent = " << num_msgs << endl;

   return 0;
}
This code is used to identify Leader among a set of asynchronous processes using LCR or Floodmax algorithm. 
The connectivity.txt file contains the num of asynchronous processes, process ids, and the neighbor processes for each process. 

Compile the program with: g++ prj2_ccast.cpp -o prj2_ccast -pthread

Run with: ./prj2_ccast <algorithm> 
where <algorithm> is either lcr or floodmax.

Sample output:

I am Thread ID 0 and my leader is 67
I am Thread ID 5 and my leader is 67
I am Thread ID 12 and my leader is 67
I am Thread ID 67 and my leader is 67

Total messages sent = 24



/// 1. Install Dependencies/// 

/* Before compiling, install ZeroMQ and Boost Lock-Free queue:
	
---- sudo apt-get update
---- sudo apt-get install libzmq3-dev libboost-all-dev

 Clone the cppzmq repository (ZeroMQ C++ bindings):

----- git clone https://github.com/zeromq/cppzmq.git 

This provides the necessary zmq.hpp header file.*/
  
  
/// 2. Create and Compile the Server (Matching Engine)///
         
/* Create a new C++ file for the server
Save the server code I provided in a file called server.cpp.

Compile it
Use g++ with threading support:
---- g++ server.cpp -o server -std=c++17 -lzmq -lpthread -Icppzmq

This creates an executable server.

Run the server
---- ./server

It will start listening for incoming connections on port 5555.
*/

/// 3. Create and Compile the Client///

/*Create a new C++ file for the client
Save the client code in a file called client.cpp.
Compile it useing 
---- g++ client.cpp -o client -std=c++17 -lzmq -Icppzmq

This creates an executable client.

Run the client
---- ./client

Now, you can place orders from the client, and the server will match orders.
*/

///4. Test the System///

/*Start the server in one terminal
---- ./server

 Start multiple clients in different terminals
 ---- ./client
 
Use the client to:
1, Place buy/sell orders
2, View the order book
3, Exit
*/

///Additional Notes///
/* Ensure server runs first before starting a client.
Open multiple clients to simulate multiple traders placing orders.
Modify the order book matching logic if needed to improve execution speed.
This setup provides a real-time order matching engine with low-latency, lock-free performance using ZeroMQ for client-server communication. 
*/




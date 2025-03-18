/*
Clients send orders using ZeroMQ (REQ/REP pattern).
 The server (matching engine) receives and processes these orders.
 Multi-threading remains lock-free, ensuring low latency.
 */
 
/*
 Installation
First, install ZeroMQ and its C++ binding (cppzmq):
Ensure cppzmq headers are available in your project.  

---- sudo apt-get install libzmq3-dev
---- git clone https://github.com/zeromq/cppzmq.git

Ensure cppzmq headers are available in your project.
*/

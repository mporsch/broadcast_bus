# BroadcastBus
A communication bus with broadcast transmissions to multiple attached terminals and receipt feedback implemented as C++14 header-only library without external dependencies.

## Design goals
* The message type is given as template argument.
* A transmitter may wait for message receipt at all other attached terminals.
* A receiver may poll for received messages or wait for receipt.
* Terminals are attached and distributed via smart pointers to handle Terminal closing and final ressource cleanup.

## Build
Build test using CMake or `$ g++ -o broadcast_bus_test broadcast_bus_test.cpp -std=c++14 -lpthread`

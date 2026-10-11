#include "../src/network.hpp"
#include <cassert>
#include <iostream>
int main(){
    day::networkCancelled=true;
    // Cancellation wins before resolving even an invalid endpoint.
    bool cancelled=false;try{day::request("invalid://must-not-connect");}catch(const std::exception& e){cancelled=std::string(e.what())=="Request cancelled";}
    assert(cancelled);
    assert(day::networkProgress(nullptr,0,0,0,0)==1);
    day::networkCancelled=false;
    assert(day::networkProgress(nullptr,0,0,0,0)==0);
    std::cout<<"Network shutdown rejects new requests and aborts in-flight progress: PASS\n";
}

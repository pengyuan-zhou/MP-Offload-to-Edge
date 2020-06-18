#include "server.hpp"
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <chrono>
#include <thread>
#include <iomanip>
#include <queue>
#include <fstream>
#include <map>
#include <iterator>
//curl files
#include <iostream>
#include <string>
#include <curl/curl.h>

extern "C" {
#include "detector.h"
}
#define MESSAGE_ECHO 0
#define EDGE 1
#define IMAGE_DETECT 2
#define BOUNDARY 3
#define PORT 51717
#define PACKET_SIZE 80000
#define RES_SIZE 528
#define TRAIN

using namespace std;
using namespace cv;

struct sockaddr_in localAddr;
struct sockaddr_in remoteAddr;
struct sockaddr_in remoteAddr1;
struct sockaddr_in frontAddr;
socklen_t addrlen = sizeof(remoteAddr);

queue<frameBuffer> frames;
queue<resBuffer> results;
int recognizedMarkerID;

map<string, int> mapOfDevices;




int main(int argc, char *argv[])
{
    result* res;
    load_params();
    res = detect(10340);

    return 0;
}


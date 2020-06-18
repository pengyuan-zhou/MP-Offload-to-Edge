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
#define BOUNDARY 2
#define PORT 52727
#define PACKET_SIZE 60000
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
queue<resBuffer> resultss;
int recognizedMarkerID;

map<string, int> mapOfDevices;

double wallclock (void)
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (double)tv.tv_sec*1000 + (double)tv.tv_usec * 0.001;
}
double what_time_is_it_now()
{
    struct timeval time;
    if (gettimeofday(&time,NULL)){
        return 0;
    }
    return (double)time.tv_sec*1000 + (double)time.tv_usec * 0.001;
}

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}


void *ThreadReceiverFunction(void *socket) {
    cout<<"Receiver Thread Created!"<<endl;
    char tmp[4];
    char Tmp[8];
    char buffer[PACKET_SIZE];
    int sock = *((int*)socket);
    cout << " sock is  " << sock << "\n" ;
    int device_ind = 1; 
    int len =20;
    char str[len];
    char str_front[len];
    double time_register_start;
    double time_to_register;
    double time_receivepic;
    double imageDelay;

    ofstream output_receive ("test_receive.txt");
    ofstream output_delay ("test_imagedelay.txt");
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        recvfrom(sock, buffer, PACKET_SIZE, 0, (struct sockaddr *)&frontAddr, &addrlen);
        time_receivepic = what_time_is_it_now();
        frameBuffer curFrame;    
        memcpy(tmp, buffer, 4);
        curFrame.frmID = *(int*)tmp;        
        memcpy(tmp, &(buffer[4]), 4);
        curFrame.dataType = *(int*)tmp;
        FILE *fd;
        if(curFrame.dataType == MESSAGE_ECHO) {
            cout<<"echo message!"<<endl;
            charint echoID;
            echoID.i = curFrame.frmID;
            char echo[4];
            memcpy(echo, echoID.b, 4);

            inet_ntop(AF_INET, &(frontAddr.sin_addr), str_front, len);
            if (mapOfDevices.find(string(str_front)) != mapOfDevices.end()) {
                output_receive<<"receiving from an old " << (device_ind-1) << " device, whose ip is " << str_front << endl;
                cout<<"receiving from an old  " << (device_ind-1) << " device, whose ip is " << str_front << endl;
                continue;}
            cout<<"receiving from the " << device_ind << " device, whose ip is " << str_front << endl;
            //pair<map<int, string>::iterator,bool> ret;
            mapOfDevices.insert(pair<string, int>(string(str_front), device_ind));
            device_ind += 1; 
            printf("device_ind now has increased to %d\n", device_ind); 
            map<string, int>::iterator it_device = mapOfDevices.begin();
            while(it_device != mapOfDevices.end()){
                output_receive << it_device->first << " "  << it_device->second << "\n" ;
                cout << it_device->first << " "  << it_device->second << "\n" ;
                it_device ++;}
            continue;

        }
        memcpy(Tmp, &(buffer[8]), 8);
        curFrame.timeCaptured = *(double*)Tmp;
        memcpy(Tmp, &(buffer[16]), 8);
        curFrame.timeSend = *(double*)Tmp;
        memcpy(tmp, &(buffer[24]), 4);
        curFrame.bufferSize = *(int*)tmp;
        if (curFrame.bufferSize==0) { continue;}
        imageDelay = time_receivepic - curFrame.timeCaptured;
         
        output_receive << "receive frameID : " << curFrame.frmID << ", at time : " <<  time_receivepic << ", sent out from vehicle at time: " << curFrame.timeCaptured <<  ", has size: "<< curFrame.bufferSize << ", transmission delay: '" << time_receivepic - curFrame.timeCaptured << "' milliseconds" << endl;
        output_delay << imageDelay << endl;
        cout<<"frame "<<curFrame.frmID<<" received, filesize: "<<curFrame.bufferSize << endl;
        curFrame.buffer = new char[curFrame.bufferSize];
        memset(curFrame.buffer, 0, curFrame.bufferSize);
        memcpy(curFrame.buffer, &(buffer[28]), curFrame.bufferSize);

        frames.push(curFrame);
    }
    output_receive.close();
    output_delay.close();

}

void *ThreadSenderFunction(void *socket) {
    cout << "Sender Thread Created!" << endl;
    char buffer[RES_SIZE];
    int sock = *((int*)socket);
    int len =20;
    char str_buffer[len];
    ofstream output_send ("test_send.txt");

    while (1) {
        if(resultss.empty()) {
            this_thread::sleep_for(chrono::milliseconds(1));
            continue;
        }

        resBuffer curRes = resultss.front();
        resultss.pop();
    
        memset(buffer, 0, sizeof(buffer));
        memcpy(buffer, curRes.resID.b, 4);
        memcpy(&(buffer[4]), curRes.resType.b, 4);
        memcpy(&(buffer[8]), curRes.resLatitude.b, 8);
        memcpy(&(buffer[16]), curRes.resLongtitude.b, 8);
        memcpy(&(buffer[24]), curRes.markerNum.b, 4);
        if(curRes.markerNum.i != 0)
            memcpy(&(buffer[28]), curRes.buffer, 100 * curRes.markerNum.i);
        map<string, int>::iterator it_device = mapOfDevices.begin();
        while(it_device != mapOfDevices.end()){
            memset((char*)&remoteAddr, 0, sizeof(remoteAddr));
            remoteAddr.sin_family = AF_INET;
            remoteAddr.sin_addr.s_addr = inet_addr((it_device->first).c_str());
            remoteAddr.sin_port = htons(51919);
            output_send << "sending to the " << it_device->second<< " device, whose ip is "<< it_device->first << endl ;
            cout << "sending to the " << it_device->second<< " device, whose ip is "<< it_device->first << endl ;
            sendto(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&remoteAddr, addrlen);
            output_send << "send_result of frameID of: " << curRes.resID.i << " sent by observer at time: " << std::fixed << std::setprecision(15) << curRes.resLongtitude.d << " whose size is: " << sizeof(buffer) << endl;
            cout << "send_result of frameID of: " << curRes.resID.i << " sent by observer at time: " << curRes.resLongtitude.d << " whose size is: " << sizeof(buffer) << endl;
            it_device++;} 
            cout<<"frame "<<curRes.resID.i<<" res sent, "<<"marker#: "<<curRes.markerNum.i;
            cout<<" at "<<setprecision(15)<<wallclock()<<endl<<endl;
    }    
    output_send.close();
}

void *ThreadProcessFunction(void *param) {
    cout<<"Process Thread Created!"<<endl;
    recognizedMarker marker;
    bool objectDetected = false;
    result* res;
    double time_process_start;
    double time_process;

    ofstream output_process("test_process.txt");
    ofstream output_process_delay("test_processdelay.txt");
    load_params();

    while (1) {
        if(frames.empty()) {
            this_thread::sleep_for(chrono::milliseconds(1));
            continue;
        }

        frameBuffer curFrame = frames.front();
        frames.pop();

        int frmID = curFrame.frmID;
        int frmDataType = curFrame.dataType;
        double latitude = 0;
        double longtitude = 0;
        int frmSize = curFrame.bufferSize;
        char* frmdata = curFrame.buffer;
        
        if(frmDataType == IMAGE_DETECT) {
            ofstream file("received.jpg", ios::out | ios::binary);
            if(file.is_open()) {
                file.write(frmdata, frmSize);
                file.close();

                time_process_start = what_time_is_it_now();
                res = detect();
                output_process << "time_process_pic of frameid of: " << frmID << " takes: '" <<  what_time_is_it_now() - time_process_start<< "' milliseconds" << endl;
                cout << "time_process_pic of frameid of: " << frmID << " takes: " <<  what_time_is_it_now() - time_process_start << " milliseconds" << endl;
                objectDetected = true;
                output_process << "resultss: " << res->num << endl;
                cout << "resultss: " << res->num << endl;
            } 
        } else if(frmDataType == EDGE) {
             cout << frmdata << endl;
             continue;
        }
        for(int i = 0; i < sizeof(curFrame.buffer); i++)
        {
            curFrame.buffer[i] = rand();
        }

        for(int i = 0; i < sizeof(frmdata); i++)
        {
            frmdata[i] = rand();
        }
        int personNum = 0;
        int carNum = 0;

        resBuffer curRes;
        if(objectDetected) {
            charfloat p;
            charint ci;
            curRes.resID.i = frmID;
            curRes.resLatitude.d = curFrame.timeSend;
            curRes.resLongtitude.d = curFrame.timeSend;
            curRes.resType.i = BOUNDARY;
            if(res->num <= 5)
                curRes.markerNum.i = res->num;
            else
                curRes.markerNum.i = 5;
            curRes.buffer = new char[100 * curRes.markerNum.i];

            for(int i = 0; i < curRes.markerNum.i; i++) {
                int pointer = 100 * i;
                struct object *cur = &(res->objects[i]); 

                p.f = cur->prob;
                memcpy(&(curRes.buffer[pointer]), p.b, 4);
                pointer += 4;
                ci.i = cur->left;
                memcpy(&(curRes.buffer[pointer]), ci.b, 4);
                pointer += 4;
                ci.i = cur->right;
                memcpy(&(curRes.buffer[pointer]), ci.b, 4);
                pointer += 4;
                ci.i = cur->top;
                memcpy(&(curRes.buffer[pointer]), ci.b, 4);
                pointer += 4;
                ci.i = cur->bot;
                memcpy(&(curRes.buffer[pointer]), ci.b, 4);
                pointer += 4;

                memcpy(&(curRes.buffer[pointer]), cur->name, strlen(cur->name));
                pointer += strlen(cur->name);
                curRes.buffer[pointer] = '.';
            }
        }
        else {
            curRes.resID.i = frmID;
            curRes.markerNum.i = 0;
        }
        delete res->objects;
        res->objects = NULL;

        resultss.push(curRes);
    }
    output_process.close();
    output_process_delay.close();
}

int main(int argc, char *argv[])
{
    pthread_t senderThread, receiverThread, processThread, annotationThread;
    int ret1, ret2, ret3, ret4;
    char fileid[4];
    int status = 0;
    int sockTCP, sockUDP;
   

    memset((char*)&localAddr, 0, sizeof(localAddr));
    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    localAddr.sin_port = htons(PORT);

    memset((char*)&remoteAddr, 0, sizeof(remoteAddr));
    remoteAddr.sin_family = AF_INET;
    remoteAddr.sin_addr.s_addr = inet_addr("INADDR_ANY");
    remoteAddr.sin_port = htons(51919);

    if((sockUDP = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        cout<<"ERROR opening udp socket"<<endl;
        exit(1);
    }
    if(bind(sockUDP, (struct sockaddr *)&localAddr, sizeof(localAddr)) < 0) {
        cout<<"ERROR on udp binding"<<endl;
        exit(1);
    }
    cout << endl << "========server started, waiting for clients==========" << endl;

    ret1 = pthread_create(&receiverThread, NULL, ThreadReceiverFunction, (void *)&sockUDP);
    ret2 = pthread_create(&processThread, NULL, ThreadProcessFunction, NULL);
    ret3 = pthread_create(&senderThread, NULL, ThreadSenderFunction, (void *)&sockUDP);

    pthread_join(receiverThread, NULL);
    pthread_join(processThread, NULL);
    pthread_join(senderThread, NULL);

    return 0;
}


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
#include<iostream>
#include<algorithm>
#include<fstream>
#include<chrono>

#include<opencv2/core/core.hpp>

#include<System.h>

#define MESSAGE_ECHO 0
#define EDGE 1
#define IMAGE_DETECT 2
#define POINT 3
#define PORT 51717
#define PACKET_SIZE 60000
#define RES_SIZE 1620
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
                //cout<<"receiving from an old  " << (device_ind-1) << " device, whose ip is " << str_front << endl;
                continue;}
            //cout<<"receiving from the " << device_ind << " device, whose ip is " << str_front << endl;
            //pair<map<int, string>::iterator,bool> ret;
            mapOfDevices.insert(pair<string, int>(string(str_front), device_ind));
            device_ind += 1; 
            printf("device_ind now has increased to %d\n", device_ind); 
            map<string, int>::iterator it_device = mapOfDevices.begin();
            while(it_device != mapOfDevices.end()){
                output_receive << it_device->first << " "  << it_device->second << "\n" ;
                //cout << it_device->first << " "  << it_device->second << "\n" ;
                it_device ++;}
            continue;

        }
        memcpy(Tmp, &(buffer[8]), 8);
        curFrame.timeCaptured = *(double*)Tmp;
        //curFrame.longtitude = *(double*)Tmp;
        memcpy(Tmp, &(buffer[16]), 8);
        curFrame.timeSend = *(double*)Tmp;
        memcpy(tmp, &(buffer[24]), 4);
        curFrame.bufferSize = *(int*)tmp;
        if (curFrame.bufferSize==0) { continue;}
        imageDelay = time_receivepic - curFrame.timeCaptured;
         
        output_receive << "receive frameID : " << curFrame.frmID << ", at time : " <<  time_receivepic << ", sent out from vehicle at time: " << curFrame.timeCaptured <<  ", has size: "<< curFrame.bufferSize << ", transmission delay: '" << time_receivepic - curFrame.timeCaptured << "' milliseconds" << endl;
        output_delay << imageDelay << endl;
        //cout<<"frame "<<curFrame.frmID<<" received, filesize: "<<curFrame.bufferSize << endl;
        curFrame.buffer = new char[curFrame.bufferSize];
        memset(curFrame.buffer, 0, curFrame.bufferSize);
        memcpy(curFrame.buffer, &(buffer[28]), curFrame.bufferSize);

        frames.push(curFrame);
        //delete curFrame.buffer;
        //}
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
        memcpy(&(buffer[8]), curRes.PointNum.b, 4);
        memcpy(&(buffer[12]), curRes.timeSend.b, 8);
        if(curRes.PointNum.i != 0)
            memcpy(&(buffer[20]), curRes.buffer, 8 * curRes.PointNum.i);
        map<string, int>::iterator it_device = mapOfDevices.begin();
        while(it_device != mapOfDevices.end()){
            memset((char*)&remoteAddr, 0, sizeof(remoteAddr));
            remoteAddr.sin_family = AF_INET;
            remoteAddr.sin_addr.s_addr = inet_addr((it_device->first).c_str());
            remoteAddr.sin_port = htons(51818);
            output_send << "sending to the " << it_device->second<< " device, whose ip is "<< it_device->first << endl ;
            //cout << "sending to the " << it_device->second<< " device, whose ip is "<< it_device->first << endl ;
            sendto(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&remoteAddr, addrlen);
            output_send << "send_result of frameID of: " << curRes.resID.i  << " whose size is: " << sizeof(buffer) << endl;
            //cout << "send_result of frameID of: " << curRes.resID.i << " whose size is: " << sizeof(buffer) << endl;
            it_device++;} 
            //cout<<" at "<<setprecision(15)<<wallclock()<<endl<<endl;
        //memset(curRes.buffer,1,sizeof(curRes.buffer)); 
        //memset(buffer,1,sizeof(buffer)); 
        //memset(str_buffer,1,sizeof(str_buffer)); 
        /*for(int i = 0; i < sizeof(curRes.buffer); i++)
        {
            curRes.buffer[i] = rand();
        }
        for(int i = 0; i < sizeof(buffer); i++)
        {
            buffer[i] = rand();
        }
        for(int i = 0; i < sizeof(str_buffer); i++)
        {
            str_buffer[i] = rand();
        }*/
    }    
    output_send.close();
}

void *ThreadProcessFunction(void *param) {
    cout<<"Process Thread Created!"<<endl;
    //recognizedMarker marker;
    //bool objectDetected = false;
    //result* res;
    double time_process_start;
    double time_process;

    ofstream output_process("test_process.txt");
    ofstream output_process_delay("test_processdelay.txt");
    //load_params();
    // Create SLAM system. It initializes all system threads and gets ready to process frames.
    ORB_SLAM2::System SLAM("../../Vocabulary/ORBvoc.txt", "TUM1.yaml",ORB_SLAM2::System::MONOCULAR,true);

    Mat im;
    Mat Tcw;
    vector<ORB_SLAM2::MapPoint*> vMPs;
    vector<KeyPoint> vKeys;

    while (1) {
        if(frames.empty()) {
            this_thread::sleep_for(chrono::milliseconds(1));
            continue;
        }

        frameBuffer curFrame = frames.front();
        frames.pop();

        int frmID = curFrame.frmID;
        int frmDataType = curFrame.dataType;
        double tframe = curFrame.timeCaptured;
        int frmSize = curFrame.bufferSize;
        char* frmdata = curFrame.buffer;
        
        if(frmDataType == IMAGE_DETECT) {
            // last change
            ofstream file("received.jpg", ios::out | ios::binary);
            if(file.is_open()) {
                file.write(frmdata, frmSize);
                file.close();
		// Read image from file
		im = imread("received.jpg",CV_LOAD_IMAGE_UNCHANGED);
                time_process_start = what_time_is_it_now();
		Tcw = SLAM.TrackMonocular(im,tframe);
		vMPs = SLAM.GetTrackedMapPoints();
		vKeys = SLAM.GetTrackedKeyPointsUn();

                output_process << "time_process_pic of frameid of: " << frmID << " takes: '" <<  what_time_is_it_now() - time_process_start<< "' milliseconds" << endl;
                //cout << "time_process_pic of frameid of: " << frmID << " takes: " <<  what_time_is_it_now() - time_process_start << " milliseconds" << endl;
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
        //memset(curFrame.buffer,1,sizeof(curFrame.buffer));
        //memset(frmdata,1,sizeof(frmdata));

        resBuffer curRes;
        curRes.resID.i = frmID;

        const int N = vKeys.size();
        cout << " N is " << N ; 
        int pointer = 0;
        int pointnum = 0;
        charfloat p;
        for(int i=0; i<N; i++) {
            if(vMPs[i]) {
                pointnum += 1; 
            }
        }
        curRes.resID.i = frmID;
        if(pointnum<=200){
            curRes.PointNum.i = pointnum ; 
            curRes.buffer = new char[8*pointnum];}
        else{
            curRes.PointNum.i = 200 ; 
            curRes.buffer = new char[1600];}
        for(int i=0; i<curRes.PointNum.i; i++) {
                p.f = vKeys[i].pt.x;
                memcpy(&(curRes.buffer[pointer]),p.b,4);
                pointer += 4;
                p.f = vKeys[i].pt.y;
                memcpy(&(curRes.buffer[pointer]),p.b,4);
                pointer += 4;
                pointnum += 1; 
                
        }

        cout << " point number is " << pointnum << endl;
	curRes.resType.i = POINT;
        curRes.timeSend.d = curFrame.timeSend;
          
        //free(res->objects);

        resultss.push(curRes);
    }
    output_process.close();
    output_process_delay.close();
}

int main(int argc, char *argv[])
{
    pthread_t senderThread, receiverThread, processThread, annotationThread;
    int ret1, ret2, ret3, ret4;
    //char buffer[PACKET_SIZE];
    char fileid[4];
    int status = 0;
    int sockTCP, sockUDP;
   

    memset((char*)&localAddr, 0, sizeof(localAddr));
    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    localAddr.sin_port = htons(PORT);

    memset((char*)&remoteAddr, 0, sizeof(remoteAddr));
    remoteAddr.sin_family = AF_INET;
    //remoteAddr.sin_addr.s_addr = inet_addr("192.168.12.42");
    remoteAddr.sin_addr.s_addr = inet_addr("INADDR_ANY");
    remoteAddr.sin_port = htons(51818);

    if((sockUDP = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        cout<<"ERROR opening udp socket"<<endl;
        exit(1);
    }
    if(bind(sockUDP, (struct sockaddr *)&localAddr, sizeof(localAddr)) < 0) {
        cout<<"ERROR on udp binding"<<endl;
        exit(1);
    }
    cout << endl << "========server started, waiting for clients==========" << endl;

    //ret4 = pthread_create(&receiverThread_echo, NULL, ThreadReceiverFunction_echo, (void *)&sockUDP);
    ret1 = pthread_create(&receiverThread, NULL, ThreadReceiverFunction, (void *)&sockUDP);
    ret2 = pthread_create(&processThread, NULL, ThreadProcessFunction, NULL);
    ret3 = pthread_create(&senderThread, NULL, ThreadSenderFunction, (void *)&sockUDP);

    //pthread_join(receiverThread_echo, NULL);
    pthread_join(receiverThread, NULL);
    pthread_join(processThread, NULL);
    pthread_join(senderThread, NULL);

    return 0;
}


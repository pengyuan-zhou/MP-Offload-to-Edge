package helsinki.semanticSlam.detection;

import android.content.Context;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationProvider;
import android.os.Build;
import android.os.Environment;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Process;
import android.util.Log;

import com.google.android.gms.common.api.GoogleApiClient;

import org.opencv.core.Mat;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.net.InetSocketAddress;
import java.net.SocketAddress;
import java.nio.channels.DatagramChannel;

import helsinki.semanticSlam.detection.network.ReceivingTask;
import helsinki.semanticSlam.detection.network.TransmissionTask;

public class ARManager {

    static private ARManager instance;

    private Handler handlerUtil;
    private Handler handlerNetwork;

    private TransmissionTask taskTransmission;
    private ReceivingTask taskReceiving;

    private DatagramChannel dataChannel;
    private SocketAddress serverAddr;
    private String ip;
    private int port;

    private static boolean isCloudBased;
    private String ip2;
    private int port2;
    private InetSocketAddress serverAddr2;
    private DatagramChannel dataChannel2;
    private TransmissionTask taskTransmission2;
    private ReceivingTask taskReceiving2;

    private ARManager(){ super(); }

    static public ARManager getInstance() {
        synchronized (ARManager.class){
            if (instance == null) instance = new ARManager();
        }
        return instance;
    }

    private Handler createAndStartThread(String name, int priority){
        HandlerThread handlerThread = new HandlerThread(name, priority);
        handlerThread.start();
        return new Handler(handlerThread.getLooper());
    }

    private void initConnection() {
        File sdcard = Environment.getExternalStorageDirectory();
        File file = new File(sdcard,"semanticSLAM/slam.txt");
        File file2 = new File(sdcard,"semanticSLAM/detection.txt");
        try {
            BufferedReader br = new BufferedReader(new FileReader(file));
            BufferedReader br2 = new BufferedReader(new FileReader(file2));

            ip = br.readLine();
            port = Integer.parseInt(br.readLine());
            br.close();

            ip2 = br2.readLine();
            port2 = Integer.parseInt(br2.readLine());
            br2.close();

            serverAddr = new InetSocketAddress(ip, port);
            dataChannel = DatagramChannel.open();
            dataChannel.configureBlocking(false);

            serverAddr2 = new InetSocketAddress(ip2, port2);
            dataChannel2 = DatagramChannel.open();
            dataChannel2.configureBlocking(false);
            // pengzhou: the receiving phone needs the following sentence
            dataChannel.bind(new InetSocketAddress(51818));
            dataChannel2.bind(new InetSocketAddress(51919));
            //dataChannel.socket().connect(serverAddr);
        } catch (IOException e) {
            Log.d(Constants.TAG, "config file error");
        } catch (Exception e) {}
    }

    public void init(Context context, boolean isCloudBased){
        ARManager.isCloudBased = isCloudBased;

        System.loadLibrary("opencv_java");
        if(isCloudBased) initConnection();

        this.handlerUtil = createAndStartThread("util thread", Process.THREAD_PRIORITY_DEFAULT); //start util thread
        this.handlerNetwork = createAndStartThread("network thread", 1);

        if(isCloudBased) {
            taskTransmission = new TransmissionTask(dataChannel, serverAddr);
            taskTransmission2 = new TransmissionTask(dataChannel2, serverAddr2);
	        //pengzhou
            taskTransmission.setData(0, null, 0,0);
            taskTransmission2.setData(0, null, 0,0);
            //handlerNetwork.post(taskTransmission);
            handlerNetwork.post(taskTransmission2);
            taskReceiving = new ReceivingTask(dataChannel);
            taskReceiving2 = new ReceivingTask(dataChannel2);

            taskReceiving.setCallback(new ReceivingTask.Callback() {
                @Override
                public void onReceivePoint(int resultID, PointCor[] pointcor) {
                    callback.onPointsDetected(pointcor);
                }

                @Override
                public void onReceiveDetect(int resultID, Detected[] detected) {
                    callback.onObjectsDetected(detected);

                }

            });
            taskReceiving2.setCallback(new ReceivingTask.Callback() {
                @Override
                public void onReceivePoint(int resultID, PointCor[] pointcor) {
                    callback.onPointsDetected(pointcor);
                }

                @Override
                public void onReceiveDetect(int resultID, Detected[] detected) {
                    callback.onObjectsDetected(detected);

                }

            });
        }
    }

    public void start() {
    }

    public void stop() {
        if(isCloudBased) handlerNetwork.post(new Runnable() {
            @Override
            public void run() {
                try {
                    dataChannel.close();
                    dataChannel2.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        });
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR2) {
            handlerUtil.getLooper().quitSafely();
            handlerNetwork.getLooper().quitSafely();
        }
    }


    public void recognizeTime(int frameID, Mat frameData, double timeCaptured, double timeSend) {
        if(ARManager.isCloudBased) {
            taskTransmission.setData(frameID, frameData, timeCaptured, timeSend);
            handlerNetwork.post(taskTransmission);
            taskTransmission2.setData(frameID, frameData, timeCaptured, timeSend);
            handlerNetwork.post(taskTransmission2);
            taskReceiving.updateLatestSentID(frameID);
            taskReceiving2.updateLatestSentID(frameID);
        }
    }
    public void driveFrame(Mat frameData) {
        if(isCloudBased) {
            handlerNetwork.post(taskReceiving);
            handlerNetwork.post(taskReceiving2);
        }
    }

    private ARManager.Callback callback;

    public void setCallback(ARManager.Callback callback) {
        this.callback = callback;
    }

    public interface Callback {
    void onPointsDetected(PointCor[] pointcor);
    void onObjectsDetected(Detected[] detected);
    }
}
package helsinki.semanticSlam.detection.network;

import android.os.Environment;
import android.util.Log;

import org.opencv.core.Point;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.channels.DatagramChannel;

import java.io.File;
import java.io.FileWriter;
import java.io.FileOutputStream;
import java.io.BufferedWriter;
import java.util.Vector;

import helsinki.semanticSlam.detection.Constants;
import helsinki.semanticSlam.detection.Detected;
import helsinki.semanticSlam.detection.PointCor;



public class ReceivingTask implements Runnable{

    private ByteBuffer resPacket = ByteBuffer.allocate(Constants.RES_SIZE);

    private byte[] res;
    private float[] floatres = new float[8];
    private Point[] pointArray = new Point[4];
    private byte[] tmp = new byte[4];
    private byte[] Tmp = new byte[8];
    private byte[] name = new byte[56];
    private double resultLat;
    private double resultLong;
    private double resultTimecap;
    private double resultTimesend;
    private int newMarkerNum;
    private int resultPointNum;
    private int lastSentID;
    private int recoTrackRatio = Constants.scale / Constants.recoScale;

    private DatagramChannel datagramChannel;


    public ReceivingTask(DatagramChannel datagramChannel){
        this.datagramChannel = datagramChannel;
    }

    public void updateLatestSentID(int lastSentID){
        this.lastSentID = lastSentID;
    }

    @Override
    public void run() {
        resPacket.clear();
        try {
            if (datagramChannel.receive(resPacket) != null) {
                res = resPacket.array();
            } else {
                res = null;
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
        File sdcard = Environment.getExternalStorageDirectory();
        File filePoint = new File(sdcard,"semanticSLAM/ResultDelay.txt");
        File fileDetect = new File(sdcard,"CloudAR/resultDelay.txt");


        if (res != null) {
            System.arraycopy(res, 0, tmp, 0, 4);
            int resultID = ByteBuffer.wrap(tmp).order(ByteOrder.LITTLE_ENDIAN).getInt();

            System.arraycopy(res, 4, tmp, 0, 4);
            int resultType = ByteBuffer.wrap(tmp).order(ByteOrder.LITTLE_ENDIAN).getInt();

            if (resultType==3){
                System.arraycopy(res, 8, tmp, 0, 4);
                resultPointNum = ByteBuffer.wrap(tmp).order(ByteOrder.LITTLE_ENDIAN).getInt();

                System.arraycopy(res, 12, Tmp, 0, 8);
                resultTimesend = ByteBuffer.wrap(Tmp).order(ByteOrder.LITTLE_ENDIAN).getDouble();

            }
            if (resultType==2){

                System.arraycopy(res, 8, Tmp, 0, 8);
                resultTimecap = ByteBuffer.wrap(Tmp).order(ByteOrder.LITTLE_ENDIAN).getDouble();

                System.arraycopy(res, 16, Tmp, 0, 8);
                resultTimesend = ByteBuffer.wrap(Tmp).order(ByteOrder.LITTLE_ENDIAN).getDouble();

                System.arraycopy(res, 24, tmp, 0, 4);
                newMarkerNum = ByteBuffer.wrap(tmp).order(ByteOrder.LITTLE_ENDIAN).getInt();

            }


            double time= (double)System.currentTimeMillis();
            double resultdelay = time - resultTimesend;


            if (resultPointNum > 0) {
                PointCor pointcor[] = new PointCor[resultPointNum];

                try{BufferedWriter bw =
                        new BufferedWriter(new FileWriter(filePoint, true));
                        bw.write(Double.toString(resultdelay));
                        bw.newLine();
                        bw.flush();}
                catch (Exception e){
                    e.printStackTrace();

                }

                int i = 0;
                while (i < resultPointNum) {
                    pointcor[i] = new PointCor();

                    System.arraycopy(res, 20 + i * 8, tmp, 0, 4);
                    pointcor[i].x = ByteBuffer.wrap(tmp).order(ByteOrder.LITTLE_ENDIAN).getFloat();

                    System.arraycopy(res, 24 + i * 8, tmp, 0, 4);
                    pointcor[i].y = ByteBuffer.wrap(tmp).order(ByteOrder.LITTLE_ENDIAN).getFloat();

                    i++;
                }

                if (callback != null){
                    callback.onReceivePoint(resultID, pointcor);

                }
            }

            if (newMarkerNum >= 0) {
                Log.d(Constants.Eval, "" + newMarkerNum + " res " + resultID + " received ");
                Detected detected[] = new Detected[newMarkerNum];

                try{BufferedWriter bw =
                        new BufferedWriter(new FileWriter(fileDetect, true));
                    bw.write(Double.toString(resultdelay));
                    bw.newLine();
                    bw.flush();}
                catch (Exception e){
                    e.printStackTrace();

                }

                int i = 0;
                while (i < newMarkerNum) {
                    detected[i] = new Detected();
                    detected[i].tcap = resultTimecap;
                    detected[i].tsend = resultTimesend;
                    System.arraycopy(res, 28 + i * 100, tmp, 0, 4);
                    detected[i].prob = ByteBuffer.wrap(tmp).order(ByteOrder.LITTLE_ENDIAN).getFloat();

                    System.arraycopy(res, 32 + i * 100, tmp, 0, 4);
                    detected[i].left = ByteBuffer.wrap(tmp).order(ByteOrder.LITTLE_ENDIAN).getInt();
                    System.arraycopy(res, 36 + i * 100, tmp, 0, 4);
                    detected[i].right = ByteBuffer.wrap(tmp).order(ByteOrder.LITTLE_ENDIAN).getInt();
                    System.arraycopy(res, 40 + i * 100, tmp, 0, 4);
                    detected[i].top = ByteBuffer.wrap(tmp).order(ByteOrder.LITTLE_ENDIAN).getInt();
                    System.arraycopy(res, 44 + i * 100, tmp, 0, 4);
                    detected[i].bot = ByteBuffer.wrap(tmp).order(ByteOrder.LITTLE_ENDIAN).getInt();

                    System.arraycopy(res, 48 + i * 100, name, 0, 20);
                    String nname = new String(name);
                    detected[i].name = nname.substring(0, nname.indexOf("."));
                    i++;
                }

                if (callback != null){
                    callback.onReceiveDetect(resultID, detected);
                }
            }
        }
    }

    private Callback callback;

    public void setCallback(Callback callback) {
        this.callback = callback;
    }

    public interface Callback {
        void onReceivePoint(int resultID, PointCor[] pointcor);
        void onReceiveDetect(int resultID, Detected[] detected);
    }
}

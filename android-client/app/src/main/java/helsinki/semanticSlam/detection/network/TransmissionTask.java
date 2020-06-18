package helsinki.semanticSlam.detection.network;

import android.util.Log;

import org.opencv.core.CvType;
import org.opencv.core.Mat;
import org.opencv.core.MatOfByte;
import org.opencv.imgcodecs.Imgcodecs;
import org.opencv.imgproc.Imgproc;

import java.io.IOException;
import java.net.SocketAddress;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.channels.DatagramChannel;

import helsinki.semanticSlam.detection.Constants;


public class TransmissionTask implements Runnable {

    private final int MESSAGE_META = 0;
    private final int IMAGE_DETECT = 2;

    private int dataType;
    private int frmID;
    private byte[] frmdataToSend;
    private byte[] frmid;
    private byte[] datatype;
    private byte[] frmsize;
    private byte[] packetContent;
    private int datasize;
    private Mat frameData;
    private double latitude;
    private double longtitude;
    private double timeCaptured;
    private double timeSend;
    private byte[] Latitude;
    private byte[] Longtitude;
    private byte[] TimeCaptured;
    private byte[] TimeSend;
    private DatagramChannel datagramChannel;
    private SocketAddress serverAddress;

    public Mat YUVMatTrans, YUVMatScaled, GrayScaled;

    public TransmissionTask(DatagramChannel datagramChannel, SocketAddress serverAddress) {
        this.datagramChannel = datagramChannel;
        this.serverAddress = serverAddress;

        YUVMatScaled = new Mat(Constants.previewHeight / Constants.recoScale, Constants.previewWidth / Constants.recoScale, CvType.CV_8UC1);
        GrayScaled = new Mat(Constants.previewHeight / Constants.recoScale, Constants.previewWidth / Constants.recoScale, CvType.CV_8UC1);
    }

    public void setData(int frmID,Mat frameData, double timeCaptured, double timeSend){
        this.frmID = frmID;
        this.frameData = frameData;
        this.timeCaptured = (double)System.currentTimeMillis();;
        this.timeSend = (double)System.currentTimeMillis();;

        if (this.frmID <= 5) dataType = MESSAGE_META;
        else dataType = IMAGE_DETECT;
    }

    @Override
    public void run() {

        if (dataType == IMAGE_DETECT) {

            Imgproc.resize(frameData, YUVMatScaled, YUVMatScaled.size(), 0, 0, Imgproc.INTER_LINEAR);
            Imgproc.cvtColor(YUVMatScaled, GrayScaled, Imgproc.COLOR_YUV420sp2GRAY);


            MatOfByte imgbuff = new MatOfByte();
            Imgcodecs.imencode(".jpg", GrayScaled, imgbuff, Constants.Image_Params);

            datasize = (int) (imgbuff.total() * imgbuff.channels());
            frmdataToSend = new byte[datasize];

            imgbuff.get(0, 0, frmdataToSend);
        } else if (dataType == MESSAGE_META) {
            datasize = 0;
            frmdataToSend = null;
        }
        packetContent = new byte[28 + datasize];

        frmid = ByteBuffer.allocate(4).order(ByteOrder.LITTLE_ENDIAN).putInt(frmID).array();
        datatype = ByteBuffer.allocate(4).order(ByteOrder.LITTLE_ENDIAN).putInt(dataType).array();
        TimeCaptured = ByteBuffer.allocate(8).order(ByteOrder.LITTLE_ENDIAN).putDouble(timeCaptured).array();
        TimeSend = ByteBuffer.allocate(8).order(ByteOrder.LITTLE_ENDIAN).putDouble(timeSend).array();
        frmsize = ByteBuffer.allocate(4).order(ByteOrder.LITTLE_ENDIAN).putInt(datasize).array();
        System.arraycopy(frmid, 0, packetContent, 0, 4);
        System.arraycopy(datatype, 0, packetContent, 4, 4);
        System.arraycopy(TimeCaptured, 0, packetContent, 8, 8);
        System.arraycopy(TimeSend, 0, packetContent, 16, 8);
        System.arraycopy(frmsize, 0, packetContent, 24, 4);
        if (frmdataToSend != null)
            System.arraycopy(frmdataToSend, 0, packetContent, 28, datasize);

        try {
            ByteBuffer buffer = ByteBuffer.allocate(packetContent.length).put(packetContent);
            buffer.flip();
            datagramChannel.send(buffer, serverAddress);

        } catch (IOException e) {
            e.printStackTrace();
        }
    }
}

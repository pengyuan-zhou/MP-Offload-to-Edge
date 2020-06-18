package helsinki.semanticSlam.ui;


import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.hardware.Camera;
import android.os.Bundle;
import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.util.Log;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.Toast;

import helsinki.semanticSlam.constant.GlobalConstant;
import helsinki.semanticSlam.rendering.render.ArObjectWrapper;
import helsinki.semanticSlam.rendering.render.ObjRendererWrapper;
import helsinki.semanticSlam.slamar.NativeHelper;
import helsinki.semanticSlam.slamar.R;
import helsinki.semanticSlam.rendering.render.GLES10Demo;
import helsinki.semanticSlam.rendering.gles.GLRootView;
import helsinki.semanticSlam.utils.TouchHelper;

import org.opencv.core.Core;
import org.opencv.core.CvType;
import org.opencv.core.Mat;


import java.util.Vector;

import helsinki.semanticSlam.detection.ARManager;
import helsinki.semanticSlam.detection.Constants;
import helsinki.semanticSlam.detection.Detected;
import helsinki.semanticSlam.detection.PointCor;

import static org.opencv.imgproc.Imgproc.resize;


public class ArCamUIActivity extends AppCompatActivity implements
        CameraGLViewBase.CvCameraViewListener2{


    private boolean recoFlag = false;
    private int frameID;
    protected double timeCaptured;
    protected double timeSend;
    private Detected[] detecteds;
    private PointCor[] pointcors;
    public int statusJava;
    public Vector argvMPsJava;
    public Vector argvKeysJava;
    private DrawOnTop mDraw;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.ar_ui_content);
        initView();
        if (Constants.Show2DView) {
            mDraw = new DrawOnTop(this);
            addContentView(mDraw, new ViewGroup.LayoutParams(ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT));
        }
        ARManager.getInstance().init(this, true);
        ARManager.getInstance().setCallback(new ARManager.Callback() {
            @Override
            public void onPointsDetected(PointCor[] pointcor) {
                mDraw.updateData(pointcor);
            }

            @Override
            public void onObjectsDetected(Detected[] detected) {
                mDraw.updataDataDetect(detected);

            }
        });


    }
    public void updateData(PointCor[] pointcor) { this.pointcors = pointcor; }

    private void initToolbar() {
        Toolbar mToolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(mToolbar);
        if (getSupportActionBar() != null) {
            getSupportActionBar().setHomeButtonEnabled(true);
            getSupportActionBar().setDisplayHomeAsUpEnabled(true);
            getSupportActionBar().setDisplayShowTitleEnabled(false);
        }
        mToolbar.setNavigationIcon(R.drawable.btn_back);
        mToolbar.setNavigationOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                onBackPressed();
            }
        });
    }

    @Override
    public boolean onCreateOptionsMenu(final Menu menu) {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.menu_main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case R.id.context_menu:
                detectPlane=true;
                break;
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    public void onBackPressed() {
        System.exit(0);
    }

    private static final String TAG = "SlamCamActivity";

    private Mat mRgba;
    private Mat mIntermediateMat;
    private Mat mGray;

    private CameraGLView mOpenCvCameraView;
    private boolean initFinished;

    private NativeHelper nativeHelper;
    TouchHelper touchHelper;

    private boolean detectPlane;

    private void initView(){
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,WindowManager.LayoutParams.FLAG_FULLSCREEN);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        nativeHelper=new NativeHelper(this);
        mOpenCvCameraView = (CameraGLView) findViewById(R.id.my_fake_glsurface_view);
        mOpenCvCameraView.setVisibility(View.VISIBLE);
        mOpenCvCameraView.setCvCameraViewListener(this);

        initFinished=false;
        Log.d("JNI_", "debug");

        touchHelper=new TouchHelper(this);
        initGLES10Demo();
        //initGLES20Demo();
        initGLES20Obj();

        View touchView=findViewById(R.id.touch_panel);
        touchView.setClickable(true);
        touchView.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                return touchHelper.handleTouchEvent(event);
            }
        });
        touchView.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Camera camera=mOpenCvCameraView.getCamera();
                if (camera!=null) camera.autoFocus(null);
            }
        });
        mOpenCvCameraView.init();
    }


    private void initGLES10Demo() {
        final GLRootView glRootView=findViewById(R.id.ar_object_view_gles1);
        glRootView.setAspectRatio(GlobalConstant.RESOLUTION_WIDTH,GlobalConstant.RESOLUTION_HEIGHT);

        GLES10Demo gles10Demo=
                GLES10Demo.newInstance()
                        .setArObjectView(glRootView)
                        .setNativeHelper(nativeHelper)
                        .setContext(this)
                        .init(touchHelper);

        nativeHelper.addOnMVPUpdatedCallback(gles10Demo);
    }


    private void initGLES20Obj() {
        final GLRootView glRootView=findViewById(R.id.ar_object_view_gles2_obj);
        glRootView.setAspectRatio(GlobalConstant.RESOLUTION_WIDTH,GlobalConstant.RESOLUTION_HEIGHT);

        ObjRendererWrapper objRendererWrapper=
                ObjRendererWrapper.newInstance()
                        .setArObjectView(glRootView)
                        .setNativeHelper(nativeHelper)
                        .setContext(this)
                        .setObjPath("patrick.obj")
                        .setTexturePath("Char_Patrick.png")
                        .setInitSize(0.20f)
                        .init(touchHelper);
        nativeHelper.addOnMVPUpdatedCallback(objRendererWrapper);
    }

    @Override
    public void onPause()
    {
        super.onPause();
        if (mOpenCvCameraView != null)
            mOpenCvCameraView.disableView();
    }

    @Override
    public void onStart() {
        Log.i(Constants.TAG, " onStart() called.");
        super.onStart();
    }

    @Override
    public void onResume()
    {
        super.onResume();
        mOpenCvCameraView.enableView();

        if (!initFinished) {
            initFinished=true;
            nativeHelper.initSLAM(Environment.getExternalStorageDirectory().getPath()+"/SLAM/");
        }
    }

    public void onDestroy() {
        super.onDestroy();
        if (mOpenCvCameraView != null)
            mOpenCvCameraView.disableView();
    }

    public void onCameraViewStarted(int width, int height) {
        mRgba = new Mat(height, width, CvType.CV_8UC4);
        mIntermediateMat = new Mat(height, width, CvType.CV_8UC4);
        mGray = new Mat(height, width, CvType.CV_8UC1);
    }

    public void onCameraViewStopped() {
        mRgba.release();
        mGray.release();
        mIntermediateMat.release();
    }

    public Mat onCameraFrame(CameraGLViewBase.CvCameraViewFrame inputFrame) {

        mRgba = inputFrame.rgba();
        mGray = inputFrame.gray();

        frameID++;
        long time= System.currentTimeMillis();
        timeCaptured = (double)System.currentTimeMillis();
        timeSend = (double)System.currentTimeMillis();
        ARManager.getInstance().recognizeTime(frameID, mGray, timeCaptured, timeSend);
        ARManager.getInstance().driveFrame(mGray);
        mDraw.invalidate();

        return mRgba;
    }
    private void showHint(final String str){
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                Toast.makeText(ArCamUIActivity.this,str,Toast.LENGTH_LONG).show();
            }
        });
    }


    class DrawOnTop extends View {
        Paint paintWord;
        Paint paintLine;
        Paint paintAlarm;
        Paint paintIcon;
        private boolean ShowFPS = true;
        private boolean ShowEdge = true;
        private boolean ShowName = true;
        private int preFrameID;
        private float dispScale = (float) (Constants.recoScale * 1.35);
        private Detected[] detecteds;
        private PointCor[] pointcors;
        private double distance;

        public DrawOnTop(ArCamUIActivity context) {
            super(context);

            paintWord = new Paint();
            paintWord.setStyle(Paint.Style.STROKE);
            paintWord.setStrokeWidth(5);
            paintWord.setColor(Color.BLUE);
            paintWord.setTextAlign(Paint.Align.CENTER);
            paintWord.setTextSize(50);

            paintAlarm = new Paint();
            paintAlarm.setStyle(Paint.Style.STROKE);
            paintAlarm.setStrokeWidth(10);
            paintAlarm.setColor(Color.RED);
            paintAlarm.setTextAlign(Paint.Align.CENTER);
            paintAlarm.setTextSize(100);

            paintLine = new Paint();
            paintLine.setStyle(Paint.Style.STROKE);
            paintLine.setStrokeWidth(10);
            paintLine.setColor(Color.GREEN);


            paintIcon = new Paint();
            paintIcon.setColor(Color.CYAN);
        }



        public double degreesToRadians(double degrees) {
            return degrees * Math.PI / 180;
        }

        public double distanceInKmBetweenEarthCoordinates(double lat1, double lon1, double lat2, double lon2) {
            double earthRadiusKm = 6371;

            double dLat = degreesToRadians(lat2-lat1);
            double dLon = degreesToRadians(lon2-lon1);

            lat1 = degreesToRadians(lat1);
            lat2 = degreesToRadians(lat2);

            double a = Math.sin(dLat/2) * Math.sin(dLat/2) +
                    Math.sin(dLon/2) * Math.sin(dLon/2) * Math.cos(lat1) * Math.cos(lat2);
            double c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1-a));
            return earthRadiusKm * c;
        }

        public void updateData(PointCor[] pointcor) { this.pointcors = pointcor; }
        public void updataDataDetect(Detected[] detected){this.detecteds = detected;}

        @Override
        protected void onDraw(final Canvas canvas) {
            super.onDraw(canvas);

            if(detecteds != null) {
                for (Detected detected : detecteds) {
                    canvas.drawText(detected.name, detected.left*dispScale, detected.top*dispScale, paintWord);

                }

            }
            if(pointcors != null) {
                for (PointCor  pointCor: pointcors ) {
                    canvas.drawCircle(pointCor.x*dispScale, pointCor.y*dispScale, 3, paintLine);
                    canvas.drawRect(pointCor.pt1y*dispScale,pointCor.pt2x*dispScale,pointCor.pt2y*dispScale,pointCor.pt1x*dispScale,paintLine);
                }
            }
        }
    }

}
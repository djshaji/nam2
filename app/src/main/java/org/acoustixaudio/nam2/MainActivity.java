package org.acoustixaudio.nam2;

import android.Manifest;
import android.app.Activity;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.View;
import android.widget.*;
import java.io.*;
import java.util.concurrent.Executors;
import java.util.concurrent.ExecutorService;

public final class MainActivity extends Activity {
    static { System.loadLibrary("nama2"); }
    private native String nativeStart();
    private native void nativeStop();
    private native String nativeLoad(String path);
    private native void nativeParameters(float input, float output, float gate, boolean bypass);
    private native boolean nativeDisconnected();
    private final ExecutorService worker = Executors.newSingleThreadExecutor();
    private final Handler main = new Handler(Looper.getMainLooper());
    private TextView status, model;
    private Button start, stop, load;
    private float inputDb=0, outputDb=-12, gateDb=-60;
    private boolean bypass=false, active=false, busy=false;
    private final Runnable monitor = new Runnable() {
        @Override public void run() {
            if (active && nativeDisconnected()) stopAudio("Audio device changed. Press Start to reconnect.");
            main.postDelayed(this,500);
        }
    };
    @Override public void onCreate(Bundle state) {
        super.onCreate(state);
        LinearLayout panel = new LinearLayout(this);
        panel.setOrientation(LinearLayout.VERTICAL); panel.setPadding(28,36,28,28);
        panel.setBackgroundColor(Color.rgb(16,22,26));
        ScrollView scroll=new ScrollView(this); scroll.setFillViewport(true); scroll.addView(panel); setContentView(scroll);
        text(panel,"NAM A2",32); text(panel,"LIVE AMP PROCESSOR",14);
        text(panel,"Mono input → Gate → NAM → Output",16);
        text(panel,"Use a wired or USB audio interface and headphones. Start with low monitoring volume.",14);
        model=text(panel,"No model loaded • clean monitoring",16);
        status=text(panel,"Stopped",16);
        load=new Button(this); load.setText("Import .nam model"); panel.addView(load);
        load.setOnClickListener(v -> {
            Intent intent=new Intent(Intent.ACTION_OPEN_DOCUMENT).setType("*/*").addCategory(Intent.CATEGORY_OPENABLE);
            startActivityForResult(intent,42);
        });
        slider(panel,"Input",-24,24,0,v -> {inputDb=v; parameters();});
        slider(panel,"Gate (−90 = off)",-90,-20,-60,v -> {gateDb=v; parameters();});
        slider(panel,"Output",-60,6,-12,v -> {outputDb=v; parameters();});
        Switch toggle=new Switch(this); toggle.setText("Bypass NAM"); toggle.setTextColor(Color.WHITE); panel.addView(toggle);
        toggle.setOnCheckedChangeListener((button,checked)->{bypass=checked;parameters();});
        start=new Button(this); start.setText("Start audio"); panel.addView(start);
        stop=new Button(this); stop.setText("Stop audio"); panel.addView(stop);
        start.setOnClickListener(v->{
            if(checkSelfPermission(Manifest.permission.RECORD_AUDIO)!=PackageManager.PERMISSION_GRANTED)
                requestPermissions(new String[]{Manifest.permission.RECORD_AUDIO},7);
            else startAudio();
        });
        stop.setOnClickListener(v->stopAudio("Stopped"));
        parameters(); buttons(); main.post(monitor);
    }
    private TextView text(LinearLayout panel,String value,int size) {
        TextView t=new TextView(this); t.setText(value);t.setTextSize(size);t.setTextColor(Color.rgb(220,237,232));t.setPadding(0,12,0,12);panel.addView(t);return t;
    }
    private interface Value { void changed(int value); }
    private void slider(LinearLayout panel,String name,int min,int max,int initial,Value value) {
        TextView label=text(panel,name+": "+initial+" dB",16);
        SeekBar bar=new SeekBar(this);bar.setMax(max-min);bar.setProgress(initial-min);panel.addView(bar);
        bar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener(){
            public void onProgressChanged(SeekBar b,int p,boolean user){label.setText(name+": "+(p+min)+" dB");value.changed(p+min);}
            public void onStartTrackingTouch(SeekBar b){} public void onStopTrackingTouch(SeekBar b){}
        });
    }
    private void parameters(){nativeParameters(inputDb,outputDb,gateDb,bypass);}
    private void buttons(){start.setEnabled(!busy&&!active);stop.setEnabled(!busy&&active);load.setEnabled(!busy&&!active);}
    private void startAudio(){
        if(busy||active)return;
        busy=true;buttons();status.setText("Starting…");
        worker.execute(()->{String result=nativeStart();main.post(()->{busy=false;active=result.startsWith("Running");status.setText(result);buttons();});});
    }
    private void stopAudio(String reason){
        active=false;busy=true;buttons();
        worker.execute(()->{nativeStop();main.post(()->{busy=false;status.setText(reason);buttons();});});
    }
    @Override public void onRequestPermissionsResult(int request,String[] permissions,int[] grants){
        super.onRequestPermissionsResult(request,permissions,grants);
        if(request==7&&grants.length>0&&grants[0]==PackageManager.PERMISSION_GRANTED)startAudio();
        else status.setText("Microphone permission is required for live audio.");
    }
    @Override protected void onActivityResult(int request,int result,Intent data){
        super.onActivityResult(request,result,data);
        if(request!=42||result!=RESULT_OK||data==null||data.getData()==null)return;
        busy=true;buttons();status.setText("Loading model…");
        final android.net.Uri uri=data.getData();
        worker.execute(()->{
            String resultText;
            File file=new File(getFilesDir(),"import.nam");
            try(InputStream source=getContentResolver().openInputStream(uri);OutputStream dest=new FileOutputStream(file)){
                if(source==null)throw new IOException("Cannot open selected document");
                byte[] buffer=new byte[16384];int count;long total=0;
                while((count=source.read(buffer))!=-1){total+=count;if(total>64L*1024*1024)throw new IOException("Model exceeds 64 MB limit");dest.write(buffer,0,count);}
                dest.flush();resultText=nativeLoad(file.getAbsolutePath());
            }catch(Exception e){resultText="Error: "+e.getMessage();}
            final String outcome=resultText;
            file.delete();
            main.post(()->{busy=false;status.setText(outcome);if(!outcome.startsWith("Error:"))model.setText("Imported NAM model");buttons();});
        });
    }
    @Override protected void onStop(){super.onStop();stopAudio("Stopped • app in background");}
    @Override protected void onDestroy(){main.removeCallbacks(monitor);worker.shutdown();super.onDestroy();}
}

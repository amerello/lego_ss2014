package com.camera.simplemjpeg;
//SOURCES: 
// https://bitbucket.org/neuralassembly/simplemjpegview
// http://www.akexorcist.com/2012/10/android-code-joystick-controller.html
import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.net.URI;

import org.apache.http.HttpResponse;
import org.apache.http.client.ClientProtocolException;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.entity.StringEntity;
import org.apache.http.impl.client.DefaultHttpClient;
import org.apache.http.params.HttpConnectionParams;
import org.apache.http.params.HttpParams;

import com.camera.simplemjpeg.R;

import android.app.Activity;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Handler;
import android.content.Intent;
import android.content.SharedPreferences;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnTouchListener;
import android.widget.ImageView;
import android.widget.RelativeLayout;

public class MjpegActivity extends Activity {
    private MjpegView mv = null;
    String URL;
    private static final int REQUEST_SETTINGS = 0; // for settings (network and resolution)
    
    ////Joystick
    RelativeLayout layout_joystick;
	ImageView image_joystick, image_border;
	JoyStickClass js;
	
    ////Joystick
    
    private int width = 640;
    private int height = 480;
    
    private int ip_ad1 = 11;
    private int ip_ad2 = 0;
    private int ip_ad3 = 0;
    private int ip_ad4 = 1;
    private int ip_port = 8090;
    private String ip_command = "?action=stream";
    
    private boolean suspending = false;
    
	final Handler handler = new Handler();
 
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        SharedPreferences preferences = getSharedPreferences("SAVED_VALUES", MODE_PRIVATE);
        width = preferences.getInt("width", width);
        height = preferences.getInt("height", height);
        ip_ad1 = preferences.getInt("ip_ad1", ip_ad1);
        ip_ad2 = preferences.getInt("ip_ad2", ip_ad2);
        ip_ad3 = preferences.getInt("ip_ad3", ip_ad3);
        ip_ad4 = preferences.getInt("ip_ad4", ip_ad4);
        ip_port = preferences.getInt("ip_port", ip_port);
        ip_command = preferences.getString("ip_command", ip_command);
                
        StringBuilder sb = new StringBuilder();
        String s_http = "http://";
        String s_dot = ".";
        String s_colon = ":";
        String s_slash = "/";
        sb.append(s_http);
        sb.append(ip_ad1);
        sb.append(s_dot);
        sb.append(ip_ad2);
        sb.append(s_dot);
        sb.append(ip_ad3);
        sb.append(s_dot);
        sb.append(ip_ad4);
        sb.append(s_colon);
        sb.append(ip_port);
        sb.append(s_slash);
        sb.append(ip_command);
        URL = new String(sb);

        setContentView(R.layout.main);
        mv = (MjpegView) findViewById(R.id.mv);  
        if(mv != null)
        	mv.setResolution(width, height);
        setTitle(R.string.title_connecting);
        
        //Joystick Code Begin
        layout_joystick = (RelativeLayout)findViewById(R.id.layout_joystick);
        js = new JoyStickClass(getApplicationContext(), layout_joystick, R.drawable.image_button);
	    js.setStickSize(75, 75);
	    js.setLayoutSize(250, 250);
	    js.setLayoutAlpha(150);
	    js.setStickAlpha(100);
	    js.setOffset(45);
	    js.setMinimumDistance(25);

	    layout_joystick.setOnTouchListener(new OnTouchListener() {
			public boolean onTouch(View arg0, MotionEvent arg1) {
				js.drawStick(arg1);
				new DoPost().execute("");
				if(arg1.getAction() == MotionEvent.ACTION_DOWN || arg1.getAction() == MotionEvent.ACTION_MOVE) {

					int direction = js.get8Direction();

				} else if(arg1.getAction() == MotionEvent.ACTION_UP) {

				}
				return true;
			}
        });
        //Joystick Code End
        new DoRead().execute(URL);
    }

    
    public void onResume() {
        super.onResume();
        if(mv!=null && suspending) {
    		new DoRead().execute(URL);
    		suspending = false;
        }
    }

    public void onStart() {
        super.onStart();
    }
    public void onPause() {
        super.onPause();
        if(mv!=null && mv.isStreaming()){
		        mv.stopPlayback();
		        suspending = true;
        }
    }
    public void onStop() {
        super.onStop();
    }

    public void onDestroy() {
    	if(mv!=null)
    		mv.freeCameraMemory();
        super.onDestroy();
    }
    
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
    	MenuInflater inflater = getMenuInflater();
    	inflater.inflate(R.layout.option_menu, menu);
    	return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
    	switch (item.getItemId()) {
    		case R.id.settings:
    			Intent settings_intent = new Intent(MjpegActivity.this, SettingsActivity.class);
    			settings_intent.putExtra("width", width);
    			settings_intent.putExtra("height", height);
    			settings_intent.putExtra("ip_ad1", ip_ad1);
    			settings_intent.putExtra("ip_ad2", ip_ad2);
    			settings_intent.putExtra("ip_ad3", ip_ad3);
    			settings_intent.putExtra("ip_ad4", ip_ad4);
    			settings_intent.putExtra("ip_port", ip_port);
    			settings_intent.putExtra("ip_command", ip_command);
    			startActivityForResult(settings_intent, REQUEST_SETTINGS);
    			return true;
    	}
    	return false;
    }

    public void onActivityResult(int requestCode, int resultCode, Intent data) {
    	switch (requestCode) {
    		case REQUEST_SETTINGS:
    			if (resultCode == Activity.RESULT_OK) {
    				width = data.getIntExtra("width", width);
    				height = data.getIntExtra("height", height);
    				ip_ad1 = data.getIntExtra("ip_ad1", ip_ad1);
    				ip_ad2 = data.getIntExtra("ip_ad2", ip_ad2);
    				ip_ad3 = data.getIntExtra("ip_ad3", ip_ad3);
    				ip_ad4 = data.getIntExtra("ip_ad4", ip_ad4);
    				ip_port = data.getIntExtra("ip_port", ip_port);
    				ip_command = data.getStringExtra("ip_command");

    				if(mv!=null)
    					mv.setResolution(width, height);
    				
    				SharedPreferences preferences = getSharedPreferences("SAVED_VALUES", MODE_PRIVATE);
    				SharedPreferences.Editor editor = preferences.edit();
    				editor.putInt("width", width);
    				editor.putInt("height", height);
    				editor.putInt("ip_ad1", ip_ad1);
    				editor.putInt("ip_ad2", ip_ad2);
    				editor.putInt("ip_ad3", ip_ad3);
    				editor.putInt("ip_ad4", ip_ad4);
    				editor.putInt("ip_port", ip_port);
    				editor.putString("ip_command", ip_command);

    				editor.commit();

    				new RestartApp().execute();
    			}
    			break;
    	}
    }

    public void setImageError() {
    	handler.post(new Runnable() {
    		@Override
    		public void run() {
    			setTitle(R.string.title_imageerror);
    			return;
    		}
    	});
    }
    
    public class DoPost extends AsyncTask<String, Void, Integer> {
    	protected Integer doInBackground(String... url) {
    		int result = 0;
    		HttpResponse res = null;         
            DefaultHttpClient http_post_client = new DefaultHttpClient();
        	HttpPost http_post = new HttpPost("http://11.0.0.1/whereveryouwant");
        	
			try {
				StringEntity strEntity;
				String str = "Values:" + " dx: " + String.valueOf(js.getX()) + " dy: " + String.valueOf(js.getY());
				strEntity = new StringEntity(str);
				http_post.setEntity(strEntity);
				res = http_post_client.execute(http_post);
				if(res.getStatusLine().getStatusCode()==401)
                    return -1;
			} catch (UnsupportedEncodingException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			} catch (ClientProtocolException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}

    		return result;
    	}
    }
    
    public class DoRead extends AsyncTask<String, Void, MjpegInputStream> {
        protected MjpegInputStream doInBackground(String... url) {
            HttpResponse res = null;         
            DefaultHttpClient httpclient = new DefaultHttpClient(); 
            HttpParams httpParams = httpclient.getParams();
            HttpConnectionParams.setConnectionTimeout(httpParams, 5*1000);
            HttpConnectionParams.setSoTimeout(httpParams, 5*1000);
            try {
                res = httpclient.execute(new HttpGet(URI.create(url[0])));
                if(res.getStatusLine().getStatusCode()==401)
                    return null;
                return new MjpegInputStream(res.getEntity().getContent());  
            } catch (ClientProtocolException e) {
            	e.printStackTrace(); //Error connecting to camera
            } catch (IOException e) {
            	e.printStackTrace(); //Error connecting to camera
            }
            return null;
        }

        protected void onPostExecute(MjpegInputStream result) {
            mv.setSource(result);
            if(result!=null) {
            	result.setSkip(1);
            	setTitle(R.string.app_name);
            }else
            	setTitle(R.string.title_disconnected);
            mv.setDisplayMode(MjpegView.SIZE_BEST_FIT);
            mv.showFps(true);
        }
    }
    
    public class RestartApp extends AsyncTask<Void, Void, Void> {
        protected Void doInBackground(Void... v) {
        	MjpegActivity.this.finish();
            return null;
        }

        protected void onPostExecute(Void v) {
        	startActivity((new Intent(MjpegActivity.this, MjpegActivity.class)));
        }
    }
}

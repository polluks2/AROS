package org.aros.bootstrap;

import java.io.FileDescriptor;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.IntBuffer;

import android.os.Handler;
import android.util.Log;


public class DisplayServer extends Thread
{
	private Handler handler;
	private FileInputStream DisplayPipe;
	private FileOutputStream InputPipe;
	private AROSBootstrap main;

	public DisplayServer(AROSBootstrap parent, FileDescriptor displayfd, FileDescriptor inputfd)
	{
		main        = parent;
		handler     = new Handler();
		DisplayPipe = new FileInputStream(displayfd);
		InputPipe   = new FileOutputStream(inputfd); 
	}

	public void ReplyCommand(int cmd, int... response)
	{
		int len = response.length + 2;
		ByteBuffer bb = ByteBuffer.allocate(len * 4);   
		bb.order(ByteOrder.nativeOrder());
        IntBuffer ib = bb.asIntBuffer();
 
        ib.put(cmd);
        ib.put(response.length);
        ib.put(response);
 
        try
        {
			InputPipe.write(bb.array());
		}
        catch (IOException e)
        {
        	Log.d("AROS.Server", "Error writing input pipe");
        	System.exit(0);
		}
	}

	public void run()
	{
		Log.d("AROS.Server", "Display server started");

		for(;;)
		{
			int[] cmd  = ReadData(2);
			int[] args = ReadData(cmd[1]);

			AROSBootstrap.ServerCommand cmdObj = main.new ServerCommand(cmd[0], args);
			handler.post(cmdObj);
		}
	}

	private int[] ReadData(int len)
	{
		byte[]raw = new byte[len * 4];
		
		try
		{
			DisplayPipe.read(raw);
		}
		catch (IOException e)
		{
			Log.v("AROS.Server", "Error reading pipe");
			System.exit(0);
		}

		ByteBuffer bb = ByteBuffer.wrap(raw);
		bb.order(ByteOrder.nativeOrder());
		IntBuffer ib = bb.asIntBuffer();
		int[] data = new int[len];

		ib.get(data);
		return data;
	}
}

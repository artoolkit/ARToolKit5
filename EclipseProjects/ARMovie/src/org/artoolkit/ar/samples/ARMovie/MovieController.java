/*
 *  MovieController.java
 *  ARToolKit5
 *
 *  Disclaimer: IMPORTANT:  This Daqri software is supplied to you by Daqri
 *  LLC ("Daqri") in consideration of your agreement to the following
 *  terms, and your use, installation, modification or redistribution of
 *  this Daqri software constitutes acceptance of these terms.  If you do
 *  not agree with these terms, please do not use, install, modify or
 *  redistribute this Daqri software.
 *
 *  In consideration of your agreement to abide by the following terms, and
 *  subject to these terms, Daqri grants you a personal, non-exclusive
 *  license, under Daqri's copyrights in this original Daqri software (the
 *  "Daqri Software"), to use, reproduce, modify and redistribute the Daqri
 *  Software, with or without modifications, in source and/or binary forms;
 *  provided that if you redistribute the Daqri Software in its entirety and
 *  without modifications, you must retain this notice and the following
 *  text and disclaimers in all such redistributions of the Daqri Software.
 *  Neither the name, trademarks, service marks or logos of Daqri LLC may
 *  be used to endorse or promote products derived from the Daqri Software
 *  without specific prior written permission from Daqri.  Except as
 *  expressly stated in this notice, no other rights or licenses, express or
 *  implied, are granted by Daqri herein, including but not limited to any
 *  patent rights that may be infringed by your derivative works or by other
 *  works in which the Daqri Software may be incorporated.
 *
 *  The Daqri Software is provided by Daqri on an "AS IS" basis.  DAQRI
 *  MAKES NO WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION
 *  THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE, REGARDING THE DAQRI SOFTWARE OR ITS USE AND
 *  OPERATION ALONE OR IN COMBINATION WITH YOUR PRODUCTS.
 *
 *  IN NO EVENT SHALL DAQRI BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL
 *  OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION,
 *  MODIFICATION AND/OR DISTRIBUTION OF THE DAQRI SOFTWARE, HOWEVER CAUSED
 *  AND WHETHER UNDER THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE),
 *  STRICT LIABILITY OR OTHERWISE, EVEN IF DAQRI HAS BEEN ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 *  Copyright 2015 Daqri, LLC.
 *  Copyright 2011-2015 ARToolworks, Inc.
 *
 *  Author(s): Philip Lamb
 *
 */

package org.artoolkit.ar.samples.ARMovie;

import java.io.IOException;
import java.lang.ref.WeakReference;
import java.util.concurrent.CountDownLatch;

import android.media.MediaPlayer;
import android.opengl.GLES11;
import android.opengl.GLES11Ext;
import android.opengl.GLSurfaceView;
import android.annotation.SuppressLint;
import android.graphics.SurfaceTexture;
import android.util.Log;
import android.view.Surface;

/**
   A class which provides a convenient controller for a media player tied
   to an OpenGL surface texture.
 
   @author Philip Lamb
   @copyright Daqri LLC.
 */
public class MovieController implements SurfaceTexture.OnFrameAvailableListener,
										MediaPlayer.OnPreparedListener,
										MediaPlayer.OnVideoSizeChangedListener,
										MediaPlayer.OnInfoListener,
										MediaPlayer.OnErrorListener,
										MediaPlayer.OnCompletionListener {
	
	private MediaPlayer mMediaPlayer = null;
	private boolean mMediaPlayerIsPrepped = false;
	private int     mTextureNeedsUpdatingCount = 0;
	private int     mTextureWasUpdatedCount = 0;
	private SurfaceTexture mSurfaceTexture = null;
	private Surface mSurface = null;
	
	// Playback prefs.
	public boolean mStartPaused = false;
	public boolean mLoop = false;
	public boolean mMute = false;
	
	// OpenGL bits.
	public int mGLTextureID = 0;
	public float[] mGLTextureMtx = new float[16];
	public int mMovieWidth = 0;
	public int mMovieHeight = 0;
	private boolean mGLInited = false;
	
	private static final String TAG = "MovieController";

	/**
	 * Create a MovieController instance.
	 * The routine nativeMovieInit will be called with a reference to this instance.
	 * In this constructor, a MediaPlayer will be instantiated, and its data source set.
	 * Note that the data source will not actually begin to be read until onResume() is called.
	 * @param moviePath The path, in the native file system, where the movie media is stored.
	 */
	public MovieController(String moviePath) throws IOException
	{
		Log.i(TAG, "MovieController()");
		// Create the MediaPlayer, set its options, and prep it.
		mMediaPlayer = new MediaPlayer();
		mMediaPlayer.setOnErrorListener(this);
		mMediaPlayer.setOnInfoListener(this);
		mMediaPlayer.setOnCompletionListener(this);
		mMediaPlayer.setOnVideoSizeChangedListener(this);
		try {
			mMediaPlayer.setDataSource(moviePath);
			ARMovieActivity.nativeMovieInit(this, new WeakReference<MovieController>(this)); // Pass a reference to this instance to the native side.
		} catch (IOException ioe) {
			Log.e(TAG, "Cannot open movie file. " + ioe.toString());
			mMediaPlayer.release();
			mMediaPlayer = null;
			throw ioe;
		}		
	}
	
	/**
	 * Create OpenGL structures required for display of the 
	 * movie content via a SurfaceTexture, and get movie ready to play.
	 * @param sv GLSurfaceView managing the OpenGL thread associated
	 * with this MovieController. 
	 * @return true if the movie was inited without error, false otherwise.
	 */
	public boolean onResume(GLSurfaceView sv) {
		Log.i(TAG, "onResume()");
		
		if (mMediaPlayer == null) {
			Log.e(TAG, "onResume() called while no MediaPlayer is active.");
			return false;
		}
		if (sv == null) {
			Log.e(TAG, "onResume() called with null GLSurfaceView.");
			return false;
		}
		
		// Create a task to run on the OpenGL thread. We need to wait for this task to complete
		// before continuing, so we use a CountDownLatch as a signalling mechanism.
		
		class InitGL implements Runnable {
			
			private final CountDownLatch doneSignal;
			InitGL(CountDownLatch doneSignal) {
				this.doneSignal = doneSignal;
			}

			// This is the method which will be called on the OpenGL thread.
			@SuppressLint("InlinedApi") // Disable warning about missing GL_TEXTURE_EXTERNAL_OES token in API level 14. Although not defined in the SDK until API level 15, it was supported in API level 14.
			public void run() {
				// Generate one texture name and bind it as an external texture.
				// Set required texture parameters. No mipmaps. Clamp to edge is required.
				Log.i(TAG, "GLSurfaceView is running InitGL");
				int[] textures = new int[1];
				GLES11.glGenTextures(1, textures, 0);
				mGLTextureID = textures[0];
				GLES11.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, mGLTextureID);		
				GLES11.glTexParameterf(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES11.GL_TEXTURE_MIN_FILTER, GLES11.GL_NEAREST);        
				GLES11.glTexParameterf(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES11.GL_TEXTURE_MAG_FILTER, GLES11.GL_LINEAR);
				GLES11.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES11.GL_TEXTURE_WRAP_S, GLES11.GL_CLAMP_TO_EDGE);
				GLES11.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES11.GL_TEXTURE_WRAP_T, GLES11.GL_CLAMP_TO_EDGE);
				doneSignal.countDown();
			}
			
		}
		
		CountDownLatch initedGLSignal = new CountDownLatch(1);
		sv.queueEvent(new InitGL(initedGLSignal));
		try {
			initedGLSignal.await();
			
			// Now, tell the MediaPlayer to render into it.
			mSurfaceTexture = new SurfaceTexture(mGLTextureID);
			mSurfaceTexture.setOnFrameAvailableListener(this);
			mSurface = new Surface(mSurfaceTexture);
			mMediaPlayer.setSurface(mSurface);

			mGLInited = true;

		} catch (InterruptedException ex) {
			return false;
		}
		
		// If we were going to do async preparation, this is what we'd do.
		//mMediaPlayer.setOnPreparedListener(this);
		//mMediaPlayer.prepareAsync();

		if (!mMediaPlayerIsPrepped) {
			try {
				mMediaPlayer.prepare();
			} catch (IOException ioe) {
				Log.e(TAG, "Cannot open movie file: " + ioe.toString());
				finish();
				return false;
			}
		}
		this.onPrepared(mMediaPlayer);
		
		return true;
	}
 
	@Override
	public void onPrepared (MediaPlayer mp) {
		Log.i(TAG, "onPrepared()");
		if (!mMediaPlayerIsPrepped) {
			if (mLoop) mp.setLooping(true);
			if (mMute) mp.setVolume(0.0f, 0.0f);
			mMovieWidth = mp.getVideoWidth();
			mMovieHeight = mp.getVideoHeight();
			mMediaPlayerIsPrepped = true;
			Log.i(TAG, "set isPrepped, with looping=" + mLoop + ", mute=" + mMute + ", size=" + mMovieWidth + "x" + mMovieHeight + ".");
		}
		if (!mStartPaused) play();
	}
	
	// SurfaceTexture.OnFrameAvailableListener method.
	// Called on "arbitrary" thread. Actually, called on same thread that initialised
	// the SurfaceTexture, if it had a looper. Otherwise, called on main UI thread, if
	// it had a looper. Otherwise, not called at all.
	@Override
	public void onFrameAvailable (SurfaceTexture surfaceTexture) {
		//Log.i(TAG, "onFrameAvailable");
		
		// onFrameAvailable checks out a buffer, so we need to keep track of how many we need
		// to check back in (with calls to updateTexImage()).
		mTextureNeedsUpdatingCount++;
		// Handle int wrap-around.
		if (mTextureNeedsUpdatingCount == Integer.MAX_VALUE) {
			mTextureNeedsUpdatingCount = (mTextureNeedsUpdatingCount - mTextureWasUpdatedCount);
			mTextureWasUpdatedCount = 0;
		}
	}

	/*
	   Handy state reference:
 	   MEDIA_PLAYER_STATE_ERROR        = 0
	   MEDIA_PLAYER_IDLE               = 1
	   MEDIA_PLAYER_INITIALIZED        = 2
	   MEDIA_PLAYER_PREPARING          = 4
	   MEDIA_PLAYER_PREPARED           = 8
	   MEDIA_PLAYER_STARTED            = 16
	   MEDIA_PLAYER_PAUSED             = 32
	   MEDIA_PLAYER_STOPPED            = 64
	   MEDIA_PLAYER_PLAYBACK_COMPLETE  = 128
	 */
	
	@Override
	public boolean onError(MediaPlayer mp, int what, int extra) {
		switch (what) {
			case MediaPlayer.MEDIA_ERROR_UNKNOWN: Log.e(TAG, "MediaPlayer returned error MEDIA_ERROR_UNKNOWN."); break;
			case MediaPlayer.MEDIA_ERROR_SERVER_DIED: Log.e(TAG, "MediaPlayer returned error MEDIA_ERROR_SERVER_DIED."); break;
			case MediaPlayer.MEDIA_ERROR_IO:  Log.e(TAG, "MediaPlayer returned error MEDIA_ERROR_IO."); break; // API 17.
			case MediaPlayer.MEDIA_ERROR_MALFORMED: Log.e(TAG, "MediaPlayer returned error MEDIA_ERROR_MALFORMED."); break;
			case MediaPlayer.MEDIA_ERROR_UNSUPPORTED: Log.e(TAG, "MediaPlayer returned error MEDIA_ERROR_UNSUPPORTED."); break;
			case MediaPlayer.MEDIA_ERROR_TIMED_OUT: Log.e(TAG, "MediaPlayer returned error MEDIA_ERROR_TIMED_OUT."); break;
			default: Log.e(TAG, "MediaPlayer returned error " + what + ", sub-error 0x" + Integer.toHexString(extra) + "."); break;
		}
		
		return false;
	}

	@Override
	public boolean onInfo(MediaPlayer mp, int what, int extra) {
		switch (what) {
			case MediaPlayer.MEDIA_INFO_UNKNOWN: Log.e(TAG, "MediaPlayer returned info MEDIA_INFO_UNKNOWN."); break;
			case MediaPlayer.MEDIA_INFO_VIDEO_TRACK_LAGGING: Log.e(TAG, "MediaPlayer returned info MEDIA_INFO_VIDEO_TRACK_LAGGING."); break;
			case MediaPlayer.MEDIA_INFO_VIDEO_RENDERING_START:  Log.e(TAG, "MediaPlayer returned info MEDIA_INFO_VIDEO_RENDERING_START."); break; // API 17.
			case MediaPlayer.MEDIA_INFO_BUFFERING_START: Log.e(TAG, "MediaPlayer returned info MEDIA_INFO_BUFFERING_START."); break;
			case MediaPlayer.MEDIA_INFO_BUFFERING_END: Log.e(TAG, "MediaPlayer returned info MEDIA_INFO_BUFFERING_END."); break;
			case MediaPlayer.MEDIA_INFO_BAD_INTERLEAVING: Log.e(TAG, "MediaPlayer returned info MEDIA_INFO_BAD_INTERLEAVING."); break;
			case MediaPlayer.MEDIA_INFO_NOT_SEEKABLE: Log.e(TAG, "MediaPlayer returned info MEDIA_INFO_NOT_SEEKABLE."); break;
			case MediaPlayer.MEDIA_INFO_METADATA_UPDATE: Log.e(TAG, "MediaPlayer returned info MEDIA_INFO_METADATA_UPDATE."); break;
			//case MediaPlayer.MEDIA_INFO_UNSUPPORTED_SUBTITLE: Log.e(TAG, "MediaPlayer returned info MEDIA_INFO_UNSUPPORTED_SUBTITLE."); break; // API 19.
			//case MediaPlayer.MEDIA_INFO_SUBTITLE_TIMED_OUT: Log.e(TAG, "MediaPlayer returned info MEDIA_INFO_SUBTITLE_TIMED_OUT."); break; // API 19.
			default: Log.e(TAG, "MediaPlayer returned info " + what + ", sub-info 0x" + Integer.toHexString(extra) + "."); break;
		}
		return false;
	}

	@Override
	public void onCompletion(MediaPlayer mp) {
		Log.i(TAG, "onCompletion()");
		// This only gets called if looping is disabled.
		// If desired, you could implement some callback here to perform
		// some action when playback finishes. E.g. call play() to start again
		// from the beginning.
	}

	@Override
	public void onVideoSizeChanged(MediaPlayer mp, int width, int height) {
		Log.i(TAG, "Video size changed to " + width + "x" + height + ".");
		mMovieWidth = width;
		mMovieHeight = height;
	}

	/** 
	 * Free all OpenGL resources used by the movie.
	 * @param sv GLSurfaceView managing the OpenGL thread associated
	 * with this MovieController. If the GL context cannot be reached,
	 * just pass NULL.
	 */
	public void onPause(GLSurfaceView sv) {
		Log.i(TAG, "onPause()");
		
		mTextureNeedsUpdatingCount = 0;
		mTextureWasUpdatedCount = 0;
		if (mSurface != null) {
			mSurface.release();
			mSurface = null;
		}
		if (mSurfaceTexture != null) {
			if (mMediaPlayer != null) mMediaPlayer.setSurface(null);
			mSurfaceTexture.release();
			mSurfaceTexture = null;
		}
		if (mGLTextureID != 0) {
			if (sv != null) {
				sv.queueEvent(new Runnable() {
					public void run() {
						int[] textures = new int[1];
						textures[0] = mGLTextureID;
						GLES11.glDeleteTextures(1, textures, 0);
						mGLTextureID = 0;
					}
				});
			}
		}
		mGLInited = false;
	}

	public void finish() {
		Log.i(TAG, "finish()");
		if (mGLInited) onPause(null); 
		if (mMediaPlayer != null) {
			mMediaPlayer.reset();
			mMediaPlayer.release();
			mMediaPlayer = null;
			mMediaPlayerIsPrepped = false;
			ARMovieActivity.nativeMovieFinal(); // Clean up the reference to this instance held by the native side.
		}
	}
	
	@Override
	protected void finalize() throws Throwable {
		// Handle case where user forgot to call finish().
		finish();
		
		super.finalize();
	}
	
	// Controller methods.
	
	/**
	 * If a new video frame is available, pushes it to the OpenGL texture.
	 * May only be called while the OpenGL ES context that owns the texture
	 * is current on the calling thread. It will implicitly bind its texture to the
	 * GL_TEXTURE_EXTERNAL_OES texture target.
	 * @return true is texture was updated, false if not.
	 */
	public boolean updateTexture() {
		if (mTextureNeedsUpdatingCount > mTextureWasUpdatedCount) {
			//Log.i(TAG, "updateTexture(): ran.");
			while (mTextureNeedsUpdatingCount > mTextureWasUpdatedCount) {
				mSurfaceTexture.updateTexImage();
				mSurfaceTexture.getTransformMatrix(mGLTextureMtx); // Stash the matrix for use by the client.
				mTextureWasUpdatedCount++;
			}
			return (true);
		} else {
			//Log.i(TAG, "updateTexture(): did nothing.");
			return (false);
		}
	}
	
	// Playback UI control methods.

	public void play() {
		Log.i(TAG, "play()");
		if (mMediaPlayer == null || !mMediaPlayerIsPrepped) return;
		mMediaPlayer.start();
	}
	
	public boolean isPlaying() {
		Log.i(TAG, "isPlaying");
		if (mMediaPlayer == null || !mMediaPlayerIsPrepped) return false;
		return mMediaPlayer.isPlaying();
	}
	
	public void pause() {
		Log.i(TAG, "pause");
		if (mMediaPlayer == null || !mMediaPlayerIsPrepped) return; // we won't check mMediaPlayer.isPlaying().
		mMediaPlayer.pause();
	}

	// These two methods are called from the native side.
	
	@SuppressWarnings({ "unused", "unchecked" })
	private static void playFromNative(Object objectInstanceWeakReference)
	{
		Log.i(TAG, "playFromNative()");
		MovieController mr = ((WeakReference<MovieController>)objectInstanceWeakReference).get();
		if (mr == null) {
			Log.i(TAG, "playFromNative() !REF");
			return;
		}
		
		mr.play();
	}
	
	@SuppressWarnings({ "unused", "unchecked" })
	private static void pauseFromNative(Object objectInstanceWeakReference)
	{
		Log.i(TAG, "pauseFromNative()");
		MovieController mr = ((WeakReference<MovieController>)objectInstanceWeakReference).get();
		if (mr == null) {
			Log.i(TAG, "pauseFromNative() !REF");
			return;
		}
		
		mr.pause();
	}

}

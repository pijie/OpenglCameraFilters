package com.cci.glcamerafilters

import android.graphics.SurfaceTexture
import android.os.Process
import android.util.Size
import android.view.Surface
import androidx.annotation.MainThread
import androidx.annotation.WorkerThread
import androidx.camera.core.Preview
import androidx.concurrent.ListenableFuture
import androidx.concurrent.callback.CallbackToFutureAdapter
import java.util.*
import java.util.concurrent.atomic.AtomicInteger

class OpenGLRenderer {
    private var mNativeContext: Long = 0
    private var mIsShutdown = false
    private var mNumOutstandingSurfaces = 0
    private val mTextureTransform = FloatArray(16)
    private var mPreviewTexture: SurfaceTexture? = null

    private val executor by lazy {
        SingleThreadHandlerExecutor(
            String.format(
                Locale.US,
                "GLRenderer-%03d",
                RENDERED_COUNT.incrementAndGet()
            ), Process.THREAD_PRIORITY_DEFAULT
        )

    }

    init {
        executor.execute {
            mNativeContext = initContext()

        }
    }

    companion object {
        init {
            System.loadLibrary("opengl_renderer")
        }

        private val RENDERED_COUNT = AtomicInteger(0)


        //
        @WorkerThread
        @JvmStatic
        private external fun initContext(): Long

        // 设置纹理输出的surface
        @WorkerThread
        @JvmStatic
        private external fun setWindowSurface(nativeContext: Long, surface: Surface?)

        // native生成的纹理id
        @WorkerThread
        @JvmStatic
        private external fun getTexName(nativeContext: Long): Int;

        // 重绘
        @WorkerThread
        @JvmStatic
        private external fun renderTexture(nativeContext: Long, timestamp: Long, textureTransform:FloatArray)

        // 释放资源
        @WorkerThread
        @JvmStatic
        private external fun closeContext(nativeContext: Long)
    }


    @MainThread
    fun attachInputPreview(preview: Preview) {

        preview.setSurfaceProvider(executor) { surfaceRequest ->
            if (mIsShutdown) {
                surfaceRequest.willNotProvideSurface();

            } else {
                val surfaceTexture = resetPreviewTexture(surfaceRequest.resolution)
                val inputSurface = Surface(surfaceTexture)
                mNumOutstandingSurfaces++;
                surfaceRequest.provideSurface(inputSurface, executor) { result->
//                    inputSurface.release()
//                    surfaceTexture.release()
//                    if (surfaceTexture == mPreviewTexture){
//                        mPreviewTexture = null
//                    }
//                    mNumOutstandingSurfaces --
//                    doShutdownExecutorIfNeeded()
                }

            }


        }

    }

    @MainThread
    fun attachOutputSurface(surface: Surface){
        executor.execute{
            if (mIsShutdown)return@execute
                setWindowSurface(mNativeContext, surface)
        }
    }

    fun invalidateSurface(){
        executor.execute{
//            renderTexture(mNativeContext,mPreviewTexture!!.timestamp)
        }
    }

    fun shutdown(){
        executor.execute{
            if (!mIsShutdown){
                closeContext(mNativeContext)
                mNativeContext = 0
                mIsShutdown = true
                doShutdownExecutorIfNeeded()
            }

        }
    }

    @WorkerThread
    private fun resetPreviewTexture(size: Size): SurfaceTexture {
        // 已经创建过移除
        mPreviewTexture?.detachFromGLContext()
        mPreviewTexture = SurfaceTexture(getTexName(mNativeContext))
        mPreviewTexture!!.setDefaultBufferSize(size.width, size.height)
        mPreviewTexture!!.setOnFrameAvailableListener({ surfaceTexture ->
            if (surfaceTexture == mPreviewTexture && !mIsShutdown) {
                surfaceTexture.updateTexImage()

                // Get texture transform from surface texture (transform to natural orientation).
                // This will be used to transform texture coordinates in the fragment shader.
                mPreviewTexture!!.getTransformMatrix(mTextureTransform)
                renderTexture(mNativeContext, surfaceTexture.timestamp,mTextureTransform)
            }

        }, executor.getHandler())
        return mPreviewTexture!!
    }

    @WorkerThread
    private fun doShutdownExecutorIfNeeded() {
        if (mIsShutdown && mNumOutstandingSurfaces == 0) {
            executor.shutdown()
        }
    }

    fun detachOutputSurface():ListenableFuture<Any>{
        return CallbackToFutureAdapter.getFuture { completer->

            executor.execute{
//                if (!mIsShutdown){
//                    setWindowSurface(mNativeContext,null)
//                }

                completer.set(null)
            }
        }
    }

}
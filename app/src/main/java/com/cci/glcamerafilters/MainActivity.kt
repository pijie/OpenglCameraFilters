package com.cci.glcamerafilters

import android.Manifest
import android.os.Bundle
import android.os.Looper
import android.view.SurfaceHolder
import android.view.SurfaceView
import android.view.View
import android.view.ViewStub
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.camera.core.AspectRatio
import androidx.camera.core.CameraSelector
import androidx.camera.core.Preview
import androidx.camera.lifecycle.ProcessCameraProvider
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import kotlin.math.abs
import kotlin.math.max
import kotlin.math.min

class MainActivity : AppCompatActivity() {
    private lateinit var mViewFinder: View
    private val lensFacing = CameraSelector.DEFAULT_BACK_CAMERA
    var openGLRenderer: OpenGLRenderer? = null

    companion object {

        private const val RATIO_4_3_VALUE = 4.0 / 3.0
        private const val RATIO_16_9_VALUE = 16.0 / 9.0
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        ActivityCompat.requestPermissions(this, arrayOf(Manifest.permission.CAMERA), 1001)


        openGLRenderer = OpenGLRenderer()

        val surfaceView = findViewById<SurfaceView>(R.id.surface_view)

        surfaceView.holder.addCallback(object : SurfaceHolder.Callback2 {
            override fun surfaceCreated(holder: SurfaceHolder) {
                openGLRenderer!!.attachOutputSurface(holder.surface)

            }

            override fun surfaceChanged(
                holder: SurfaceHolder,
                format: Int,
                width: Int,
                height: Int
            ) {
//                openGLRenderer!!.attachOutputSurface(holder.surface)

            }

            override fun surfaceDestroyed(holder: SurfaceHolder) {
            }

            override fun surfaceRedrawNeeded(holder: SurfaceHolder) {
//                openGLRenderer!!.attachOutputSurface(holder.surface)

            }
        })
        startBindCameraX()
    }


    private fun startBindCameraX() {
        val cameraProviderFuture = ProcessCameraProvider.getInstance(this)
        cameraProviderFuture.addListener({
            bindCameraX(cameraProviderFuture.get())
        }, ContextCompat.getMainExecutor(this))
    }

    private fun bindCameraX(cameraProvider: ProcessCameraProvider) {
        cameraProvider.unbindAll()
        if (!cameraProvider.hasCamera(lensFacing)) {
            toast("相机不可用")
            return
        }

        val metrics = resources.displayMetrics
        val aspectRatio = aspectRatio(metrics.widthPixels, metrics.heightPixels)
        val rotation = 0
        val preview = Preview.Builder()
            .setTargetAspectRatio(aspectRatio)
//            .setTargetRotation(rotation)
            .build()

        openGLRenderer!!.attachInputPreview(preview)
        cameraProvider.bindToLifecycle(this, lensFacing, preview)
    }

    private fun aspectRatio(width: Int, height: Int): Int {
        val previewRatio = max(width, height).toDouble() / min(width, height)
        if (abs(previewRatio - RATIO_4_3_VALUE) <= abs(previewRatio - RATIO_16_9_VALUE)) {
            return AspectRatio.RATIO_4_3
        }
        return AspectRatio.RATIO_16_9
    }


    private fun toast(msg: String) {
        if (Looper.myLooper() == Looper.getMainLooper()) {
            Toast.makeText(this, msg, Toast.LENGTH_SHORT).show()
        } else {
            runOnUiThread {
                Toast.makeText(this, msg, Toast.LENGTH_SHORT).show()
            }
        }
    }

    override fun onDestroy() {
        openGLRenderer?.shutdown()
        super.onDestroy()
    }
}
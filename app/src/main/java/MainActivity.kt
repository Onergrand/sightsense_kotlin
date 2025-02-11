package com.sightsense

import android.Manifest
import android.content.Context
import android.content.pm.PackageManager
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.media.MediaMetadataRetriever
import android.media.MediaPlayer
import android.os.Bundle
import android.util.Log
import android.view.View
import android.widget.AdapterView
import android.widget.ArrayAdapter
import android.widget.ImageView
import android.widget.Spinner
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import androidx.camera.core.CameraSelector
import androidx.camera.core.ImageCapture
import androidx.camera.core.ImageCaptureException
import androidx.camera.core.ImageProxy
import androidx.camera.lifecycle.ProcessCameraProvider
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import kotlinx.coroutines.*
import java.io.File
import java.nio.ByteBuffer
import java.util.concurrent.ExecutorService
import java.util.concurrent.Executors

class MainActivity : AppCompatActivity() {

    private lateinit var startButton: androidx.appcompat.widget.AppCompatButton
    private lateinit var stopButton: androidx.appcompat.widget.AppCompatButton
    private lateinit var imageView: ImageView
    private lateinit var noImageText: TextView

    private var imageCapture: ImageCapture? = null
    private lateinit var cameraExecutor: ExecutorService

    private val coroutineScope = CoroutineScope(Dispatchers.Main)
    private var captureJob: Job? = null
    private var isCapturing = false

    private val REQUIRED_PERMISSIONS = arrayOf(Manifest.permission.CAMERA)
    private val TAG = "MainActivity"

    private var selectedCamera: String = "Основная"
    private var selectedMode: String = "Режим 1"




    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)


        startButton = findViewById(R.id.main_start)
        stopButton = findViewById(R.id.main_stop)
        imageView = findViewById(R.id.camera)
        noImageText = findViewById(R.id.noImageText)

        cameraExecutor = Executors.newSingleThreadExecutor()

        if (allPermissionsGranted()) {
            startCamera()
        } else {
            ActivityCompat.requestPermissions(
                this, REQUIRED_PERMISSIONS, REQUEST_CODE_PERMISSIONS
            )
        }

        startButton.setOnClickListener {
            startButton.isEnabled = false
            stopButton.isEnabled = true
            noImageText.visibility = View.GONE
            startCapturingImages(this)
        }

        stopButton.setOnClickListener {
            stopButton.isEnabled = false
            startButton.isEnabled = true
            stopCapturingImages()
            imageView.setImageBitmap(null)
            noImageText.visibility = View.VISIBLE
        }

        stopButton.isEnabled = false

        val cameraSpinner: Spinner = findViewById(R.id.cameraForm)

        val cameraOptions = resources.getStringArray(R.array.camera_options)
        val adapter = ArrayAdapter(this, R.layout.spinner_text, cameraOptions)
        adapter.setDropDownViewResource(R.layout.spinner_text)
        cameraSpinner.adapter = adapter

        cameraSpinner.onItemSelectedListener = object : AdapterView.OnItemSelectedListener {
            override fun onItemSelected(parent: AdapterView<*>?, view: View?, position: Int, id: Long) {
                selectedCamera = cameraOptions[position]
                startCamera()
            }
            override fun onNothingSelected(parent: AdapterView<*>?) {

            }
        }

        val modeSpinner: Spinner = findViewById(R.id.modeForm)
        val modeOptions = resources.getStringArray(R.array.mode_options)
        val modeAdapter = ArrayAdapter(this, R.layout.spinner_text, modeOptions)
        modeAdapter.setDropDownViewResource(R.layout.spinner_text)
        modeSpinner.adapter = modeAdapter
        modeSpinner.onItemSelectedListener = object : AdapterView.OnItemSelectedListener {
            override fun onItemSelected(parent: AdapterView<*>?, view: View?, position: Int, id: Long) {
                selectedMode = modeOptions[position]
            }

            override fun onNothingSelected(parent: AdapterView<*>?) {
            }
        }
    }

    fun createAudioFile(context: Context) {
        val fileName = "hificode.wav"
        val filePath = File(context.filesDir, fileName).absolutePath

        val file = File(context.filesDir, "hificode.wav")
        if (file.exists()) {
            file.delete()
        }

        if (file.exists()) {
            println("Файл существует")
        } else {
            println("Файл не найден")
        }

        if (file.exists()) {
            val mediaPlayer = MediaPlayer().apply {
                setDataSource(file.absolutePath) // Указываем путь к файлу
                prepare() // Подготовка к воспроизведению
                start() // Воспроизведение
            }
        } else {
            Log.e("AudioPlayer", "Файл не найден: ${file.absolutePath}")
        }
    }


    private fun allPermissionsGranted() = REQUIRED_PERMISSIONS.all {
        ContextCompat.checkSelfPermission(
            baseContext, it
        ) == PackageManager.PERMISSION_GRANTED
    }

    override fun onRequestPermissionsResult(
        requestCode: Int, permissions: Array<String>, grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode == REQUEST_CODE_PERMISSIONS) {
            if (allPermissionsGranted()) {
                startCamera()
            } else {
                finish()
            }
        }
    }

    private fun startCamera() {
        val cameraProviderFuture = ProcessCameraProvider.getInstance(this)
        cameraProviderFuture.addListener({
            val cameraProvider: ProcessCameraProvider = cameraProviderFuture.get()

            cameraProvider.unbindAll()

            imageCapture = ImageCapture.Builder()
                .setCaptureMode(ImageCapture.CAPTURE_MODE_MINIMIZE_LATENCY)
                .build()

            val cameraSelector = if (selectedCamera == "Основная") {
                CameraSelector.DEFAULT_BACK_CAMERA
            } else {
                CameraSelector.DEFAULT_FRONT_CAMERA
            }

            try {
                cameraProvider.bindToLifecycle(
                    this, cameraSelector, imageCapture
                )
            } catch (exc: Exception) {
                Log.e(TAG, "Ошибка привязки камеры: ${exc.message}")
            }
        }, ContextCompat.getMainExecutor(this))
    }

    private fun startCapturingImages(context: Context) {
        isCapturing = true
        captureJob = coroutineScope.launch {
            while (isActive && isCapturing) {
                takePhoto(context)
                delay(1000L)
            }
        }
    }

    private fun stopCapturingImages() {
        isCapturing = false
        captureJob?.cancel()
        imageView.setImageBitmap(null)
        noImageText.visibility = View.VISIBLE
    }


    private fun takePhoto(context: Context) {
        val imageCapture = imageCapture ?: return

        imageCapture.takePicture(
            ContextCompat.getMainExecutor(this),
            object : ImageCapture.OnImageCapturedCallback() {
                override fun onCaptureSuccess(imageProxy: ImageProxy) {

                    val fileName = "hificode.wav"
                    val file = File(context.filesDir, fileName)

                    val bitmap = imageProcessing.process(imageProxy.toBitmapWithRotation(), 1, 1.0, file.absolutePath)
                    runOnUiThread {
                        if (isCapturing) {
                            imageView.setImageBitmap(bitmap)
                        } else {
                            imageView.setImageBitmap(null)
                        }
                    }

                    if (file.exists()) {
                        val mediaPlayer = MediaPlayer().apply {
                            setDataSource(file.absolutePath) // Указываем путь к файлу
                            prepare() // Подготовка к воспроизведению
                            start() // Воспроизведение

                            val durationMs = getAudioDuration(file)
                            println("Длина аудиофайла: ${durationMs} миллисекунд")

                        }
                    } else {
                        Log.e("AudioPlayer", "Файл не найден: ${file.absolutePath}")
                    }

                    imageProxy.close()
                }


                override fun onError(exception: ImageCaptureException) {
                    Log.e(TAG, "Ошибка захвата изображения: ${exception.message}")
                }
            }
        )
    }

    fun getAudioDuration(file: File): Long {
        val retriever = MediaMetadataRetriever()
        try {
            retriever.setDataSource(file.absolutePath)
            val durationStr = retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_DURATION)
            return durationStr?.toLongOrNull() ?: 0L
        } catch (e: Exception) {
            e.printStackTrace()
            return 0L
        } finally {
            retriever.release()
        }
    }


    private fun ImageProxy.toBitmapWithRotation(): Bitmap {
        val buffer: ByteBuffer = planes[0].buffer
        val bytes = ByteArray(buffer.remaining())
        buffer.get(bytes)

        val originalBitmap = BitmapFactory.decodeByteArray(bytes, 0, bytes.size)

        val rotationMatrix = android.graphics.Matrix().apply {
            postRotate(imageInfo.rotationDegrees.toFloat())
        }

        return Bitmap.createBitmap(
            originalBitmap,
            0,
            0,
            originalBitmap.width,
            originalBitmap.height,
            rotationMatrix,
            true
        )
    }


    override fun onDestroy() {
        super.onDestroy()
        cameraExecutor.shutdown()
        captureJob?.cancel()
    }

    companion object {
        private const val REQUEST_CODE_PERMISSIONS = 10

        init {
            System.loadLibrary("opencv_java4")
        }
    }
}

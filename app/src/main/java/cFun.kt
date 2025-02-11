package com.sightsense

import android.content.Context
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import org.opencv.core.Mat
import org.opencv.core.Size
import org.opencv.imgproc.Imgproc
import java.io.File


object cFun {
    init {
        System.loadLibrary("native-lib")
    }


    external fun stereo(imageData: DoubleArray)

    external fun newStereo(imageData: DoubleArray, path: String)
    external fun newBinaural(imageData1: DoubleArray, imageData2: DoubleArray, path: String)


    external fun binaural(imageData1: DoubleArray, imageData2: DoubleArray)


    fun stereoWithMat(image: Mat, N: Int, M: Int, savePath: String) {
        try {
            val imageData = loadImageToDoubleArray(image, N, M)

            newStereo(imageData, savePath)
            println("stereo function was executed successfully.")
        } catch (e: Exception) {
            println("Error: ${e.message}")
        }
    }

    fun binauralWithMat(image1: Mat, image2: Mat, N: Int, M: Int, savePath: String) {
        try {
            val imageData1 = loadImageToDoubleArray(image1, N, M)
            val imageData2 = loadImageToDoubleArray(image2, N, M)
            newBinaural(imageData1, imageData2, savePath)
            println("binaural function was executed successfully.")
        } catch (e: Exception) {
            println("Error: ${e.message}")
        }
    }


    private fun loadImageToDoubleArray(image: Mat, N: Int, M: Int): DoubleArray {
        val resizedMat = Mat()
        Imgproc.resize(image, resizedMat, Size(N.toDouble(), M.toDouble()))

        val grayMat = if (resizedMat.channels() > 1) {
            val tmp = Mat()
            Imgproc.cvtColor(resizedMat, tmp, Imgproc.COLOR_BGR2GRAY)
            tmp
        } else {
            resizedMat
        }

        val imageData = DoubleArray(grayMat.rows() * grayMat.cols())

        for (i in 0 until grayMat.rows()) {
            for (j in 0 until grayMat.cols()) {
                imageData[i * grayMat.cols() + j] = grayMat.get(i, j)[0]
            }
        }

        return imageData
    }
}

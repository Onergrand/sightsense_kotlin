package com.sightsense

import android.graphics.Bitmap
import org.opencv.android.Utils
import org.opencv.core.Core
import org.opencv.core.CvType
import org.opencv.core.Mat
import org.opencv.core.Rect
import org.opencv.core.Scalar
import org.opencv.core.Size
import org.opencv.imgproc.Imgproc
import kotlin.math.pow

object imageProcessing {
    const val N = 176
    const val M = 64

    private fun convertImage(frame: Mat, contrast: Double) : Mat {

//        можно увеличить производительность за счет Core.multiply() и Core.add()

        val grayMat = if (frame.channels() == 1) {
            frame.clone() // Уже серое изображение, просто копируем
        } else {
            val temp = Mat()
            Imgproc.cvtColor(frame, temp, Imgproc.COLOR_BGR2GRAY)
            temp
        }

        val resizedMat = Mat()
        Imgproc.resize(grayMat, resizedMat, Size(N.toDouble(), M.toDouble()))

        val meanValue = Core.mean(resizedMat).`val`[0]

        val contrastMat = Mat(resizedMat.size(), CvType.CV_64F)
        resizedMat.convertTo(contrastMat, CvType.CV_64F)

        for (i in 0 until contrastMat.rows()) {
            for (j in 0 until contrastMat.cols()) {
                val pixel = contrastMat.get(i, j)[0]
                val newPixel = pixel + contrast * (pixel - meanValue)
                contrastMat.put(i, j, newPixel.coerceIn(0.0, 255.0))
            }
        }

        val typeMat = Mat(contrastMat.size(), CvType.CV_64F)
        for (i in 0 until contrastMat.rows()) {
            for (j in 0 until contrastMat.cols()) {
                val pixel = contrastMat.get(i, j)[0]
                val newPixel = if (pixel == 0.0) 0.0 else 10.0.pow((pixel / 16.0 - 15.0) / 10.0)
                typeMat.put(i, j, newPixel)
            }
        }

        Core.flip(typeMat, typeMat, 1)
        Core.flip(typeMat, typeMat, 0)

        return typeMat
    }

    private fun mergeImageHalves(array1: Mat, array2: Mat): Mat {
        val normArray1 = Mat()
        val normArray2 = Mat()
        Core.normalize(array1, normArray1, 0.0, 255.0, Core.NORM_MINMAX, CvType.CV_8U)
        Core.normalize(array2, normArray2, 0.0, 255.0, Core.NORM_MINMAX, CvType.CV_8U)

        val bitmap1 = Bitmap.createBitmap(normArray1.cols(), normArray1.rows(), Bitmap.Config.ARGB_8888)
        val bitmap2 = Bitmap.createBitmap(normArray2.cols(), normArray2.rows(), Bitmap.Config.ARGB_8888)
        Utils.matToBitmap(normArray1, bitmap1)
        Utils.matToBitmap(normArray2, bitmap2)

        if (bitmap1.height != bitmap2.height) {
            throw IllegalArgumentException("Высоты изображений не совпадают. Убедитесь, что изображения одинаковой высоты.")
        }

        val leftHalf = Bitmap.createBitmap(bitmap1, 0, 0, bitmap1.width / 2, bitmap1.height)
        val rightHalf = Bitmap.createBitmap(bitmap2, bitmap2.width / 2, 0, bitmap2.width / 2, bitmap2.height)

        val resultBitmap = Bitmap.createBitmap(leftHalf.width + rightHalf.width, leftHalf.height, Bitmap.Config.ARGB_8888)
        val canvas = android.graphics.Canvas(resultBitmap)
        canvas.drawBitmap(leftHalf, 0f, 0f, null)
        canvas.drawBitmap(rightHalf, leftHalf.width.toFloat(), 0f, null)

        val resultMat = Mat()
        Utils.bitmapToMat(resultBitmap, resultMat)

        return resultMat
    }

    fun process(image: Bitmap, mode: Int, contrast: Double, savePath: String) : Bitmap {
        val srcMat = Mat()
        Utils.bitmapToMat(image, srcMat)

        val size = Size(N.toDouble(), M.toDouble())
        val resizedMat = Mat()
        Imgproc.resize(srcMat, resizedMat, size)

        val grayMat = Mat()
        Imgproc.cvtColor(resizedMat, grayMat, Imgproc.COLOR_BGR2GRAY)


        val halfWidth = grayMat.cols() / 2
        var firstImage = Mat(grayMat, Rect(0, 0, halfWidth, grayMat.rows()))
        val secondImage = Mat(grayMat, Rect(halfWidth, 0, grayMat.cols() - halfWidth, grayMat.rows()))

        val finalOutput: Mat

        try {
            when (mode) {
                0 -> {
                    val meanFrame = Mat()
                    Core.add(firstImage, secondImage, meanFrame)
                    Core.multiply(meanFrame, Scalar(0.5), meanFrame)
                    val newFrame = convertImage(meanFrame, contrast)

                    cFun.stereoWithMat(newFrame, N, M, savePath)

                    finalOutput = newFrame
                }
                1 -> {
                    val newFirstImage = convertImage(firstImage, contrast)
                    val newSecondImage = convertImage(secondImage, contrast)
                    finalOutput = mergeImageHalves(firstImage, secondImage)

                    cFun.binauralWithMat(newFirstImage, newSecondImage, N, M, savePath)
                }
                2 -> {
                    val newLeft = Mat.zeros(grayMat.rows(), 13, CvType.CV_64F)
                    val newRight = Mat.zeros(grayMat.rows(), 13, CvType.CV_64F)

                    val flippedFirst = Mat()
                    Core.flip(firstImage, flippedFirst, 1)
                    firstImage = flippedFirst

                    firstImage.col(0).convertTo(newLeft.col(0), CvType.CV_64F)
                    secondImage.col(0).convertTo(newRight.col(0), CvType.CV_64F)

                    var fs = 1
                    var step = 2
                    for (i in 1 until 13) {
                        val subMatLeft = firstImage.colRange(fs, fs + step)
                        val subMatRight = secondImage.colRange(fs, fs + step)
                        val meanLeft = Mat()
                        val meanRight = Mat()
                        Core.reduce(subMatLeft, meanLeft, 1, Core.REDUCE_AVG)
                        Core.reduce(subMatRight, meanRight, 1, Core.REDUCE_AVG)
                        meanLeft.copyTo(newLeft.col(i))
                        meanRight.copyTo(newRight.col(i))
                        fs += step
                        step += 1
                    }
                    val newFirstImage = convertImage(newLeft, contrast)
                    val newSecondImage = convertImage(newRight, contrast)
                    finalOutput = mergeImageHalves(firstImage, secondImage)
                    cFun.binauralWithMat(newFirstImage, newSecondImage, N, M, savePath)
                }
                else -> {
                    throw Exception("Something went wrong!")
                }
            }
        } catch (e: Exception) {
            throw e
        }

        val resultBitmap = Bitmap.createBitmap(finalOutput.cols(), finalOutput.rows(), Bitmap.Config.ARGB_8888)
        Utils.matToBitmap(finalOutput, resultBitmap)
        return resultBitmap
    }
}
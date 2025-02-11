#include <jni.h>
#include "hificode.h"  // Подключаем ваш заголовочный файл
#define LOG_TAG "MyNativeCode"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

extern "C" {

// Функция-обёртка для stereo. Ожидается, что с Kotlin передаётся одномерный массив длиной M*N,
// который мы преобразуем в двумерный массив double A[M][N].
JNIEXPORT void JNICALL
Java_com_sightsense_cFun_stereo(JNIEnv *env, jobject /* this */, jdoubleArray imageData) {
    double A[M][N];
    jsize len = env->GetArrayLength(imageData);
    if (len != M * N) {
        // Если длина массива неверная, можно выбросить исключение или просто завершить функцию.
        return;
    }
    // Получаем указатель на элементы массива
    jdouble* data = env->GetDoubleArrayElements(imageData, nullptr);
    // Заполняем двумерный массив A (предполагается row-major порядок)
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            A[i][j] = data[i * N + j];
        }
    }
    // Освобождаем ресурсы массива Java
    env->ReleaseDoubleArrayElements(imageData, data, 0);

    // Вызываем функцию stereo из hificode.c
    stereo(A);
}

// Функция-обёртка для binaural. Ожидается, что с Kotlin передаются два одномерных массива длиной M*N.
JNIEXPORT void JNICALL
Java_com_sightsense_cFun_binaural(JNIEnv *env, jobject /* this */,
                                       jdoubleArray imageData1, jdoubleArray imageData2) {
    double A[M][N];
    double B[M][N];
    jsize len1 = env->GetArrayLength(imageData1);
    jsize len2 = env->GetArrayLength(imageData2);
    if (len1 != M * N || len2 != M * N) {
        // Если длины массивов неверные, можно выбросить исключение или завершить функцию.
        return;
    }
    jdouble* data1 = env->GetDoubleArrayElements(imageData1, nullptr);
    jdouble* data2 = env->GetDoubleArrayElements(imageData2, nullptr);
    // Заполняем двумерные массивы A и B
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            A[i][j] = data1[i * N + j];
            B[i][j] = data2[i * N + j];
        }
    }
    env->ReleaseDoubleArrayElements(imageData1, data1, 0);
    env->ReleaseDoubleArrayElements(imageData2, data2, 0);

    // Вызываем функцию binaural из hificode.c
    binaural(A, B);
}





JNIEXPORT void JNICALL
Java_com_sightsense_cFun_newStereo(JNIEnv *env, jobject /* this */, jdoubleArray imageData, jstring path) {
    double A[M][N];
    jsize len = env->GetArrayLength(imageData);
    if (len != M * N) {
        return;
    }
    jdouble* data = env->GetDoubleArrayElements(imageData, nullptr);
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            A[i][j] = data[i * N + j];
        }
    }
    env->ReleaseDoubleArrayElements(imageData, data, 0);


    const char *nativePath = env->GetStringUTFChars(path, nullptr);

    char mutablePath[1024];
    strncpy(mutablePath, nativePath, sizeof(mutablePath) - 1);
    mutablePath[sizeof(mutablePath) - 1] = '\0';

    env->ReleaseStringUTFChars(path, nativePath);

    newStereo(A, mutablePath);
}

JNIEXPORT void JNICALL
Java_com_sightsense_cFun_newBinaural(JNIEnv *env, jobject /* this */, jdoubleArray imageData1,
                                     jdoubleArray imageData2, jstring path) {
    double A[M][N];
    double B[M][N];
    jsize len1 = env->GetArrayLength(imageData1);
    jsize len2 = env->GetArrayLength(imageData2);


    if (len1 != M * N || len2 != M * N) {
        return;
    }
    jdouble* data1 = env->GetDoubleArrayElements(imageData1, nullptr);
    jdouble* data2 = env->GetDoubleArrayElements(imageData2, nullptr);
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            A[i][j] = data1[i * N + j];
            B[i][j] = data2[i * N + j];
        }
    }
    env->ReleaseDoubleArrayElements(imageData1, data1, 0);
    env->ReleaseDoubleArrayElements(imageData2, data2, 0);


    const char *nativePath = env->GetStringUTFChars(path, nullptr);

    char mutablePath[1024];
    strncpy(mutablePath, nativePath, sizeof(mutablePath) - 1);
    mutablePath[sizeof(mutablePath) - 1] = '\0';

    env->ReleaseStringUTFChars(path, nativePath);

    newBinaural(A, B, mutablePath);
}


} // extern "C"

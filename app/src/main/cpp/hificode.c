#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h> // Импорт библиотек
#include <string.h>

#include <android/log.h>
#include <jni.h>

#define LOG_TAG "MyNativeCode"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#define FNAME "hificode.wav"    /* User-defined parameters   */
#define FL     500   /* Lowest  frequency (Hz) in soundscape   */
#define FH    5000   /* Highest frequency (Hz)                 */
#define FS   44100   /* Sample  frequency (Hz)                 */
#define T     1.05   /* Image to sound conversion time (s)     */
#define HIFI     1   /* 8-bit|16-bit=0|1 sound quality         */
#define STEREO   1   /* Mono|Stereo=0|1 sound selection        */
#define DELAY    0   /* Nodelay|Delay=0|1 model   (STEREO=1)   */
#define FADE     1   /* Relative fade No|Yes=0|1  (STEREO=1)   */
#define DIFFR    1   /* Diffraction No|Yes=0|1    (STEREO=1)   */
#define BSPL     1   /* Rectangular|B-spline=0|1 time window   */
#define CONTRAST 2   /* Contrast enhancement, 0=none           */

#define C_ALLOC(number, type) ((type *) calloc((number),sizeof(type)) )
#define TwoPi 6.283185307179586476925287
#define Pi    3.141592653589793238462643
#define HIST  (1+HIFI)*(1+STEREO)
#define M     64
#define N    176

#define PERSON 1
#define BICYCLE 2
#define VEHICLE 3
#define MOTORBIKE 4


FILE *fp; unsigned long ir=0L, ia=9301L, ic=49297L, im=233280L;
void wi(unsigned int i) { int b1,b0; b0=i%256; b1=(i-b0)/256; putc(b0,fp); putc(b1,fp); }
void wl(long l) { unsigned int i1,i0; i0=l%65536L; i1=(l-i0)/65536L; wi(i0); wi(i1); }
double rnd(void){ ir = (ir*ia+ic) % im; return ir / (1.0*im); }

void fill_in_file()
{
    double ns=2L*(long)(0.5*FS*T);

    /* Write 8/16-bit mono/stereo .wav file */
    fp = fopen(FNAME,"wb"); fprintf(fp,"RIFF"); wl(ns*HIST+36L);
    fprintf(fp,"WAVEfmt "); wl(16L); wi(1); wi(STEREO?2:1); wl(0L+FS);
    wl(0L+FS*HIST); wi(HIST); wi(HIFI?16:8); fprintf(fp,"data"); wl(ns*HIST); // Запись заоловков файла RIFF (Resource Interchange File Format)
    // http://truelogic.org/wordpress/2015/09/04/parsing-a-wav-file-in-c/ Тут показаны какие заголовки записываются
}


double GetAmplitude(double dTime)
{
    double dAttackTime = 0.100;
    double dDecayTime = 0.01;
    double dStartAmplitude = 0.2;
    double dSustainAmplitude = 0.5;
    double dTriggerOnTime = 0.0;
    double dAmplitude = 0.0;
    double dLifeTime = dTime - dTriggerOnTime;


    if (dLifeTime <= dAttackTime)
    {
        // In attack Phase - approach max amplitude
        dAmplitude = (dLifeTime / dAttackTime) * dStartAmplitude;
    }

    if (dLifeTime > dAttackTime && dLifeTime <= (dAttackTime + dDecayTime))
    {
        // In decay phase - reduce to sustained amplitude
        dAmplitude = ((dLifeTime - dAttackTime) / dDecayTime) * (dSustainAmplitude - dStartAmplitude) + dStartAmplitude;
    }

    if (dLifeTime > (dAttackTime + dDecayTime))
    {
        // In sustain phase - dont change until note released
        dAmplitude = dSustainAmplitude;
    }


    // Amplitude should not be negative
    if (dAmplitude <= 0.0001)
    {
        dAmplitude = 0.0;
    }

    return dAmplitude;
}


// double wave(int class, double a, double w, double t) // type, amplitude, frequency*2pi, time, phase
// {
//    switch (class)
//    {
//    case PERSON:
//       return a * sin(w * t); //sine
//    case BICYCLE:
//       return -2.0 * a / Pi* atan(1.0 / tan(w * t));// saw можно менять минус
//    case VEHICLE:
//       return 2 * a / Pi * asin(sin(w*t));// triangle
//    case MOTORBIKE:
//       return a * sin(w * t) > 0 ? 0.5:-0.5;// quadratic
//    default:
//       return 0.0;
//    }
// }

double wave(int class, double a, double f, double t) // type, amplitude, frequency*2pi, time, phase
{
    switch (class)
    {
        case PERSON:
            return a * sin(f*2*Pi * t) + (a * sin((f+25)*2*Pi * t) > 0 ? 1:-1); //sine
        case BICYCLE:
            return -2.0 * a / Pi* atan(1.0 / tan(f*2*Pi * t));// saw можно менять минус
        case VEHICLE:
            return 2 * a / Pi * asin(sin(f*2*Pi*t))+(-2.0 * a / Pi* atan(1.0 / tan(f*2*Pi * t)));// triangle
        case MOTORBIKE:
            return a * sin(f*2*Pi * t) > 0 ? 1:-1;// quadratic
        default:
            return 0.01*sin(100*2*Pi*t);
    }
}


void nn_convert(double A[M][N]) // Функця для монокартинки
{
    int i, j, ss;
    long k=0L, l, ns=2L*(long)(0.5*FS*T), m=ns/N, sso=HIFI?0L:128L, ssm=HIFI?32768L:128L;

    double a, dt=1.0/FS, *w, *phi0, tau1, tau2, x, theta,
            scale=0.5/sqrt((double)M), q, q2, r, sl, sr, tl, tr, yl, ypl, yr, ypr, zl, zr, hrtf, hrtfl, hrtfr,
            v=340.0,  /* v = speed of sound (m/s) */
    hs=0.20;  /* hs = characteristic acoustical size of head (m) */
    w    = C_ALLOC(M, double); // Омега (Циклическая частота)
    phi0 = C_ALLOC(M, double); // Фаза

    int sc = 0;
    /* Set lin|exp (0|1) frequency distribution and random initial phase */
    //for (i=0; i<M; i++) w[i] = TwoPi * FL * pow(1.0* FH/FL,1.0*i/(M-1)); // Заполнение распределения омеги
    for (i=0; i<M; i++) phi0[i] = TwoPi * rnd(); // Заполнение распределения омеги
    //for (i=0; i<M; i++) w[i] = TwoPi * 261.63 * (double)pow(2, sc/12); sc+=2;
    //for (i=0; i<M; i++) w[i] = TwoPi * 130.81 * (double)pow(2, i/12);
    for (i=0; i<M; i++) w[i] = 130.81 * (double)pow(2, i/12);

    fill_in_file();

    tau1 = 0.5 / w[M-1]; tau2 = 0.25 * (tau1*tau1);
    yl = yr = zl = zr = 0.0;
    /* Not optimized for speed */

    while (k < ns) {
        if (BSPL) { q = 1.0 * (k%m)/(m-1); q2 = 0.5*q*q; }
        j = k / m; if (j>N-1) j=N-1;
        r = 1.0 * k/(ns-1);  /* Binaural attenuation/delay parameter */
        theta = (r-0.5) * TwoPi/3; x = 0.5 * hs * (theta + sin(theta));
        tl = tr = k * dt; if (DELAY) tr += x / v; /* Time delay model */
        x  = fabs(x); sl = sr = 0.0; hrtfl = hrtfr = 1.0;

        for (i=0; i<M; i++) {
            if (DIFFR) {
                /* First order frequency-dependent azimuth diffraction model */
                if (TwoPi*v/w[i] > x) hrtf = 1.0; else hrtf = TwoPi*v/(x*w[i]);
                if (theta < 0.0) { hrtfl =  1.0; hrtfr = hrtf; }
                else             { hrtfl = hrtf; hrtfr =  1.0; }
            }
            if (FADE) {
                /* Simple frequency-independent relative fade model */
                hrtfl *= (1.0-0.7*r);
                hrtfr *= (0.3+0.7*r);
            }
            if (BSPL) {
                if (j==0) a = (1.0-q2)+q2;
                else if (j==N-1) a = (q2-q+0.5)+(0.5+q-q2);
                else a = (q2-q+0.5)+(0.5+q-q*q)+q2;
            }
            else a = 1.0;
            // if ((int)A[i][j]){
            sl += hrtfl * wave((int)A[i][j], 1.0, w[i], tl);
            sr += hrtfr * wave((int)A[i][j], 1.0, w[i], tr);
            // }
        }
        if (k < ns/(5*N)) sl = (2.0*rnd()-1.0) / scale;  /* Left "click" */
        if (tl < 0.0) sl = 0.0;
        if (tr < 0.0) sr = 0.0;
        ypl = yl; yl = tau1/dt + tau2/(dt*dt);
        yl  = (sl + yl * ypl + tau2/dt * zl) / (1.0 + yl); zl = (yl - ypl) / dt;
        ypr = yr; yr = tau1/dt + tau2/(dt*dt);
        yr  = (sr + yr * ypr + tau2/dt * zr) / (1.0 + yr); zr = (yr - ypr) / dt;
        l   = sso + 0.5 + scale * ssm * yl;
        if (l >= sso-1+ssm) l = sso-1+ssm; if (l < sso-ssm) l = sso-ssm;
        ss  = (unsigned int) l;
        if (HIFI) wi(ss); else putc(ss,fp);  /* Left channel */
        l   = sso + 0.5 + scale * ssm * yr;
        if (l >= sso-1+ssm) l = sso-1+ssm; if (l < sso-ssm) l = sso-ssm;
        ss  = (unsigned int) l;
        if (HIFI) wi(ss); else putc(ss,fp);  /* Right channel */
        k++;
    }
    fclose(fp);
    //playSound("hificode.wav");  /* Play the soundscape */
    k=0;  /* Reset sample count */
}


void stereo(double A[M][N]) // Функця для монокартинки
{
    int i, j, ss;
    long k=0L, l, ns=2L*(long)(0.5*FS*T), m=ns/N, sso=HIFI?0L:128L, ssm=HIFI?32768L:128L;

    double a, dt=1.0/FS, *w, *phi0, tau1, tau2, x, theta,
            scale=0.5/sqrt((double)M), q, q2, r, sl, sr, tl, tr, yl, ypl, yr, ypr, zl, zr, hrtf, hrtfl, hrtfr,
            v=340.0,  /* v = speed of sound (m/s) */
    hs=0.20;  /* hs = characteristic acoustical size of head (m) */
    w    = C_ALLOC(M, double); // Омега (Циклическая частота)
    phi0 = C_ALLOC(M, double); // Фаза



    /* Set lin|exp (0|1) frequency distribution and random initial phase */
    for (i=0; i<M; i++) w[i] = TwoPi * FL * pow(1.0* FH/FL,1.0*i/(M-1)); // Заполнение распределения омеги
    for (i=0; i<M; i++) phi0[i] = TwoPi * rnd(); // Заполнение распределения омеги

    fill_in_file();

    tau1 = 0.5 / w[M-1]; tau2 = 0.25 * (tau1*tau1);
    yl = yr = zl = zr = 0.0;
    /* Not optimized for speed */

    while (k < ns) {
        if (BSPL) { q = 1.0 * (k%m)/(m-1); q2 = 0.5*q*q; }
        j = k / m; if (j>N-1) j=N-1;
        r = 1.0 * k/(ns-1);  /* Binaural attenuation/delay parameter */
        theta = (r-0.5) * TwoPi/3; x = 0.5 * hs * (theta + sin(theta));
        tl = tr = k * dt; if (DELAY) tr += x / v; /* Time delay model */
        x  = fabs(x); sl = sr = 0.0; hrtfl = hrtfr = 1.0;
        for (i=0; i<M; i++) {
            if (DIFFR) {
                /* First order frequency-dependent azimuth diffraction model */
                if (TwoPi*v/w[i] > x) hrtf = 1.0; else hrtf = TwoPi*v/(x*w[i]);
                if (theta < 0.0) { hrtfl =  1.0; hrtfr = hrtf; }
                else             { hrtfl = hrtf; hrtfr =  1.0; }
            }
            if (FADE) {
                /* Simple frequency-independent relative fade model */
                hrtfl *= (1.0-0.7*r);
                hrtfr *= (0.3+0.7*r);
            }
            if (BSPL) {
                if (j==0) a = (1.0-q2)*A[i][j]+q2*A[i][j+1];
                else if (j==N-1) a = (q2-q+0.5)*A[i][j-1]+(0.5+q-q2)*A[i][j];
                else a = (q2-q+0.5)*A[i][j-1]+(0.5+q-q*q)*A[i][j]+q2*A[i][j+1];
            }
            else a = A[i][j];
            sl += hrtfl * a * sin(w[i] * tl + phi0[i]); // Формула гармонического колебания A*sin(wt + ф)
            sr += hrtfr * a * sin(w[i] * tr + phi0[i]);
        }
        if (k < ns/(5*N)) sl = (2.0*rnd()-1.0) / scale;  /* Left "click" */
        if (tl < 0.0) sl = 0.0;
        if (tr < 0.0) sr = 0.0;
        ypl = yl; yl = tau1/dt + tau2/(dt*dt);
        yl  = (sl + yl * ypl + tau2/dt * zl) / (1.0 + yl); zl = (yl - ypl) / dt;
        ypr = yr; yr = tau1/dt + tau2/(dt*dt);
        yr  = (sr + yr * ypr + tau2/dt * zr) / (1.0 + yr); zr = (yr - ypr) / dt;
        l   = sso + 0.5 + scale * ssm * yl;
        if (l >= sso-1+ssm) l = sso-1+ssm; if (l < sso-ssm) l = sso-ssm;
        ss  = (unsigned int) l;
        if (HIFI) wi(ss); else putc(ss,fp);  /* Left channel */
        l   = sso + 0.5 + scale * ssm * yr;
        if (l >= sso-1+ssm) l = sso-1+ssm; if (l < sso-ssm) l = sso-ssm;
        ss  = (unsigned int) l;
        if (HIFI) wi(ss); else putc(ss,fp);  /* Right channel */
        k++;
    }
    fclose(fp);
    k=0;  /* Reset sample count */
}

void binaural(double A[M][N], double B[M][N]) // Функция для стереокартинки
{
    int i, j, ss, b, b1;
    long k=0L, l, ns = 2L*(long)(0.5*FS*T), m=ns/N,
            sso=HIFI?0L:128L, ssm=HIFI?32768L:128L;
    double a, t, dt=1.0/FS, y, yp, z, *w, *phi0, tau1, tau2, x, theta, ssl, ssr,
            scale=0.5/sqrt((double)M), q, q2, r, sl, sr, tl, tr, yl, ypl, yr, ypr, ll, lr,
            zl, zr, hrtf, hrtfl, hrtfr, v=340.0,  /* v = speed of sound (m/s) */
    hs=0.20;  /* hs = characteristic acoustical size of head (m) */
    w    = C_ALLOC(M, double);
    phi0 = C_ALLOC(M, double);


    /* Set lin|exp (0|1) frequency distribution and random initial phase */
    for (i=0; i<M; i++) w[i] = TwoPi * FL * pow(1.0* FH/FL,1.0*i/(M-1));

    for (i=0; i<M; i++) phi0[i] = TwoPi * rnd();


    fill_in_file();
    tau1 = 0.5 / w[M-1]; tau2 = 0.25 * tau1*tau1;
    y = yl = yr = z = zl = zr = 0.0;
    /* Not optimized for speed */
    while (k < ns) {  /* Not optimized for speed (or anything else) */
        sl = sr = 0.0; t = k * dt; j = k / m; if (j>N-1) j=N-1;
        if (k < ns/(5*N)) {sl = (2.0*rnd()-1.0) / scale; sr = (2.0*rnd()-1.0) / scale;}  /* "click" */

        else for (i=0; i<M; i++){
                sl += A[i][j] * sin(w[i] * t + phi0[i]);
                sr += B[i][j] * sin(w[i] * t + phi0[i]);

            }

        ypl = yl; yl = tau1/dt + tau2/(dt*dt);
        yl  = (sl + yl * ypl + tau2/dt * zl) / (1.0 + yl); zl = (yl - ypl) / dt;


        ypr = yr; yr = tau1/dt + tau2/(dt*dt);
        yr  = (sr + yr * ypr + tau2/dt * zr) / (1.0 + yr); zr = (yr - ypr) / dt;

        l   = sso + 0.5 + scale * ssm * yl;
        if (l >= sso-1+ssm) l = sso-1+ssm; if (l < sso-ssm) l = sso-ssm;
        ss  = (unsigned int) l;
        if (HIFI) wi(ss); else putc(ss,fp);  /* Left channel */
        l   = sso + 0.5 + scale * ssm * yr;
        if (l >= sso-1+ssm) l = sso-1+ssm; if (l < sso-ssm) l = sso-ssm;
        ss  = (unsigned int) l;
        if (HIFI) wi(ss); else putc(ss,fp);  /* Right channel */
        k++;
    }
    if ((ceil(ns/2))!=(ns/2)) putc(0,fp);
    fclose(fp);
    k=0;  /* Reset sample count */
}




// ################################################################################################
// ################################################################################################
// ################################################################################################
// ################################################################################################
// ################################################################################################


void fill_in_file_with_path(char *path)
{
    double ns=2L*(long)(0.5*FS*T);
    fp = fopen(path, "wb");
    if (fp == NULL) {
        LOGE("Ошибка: не удалось открыть файл %s", path);
        return;
    }

    fprintf(fp,"RIFF");
    wl(ns*HIST+36L);
    fprintf(fp,"WAVEfmt ");
    wl(16L);
    wi(1);
    wi(STEREO?2:1);
    wl(0L+FS);
    wl(0L+FS*HIST);
    wi(HIST);
    wi(HIFI?16:8);
    fprintf(fp,"data");
    wl(ns*HIST); // Запись заоловков файла RIFF (Resource Interchange File Format)
    // http://truelogic.org/wordpress/2015/09/04/parsing-a-wav-file-in-c/ Тут показаны какие заголовки записываются

}


void newStereo(double A[M][N], char *path) // Функця для монокартинки
{
    int i, j, ss;
    long k=0L, l, ns=2L*(long)(0.5*FS*T), m=ns/N, sso=HIFI?0L:128L, ssm=HIFI?32768L:128L;

    double a, dt=1.0/FS, *w, *phi0, tau1, tau2, x, theta,
            scale=0.5/sqrt((double)M), q, q2, r, sl, sr, tl, tr, yl, ypl, yr, ypr, zl, zr, hrtf, hrtfl, hrtfr,
            v=340.0,  /* v = speed of sound (m/s) */
    hs=0.20;  /* hs = characteristic acoustical size of head (m) */
    w    = C_ALLOC(M, double); // Омега (Циклическая частота)
    phi0 = C_ALLOC(M, double); // Фаза



    /* Set lin|exp (0|1) frequency distribution and random initial phase */
    for (i=0; i<M; i++) w[i] = TwoPi * FL * pow(1.0* FH/FL,1.0*i/(M-1)); // Заполнение распределения омеги
    for (i=0; i<M; i++) phi0[i] = TwoPi * rnd(); // Заполнение распределения омеги

    fill_in_file_with_path(path);

    tau1 = 0.5 / w[M-1]; tau2 = 0.25 * (tau1*tau1);
    yl = yr = zl = zr = 0.0;
    /* Not optimized for speed */

    while (k < ns) {
        if (BSPL) { q = 1.0 * (k%m)/(m-1); q2 = 0.5*q*q; }
        j = k / m; if (j>N-1) j=N-1;
        r = 1.0 * k/(ns-1);  /* Binaural attenuation/delay parameter */
        theta = (r-0.5) * TwoPi/3; x = 0.5 * hs * (theta + sin(theta));
        tl = tr = k * dt; if (DELAY) tr += x / v; /* Time delay model */
        x  = fabs(x); sl = sr = 0.0; hrtfl = hrtfr = 1.0;
        for (i=0; i<M; i++) {
            if (DIFFR) {
                /* First order frequency-dependent azimuth diffraction model */
                if (TwoPi*v/w[i] > x) hrtf = 1.0; else hrtf = TwoPi*v/(x*w[i]);
                if (theta < 0.0) { hrtfl =  1.0; hrtfr = hrtf; }
                else             { hrtfl = hrtf; hrtfr =  1.0; }
            }
            if (FADE) {
                /* Simple frequency-independent relative fade model */
                hrtfl *= (1.0-0.7*r);
                hrtfr *= (0.3+0.7*r);
            }
            if (BSPL) {
                if (j==0) a = (1.0-q2)*A[i][j]+q2*A[i][j+1];
                else if (j==N-1) a = (q2-q+0.5)*A[i][j-1]+(0.5+q-q2)*A[i][j];
                else a = (q2-q+0.5)*A[i][j-1]+(0.5+q-q*q)*A[i][j]+q2*A[i][j+1];
            }
            else a = A[i][j];
            sl += hrtfl * a * sin(w[i] * tl + phi0[i]); // Формула гармонического колебания A*sin(wt + ф)
            sr += hrtfr * a * sin(w[i] * tr + phi0[i]);
        }
        if (k < ns/(5*N)) sl = (2.0*rnd()-1.0) / scale;  /* Left "click" */
        if (tl < 0.0) sl = 0.0;
        if (tr < 0.0) sr = 0.0;
        ypl = yl; yl = tau1/dt + tau2/(dt*dt);
        yl  = (sl + yl * ypl + tau2/dt * zl) / (1.0 + yl); zl = (yl - ypl) / dt;
        ypr = yr; yr = tau1/dt + tau2/(dt*dt);
        yr  = (sr + yr * ypr + tau2/dt * zr) / (1.0 + yr); zr = (yr - ypr) / dt;
        l   = sso + 0.5 + scale * ssm * yl;
        if (l >= sso-1+ssm) l = sso-1+ssm; if (l < sso-ssm) l = sso-ssm;
        ss  = (unsigned int) l;
        if (HIFI) wi(ss); else putc(ss,fp);  /* Left channel */
        l   = sso + 0.5 + scale * ssm * yr;
        if (l >= sso-1+ssm) l = sso-1+ssm; if (l < sso-ssm) l = sso-ssm;
        ss  = (unsigned int) l;
        if (HIFI) wi(ss); else putc(ss,fp);  /* Right channel */
        k++;
    }
    fclose(fp);
    k=0;  /* Reset sample count */
}

void newBinaural(double A[M][N], double B[M][N], char *path) // Функция для стереокартинки
{
    int i, j, ss, b, b1;
    long k=0L, l, ns = 2L*(long)(0.5*FS*T), m=ns/N,
            sso=HIFI?0L:128L, ssm=HIFI?32768L:128L;
    double a, t, dt=1.0/FS, y, yp, z, *w, *phi0, tau1, tau2, x, theta, ssl, ssr,
            scale=0.5/sqrt((double)M), q, q2, r, sl, sr, tl, tr, yl, ypl, yr, ypr, ll, lr,
            zl, zr, hrtf, hrtfl, hrtfr, v=340.0,  /* v = speed of sound (m/s) */
    hs=0.20;  /* hs = characteristic acoustical size of head (m) */
    w    = C_ALLOC(M, double);
    phi0 = C_ALLOC(M, double);


    /* Set lin|exp (0|1) frequency distribution and random initial phase */
    for (i=0; i<M; i++) w[i] = TwoPi * FL * pow(1.0* FH/FL,1.0*i/(M-1));

    for (i=0; i<M; i++) phi0[i] = TwoPi * rnd();


    fill_in_file_with_path(path);
    tau1 = 0.5 / w[M-1]; tau2 = 0.25 * tau1*tau1;
    y = yl = yr = z = zl = zr = 0.0;
    /* Not optimized for speed */
    while (k < ns) {  /* Not optimized for speed (or anything else) */
        sl = sr = 0.0; t = k * dt; j = k / m; if (j>N-1) j=N-1;
        if (k < ns/(5*N)) {sl = (2.0*rnd()-1.0) / scale; sr = (2.0*rnd()-1.0) / scale;}  /* "click" */

        else for (i=0; i<M; i++){
                sl += A[i][j] * sin(w[i] * t + phi0[i]);
                sr += B[i][j] * sin(w[i] * t + phi0[i]);

            }

        ypl = yl; yl = tau1/dt + tau2/(dt*dt);
        yl  = (sl + yl * ypl + tau2/dt * zl) / (1.0 + yl); zl = (yl - ypl) / dt;


        ypr = yr; yr = tau1/dt + tau2/(dt*dt);
        yr  = (sr + yr * ypr + tau2/dt * zr) / (1.0 + yr); zr = (yr - ypr) / dt;

        l   = sso + 0.5 + scale * ssm * yl;
        if (l >= sso-1+ssm) l = sso-1+ssm; if (l < sso-ssm) l = sso-ssm;
        ss  = (unsigned int) l;
        if (HIFI) wi(ss); else putc(ss,fp);  /* Left channel */
        l   = sso + 0.5 + scale * ssm * yr;
        if (l >= sso-1+ssm) l = sso-1+ssm; if (l < sso-ssm) l = sso-ssm;
        ss  = (unsigned int) l;
        if (HIFI) wi(ss); else putc(ss,fp);  /* Right channel */
        k++;
    }
    if ((ceil(ns/2))!=(ns/2)) putc(0,fp);
    fclose(fp);
    k=0;  /* Reset sample count */
}



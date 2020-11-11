/*
 * utils.cpp
 *
 *  Created on: Feb 27, 2020
 *      Author: ypmen
 */

#include <limits>
#include <assert.h>
#include "utils.h"
#include "AVL.h"
#include "dedisperse.h"

long double to_longdouble(double value1, double value2)
{
    return (long double)(value1) + (long double)(value2);
}

int gcd(int a, int b)
{
    if (b == 0)
        return a;
    return gcd(b, a % b);
}

// Returns LCM of array elements
long int findlcm(int arr[], int n)
{
    // Initialize result
    long int ans = arr[0];

    // ans contains LCM of arr[0], ..arr[i]
    // after i'th iteration,
    for (int i = 1; i < n; i++)
        ans = (((arr[i] * ans)) /
               (gcd(arr[i], ans)));
    return ans;
}

fftwf_plan plan_transpose(int rows, int cols, float *in, float *out)
{
    const unsigned flags = FFTW_ESTIMATE; /* other flags are possible */
    fftw_iodim howmany_dims[2];

    howmany_dims[0].n = rows;
    howmany_dims[0].is = cols;
    howmany_dims[0].os = 1;

    howmany_dims[1].n = cols;
    howmany_dims[1].is = 1;
    howmany_dims[1].os = rows;

    return fftwf_plan_guru_r2r(/*rank=*/0, /*dims=*/NULL,
                               /*howmany_rank=*/2, howmany_dims,
                               in, out, /*kind=*/NULL, flags);
}

fftw_plan plan_transpose(int rows, int cols, double *in, double *out)
{
    const unsigned flags = FFTW_ESTIMATE; /* other flags are possible */
    fftw_iodim howmany_dims[2];

    howmany_dims[0].n = rows;
    howmany_dims[0].is = cols;
    howmany_dims[0].os = 1;

    howmany_dims[1].n = cols;
    howmany_dims[1].is = 1;
    howmany_dims[1].os = rows;

    return fftw_plan_guru_r2r(/*rank=*/0, /*dims=*/NULL,
                              /*howmany_rank=*/2, howmany_dims,
                              in, out, /*kind=*/NULL, flags);
}

void transpose(float *out, float *in, int m, int n)
{
    const int tilex = 16;
    const int tiley = 64;

    assert(n % tilex == 0);
    assert(m % tiley == 0);

    int blockx = n / tilex;
    int blocky = m / tiley;

    float *temp = new float[num_threads * tiley * tilex];
#ifdef _OPENMP
#pragma omp parallel for num_threads(num_threads)
#endif
    for (long int s = 0; s < blocky*blockx; s++)
    {
        float *ptemp = temp;
#ifdef _OPENMP
        ptemp = temp + omp_get_thread_num()*tiley*tilex;
#endif
        long int l = s/blockx;
        long int k = s%blockx;

        for (long int i = 0; i < tiley; i++)
        {
            for (long int j = 0; j < tilex; j++)
            {
                ptemp[j * tiley + i] = in[l * tiley * n + i * n + k * tilex + j];
            }
        }
        for (long int i = 0; i < tilex; i++)
        {
            for (long int j = 0; j < tiley; j++)
            {
                out[k * tilex * m + i * m + l * tiley + j] = ptemp[i * tiley + j];
            }
        }
    }

    delete[] temp;
}

void transpose_pad(float *out, float *in, int m, int n)
{
    const int tilex = 16;
    const int tiley = 64;

    int npad = ceil(n*1./tilex)*tilex;
    int mpad = ceil(m*1./tiley)*tiley;

    int blockx = npad / tilex;
    int blocky = mpad / tiley;

    float *temp = new float[num_threads * tiley * tilex];
#ifdef _OPENMP
#pragma omp parallel for num_threads(num_threads)
#endif
    for (long int s = 0; s < blocky*blockx; s++)
    {
        float *ptemp = temp;
#ifdef _OPENMP
        ptemp = temp + omp_get_thread_num()*tiley*tilex;
#endif
        long int l = s/blockx;
        long int k = s%blockx;
        
        if ((l+1)*tiley <= m and (k+1)*tilex <= n)
        {
            for (long int i = 0; i < tiley; i++)
            {
                for (long int j = 0; j < tilex; j++)
                {
                    ptemp[j * tiley + i] = in[(l * tiley + i) * n + k * tilex + j];
                }
            }
            for (long int i = 0; i < tilex; i++)
            {
                for (long int j = 0; j < tiley; j++)
                {
                    out[(k * tilex + i) * m + l * tiley + j] = ptemp[i * tiley + j];
                }
            }
        }
        else if ((l+1)*tiley > m and (k+1)*tilex <= n)
        {
            for (long int i = 0; i < m-l*tiley; i++)
            {
                for (long int j = 0; j < tilex; j++)
                {
                    ptemp[j * tiley + i] = in[(l * tiley + i) * n + k * tilex + j];
                }
            }
            for (long int i = 0; i < tilex; i++)
            {
                for (long int j = 0; j < m-l*tiley; j++)
                {
                    out[(k * tilex + i) * m + l * tiley + j] = ptemp[i * tiley + j];
                }
            }
        }
        else if ((l+1)*tiley <= m and (k+1)*tilex > n)
        {
            for (long int i = 0; i < tiley; i++)
            {
                for (long int j = 0; j < n-k*tilex; j++)
                {
                    ptemp[j * tiley + i] = in[(l * tiley + i) * n + k * tilex + j];
                }
            }
            for (long int i = 0; i < n-k*tilex; i++)
            {
                for (long int j = 0; j < tiley; j++)
                {
                    out[(k * tilex + i) * m + l * tiley + j] = ptemp[i * tiley + j];
                }
            }
        }
        else
        {
            for (long int i = 0; i < m-l*tiley; i++)
            {
                for (long int j = 0; j < n-k*tilex; j++)
                {
                    ptemp[j * tiley + i] = in[(l * tiley + i) * n + k * tilex + j];
                }
            }
            for (long int i = 0; i < n-k*tilex; i++)
            {
                for (long int j = 0; j < m-l*tiley; j++)
                {
                    out[(k * tilex + i) * m + l * tiley + j] = ptemp[i * tiley + j];
                }
            }
        }
    }

    delete[] temp;
}

void transpose_pad(float *out, float *in, int m, int n, int tiley, int tilex)
{
    int npad = ceil(n*1./tilex)*tilex;
    int mpad = ceil(m*1./tiley)*tiley;

    int blockx = npad / tilex;
    int blocky = mpad / tiley;

    float *temp = new float[num_threads * tiley * tilex];
#ifdef _OPENMP
#pragma omp parallel for num_threads(num_threads)
#endif
    for (long int s = 0; s < blocky*blockx; s++)
    {
        float *ptemp = temp;
#ifdef _OPENMP
        ptemp = temp + omp_get_thread_num()*tiley*tilex;
#endif
        long int l = s/blockx;
        long int k = s%blockx;
        
        for (long int i = 0; i < tiley; i++)
        {
            for (long int j = 0; j < tilex; j++)
            {
                if (l * tiley + i < m and k * tilex + j < n)
                    ptemp[j * tiley + i] = in[(l * tiley + i) * n + k * tilex + j];
            }
        }
        for (long int i = 0; i < tilex; i++)
        {
            for (long int j = 0; j < tiley; j++)
            {
                if (k * tilex + i < n and l * tiley + j < m)
                    out[(k * tilex + i) * m + l * tiley + j] = ptemp[i * tiley + j];
            }
        }
    }

    delete[] temp;    
}

void runMedian(float *data, float *datMedian, long int size, int w)
{
    AVLTree<float> treeMedian;

    w = w > size ? size : w;
    long int k = 0;

    int a = -floor(w * 0.5);
    int b = ceil(w * 0.5);

    for (long int i = 0; i < b; i++)
    {
        treeMedian.insertValue(data[i]);
    }
    datMedian[k++] = treeMedian.getMedian();

    for (long int i = 1; i < size; i++)
    {
        a++;
        b++;
        if (a > 0)
            treeMedian.removeValue(data[a - 1]);
        if (b <= size)
            treeMedian.insertValue(data[b - 1]);

        datMedian[k++] = treeMedian.getMedian();
    }
}

void cmul(vector<complex<float>> &x, vector<complex<float>> &y)
{
    assert(x.size() == y.size());

    long int size = x.size();

    vector<float> xr(size);
    vector<float> xi(size);
    vector<float> yr(size);
    vector<float> yi(size);
    
    for (long int i=0; i<size; i++)
    {
        xr[i] = real(x[i]);
        xi[i] = imag(x[i]);
    }
    for (long int i=0; i<size; i++)
    {
        yr[i] = real(y[i]);
        yi[i] = imag(y[i]);
    }

    for (long int i=0; i<size; i++)
    {
        float real = xr[i]*yr[i]-xi[i]*yi[i];
        float imag = xr[i]*yi[i]+xi[i]*yr[i];
        x[i].real(real);
        x[i].imag(imag);
    }
}

void get_s_radec(double ra, double dec, string &s_ra, string &s_dec)
{
    int ra_hh, ra_mm;
    float ra_ss;

    int ra_sign = ra>=0 ? 1:-1;
    ra *= ra_sign;
    ra_hh = ra/10000;
    ra_mm = (ra-ra_hh*10000)/100;
    ra_ss = ra-ra_hh*10000-ra_mm*100;

    int dec_dd, dec_mm;
    float dec_ss;

    int dec_sign = dec>=0 ? 1:-1;
    dec *= dec_sign;
    dec_dd = dec/10000;
    dec_mm = (dec-dec_dd*10000)/100;
    dec_ss = dec-dec_dd*10000-dec_mm*100;

    stringstream ss_ra_hh;
    ss_ra_hh << setw(2) << setfill('0') << ra_hh;
    string s_ra_hh = ss_ra_hh.str();

    stringstream ss_ra_mm;
    ss_ra_mm << setw(2) << setfill('0') << ra_mm;
    string s_ra_mm = ss_ra_mm.str();

    stringstream ss_ra_ss;
    ss_ra_ss << setprecision(2) << setw(5) << setfill('0') << fixed << ra_ss;
    string s_ra_ss = ss_ra_ss.str();

    stringstream ss_dec_dd;
    ss_dec_dd << setw(2) << setfill('0') << dec_dd;
    string s_dec_dd = ss_dec_dd.str();

    stringstream ss_dec_mm;
    ss_dec_mm << setw(2) << setfill('0') << dec_mm;
    string s_dec_mm = ss_dec_mm.str();

    stringstream ss_dec_ss;
    ss_dec_ss << setprecision(2) << setw(5) << setfill('0') << fixed << dec_ss;
    string s_dec_ss = ss_dec_ss.str();

    s_ra = s_ra_hh + ":" + s_ra_mm + ":" + s_ra_ss;
    if (dec_sign < 0)
        s_dec = "-" + s_dec_dd + ":" + s_dec_mm + ":" + s_dec_ss;
    else
        s_dec = s_dec_dd + ":" + s_dec_mm + ":" + s_dec_ss;
}

bool inverse_matrix4x4(const double m[16], double invOut[16])
{
    double inv[16], det;
    int i;

    inv[0] = m[5]  * m[10] * m[15] - 
             m[5]  * m[11] * m[14] - 
             m[9]  * m[6]  * m[15] + 
             m[9]  * m[7]  * m[14] +
             m[13] * m[6]  * m[11] - 
             m[13] * m[7]  * m[10];

    inv[4] = -m[4]  * m[10] * m[15] + 
              m[4]  * m[11] * m[14] + 
              m[8]  * m[6]  * m[15] - 
              m[8]  * m[7]  * m[14] - 
              m[12] * m[6]  * m[11] + 
              m[12] * m[7]  * m[10];

    inv[8] = m[4]  * m[9] * m[15] - 
             m[4]  * m[11] * m[13] - 
             m[8]  * m[5] * m[15] + 
             m[8]  * m[7] * m[13] + 
             m[12] * m[5] * m[11] - 
             m[12] * m[7] * m[9];

    inv[12] = -m[4]  * m[9] * m[14] + 
               m[4]  * m[10] * m[13] +
               m[8]  * m[5] * m[14] - 
               m[8]  * m[6] * m[13] - 
               m[12] * m[5] * m[10] + 
               m[12] * m[6] * m[9];

    inv[1] = -m[1]  * m[10] * m[15] + 
              m[1]  * m[11] * m[14] + 
              m[9]  * m[2] * m[15] - 
              m[9]  * m[3] * m[14] - 
              m[13] * m[2] * m[11] + 
              m[13] * m[3] * m[10];

    inv[5] = m[0]  * m[10] * m[15] - 
             m[0]  * m[11] * m[14] - 
             m[8]  * m[2] * m[15] + 
             m[8]  * m[3] * m[14] + 
             m[12] * m[2] * m[11] - 
             m[12] * m[3] * m[10];

    inv[9] = -m[0]  * m[9] * m[15] + 
              m[0]  * m[11] * m[13] + 
              m[8]  * m[1] * m[15] - 
              m[8]  * m[3] * m[13] - 
              m[12] * m[1] * m[11] + 
              m[12] * m[3] * m[9];

    inv[13] = m[0]  * m[9] * m[14] - 
              m[0]  * m[10] * m[13] - 
              m[8]  * m[1] * m[14] + 
              m[8]  * m[2] * m[13] + 
              m[12] * m[1] * m[10] - 
              m[12] * m[2] * m[9];

    inv[2] = m[1]  * m[6] * m[15] - 
             m[1]  * m[7] * m[14] - 
             m[5]  * m[2] * m[15] + 
             m[5]  * m[3] * m[14] + 
             m[13] * m[2] * m[7] - 
             m[13] * m[3] * m[6];

    inv[6] = -m[0]  * m[6] * m[15] + 
              m[0]  * m[7] * m[14] + 
              m[4]  * m[2] * m[15] - 
              m[4]  * m[3] * m[14] - 
              m[12] * m[2] * m[7] + 
              m[12] * m[3] * m[6];

    inv[10] = m[0]  * m[5] * m[15] - 
              m[0]  * m[7] * m[13] - 
              m[4]  * m[1] * m[15] + 
              m[4]  * m[3] * m[13] + 
              m[12] * m[1] * m[7] - 
              m[12] * m[3] * m[5];

    inv[14] = -m[0]  * m[5] * m[14] + 
               m[0]  * m[6] * m[13] + 
               m[4]  * m[1] * m[14] - 
               m[4]  * m[2] * m[13] - 
               m[12] * m[1] * m[6] + 
               m[12] * m[2] * m[5];

    inv[3] = -m[1] * m[6] * m[11] + 
              m[1] * m[7] * m[10] + 
              m[5] * m[2] * m[11] - 
              m[5] * m[3] * m[10] - 
              m[9] * m[2] * m[7] + 
              m[9] * m[3] * m[6];

    inv[7] = m[0] * m[6] * m[11] - 
             m[0] * m[7] * m[10] - 
             m[4] * m[2] * m[11] + 
             m[4] * m[3] * m[10] + 
             m[8] * m[2] * m[7] - 
             m[8] * m[3] * m[6];

    inv[11] = -m[0] * m[5] * m[11] + 
               m[0] * m[7] * m[9] + 
               m[4] * m[1] * m[11] - 
               m[4] * m[3] * m[9] - 
               m[8] * m[1] * m[7] + 
               m[8] * m[3] * m[5];

    inv[15] = m[0] * m[5] * m[10] - 
              m[0] * m[6] * m[9] - 
              m[4] * m[1] * m[10] + 
              m[4] * m[2] * m[9] + 
              m[8] * m[1] * m[6] - 
              m[8] * m[2] * m[5];

    det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

    if (det == 0)
        return false;

    det = 1.0 / det;

    for (i = 0; i < 16; i++)
        invOut[i] = inv[i] * det;

    return true;
}

bool get_inverse_matrix3x3(const double m[3][3], double invOut[3][3])
{
    double det = m[0][0] * (m[1][1] * m[2][2] - m[2][1] * m[1][2]) -
                m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
                m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);

    if (det == 0) return false;

    double invdet = 1 / det;

    invOut[0][0] = (m[1][1] * m[2][2] - m[2][1] * m[1][2]) * invdet;
    invOut[0][1] = (m[0][2] * m[2][1] - m[0][1] * m[2][2]) * invdet;
    invOut[0][2] = (m[0][1] * m[1][2] - m[0][2] * m[1][1]) * invdet;
    invOut[1][0] = (m[1][2] * m[2][0] - m[1][0] * m[2][2]) * invdet;
    invOut[1][1] = (m[0][0] * m[2][2] - m[0][2] * m[2][0]) * invdet;
    invOut[1][2] = (m[1][0] * m[0][2] - m[0][0] * m[1][2]) * invdet;
    invOut[2][0] = (m[1][0] * m[2][1] - m[2][0] * m[1][1]) * invdet;
    invOut[2][1] = (m[2][0] * m[0][1] - m[0][0] * m[2][1]) * invdet;
    invOut[2][2] = (m[0][0] * m[1][1] - m[1][0] * m[0][1]) * invdet;

    return true;
}

template <typename T>
bool get_error_from_chisq_matrix(T &xerr, T &yerr, vector<T> &x, vector<T> &y, vector<T> &mxchisq)
{
    long int m = y.size();
    long int n = x.size();
    assert(mxchisq.size() == m*n);

    double A[4][4];
    double B[4];
    for (long int i=0; i<4; i++)
    {   
        B[i] = 0.;
        for (long int j=0; j<4; j++)
        {
            A[i][j] = 0.;
        }
    }

    vector<T> x2(n), x3(n), x4(n);
    vector<T> y2(m), y3(m), y4(m);

    for (long int j=0; j<n; j++)
    {
        x2[j] = x[j]*x[j];
        x3[j] = x2[j]*x[j];
        x4[j] = x3[j]*x[j];
    }

    for (long int i=0; i<m; i++)
    {
        y2[i] = y[i]*y[i];
        y3[i] = y2[i]*y[i];
        y4[i] = y3[i]*y[i];
    }    

    for (long int i=0; i<m; i++)
    {
        for (long int j=0; j<n; j++)
        {
            A[0][0] += x4[j];
            A[0][1] = A[1][0] += 2.*x3[j]*y[i];
            A[0][2] = A[2][0] += x2[j]*y2[i];
            A[0][3] = A[3][0] += x2[j];
            A[1][1] += 4.*x2[j]*y2[i];
            A[1][2] = A[2][1] += 2*x[j]*y3[i];
            A[1][3] = A[3][1] += 2*x[j]*y[i];
            A[2][2] += y4[i];
            A[2][3] = A[3][2] += y2[i];
            A[3][3] += 1.;
            B[0] += x2[j]*mxchisq[i*n+j];
            B[1] += 2.*x[j]*y[i]*mxchisq[i*n+j];
            B[2] += y2[i]*mxchisq[i*n+j];
            B[3] += mxchisq[i*n+j];
        }
    }

    double invA[4][4];

    if (!get_inverse_matrix4x4(A, invA)) return false;

    double a=0., b=0., c=0., d=0.;

    for (long int i=0; i<4; i++)
    {
        a += invA[0][i]*B[i];
        b += invA[1][i]*B[i];
        c += invA[2][i]*B[i];
        d += invA[3][i]*B[i];
    }

    xerr = sqrt(abs(c/(a*c-b*b)));
    yerr = sqrt(abs(a/(a*c-b*b)));

    return true;
}

template <typename T>
bool get_error_from_chisq_matrix(T &xerr, vector<T> &x, vector<T> &vchisq)
{
    int n = x.size();
    assert(n == vchisq.size());

    double A[3][3];
    double B[3];
    for (long int i=0; i<3; i++)
    {
        B[i] = 0.;
        for (long int j=0; j<3; j++)
        {
            A[i][j] = 0.;
        }
    }

    vector<T> x2(n), x3(n), x4(n);
    for (long int i=0; i<n; i++)
    {
        x2[i] = x[i]*x[i];
        x3[i] = x2[i]*x[i];
        x4[i] = x3[i]*x[i];
    }

    for (long int i=0; i<n; i++)
    {
        A[0][0] += x4[i];
        A[0][1] = A[1][0] += x3[i];
        A[0][2] = A[2][0] += x2[i];
        A[1][1] += x2[i];
        A[1][2] = A[2][1] += x[i];
        A[2][2] += 1.;
        
        B[0] += x2[i]*vchisq[i];
        B[1] += x[i]*vchisq[i];
        B[2] += vchisq[i];
    }

    double invA[3][3];

    if (!get_inverse_matrix3x3(A, invA)) return false;

    double a=0., b=0., c=0.;
    for (long int i=0; i<3; i++)
    {
        a += invA[0][i]*B[i];
        b += invA[1][i]*B[i];
        c += invA[2][i]*B[i];
    }

    xerr = sqrt(abs(1./a));

    return true;
}

template bool get_error_from_chisq_matrix<float>(float &xerr, float &yerr, vector<float> &x, vector<float> &y, vector<float> &mxchisq);
template bool get_error_from_chisq_matrix<float>(float &xerr, vector<float> &x, vector<float> &vchisq);
template bool get_error_from_chisq_matrix<double>(double &xerr, double &yerr, vector<double> &x, vector<double> &y, vector<double> &mxchisq);
template bool get_error_from_chisq_matrix<double>(double &xerr, vector<double> &x, vector<double> &vchisq);


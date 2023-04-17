#include <iostream>
#include <fstream>
#include <vector>
#include <amp.h>

using namespace std;
using uchar = unsigned char;

#undef RGB

#pragma pack(push,1)
struct RGB
{
    uchar B;
    uchar G;
    uchar R;

    RGB(int _R = 0, int _G = 0, int _B = 0)
    {
        R = (uchar)_R;
        B = (uchar)_B;
        G = (uchar)_G;
    }
};
struct BMPheader
{
    char type[2] = { 'B','M' };
    __int32 size = -1;
    __int32 reserved = 0;
    __int32 offset = 54;
    __int32 headSize = 40;
    __int32 cols = -1;
    __int32 rows = -1;
    __int16 planes = 1;
    __int16 bitCount = 24;
    __int32 compression = 0;
    __int32 imageSize = -1;
    char otherParams[16];
};
#pragma pack(pop)

struct BMPimage
{
    BMPheader head;
    int w, h;
    vector <vector <RGB>> mat;
};

BMPimage load_image(string name)
{
    ifstream img(name, ios::binary);
    BMPimage out;
    img.read((char*)&out.head, sizeof(BMPheader));
    out.w = out.head.cols;
    out.h = out.head.rows;
    out.mat = vector <vector <RGB>>(out.h, vector<RGB>(out.w));
    for (int y = 0; y < out.h; y++)
    {
        for (int x = 0; x < out.w; x++)
        {
            RGB col;
            img.read((char*)&col, 3);
            out.mat[y][x] = col;
        }
        char del[4];
        img.read(del, out.w % 4);
    }
    img.close();
    return out;
}

BMPimage new_image(int rows, int cols)
{
    BMPimage img;
    img.w = cols;
    img.h = rows;
    img.head.rows = rows;
    img.head.cols = cols;
    img.mat = vector <vector <RGB>>(rows, vector<RGB>(cols));
    return img;
}

void save_image(BMPimage img, string name)
{
    ofstream file(name, ios::binary);
    file.write((char*)&img.head, sizeof(BMPheader));
    for (int y = 0; y < img.h; y++)
    {
        for (int x = 0; x < img.w; x++)
            file.write((char*)&img.mat[y][x], sizeof(RGB));
        char zero = 0;
        for (int i = 0; i < img.w % 4; i++)
            file.write((char*)&zero, 1);
    }
    file.close();
}

using namespace concurrency;

// функция генерации картинки с изображённым на нём множеством Мандельброта 
// ((X, Y) - точка соответствующая центру, k - степень приближения, W - ширина, H - высота, colors - раскраска множества)
BMPimage draw_mandelbrot(double X, double Y, double k, int W, int H, const vector <RGB>& colors)
{
    int lim = colors.size() - 1;
    BMPimage img = new_image(H, W);
    vector <int> colors_n(H * W);
    array_view <int, 2> arr(H, W, colors_n);
    parallel_for_each(arr.extent, [=](index<2> idx) restrict(amp)
        {
            double x0 = idx[1] - W / 2.0;
            double y0 = idx[0] - H / 2.0;
            int n;
            double x = X + x0 * k;
            double y = Y + y0 * k;
            double xc = 0, yc = 0;
            for (int i = 0; i <= lim; i++)
            {
                if (i == lim)
                {
                    n = lim;
                    break;
                }
                if (xc * xc + yc * yc >= 4)
                {
                    n = i;
                    break;
                }
                double xn = xc * xc - yc * yc + x;
                double yn = 2 * xc * yc + y;
                xc = xn;
                yc = yn;
            }
            arr[idx] = n;
        });
    arr.synchronize();
    for (int y = 0; y < img.h; y++)
        for (int x = 0; x < img.w; x++)
            img.mat[y][x] = colors[colors_n[x + y * W]];
    return img;
}

// функция создания рекурентного градиента
void make_gradient(vector <RGB>& colors, int l, int r, int c = 0)
{
    if (l >= r)
        return;
    int m = (l + r) / 2;
    int n = max(m - l, 1);
    for (int i = l; i <= m; i++)
    {
        vector <int> col(3);
        for (int j = 0; j < 3; j++)
            col[j] = 255 * (i - l) / n;
        col[c % 3] = 0;
        colors[i] = RGB(col[0], col[1], col[2]);
    }
    make_gradient(colors, m + 1, r, c + 1 + rand() % 2);
}

int main()
{
    // пример использования
    double X = -0.56267837374;
    double Y = 0.65679461735;
    double k = 7 * 1e-7;
    int lim = 1000;
    vector <RGB> colors(lim + 1);
    make_gradient(colors, 0, lim);

    BMPimage img = draw_mandelbrot(X, Y, k, 4000, 3000, colors);
    save_image(img, "mandelbrot.bmp");
}
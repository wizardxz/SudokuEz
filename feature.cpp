/*
* This file is part of SudokuEz
*
* Copyright (C) 2012-2017 Zhong Xu
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*
*/

//#define SUDOKU_DEBUG

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d/features2d.hpp>
#include "ml.h"

using namespace std;
using namespace cv;

const int UNIFIED_LENGTH = 20;

bool extract_feature(Mat img, float feature[], Mat& processed_img)
{
    int sidelength = img.rows;
    img = img.rowRange((int)(sidelength * .1), (int)(sidelength * .9))
            .colRange((int)(sidelength * .1), (int)(sidelength * .9));

    Mat gray, bin;
    cvtColor(img, gray, CV_BGR2GRAY);
    int block_size = (int)(MIN(img.cols,img.rows) * 1.0)|1;

    adaptiveThreshold( gray, bin, 255,
        CV_ADAPTIVE_THRESH_MEAN_C, CV_THRESH_BINARY_INV, block_size, 3 );

    Mat erode_bin;
    erode(bin, erode_bin, Mat());
    Moments m = moments(erode_bin, true);
    if (m.m00 == 0)
        return false;
    int cx = int(m.m10 / m.m00 + .5);
//    int cy = int(m.m01 / m.m00 + .5);

    int r = (int)(sidelength * .25);
    if (cx - r < 0 || cx + r > erode_bin.cols)
        return false;

    Mat cropped_erode_bin = erode_bin.colRange(cx - r, cx + r);

    Mat uni_bin;
    resize(cropped_erode_bin, uni_bin,
           Size(UNIFIED_LENGTH, UNIFIED_LENGTH));

    uni_bin.copyTo(processed_img);

    int point_num = 0;
    for (int y = 0; y < UNIFIED_LENGTH; y++)
        for (int x = 0; x < UNIFIED_LENGTH; x++)
        {
            if (uni_bin.at<uchar>(y, x) != 0)
                point_num += 1;
        }
    if (point_num < 20)
        return false;

    int     left[UNIFIED_LENGTH], right[UNIFIED_LENGTH],
            top[UNIFIED_LENGTH], bottom[UNIFIED_LENGTH];

    for (int x = 0; x < UNIFIED_LENGTH; x++)
    {
        top[x] = UNIFIED_LENGTH;
        bottom[x] = 0;
        for (int y = 0; y < UNIFIED_LENGTH; y++)
        {
            if (uni_bin.at<uchar>(y, x) != 0)
            {
                if (y < top[x]) top[x] = y;
                if (y > bottom[x]) bottom[x] = y;
            }
        }
    }

    for (int y = 0; y < UNIFIED_LENGTH; y++)
    {
        left[y] = UNIFIED_LENGTH;
        right[y] = 0;
        for (int x = 0; x < UNIFIED_LENGTH; x++)
        {
            if (uni_bin.at<uchar>(y, x) != 0)
            {
                if (x < left[y]) left[y] = x;
                if (x > right[y]) right[y] = x;
            }
        }
    }

    for (int i = 0; i < UNIFIED_LENGTH; i++)
    {
        feature[i + UNIFIED_LENGTH * 0] = (float)top[i];
        feature[i + UNIFIED_LENGTH * 1] = (float)bottom[i];
        feature[i + UNIFIED_LENGTH * 2] = (float)left[i];
        feature[i + UNIFIED_LENGTH * 3] = (float)right[i];

    }

    return true;
}


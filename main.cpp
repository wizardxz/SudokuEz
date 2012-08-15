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

#include <ctime>
#include <iostream>
#include <sstream>
#include <fstream>
#include <dirent.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <ml.h>
#include "box.h"

using namespace std;
using namespace cv;

const int MAX_SAMPLES = 1000;
const int FEATURE_SIZE = 80;
const int RESIZED_IMG_ROWS = 1000;


bool get_cropped_imgs(Mat, Mat[], Rect[], vector<Box>&);
bool extract_feature(Mat, float[], Mat&);
bool go(int data[], int, int[]);

const char* keys =
{
    "{     m|      mode|            rec| working mode : rec(recognition), col(collection), tra(train)}"
    "{     c|    camera|          false| with camera}"
    "{     f|  filename|       news.jpg| filename}"
    "{     s|       svm| train_data/svm| support vector mechine}"
    "{     p|  pictures|     train_data| picture directory}"
};

bool get_solution(Mat cropped_imgs[], CvSVM& svm, int data[], int result[])
{
    //recognize numbers
    for (int i = 0; i < 81; i++)
    {
        float feature[FEATURE_SIZE];
        int value = 0;
        Mat pimg;
        if (extract_feature(cropped_imgs[i], feature, pimg))
        {
            Mat test(1, FEATURE_SIZE, CV_32FC1, Scalar::all(0));
            for (int j = 0; j < FEATURE_SIZE; j++)
                test.at<float>(0, j) = feature[j];
            value = (int)svm.predict(test);
        }
        data[i] = value;
    }
    //solve sudoku
    for (int i = 0; i < 81; i++)
        result[i] = data[i];
    return go(data, 0, result);
}

void draw_solution(Mat& img, int data[], int result[], Rect rects[])
{
    for (int i = 0; i < 81; i++)
    {
        Point p1(rects[i].x, rects[i].y);
        Point p2(rects[i].x + rects[i].width, rects[i].y + rects[i].height);
        rectangle(img, p1, p2, Scalar(0, 0, 255), 3);
    }
    for (int i = 0; i < 81; i++)
    {
        Point p1(rects[i].x, rects[i].y);
        Point p2(rects[i].x + rects[i].width, rects[i].y + rects[i].height);
        Point p3(rects[i].x, rects[i].y + rects[i].height);
        stringstream ss;
        ss << result[i];
        Scalar color;
        if (data[i] == 0)
            color = Scalar(0, 0, 255);
        else
            color = Scalar(255, 0, 0);
        putText(img, ss.str(), p3, FONT_HERSHEY_SIMPLEX, 2, color, 2);
    }
}

void draw_detected_boxes(Mat& img, vector<Box>& detected_boxes)
{
    vector<vector<Point> > boxes_contours;
    for (vector<Box>::iterator ib = detected_boxes.begin(); ib < detected_boxes.end(); ib++)
    {
        boxes_contours.push_back(ib->get_contour());
    }

    drawContours(img, boxes_contours, -1, Scalar(0, 255, 0), 3);
}

void recognition_by_camera(string svm_filename)
{
    //load svm
    CvSVM svm = CvSVM();
    svm.load(svm_filename.c_str());

    VideoCapture cap;
    cap.open(0);

    if( !cap.isOpened() )
    {
        cout << "Can not open camera." << endl;
        return;
    }

    namedWindow("result", CV_WINDOW_AUTOSIZE);

    while (true)
    {
        Mat src_img;

        cap >> src_img;
        Mat img;
        resize(src_img, img, Size(src_img.cols * RESIZED_IMG_ROWS / src_img.rows, RESIZED_IMG_ROWS));


        Mat cropped_imgs[81];
        Rect rects[81];
        vector<Box> detected_boxes;
        bool succeed = false;

        if (get_cropped_imgs(img, cropped_imgs, rects, detected_boxes))
        {
            int data[81], result[81];
            succeed = get_solution(cropped_imgs, svm, data, result);

            if (succeed)
            {
                for (int i = 0; i < 81; i++)
                {
                    cout << result[i];
                    if ((i + 1) % 9 == 0) cout << endl;
                }
                cout << endl;
            }

            draw_solution(img, data, result, rects);
        }
        else
        {
            draw_detected_boxes(img, detected_boxes);
        }
        imshow("result", img);
        char k = (char)waitKey(succeed?15000:30);
        succeed = false;
        if( k == 27 ) break;

    }
}

void recognition_by_filename(string svm_filename, string filename)
{
    //load svm
    CvSVM svm = CvSVM();
    svm.load(svm_filename.c_str());

    Mat src_img = imread(filename);
    Mat img;
    resize(src_img, img, Size(src_img.cols * RESIZED_IMG_ROWS / src_img.rows, RESIZED_IMG_ROWS));
#ifdef SUDOKU_DEBUG
    cout << "before " << src_img.rows << "," << src_img.cols << endl;
    cout << "after " << img.rows << "," << img.cols << endl;
#endif

    Mat cropped_imgs[81];
    Rect rects[81];
    vector<Box> detected_boxes;

    if (get_cropped_imgs(img, cropped_imgs, rects, detected_boxes))
    {
        int data[81], result[81];
        get_solution(cropped_imgs, svm, data, result);

        for (int i = 0; i < 81; i++)
        {
            cout << result[i];
            if ((i + 1) % 9 == 0) cout << endl;
        }

        draw_solution(img, data, result, rects);

        imwrite("result.png", img);
        namedWindow("result", CV_WINDOW_NORMAL);
        imshow("result", img);
        waitKey(0);
    }
}

void collection(string filename, string pictures_directory)
{
    Mat src_img = imread(filename);
    Mat cropped_imgs[81];
    Rect rects[81];
    vector<Box> detected_boxes;

    if (get_cropped_imgs(src_img, cropped_imgs, rects, detected_boxes))
    {
        time_t t = time(0);
        for (int y = 0; y < 9; y++)
        {
            for (int x = 0; x < 9; x++)
            {
                stringstream ss;
                ss << pictures_directory << "unsorted/" << t << y << x << ".png";
                cout << "Save "<< ss.str() << endl;
                imwrite(ss.str(), cropped_imgs[y * 9 + x]);
            }
        }
        cout << "Collection completed. You can classify images manually now." << endl;
    }
}

void train(string svm_filename, string pictures_directory)
{
    Mat src(MAX_SAMPLES, FEATURE_SIZE, CV_32FC1, Scalar::all(0));
    Mat dest(MAX_SAMPLES, 1, CV_32FC1, Scalar::all(0));
    int num = 0;
#ifdef SUDOKU_DEBUG
    fstream fout;
    fout.open("debug/characters.csv", fstream::out);
#endif

    for (int i = 0; i <= 9; i++)
    {
        stringstream path, debug_path;
        path << pictures_directory << i;

        DIR *pdir = opendir(path.str().c_str());
        struct dirent* ent = NULL;
        while (NULL != (ent = readdir(pdir)))
        {
            if (ent->d_type == 8) // file
            {
                stringstream filename;
                filename << path.str() << "/" << ent->d_name;
                Mat img = imread(filename.str().c_str());

                float feature[FEATURE_SIZE];
                Mat pimg;
                if (extract_feature(img, feature, pimg))
                {
#ifdef SUDOKU_DEBUG
                    fout << i << " ";
#endif
                    for (int j = 0; j < FEATURE_SIZE; j++)
                    {
                        src.at<float>(num, j) = feature[j];
#ifdef SUDOKU_DEBUG
                        fout << feature[j] << " ";
#endif
                    }
#ifdef SUDOKU_DEBUG
                    fout << endl;
#endif
                    dest.at<float>(num, 0) = (float)i;
                    num += 1;
                }
            }
        }
    }
#ifdef SUDOKU_DEBUG
    fout.close();
#endif
    src = src.rowRange(0, num);
    dest = dest.rowRange(0, num);

    CvSVM svm = CvSVM();
	CvSVMParams param;
	CvTermCriteria criteria;
	criteria = cvTermCriteria( CV_TERMCRIT_EPS, 1000, FLT_EPSILON );
	param = CvSVMParams( CvSVM::C_SVC, CvSVM::LINEAR, 10.0, 0.09, 1.0,
		10.0, 0.5, 1.0, NULL, criteria );
	svm.train( src, dest, Mat(), Mat(), param );

    svm.save(svm_filename.c_str());
    cout << src.rows << " samples are trained, result is " << svm_filename << endl;
}

int main( int argc, const char** argv )
{
    CommandLineParser parser(argc, argv, keys);

    string mode = parser.get<string>("mode");
    bool use_camera = parser.get<bool>("camera");
    string filename = parser.get<string>("filename");
    string svm_filename = parser.get<string>("svm");
    string pictures_directory = parser.get<string>("pictures");
    if (pictures_directory[pictures_directory.length() - 1] != '/')
        pictures_directory = pictures_directory + "/";

    if (mode == "rec")
    {
        if (use_camera)
            recognition_by_camera(svm_filename);
        else
            recognition_by_filename(svm_filename, filename);
    }
    else if (mode == "col")
    {
        if (use_camera)
            cout << "Camera can be only used in Recognition mode." << endl;
        else
            collection(filename, pictures_directory);
    }
    else if (mode == "tra")
    {
        train(svm_filename, pictures_directory);
    }
    else
        cout << "Invalid mode." << endl;

    return 0;
}


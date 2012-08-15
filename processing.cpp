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

#include <iostream>
#include <sstream>
#include <fstream>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <ml.h>
#include <algorithm>
#include <numeric>
#include "box.h"

using namespace std;
using namespace cv;

const int MIN_BOX_SIZE = 200;
const int MAX_APPROX = 10;
const double FITTING_SQUARE_AREA_RATIO = .8;
const int MIN_BOXES_DISTANCE = 3;
const int FEATURE_SIZE = 80;
const int MAX_SAMPLES = 1000;


bool extract_feature(Mat, float [], Mat&);

void morphology_filter(vector<vector<Point> >& contours, vector<Box>& boxes)
{
    boxes.clear();
    for (vector<vector<Point> >::iterator pc = contours.begin();
        pc < contours.end(); pc++)
    {
        //reject contours with too small perimeter
        if (contourArea(*pc) < MIN_BOX_SIZE)
            continue;

        vector<Point> poly_contour;
        for (int approx = 0; approx < MAX_APPROX; approx++)
        {
            approxPolyDP(*pc, poly_contour, approx, true);

            approxPolyDP(poly_contour, poly_contour, approx, true);

            //only 4-side polygons are accepted
            if (poly_contour.size() != 4)
                continue;

            Box box(poly_contour);

            //polygons are approx square
            int length = box.get_sidelength();
            double area = box.get_area();
            double rarea = pow(length, 2);
            if (area / rarea < FITTING_SQUARE_AREA_RATIO)
                continue;

            boxes.push_back(box);

        }
    }
}

void majority_filter(vector<Box>& boxes)
{
    double sum_sl = 0.0;
    for (vector<Box>::iterator ib = boxes.begin();
        ib < boxes.end(); ib++)
    {
        sum_sl += ib->get_sidelength();
    }
    double avg_sl = sum_sl / boxes.size();

    for (vector<Box>::iterator ib = boxes.begin(); ib < boxes.end();)
    {
        double var = abs(ib->get_sidelength() / avg_sl - 1);
        if (var > 0.3)
            ib = boxes.erase(ib);
        else
            ib += 1;
    }
}

double dist(Point a, Point b)
{
    return sqrt(pow(a.x - b.x, 2) + pow(a.y - b.y, 2));
}

void distinct_filter(vector<Box>& boxes)
{
    for (vector<Box>::iterator ib1 = boxes.begin(); ib1 < boxes.end(); ib1++)
    {
        for (vector<Box>::iterator ib2 = ib1 + 1; ib2 < boxes.end();)
        {
            if (dist(ib1->get_center(), ib2->get_center()) < MIN_BOXES_DISTANCE)
                ib2 = boxes.erase(ib2);
            else
                ib2 += 1;
        }
    }
}

//get boxes' offsets from one original box.
bool get_offset(Box* origin, Point pos, vector<Box>& boxes,
                double& length, map<Box*, Point >& offset,
                int& min_x, int& min_y, int& max_x, int& max_y)
{
    int x0 = pos.x, y0 = pos.y;

    vector<Box*> visited;

    //search neighbour boxes from near to far

    for (int l = 1; l < 9; l++)//9 levels(distance)
    {
        //l =
        //9999999999999999999
        //9888888888888888889
        //...
        //9876543222223456789
        //9876543211123456789
        //9876543210123456789
        //9876543211123456789
        //9876543222223456789
        //...
        //9888888888888888889
        //9999999999999999999
        for (int i = 0; i < 4; i++)// 4 directions
        {
            //i =
            //30000
            //33001
            //33*11
            //32211
            //22221
            for (int da = (i<2?-1:1) * (l - 1); da != (i<2?1:-1) * (l + 1); da += (i<2?1:-1))
            {
                //get dx, dy from l and i
                int dp = ((i+1)%4<2?-1:1) * l;
                int dx, dy;
                if (i%2 == 0)   { dx = da; dy = dp; } // x-axis change
                else            { dx = dp; dy = da; } // y-axis change

                bool has_visited = false;
                for (map<Box*, Point >::iterator io = offset.begin();
                    io != offset.end(); io++)
                {
                    Point visited_pos = io->second;
                    if (dx + x0 == visited_pos.x && dy + y0 == visited_pos.y)
                    {
                        has_visited = true;
                        break;
                    }
                }
                if (has_visited)
                    continue;

                double dl = sqrt(dx * dx + dy * dy);
                for (vector<Box>::iterator ib = boxes.begin(); ib < boxes.end(); ib++)
                {
                    Point exp = origin->get_center() + Point((int)(dx * length), (int)(dy * length));//expected pos
                    double error = dist(ib->get_center(), exp);
                    if (error < length * .2 || error < MIN(dl * length * .1, error < length * .9))
                    {   //captured
                        offset[&(*ib)] = Point(x0 + dx, y0 + dy);
                        visited.push_back(&(*ib));

                        if (x0 + dx < min_x) min_x = x0 + dx;
                        if (x0 + dx > max_x) max_x = x0 + dx;
                        if (y0 + dy < min_y) min_y = y0 + dy;
                        if (y0 + dy > max_y) max_y = y0 + dy;

                        //adjust length
                        length = dist(ib->get_center(), origin->get_center()) / dl;

                    }
                }
            }
        }
    }

    if (max_x - min_x == 8 && max_y - min_y == 8)
        return true;

    // if we can not find boxes on all boundaries, try to search recursively from another box.
    for (vector<Box*>::reverse_iterator ib = visited.rbegin(); ib < visited.rend(); ib++)
    {
        Point pos = offset[*ib];

        bool result = get_offset(*ib, pos, boxes,
                                 length, offset,
                                 min_x, min_y, max_x, max_y);
        if (result == true)
            return true;
    }

    return false;
}

//calculate cof_mat from offset by the least squares technique. the cof_mat is used to obtain fitted coords.
Mat get_cof_mat(Box* origin, map<Box*, Point>& offset)
{
    Mat m(offset.size(), 2, CV_64FC1), s(offset.size(), 2, CV_64FC1);
    int x0 = origin->get_center().x;
    int y0 = origin->get_center().y;
    int row = 0;
    for (map<Box*, Point>::iterator io = offset.begin(); io != offset.end(); io++)
    {
        Box* box = io->first;
        int offx = io->second.x;
        int offy = io->second.y;
        int x = box->get_center().x - x0;
        int y = box->get_center().y - y0;

        m.at<double>(row, 0) = offx;
        m.at<double>(row, 1) = offy;
        s.at<double>(row, 0) = x;
        s.at<double>(row, 1) = y;
        row += 1;
#ifdef SUDOKU_DEBUG
        cout << box->num << "\t" << offx << "\t" << offy << endl;
#endif
    }
    return (m.t() * m).inv() * m.t() * s;
}

Point get_fitted_coord(Point pos, Mat cof, Point origin_pos)
{
    Mat pos_mat(pos);
    pos_mat.convertTo(pos_mat, CV_64FC1);
    Mat result = pos_mat.t() * cof;
    return Point((int)result.at<double>(0, 0), (int)result.at<double>(0, 1)) + origin_pos;
}

#ifdef SUDOKU_DEBUG
void show_boxes(Mat img, vector<Box>& boxes, Scalar color)
{
    vector<vector<Point> > boxes_contours;
    for (vector<Box>::iterator ib = boxes.begin(); ib < boxes.end(); ib++)
    {
        boxes_contours.push_back(ib->get_contour());
    }

    drawContours(img, boxes_contours, -1, color, 3);
    namedWindow("selected boxes", CV_WINDOW_NORMAL);
    imshow("selected boxes", img);
    waitKey(0);
}
#endif

bool get_cropped_imgs(Mat img, Mat cropped_imgs[], Rect rects[], vector<Box>& detected_boxes)
{
    //convert to binary
    Mat gray, bin;
    cvtColor(img, gray, CV_BGR2GRAY);
    int block_size = (int)(MIN(img.cols,img.rows) * 0.1)|1;

    adaptiveThreshold( gray, bin, 255,
        CV_ADAPTIVE_THRESH_MEAN_C, CV_THRESH_BINARY_INV, block_size, 3 );

    //dilate img
    Mat dil_bin;
    dilate(bin, dil_bin, Mat(), Point(-1,-1), 2);

#ifdef SUDOKU_DEBUG
    cout << "block_size = " << block_size << endl;
    namedWindow("dil_bin", CV_WINDOW_NORMAL);
    imshow("dil_bin", dil_bin);
    waitKey(0);
#endif

    //find contours of img
    vector<vector<Point> > contours;
    vector<Vec4i> hierarchy;
    findContours(dil_bin, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE);

#ifdef SUDOKU_DEBUG
    cout << contours.size() << " contours found.\n";
    Mat img1 = img.clone();
    drawContours(img1, contours, -1, Scalar(0, 0, 255), 1);
    namedWindow("contours", CV_WINDOW_NORMAL);
    imshow("contours", img1);
    waitKey(0);
#endif

    //get boxes from contours
    vector<Box> boxes;
    morphology_filter(contours, boxes);
#ifdef SUDOKU_DEBUG
    show_boxes(img.clone(), boxes, Scalar(255, 0, 0));
#endif
    detected_boxes = boxes;

    majority_filter(boxes);
#ifdef SUDOKU_DEBUG
    show_boxes(img.clone(), boxes, Scalar(0, 255, 0));
#endif
    distinct_filter(boxes);

#ifdef SUDOKU_DEBUG
    show_boxes(img.clone(), boxes, Scalar(0, 0, 255));
#endif


#ifdef SUDOKU_DEBUG
    fstream fout;
    fout.open("debug/coords.txt", fstream::out);
    for (vector<Box>::iterator ib = boxes.begin(); ib < boxes.end(); ib++)
    {
        vector<Point> contour = ib->get_contour();
        Point center = ib->get_center();
        fout << ib->num << " " << center.x << " " << center.y << " ";
        for (vector<Point>::iterator ip = contour.begin(); ip < contour.end(); ip++)
        {
            fout << ip->x << " " << ip->y << " ";
        }
        fout << endl;
    }
    fout.close();
#endif

    if (boxes.size() == 0)
        return false;
    //get offset of important boxes by finding neighbours of boxes
    Box* origin = &boxes[0];
    double length = origin->get_sidelength() * 1.1;
    map<Box*, Point > offset;
    offset[origin] = Point(0, 0);

    int min_offx = 0, min_offy = 0, max_offx = 0, max_offy = 0;
    bool succeed = get_offset(origin, offset[origin], boxes,
                              length, offset,
                              min_offx, min_offy, max_offx, max_offy);
    if (succeed)
    {
        Mat cof = get_cof_mat(origin, offset);

        int r = (int)(length / 2);
        for (int y = 0; y < 9; y++)
        {
            int dy = y + min_offy;
            for (int x = 0; x < 9; x++)
            {
                int dx = x + min_offx;
                Point fp = get_fitted_coord(Point(dx, dy), cof, origin->get_center());

                cropped_imgs[y * 9 + x] = img.rowRange(fp.y - r, fp.y + r).colRange(fp.x - r, fp.x + r);
                rects[y * 9 + x] = Rect(fp.x - r, fp.y - r, 2 * r, 2 * r);
            }
        }
    }

    return succeed;
}


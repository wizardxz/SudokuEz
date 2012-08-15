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

bool h_test(int result[], int n, int i)
{
    int y = n / 9;
    bool exist[] = {false, false, false, false, false, false, false, false, false};

    for (int x = 0; x < 9; x++)
    {
        int cur = y * 9 + x;
        int v = result[cur];
        if (v != 0)
            exist[v - 1] = true;
    }
    return !exist[i - 1];
}

bool v_test(int result[], int n, int i)
{
    int x = n % 9;
    bool exist[] = {false, false, false, false, false, false, false, false, false};
    for (int y = 0; y < 9; y++)
    {
        int cur = y * 9 + x;
        int v = result[cur];
        if (v != 0)
            exist[v - 1] = true;
    }
    return !exist[i - 1];
}

bool b_test(int result[], int n, int i)
{
    int y0 = n / 9 / 3 * 3;
    int x0 = n % 9 / 3 * 3;
    bool exist[] = {false, false, false, false, false, false, false, false, false};
    for (int y = 0; y < 3; y++)
    {
        for (int x = 0; x < 3; x++)
        {
            int cur = (y + y0) * 9 + x + x0;
            int v = result[cur];
            if (v != 0)
                exist[v - 1] = true;
        }
    }
    return !exist[i - 1];
}

bool go(int data[], int n, int result[])
{
    if (n == 81)
    {
        return true;
    }

    if (data[n] == 0)
    {
        for (int i = 1; i <= 9; i++)
        {
            if (h_test(result, n, i) &&
                v_test(result, n, i) &&
                b_test(result, n, i))
            {
                result[n] = i;
                if (go(data, n + 1, result))
                    return true;
            }
        }
        result[n] = 0;
    }
    else
    {
        if (go(data, n + 1, result))
            return true;
    }
    return false;
}

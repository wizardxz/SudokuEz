SudokuEz
========

Sudoku game solver using camera-captured image or static image file.

#.  Recognition with camera::

        ./sudoku -c

#.  Recognition with static image file::

        ./sudoku -f news.jpg

#.  Collecting images::

        ./sudoku -m col -f news.jpg -p train_data

    After executing this command, you will see lots of unclassified images in
    the directory of ``train_data/unclassified/``.  You must move these images
    into corresponding directories in ``train_data/``

#.  Train based on your collected images::

        ./sudoku -m tra -s train_data/svn
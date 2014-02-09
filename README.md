Recipe Manager for Space Station 13

Licensed under GPLv3

Copyright 2013 by mysha

Recipe lists
------------
You can download the most recent public recipe lists here: http://myshak.github.io/

Running
-------
The latest executable version for Windows can be downloaded at https://github.com/myshak/ss13recipemanager/releases/latest

For Linux, OSX and other operating systems you will have to compile it yourself.

Compilation
-----------
Required are: a compiler supporting C++11 (g++ >= 4.6, clang >= 3.1) and Qt (version 4.8).

Under Debian/Ubuntu you can install the dependencies by invoking:

    # sudo apt-get install build-essential libqtcore4 libqtgui4 libqt4-dev qt4-qmake

You can then build it by invoking:

    # git clone https://github.com/myshak/ss13recipemanager.git
    # mkdir build
    # cd build
    # qmake ../ss13recipemanager/ss13recipes.pro
    # make 

You can also use QtCreator to build it.

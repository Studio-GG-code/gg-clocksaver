**To change the 'title' or counter:**


If you have the C++ and Qt development libraries installed, you can compile the program manually like this:

g++ main.cpp -o gg-clocksaver $(pkg-config --cflags --libs Qt6Widgets) -lX11 -lXss

// Graphics operation adaptation module for 2D

#ifndef GraphOperaAdapt2dH
#define GraphOperaAdapt2dH

#include "SColor.h"

class GOA2D {
public:
    GOA2D() {}
    ~GOA2D() {}

    static void DrawPoint(int x, int y, SColor color) {
        // Draw a point at (x, y) with color
    }
    static void DrawLine(int x1, int y1, int x2, int y2, SColor color) {
        // Draw a line from (x1, y1) to (x2, y2) with color
    }

    static void DrawRect(int x, int y, int width, int height, SColor color) {
        // Draw a rectangle with top-left corner at (x, y), width, height, and color

    }
    static void DrawCircle(int x, int y, int radius, SColor color) {
        // Draw a circle with center at (x, y), radius, and color
    }
    static void DrawText(int x, int y, const char* text, SColor color) {
        // Draw text at position (x, y) with color

    }

};

#endif // GraphOperaAdapt2dH
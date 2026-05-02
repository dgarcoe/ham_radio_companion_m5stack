#pragma once

// Hand-curated, very-low-resolution continental coastline outlines for the
// grayline map. Each polyline is a sequence of (lat, lon) vertices connected
// with straight line segments. Resolution is intentionally coarse (~1-3
// degrees) so the data fits in <2 KB of flash and renders cleanly on a
// 320x188 equirectangular map where each pixel is roughly 1 degree wide.

namespace Coastlines {

struct Point {
    float lat;
    float lon;
};

struct Polyline {
    const Point* pts;
    int          n;
    bool         closed;   // if true, draws the closing segment back to pts[0]
};

extern const Polyline kPolylines[];
extern const int      kPolylineCount;

}

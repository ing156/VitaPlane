#include "chat_bubble.h"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <utility>
#include <vector>

struct Message {
    int64_t sender_id = 1, date = 1000;
    bool sender_chat = false, outgoing = false, service_message = false;
};
using chat_bubble::Point;
static float cross(Point a, Point b, Point p) {
    return (b.x-a.x)*(p.y-a.y)-(b.y-a.y)*(p.x-a.x);
}
static bool triangle(Point p, Point a, Point b, Point c) {
    const float x = cross(a,b,p), y = cross(b,c,p), z = cross(c,a,p);
    return (x > 0 && y > 0 && z > 0) || (x < 0 && y < 0 && z < 0);
}
static bool inside(Point p, const chat_bubble::Shape& s) {
    bool result = false;
    for (int i = 0, j = s.count-1; i < s.count; j = i++) {
        const auto a = s.points[i], b = s.points[j];
        if ((a.y > p.y) != (b.y > p.y) && p.x < (b.x-a.x)*(p.y-a.y)/(b.y-a.y)+a.x)
            result = !result;
    }
    return result;
}
int main() {
    using chat_bubble::joins;
    Message a, b;
    assert(joins(a,b));
    b.date += 300; assert(joins(a,b));
    ++b.date; assert(joins(a,b));
    // Displayed minutes hide seconds: 16:57:01 -> 17:02:59 must stay grouped.
    a.date = 16*3600 + 57*60 + 1;
    b.date = 17*3600 + 2*60 + 59;
    assert(joins(a,b));
    b.date += 3600; assert(joins(a,b));
    b = a; --b.date; assert(!joins(a,b));
    b = a; b.sender_id = 2; assert(!joins(a,b));
    b = a; b.sender_chat = true; assert(!joins(a,b));
    b = a; b.outgoing = true; assert(!joins(a,b));
    b = a; b.service_message = true; assert(!joins(a,b) && !joins(b,a));
    b = a; b.date = 0; assert(!joins(a,b) && !joins(b,a));
    a.sender_id = b.sender_id = 0; assert(!joins(a,b));

    struct Row {
        int index, top, height;
        bool after;
        int gap() const { return after ? 2 : 6; }
    };
    using Position = std::pair<int, int>;
    auto avatars = [](const std::vector<Row>& rows, int top, int bottom) {
        std::vector<Position> positions;
        chat_bubble::for_each_avatar(rows, top, bottom,
            [&](int index, int y) { positions.emplace_back(index, y); });
        return positions;
    };
    assert(avatars({}, 70, 544).empty());
    // One avatar at the group's bottom, including when its first messages are offscreen.
    assert((avatars({{0,70,80,true}, {1,150,80,false}}, 70, 544) ==
        std::vector<Position>{{1,196}}));
    assert((avatars({{7,-10,90,true}, {8,80,80,false}}, 70, 544) ==
        std::vector<Position>{{8,126}}));
    // The last visible row may continue below the viewport or onto the next loaded page.
    for (int bottom : {494, 506, 544})
        assert((avatars({{0,-200,800,true}}, 70, bottom) ==
            std::vector<Position>{{0,bottom-chat_bubble::avatar_size}}));
    // Scrolling pushes the old avatar out with its group instead of into the next sender.
    assert((avatars({{0,10,80,false}, {1,90,120,false}}, 70, 544) ==
        std::vector<Position>{{0,56}, {1,176}}));
    assert((avatars({{0,-10,80,false}, {1,70,120,false}}, 70, 544) ==
        std::vector<Position>{{1,156}}));
    assert(avatars({{0,544,80,false}}, 70, 544).empty());
    // A date/service boundary splits groups even when both sides have the same sender.
    assert((avatars({{0,70,80,false}, {1,176,80,false}}, 70, 544) ==
        std::vector<Position>{{0,116}, {1,222}}));

    const std::vector<Row> end{{2,400,100,false}};
    assert(!chat_bubble::has_messages_below(std::vector<Row>{}, 0, false, 494));
    assert(!chat_bubble::has_messages_below(end, 3, true, 494));
    assert(chat_bubble::has_messages_below(end, 3, true, 493));
    assert(chat_bubble::has_messages_below(end, 4, true, 544));
    assert(chat_bubble::has_messages_below(end, 3, false, 544));
    assert(chat_bubble::has_messages_below(end, 3, true, 312));

    for (bool out : {false,true}) for (bool before : {false,true}) for (bool after : {false,true})
        for (float width : {162.f, 408.f, 712.f}) for (float height : {64.f, 180.f}) {
            const auto s = chat_bubble::shape(20,30,width,height,out,before,after);
            assert(s.count <= (int)s.points.size());
            auto outer = s;
            for (int i = 0; i < s.count; ++i) {
                const auto n = chat_bubble::outward(s,i);
                assert(std::isfinite(n.x) && std::isfinite(n.y));
                outer.points[i] = {s.points[i].x+2*n.x, s.points[i].y+2*n.y};
                assert(cross(s.center,s.points[i],s.points[(i+1)%s.count]) >= -0.001f);
            }
            // Compare actual fan/ring triangle coverage against independent polygon hit testing.
            // Fractional samples avoid shared triangle edges, which the GPU owns only once.
            for (float y = 27.173f; y < 33+height; y += 2.719f)
                for (float x = 10.237f; x < 30+width; x += 2.317f) {
                    const Point p{x,y};
                    int fills = 0, borders = 0;
                    for (int i = 0; i < s.count; ++i) {
                        const int j = (i+1)%s.count;
                        fills += triangle(p,s.center,s.points[i],s.points[j]);
                        borders += triangle(p,s.points[i],outer.points[i],s.points[j]);
                        borders += triangle(p,outer.points[i],s.points[j],outer.points[j]);
                    }
                    assert(fills == (inside(p,s) ? 1 : 0));
                    assert(borders == (inside(p,outer) && !inside(p,s) ? 1 : 0));
                    assert(fills+borders <= 1);
                }
            float min_x = 20, max_x = 20+width;
            for (int i = 0; i < s.count; ++i) {
                min_x = std::min(min_x,s.points[i].x);
                max_x = std::max(max_x,s.points[i].x);
            }
            assert((min_x < 20) == (!out && !after));
            assert((max_x > 20+width) == (out && !after));
        }
    std::puts("chat bubble grouping and single-coverage geometry: OK");
}

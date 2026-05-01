#include "contests.h"

#include <algorithm>

namespace Contests {

// One row per recurring contest. weekN: 1=first Saturday, 2=second, -1=last.
// durationDays: 2 = Sat-Sun, 1 = single day. All times are UTC.
struct ContestDef {
    const char* name;
    const char* mode;
    int   month;        // 1-12
    int   weekN;        // Nth Saturday of month, or -1 for last
    int   startHourUtc; // start hour (UTC)
    int   durationHours;
};

// Curated list of major HF contests. Approximate to standard rules - rarely
// off by more than an hour, perfectly fine for "what's on this weekend".
static const ContestDef kDefs[] = {
    { "ARRL RTTY Roundup",   "RTTY", 1,  1,  18, 30 },
    { "NA QSO Party CW",     "CW",   1,  2,  18, 12 },
    { "NA QSO Party SSB",    "SSB",  1,  3,  18, 12 },
    { "ARRL DX CW",          "CW",   2,  3,   0, 48 },
    { "ARRL DX SSB",         "SSB",  3,  1,   0, 48 },
    { "CQ WPX SSB",          "SSB",  3, -1,   0, 48 },
    { "CQ WPX CW",           "CW",   5, -1,   0, 48 },
    { "ARRL Field Day",      "MIX",  6,  4,  18, 24 },
    { "IARU HF World Champ", "MIX",  7,  2,  12, 24 },
    { "WAE DX CW",           "CW",   8,  2,   0, 48 },
    { "WAE DX SSB",          "SSB",  9,  2,   0, 48 },
    { "CQ WW RTTY",          "RTTY", 9, -1,   0, 48 },
    { "CQ WW DX SSB",        "SSB", 10, -1,   0, 48 },
    { "ARRL Sweepstakes CW", "CW",  11,  1,  21, 30 },
    { "ARRL Sweepstakes SSB","SSB", 11,  3,  21, 30 },
    { "CQ WW DX CW",         "CW",  11, -1,   0, 48 },
    { "ARRL 160m",           "CW",  12,  1,  22, 48 },
    { "ARRL 10m",            "MIX", 12,  2,   0, 48 },
    { "Stew Perry TBDC",     "CW",  12, -1,  15, 24 },
};
static constexpr int kDefCount = sizeof(kDefs) / sizeof(kDefs[0]);

static std::vector<Contest> s_upcoming;
static int s_lastDayKey = -1;

static int daysInMonth(int year, int month) {
    static const int dim[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    int d = dim[month - 1];
    if (month == 2 && ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0)) d = 29;
    return d;
}

// First Saturday of (year, month), as day-of-month (1..31).
static int firstSatOfMonth(int year, int month) {
    struct tm t = {};
    t.tm_year = year - 1900;
    t.tm_mon  = month - 1;
    t.tm_mday = 1;
    t.tm_hour = 12;       // safely inside the day
    mktime(&t);           // populates tm_wday (Sun=0..Sat=6)
    int wday = t.tm_wday;
    int daysToSat = (6 - wday + 7) % 7;
    return 1 + daysToSat;
}

// Nth Saturday of (year, month). n=1..N or -1 for last.
static int saturdayOfMonth(int year, int month, int n) {
    int firstSat = firstSatOfMonth(year, month);
    if (n == -1) {
        int last = firstSat;
        int dim = daysInMonth(year, month);
        while (last + 7 <= dim) last += 7;
        return last;
    }
    int day = firstSat + (n - 1) * 7;
    return day;
}

// Build a UTC time_t from broken-down components. Assumes the lwIP TZ has
// already been configured to "UTC0" via configTime(0,0,...) at boot.
static time_t makeUtcTime(int year, int month, int day, int hour, int minute) {
    struct tm t = {};
    t.tm_year = year - 1900;
    t.tm_mon  = month - 1;
    t.tm_mday = day;
    t.tm_hour = hour;
    t.tm_min  = minute;
    t.tm_sec  = 0;
    return mktime(&t);
}

static Contest contestFromDef(const ContestDef& d, int year) {
    int day = saturdayOfMonth(year, d.month, d.weekN);
    Contest c;
    c.name = d.name;
    c.mode = d.mode;
    c.startUtc = makeUtcTime(year, d.month, day, d.startHourUtc, 0);
    c.endUtc   = c.startUtc + d.durationHours * 3600L;
    return c;
}

bool ready() { return time(nullptr) > 1700000000; }

const std::vector<Contest>& upcoming() { return s_upcoming; }

void refresh() {
    s_upcoming.clear();
    if (!ready()) return;

    time_t now = time(nullptr);
    struct tm tnow;
    gmtime_r(&now, &tnow);
    int year = tnow.tm_year + 1900;

    // Generate contests for this year and next; keep ones that haven't ended.
    for (int yr = year; yr <= year + 1; yr++) {
        for (int i = 0; i < kDefCount; i++) {
            Contest c = contestFromDef(kDefs[i], yr);
            if (c.endUtc < now) continue;
            s_upcoming.push_back(c);
        }
    }
    std::sort(s_upcoming.begin(), s_upcoming.end(),
              [](const Contest& a, const Contest& b) {
                  return a.startUtc < b.startUtc;
              });
    if (s_upcoming.size() > 24) s_upcoming.resize(24);
}

void begin() {
    s_lastDayKey = -1;
}

void loop() {
    if (!ready()) return;
    time_t now = time(nullptr);
    int dayKey = (int)(now / 86400);     // refresh once per UTC day
    if (dayKey != s_lastDayKey) {
        s_lastDayKey = dayKey;
        refresh();
    }
}

}

#include "sat_sgp4.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

static const double RE    = 6378.137;        // km, WGS84 equatorial radius
static const double GM    = 398600.4418;     // km^3/s^2
static const double J2    = 1.08262998905e-3;
static const double F     = 1.0 / 298.257223563;
static const double E2    = F * (2.0 - F);   // first eccentricity squared

static const double PI    = 3.14159265358979323846;
static const double TWOPI = 2.0 * PI;
static const double D2R   = PI / 180.0;
static const double R2D   = 180.0 / PI;

double jdFromUnix(time_t t) {
    return 2440587.5 + (double)t / 86400.0;
}
time_t unixFromJD(double jd) {
    return (time_t)((jd - 2440587.5) * 86400.0);
}

// "YYDDD.FFFFFFFF" -> Julian date.
static double tleEpochJD(const char* s) {
    double epoch = atof(s);
    int yr2 = (int)(epoch / 1000.0);
    double doy = epoch - yr2 * 1000.0;        // 1-based day-of-year (Jan 1 = 1.0)
    int yr = (yr2 >= 57) ? 1900 + yr2 : 2000 + yr2;
    // JD of Jan 1.0 of `yr`, computed via Meeus (with Y--, M+=12 for January).
    int Y = yr - 1;
    int M = 13;
    int A = Y / 100;
    int B = 2 - A + A / 4;
    double jdJan1 = (long)(365.25 * (Y + 4716)) + (long)(30.6001 * (M + 1))
                  + 1 + B - 1524.5;
    return jdJan1 + (doy - 1.0);
}

// Bstar in 8-char "[s]MMMMM[s]E" form (implied 0.MMMMM x 10^E).
static double parseBstar(const char* s) {
    char buf[9]; memcpy(buf, s, 8); buf[8] = 0;
    int i = 0;
    while (buf[i] == ' ') i++;
    double sign = 1.0;
    if (buf[i] == '-') { sign = -1.0; i++; }
    else if (buf[i] == '+') { i++; }
    char m[6]; memcpy(m, buf + i, 5); m[5] = 0;
    double mant = atof(m) * 1e-5;
    i += 5;
    int eSign = 1;
    if (buf[i] == '-') { eSign = -1; i++; }
    else if (buf[i] == '+') { i++; }
    int ev = buf[i] - '0';
    return sign * mant * pow(10.0, eSign * ev);
}

bool sgp4Init(SatElset* sat, const char* name, const char* line1, const char* line2) {
    memset(sat, 0, sizeof(*sat));
    if (!name || !line1 || !line2) return false;
    if (strlen(line1) < 69 || strlen(line2) < 69) return false;
    if (line1[0] != '1' || line2[0] != '2') return false;

    strncpy(sat->name, name, 24);
    sat->name[24] = 0;

    char buf[16];
    memcpy(buf, line1 + 2, 5); buf[5] = 0;
    sat->norad = atoi(buf);

    memcpy(buf, line1 + 18, 14); buf[14] = 0;
    sat->jdEpoch = tleEpochJD(buf);

    // ndot (rev/day^2) -> rad/s^2
    memcpy(buf, line1 + 33, 10); buf[10] = 0;
    double ndotRevDay2 = atof(buf);
    sat->ndot = ndotRevDay2 * TWOPI / (86400.0 * 86400.0);

    sat->bstar = parseBstar(line1 + 53);

    memcpy(buf, line2 + 8, 8);  buf[8]  = 0; sat->inclo = atof(buf) * D2R;
    memcpy(buf, line2 + 17, 8); buf[8]  = 0; sat->nodeo = atof(buf) * D2R;
    memcpy(buf, line2 + 26, 7); buf[7]  = 0; sat->ecco  = atof(buf) * 1e-7;
    memcpy(buf, line2 + 34, 8); buf[8]  = 0; sat->argpo = atof(buf) * D2R;
    memcpy(buf, line2 + 43, 8); buf[8]  = 0; sat->mo    = atof(buf) * D2R;

    // Mean motion (rev/day) -> rad/s
    memcpy(buf, line2 + 52, 11); buf[11] = 0;
    double noRevDay = atof(buf);
    sat->no = noRevDay * TWOPI / 86400.0;

    if (sat->no <= 0) return false;

    // Semi-major axis from Kepler's 3rd law.
    double a = cbrt(GM / (sat->no * sat->no));
    double p = a * (1.0 - sat->ecco * sat->ecco);
    if (p <= 0) return false;

    double cosi = cos(sat->inclo);
    double q    = (RE / p) * (RE / p);

    sat->dnode = -1.5 * sat->no * J2 * q * cosi;
    sat->dargp =  0.75 * sat->no * J2 * q * (5.0 * cosi * cosi - 1.0);

    sat->valid = true;
    return true;
}

static double solveKepler(double M, double e) {
    M = fmod(M, TWOPI);
    if (M < 0) M += TWOPI;
    double E = M;
    for (int i = 0; i < 20; i++) {
        double dE = (M - E + e * sin(E)) / (1.0 - e * cos(E));
        E += dE;
        if (fabs(dE) < 1e-11) break;
    }
    return E;
}

void sgp4Pos(const SatElset* sat, double tsince_s, double pos[3]) {
    pos[0] = pos[1] = pos[2] = 0;
    if (!sat->valid) return;

    double dt = tsince_s;

    double n = sat->no + sat->ndot * dt;
    if (n <= 0) n = sat->no;

    double M    = sat->mo    + sat->no    * dt + 0.5 * sat->ndot * dt * dt;
    double node = sat->nodeo + sat->dnode * dt;
    double argp = sat->argpo + sat->dargp * dt;

    double a = cbrt(GM / (n * n));
    double e = sat->ecco;

    double E = solveKepler(M, e);
    double sinE = sin(E), cosE = cos(E);

    double xp = a * (cosE - e);
    double yp = a * sqrt(1.0 - e * e) * sinE;

    double cO = cos(node), sO = sin(node);
    double ci = cos(sat->inclo), si = sin(sat->inclo);
    double cw = cos(argp), sw = sin(argp);

    double Px =  cO * cw - sO * sw * ci;
    double Py =  sO * cw + cO * sw * ci;
    double Pz =  sw * si;
    double Qx = -cO * sw - sO * cw * ci;
    double Qy = -sO * sw + cO * cw * ci;
    double Qz =  cw * si;

    pos[0] = Px * xp + Qx * yp;
    pos[1] = Py * xp + Qy * yp;
    pos[2] = Pz * xp + Qz * yp;
}

static double gmstRad(double jd) {
    double T = (jd - 2451545.0) / 36525.0;
    double th = 280.46061837
              + 360.98564736629 * (jd - 2451545.0)
              + T * T * (0.000387933 - T / 38710000.0);
    th = fmod(th, 360.0);
    if (th < 0) th += 360.0;
    return th * D2R;
}

void eciToLLA(const double pos[3], double jd,
              double* lat_deg, double* lon_deg, double* alt_km) {
    double th = gmstRad(jd);
    double cT = cos(th), sT = sin(th);
    double xe =  pos[0] * cT + pos[1] * sT;
    double ye = -pos[0] * sT + pos[1] * cT;
    double ze =  pos[2];

    double lon = atan2(ye, xe);
    double r   = sqrt(xe * xe + ye * ye);
    double lat = atan2(ze, r);
    for (int i = 0; i < 5; i++) {
        double s = sin(lat);
        double N = RE / sqrt(1.0 - E2 * s * s);
        lat = atan2(ze + E2 * N * s, r);
    }
    double s = sin(lat);
    double N = RE / sqrt(1.0 - E2 * s * s);
    double alt = r / cos(lat) - N;

    *lat_deg = lat * R2D;
    *lon_deg = lon * R2D;
    *alt_km  = alt;
}

double computeElev(double obs_lat, double obs_lon,
                   double sat_lat, double sat_lon, double sat_alt_km) {
    double oLat = obs_lat * D2R, oLon = obs_lon * D2R;
    double sLat = sat_lat * D2R, sLon = sat_lon * D2R;

    double cOL = cos(oLat), sOL = sin(oLat);
    double cOG = cos(oLon), sOG = sin(oLon);
    double ox = RE * cOL * cOG;
    double oy = RE * cOL * sOG;
    double oz = RE * sOL;

    double rs = RE + sat_alt_km;
    double cSL = cos(sLat), sSL = sin(sLat);
    double cSG = cos(sLon), sSG = sin(sLon);
    double sx = rs * cSL * cSG;
    double sy = rs * cSL * sSG;
    double sz = rs * sSL;

    double rx = sx - ox, ry = sy - oy, rz = sz - oz;
    double rng = sqrt(rx * rx + ry * ry + rz * rz);
    if (rng <= 0) return -90.0;

    double ux = cOL * cOG, uy = cOL * sOG, uz = sOL;
    double dot = rx * ux + ry * uy + rz * uz;
    return asin(dot / rng) * R2D;
}

double computeAzim(double obs_lat, double obs_lon,
                   double sat_lat, double sat_lon) {
    double oLat = obs_lat * D2R;
    double sLat = sat_lat * D2R;
    double dLon = (sat_lon - obs_lon) * D2R;
    double y = sin(dLon) * cos(sLat);
    double x = cos(oLat) * sin(sLat) - sin(oLat) * cos(sLat) * cos(dLon);
    double az = atan2(y, x) * R2D;
    if (az < 0) az += 360.0;
    return az;
}

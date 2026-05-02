#pragma once

#include <time.h>

// Compact, simplified SGP4-style propagator suitable for amateur LEO satellites.
// Models the dominant secular perturbations (J2 nodal/perigee precession +
// linear drag) from a TLE; ignores short-period and long-period terms. Pass
// predictions are accurate to a couple of minutes for ~3-day horizons, which
// is plenty for "when's ISS over me next?".

struct SatElset {
    int    norad;
    char   name[25];

    // TLE-derived state at epoch
    double jdEpoch;     // epoch (Julian date)
    double ecco;        // eccentricity
    double inclo;       // inclination (rad)
    double nodeo;       // RAAN at epoch (rad)
    double argpo;       // argument of perigee at epoch (rad)
    double mo;          // mean anomaly at epoch (rad)
    double no;          // mean motion at epoch (rad/s)
    double ndot;        // dn/dt (rad/s^2)
    double bstar;       // drag coefficient (unused beyond ndot here)

    // Derived secular rates from J2
    double dnode;       // RAAN precession rate (rad/s)
    double dargp;       // argp precession rate (rad/s)

    bool   valid;
};

// Parse two TLE lines into an element set.
bool sgp4Init(SatElset* sat, const char* name, const char* line1, const char* line2);

// ECI position (km) tsince_s seconds after epoch.
void sgp4Pos(const SatElset* sat, double tsince_s, double pos[3]);

// ECI -> geodetic (WGS84). jd is the Julian date for the position.
void eciToLLA(const double pos[3], double jd,
              double* lat_deg, double* lon_deg, double* alt_km);

// Elevation (deg) of a satellite at (sat_lat, sat_lon, sat_alt km) seen from
// a ground observer at (obs_lat, obs_lon).
double computeElev(double obs_lat, double obs_lon,
                   double sat_lat, double sat_lon, double sat_alt_km);

// Azimuth (deg, 0=N, 90=E) on the great-circle path obs->sat.
double computeAzim(double obs_lat, double obs_lon,
                   double sat_lat, double sat_lon);

// Julian-date / Unix conversions.
double jdFromUnix(time_t t);
time_t unixFromJD(double jd);

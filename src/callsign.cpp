#include "callsign.h"

#include <string.h>

namespace Callsign {

// Compact prefix-to-continent table. Tried longest-first; the first match
// wins. Coverage is biased towards prefixes that actually show up as
// spotters on the major DX clusters (EU, NA, JA, VK, etc.). Anything not
// in the table returns nullptr - callers can decide what to do with
// "unknown" entries.

struct Entry { const char* pfx; const char* cont; };

// 3-character prefixes: handle splits within a single first-letter block
// (e.g. UA1-6 vs UA9/0, KH6/KL7/KP4 vs other K, EA8/9 vs EA, VK0/9 vs VK).
static const Entry kPfx3[] = {
    {"CE9","AN"}, {"VK0","AN"},
    {"KH0","OC"}, {"KH2","OC"}, {"KH3","OC"}, {"KH4","OC"}, {"KH5","OC"},
    {"KH6","OC"}, {"KH7","OC"}, {"KH8","OC"}, {"KH9","OC"},
    {"KL1","NA"}, {"KL2","NA"}, {"KL3","NA"}, {"KL4","NA"}, {"KL5","NA"},
    {"KL6","NA"}, {"KL7","NA"}, {"KL8","NA"}, {"KL9","NA"},
    {"KP1","NA"}, {"KP2","NA"}, {"KP3","NA"}, {"KP4","NA"}, {"KP5","NA"},
    {"KP6","OC"},
    {"UA0","AS"}, {"UA8","AS"}, {"UA9","AS"},
    {"VK9","OC"},
    {"EA6","EU"}, {"EA8","AF"}, {"EA9","AF"},
    {"FG0","NA"}, {"FJ0","NA"}, {"FK0","OC"}, {"FM0","NA"}, {"FO0","OC"},
    {"FP0","NA"}, {"FR0","AF"}, {"FT0","AN"}, {"FW0","OC"}, {"FY0","SA"},
    {"IG9","EU"},
    {"VP2","NA"}, {"VP5","NA"}, {"VP6","OC"}, {"VP8","SA"}, {"VP9","NA"},
};

// 2-character prefixes: the bulk of the table.
static const Entry kPfx2[] = {
    // USA (extra/general/technician/standard letter blocks)
    {"AA","NA"},{"AB","NA"},{"AC","NA"},{"AD","NA"},{"AE","NA"},
    {"AF","NA"},{"AG","NA"},{"AH","OC"},{"AI","NA"},{"AJ","NA"},
    {"AK","NA"},{"AL","NA"},
    // Spain - special-event prefixes
    {"AM","EU"},{"AN","EU"},{"AO","EU"},
    // Pakistan
    {"AP","AS"},
    // Misc A-block exceptions
    {"A2","AF"}, {"A3","OC"}, {"A4","AS"}, {"A5","AS"}, {"A6","AS"},
    {"A7","AS"}, {"A9","AS"},
    // China / Taiwan / North Korea
    {"BA","AS"},{"BD","AS"},{"BG","AS"},{"BH","AS"},{"BI","AS"},
    {"BJ","AS"},{"BL","AS"},{"BM","AS"},{"BN","AS"},
    {"BO","AS"},{"BR","AS"},{"BS","AS"},{"BT","AS"},{"BU","AS"},
    {"BV","AS"},{"BW","AS"},{"BX","AS"},{"BY","AS"},
    // Chile
    {"CE","SA"}, {"CA","SA"}, {"CB","SA"}, {"CC","SA"}, {"CD","SA"},
    // Canada
    {"CF","NA"},{"CG","NA"},{"CH","NA"},{"CI","NA"},{"CJ","NA"},
    {"CK","NA"}, {"CY","NA"}, {"CZ","NA"},
    // Cuba
    {"CL","NA"},{"CM","NA"}, {"CO","NA"},
    // Morocco
    {"CN","AF"},
    // Bolivia
    {"CP","SA"},
    // Portugal
    {"CQ","EU"},{"CR","EU"},{"CS","EU"},{"CT","EU"},{"CU","EU"},
    // Uruguay
    {"CV","SA"},{"CW","SA"},{"CX","SA"},
    // Germany
    {"DA","EU"},{"DB","EU"},{"DC","EU"},{"DD","EU"},{"DE","EU"},
    {"DF","EU"},{"DG","EU"},{"DH","EU"},{"DI","EU"},{"DJ","EU"},
    {"DK","EU"},{"DL","EU"},{"DM","EU"},{"DN","EU"},{"DO","EU"},
    {"DP","EU"},{"DQ","EU"},{"DR","EU"},
    // South Korea
    {"DS","AS"},{"DT","AS"},
    // Philippines
    {"DU","OC"},
    // Africa block
    {"D2","AF"},{"D3","AF"},{"D4","AF"},{"D6","AF"},{"D9","AS"},
    // Spain
    {"EA","EU"},{"EB","EU"},{"EC","EU"},{"ED","EU"},{"EE","EU"},
    {"EF","EU"},{"EG","EU"},{"EH","EU"},
    // Ireland
    {"EI","EU"},{"EJ","EU"},
    // Armenia / Liberia / Iran / Moldova / Estonia / Ethiopia
    {"EK","AS"}, {"EL","AF"}, {"EP","AS"},{"EQ","AS"},
    {"ER","EU"}, {"ES","EU"}, {"ET","AF"},
    // Belarus / Kyrgyzstan / Tajikistan / Turkmenistan
    {"EU","EU"},{"EV","EU"},{"EW","EU"},
    {"EX","AS"},{"EY","AS"},{"EZ","AS"},
    // E7 - Bosnia and Herzegovina
    {"E7","EU"}, {"E5","OC"}, {"E6","OC"},
    // Italy
    {"IA","EU"},{"IB","EU"},{"IC","EU"},{"ID","EU"},{"IE","EU"},
    {"IF","EU"},{"IG","EU"},{"IH","EU"},{"II","EU"},{"IJ","EU"},
    {"IK","EU"},{"IL","EU"},{"IM","EU"},{"IN","EU"},{"IO","EU"},
    {"IP","EU"},{"IQ","EU"},{"IR","EU"},{"IS","EU"},{"IT","EU"},
    {"IU","EU"},{"IV","EU"},{"IW","EU"},{"IX","EU"},{"IY","EU"},
    {"IZ","EU"},
    // Japan
    {"JA","AS"},{"JE","AS"},{"JF","AS"},{"JG","AS"},{"JH","AS"},
    {"JI","AS"},{"JJ","AS"},{"JK","AS"},{"JL","AS"},{"JM","AS"},
    {"JN","AS"},{"JO","AS"},{"JP","AS"},{"JQ","AS"},{"JR","AS"},
    {"JS","AS"},
    // Mongolia / Kazakhstan / Indonesia
    {"JT","AS"},{"JU","AS"},{"JV","AS"},
    {"JD","AS"},  // Minami Torishima / Ogasawara
    {"JY","AS"},  // Jordan
    // USA letter blocks
    {"KA","NA"},{"KB","NA"},{"KC","NA"},{"KD","NA"},{"KE","NA"},
    {"KF","NA"},{"KG","NA"},{"KI","NA"},{"KJ","NA"},
    {"KK","NA"},{"KM","NA"},{"KN","NA"},{"KO","NA"},{"KQ","NA"},
    {"KR","NA"},{"KS","NA"},{"KT","NA"},{"KU","NA"},{"KV","NA"},
    {"KW","NA"},{"KX","NA"},{"KY","NA"},{"KZ","NA"},
    // Norway / Argentina / Bulgaria / Lithuania / Luxembourg
    {"LA","EU"},{"LB","EU"},{"LG","EU"},{"LJ","EU"},{"LN","EU"},
    {"LU","SA"},
    {"LX","EU"}, {"LY","EU"}, {"LZ","EU"},
    // Argentina / Lesotho
    {"L2","SA"},{"L3","SA"},{"L4","SA"},{"L5","SA"},{"L6","SA"},
    {"L7","SA"},{"L8","SA"},{"L9","SA"},
    // UK
    {"MA","EU"},{"MB","EU"},{"MD","EU"},{"MI","EU"},
    {"MJ","EU"},{"MM","EU"},{"MR","EU"},{"MS","EU"},{"MT","EU"},
    {"MU","EU"},{"MW","EU"},
    {"MX","EU"},{"MY","AS"},
    // USA letter blocks (N)
    {"NA","NA"},{"NB","NA"},{"NC","NA"},{"ND","NA"},{"NE","NA"},
    {"NF","NA"},{"NG","NA"},{"NI","NA"},{"NJ","NA"},
    {"NK","NA"},{"NL","NA"},{"NM","NA"},{"NN","NA"},{"NO","NA"},
    {"NP","NA"},{"NQ","NA"},{"NR","NA"},{"NS","NA"},{"NT","NA"},
    {"NU","NA"},{"NV","NA"},{"NW","NA"},{"NX","NA"},{"NY","NA"},
    {"NZ","NA"},
    // Austria / Finland / Czech / Slovakia / Belgium / Faroe / Denmark
    {"OE","EU"}, {"OF","EU"},{"OG","EU"},{"OH","EU"},{"OI","EU"},{"OJ","EU"},
    {"OK","EU"},{"OL","EU"},
    {"OM","EU"},
    {"ON","EU"},{"OO","EU"},{"OP","EU"},{"OQ","EU"},{"OR","EU"},{"OS","EU"},{"OT","EU"},
    {"OY","EU"},
    {"OU","EU"},{"OV","EU"},{"OW","EU"},{"OX","NA"},{"OZ","EU"},
    // Netherlands
    {"PA","EU"},{"PB","EU"},{"PC","EU"},{"PD","EU"},{"PE","EU"},
    {"PF","EU"},{"PG","EU"},{"PH","EU"},{"PI","EU"},
    // Curacao / Aruba / Suriname
    {"PJ","NA"}, {"PZ","SA"},
    // Brazil
    {"PP","SA"},{"PQ","SA"},{"PR","SA"},{"PS","SA"},{"PT","SA"},
    {"PU","SA"},{"PV","SA"},{"PW","SA"},{"PX","SA"},{"PY","SA"},
    // Indonesia
    {"PK","OC"},{"PL","OC"},{"PM","OC"},{"PN","OC"},{"PO","OC"},
    // Russia, European
    {"R1","EU"},{"R2","EU"},{"R3","EU"},{"R4","EU"},{"R5","EU"},
    {"R6","EU"},{"R7","EU"},
    {"R8","AS"},{"R9","AS"},{"R0","AS"},
    {"RA","EU"},{"RB","EU"},{"RC","EU"},{"RD","EU"},{"RE","EU"},
    {"RF","EU"},{"RG","EU"},{"RJ","EU"},{"RK","EU"},
    {"RL","EU"},{"RM","EU"},{"RN","EU"},{"RO","EU"},{"RP","EU"},
    {"RQ","EU"},{"RR","EU"},{"RS","EU"},{"RT","EU"},{"RU","EU"},
    {"RV","EU"},{"RW","EU"},{"RX","EU"},{"RY","EU"},{"RZ","EU"},
    // Sweden / Poland / Greece / San Marino / Slovenia / etc.
    {"SA","EU"},{"SB","EU"},{"SC","EU"},{"SD","EU"},{"SE","EU"},
    {"SF","EU"},{"SG","EU"},{"SH","EU"},{"SI","EU"},{"SJ","EU"},
    {"SK","EU"},{"SL","EU"},{"SM","EU"},
    {"SN","EU"},{"SO","EU"},{"SP","EU"},{"SQ","EU"},{"SR","EU"},
    {"S5","EU"}, {"S7","AF"}, {"S9","AF"},
    {"SU","AF"}, {"SV","EU"}, {"SW","EU"}, {"SX","EU"}, {"SY","EU"}, {"SZ","EU"},
    // Turkey / Iceland / Guatemala / Costa Rica / Panama
    {"TA","EU"},{"TB","EU"},{"TC","EU"},
    {"TF","EU"},
    {"TG","NA"}, {"TI","NA"}, {"HP","NA"},
    {"TJ","AF"},{"TL","AF"},{"TN","AF"},{"TR","AF"},{"TT","AF"},
    {"TU","AF"},{"TY","AF"},{"TZ","AF"},
    // Russia (UA1-9 European/Asian split goes via R-prefix above for 1-char fallback;
    // here we cover the common UA explicit prefixes that DX clusters emit)
    {"UA","EU"},{"UB","EU"},{"UC","EU"},{"UD","EU"},{"UE","EU"},
    {"UF","EU"},{"UG","EU"},{"UH","EU"},{"UI","EU"},
    // Uzbekistan
    {"UJ","AS"},{"UK","AS"},{"UL","AS"},{"UM","AS"},
    // Kazakhstan
    {"UN","AS"},{"UO","AS"},{"UP","AS"},{"UQ","AS"},
    // Ukraine
    {"UR","EU"},{"US","EU"},{"UT","EU"},{"UU","EU"},{"UV","EU"},
    {"UW","EU"},{"UX","EU"},{"UY","EU"},{"UZ","EU"},
    // Canada
    {"VA","NA"},{"VB","NA"},{"VC","NA"},{"VD","NA"},{"VE","NA"},
    {"VF","NA"},{"VG","NA"},{"VO","NA"},{"VX","NA"},{"VY","NA"},
    // Australia / India / Hong Kong / Brunei / Anguilla
    {"VK","OC"}, {"VU","AS"}, {"VR","AS"}, {"VS","AS"}, {"VP","NA"},
    // USA letter blocks (W)
    {"WA","NA"},{"WB","NA"},{"WC","NA"},{"WD","NA"},{"WE","NA"},
    {"WF","NA"},{"WG","NA"},{"WI","NA"},{"WJ","NA"},
    {"WK","NA"},{"WL","NA"},{"WM","NA"},{"WN","NA"},{"WO","NA"},
    {"WP","NA"},{"WQ","NA"},{"WR","NA"},{"WS","NA"},{"WT","NA"},
    {"WU","NA"},{"WV","NA"},{"WW","NA"},{"WX","NA"},{"WY","NA"},
    {"WZ","NA"},
    // Mexico
    {"XE","NA"},{"XF","NA"},
    // Cambodia / Laos / Myanmar / China(misc) / Canada special
    {"XU","AS"},{"XV","AS"},{"XW","AS"},{"XX","AS"},{"XY","AS"},{"XZ","AS"},
    {"XA","NA"},{"XB","NA"},{"XC","NA"},
    // Afghanistan / Indonesia
    {"YA","AS"}, {"YB","OC"},{"YC","OC"},{"YD","OC"},{"YE","OC"},
    {"YF","OC"},{"YG","OC"},{"YH","OC"},
    // Iraq / Vanuatu / Syria / Latvia / Nicaragua
    {"YI","AS"}, {"YJ","OC"}, {"YK","AS"},
    {"YL","EU"}, {"YM","EU"}, {"YN","NA"},
    // Romania
    {"YO","EU"},{"YP","EU"},{"YQ","EU"},{"YR","EU"},
    // El Salvador / Yugoslavia (now Serbia)
    {"YS","NA"}, {"YT","EU"},{"YU","EU"},
    // Venezuela
    {"YV","SA"},{"YW","SA"},{"YX","SA"},{"YY","SA"},
    // Albania / Bulgaria(LZ above) / Bosnia / Macedonia / Cyprus / Tanzania / Nigeria / Madagascar / Papua / Singapore / Rwanda / Trinidad / Botswana / Tonga / Oman / Bangladesh / Maldives
    {"ZA","EU"}, {"ZB","EU"}, {"ZC","EU"},
    {"ZD","AF"}, {"ZF","NA"},
    {"ZK","OC"}, {"ZL","OC"}, {"ZM","OC"},
    {"ZP","SA"},
    {"ZR","AF"},{"ZS","AF"},{"ZT","AF"},{"ZU","AF"},
    // Z3 / Z6 / Z8
    {"Z3","EU"}, {"Z6","EU"}, {"Z8","AF"},
    // 3-letter-class numeric prefixes that show up commonly
    {"3A","EU"}, {"3B","AF"}, {"3C","AF"}, {"3D","OC"}, {"3E","NA"},
    {"3F","NA"}, {"3G","SA"}, {"3H","AS"}, {"3V","AF"}, {"3W","AS"},
    {"3X","AF"}, {"3Y","AN"}, {"3Z","EU"},
    {"4A","NA"}, {"4B","NA"}, {"4C","NA"}, {"4D","OC"}, {"4E","OC"},
    {"4F","OC"}, {"4G","OC"}, {"4H","OC"}, {"4I","OC"},
    {"4J","AS"}, {"4K","AS"}, {"4L","AS"}, {"4M","SA"},
    {"4N","EU"}, {"4O","EU"},
    {"4S","AS"}, {"4T","SA"}, {"4U","EU"}, {"4V","NA"}, {"4W","OC"},
    {"4X","AS"}, {"4Y","EU"}, {"4Z","AS"},
    {"5A","AF"}, {"5B","AS"}, {"5H","AF"}, {"5I","AF"},
    {"5N","AF"}, {"5O","AF"}, {"5R","AF"}, {"5T","AF"},
    {"5U","AF"}, {"5V","AF"}, {"5W","OC"}, {"5X","AF"}, {"5Y","AF"}, {"5Z","AF"},
    {"6V","AF"}, {"6W","AF"}, {"6Y","NA"},
    {"7O","AS"}, {"7P","AF"}, {"7Q","AF"}, {"7T","AF"}, {"7X","AF"},
    {"7Z","AS"},
    {"8P","NA"}, {"8Q","AS"}, {"8R","SA"}, {"8S","EU"},
    {"9A","EU"}, {"9G","AF"}, {"9H","EU"}, {"9I","AF"}, {"9J","AF"},
    {"9K","AS"}, {"9L","AF"}, {"9M","AS"}, {"9N","AS"}, {"9Q","AF"},
    {"9R","AF"}, {"9S","AF"}, {"9U","AF"}, {"9V","AS"}, {"9X","AF"},
    {"9Y","NA"}, {"9Z","NA"},
};

// 1-character fallback for the common "single-letter" countries.
static const Entry kPfx1[] = {
    {"F","EU"},   // France (mainland)
    {"G","EU"},   // United Kingdom
    {"I","EU"},   // Italy
    {"K","NA"},   // USA
    {"M","EU"},   // United Kingdom
    {"N","NA"},   // USA
    {"R","EU"},   // Russia (default to European; UA9/0 handled in 3-char table)
    {"W","NA"},   // USA
};

const char* continent(const String& callIn) {
    String call = callIn;
    call.toUpperCase();
    int slash = call.indexOf('/');
    if (slash > 0) call = call.substring(0, slash);
    int len = call.length();
    if (len < 1) return nullptr;

    const char* cs = call.c_str();

    // Try 3-character match first (most specific).
    if (len >= 3) {
        for (auto& e : kPfx3) {
            if (cs[0] == e.pfx[0] && cs[1] == e.pfx[1] && cs[2] == e.pfx[2]) {
                return e.cont;
            }
        }
    }
    // Then 2-character.
    if (len >= 2) {
        for (auto& e : kPfx2) {
            if (cs[0] == e.pfx[0] && cs[1] == e.pfx[1]) return e.cont;
        }
    }
    // Finally 1-character.
    for (auto& e : kPfx1) {
        if (cs[0] == e.pfx[0]) return e.cont;
    }
    return nullptr;
}

}

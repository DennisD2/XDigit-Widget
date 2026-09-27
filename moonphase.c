
#include "multi-zone-clock.h"
#include "moonphase.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Berechnet das Julianische Datum für den Mittag (12:00 UTC) des Tages
double julianDate(DigitStruct *d) {
    int y = d->year;
    int m = d->month;

    if (m <= 2) {
        y--;
        m += 12;
    }

    // Korrektur für den Gregorianischen Kalender ab Oktober 1582
    double a = floor((double)y / 100.0);
    double b = 2.0 - a + floor(a / 4.0);

    double jd = floor(365.25 * (double)(y + 4716)) + floor(30.6001 * (double)(m + 1)) + (double)d->day + b - 1524.5;
    return jd;
}

// Hilfsfunktion: Normalisiert Winkel auf 0-360 Grad und konvertiert in Bogenmaß (Rad)
double toRad(double deg) {
    deg = fmod(deg, 360.0);
    if (deg < 0) {
        deg += 360.0;
    }
    return deg * M_PI / 180.0;
}

// Berechnet das präzise Mondalter in Tagen (0 bis 29.53059) nach Jean Meeus
double moonAge(DigitStruct *d) {
    double jd = julianDate(d);

    // T = Julianische Jahrhunderte seit J2000.0
    double T = (jd - 2451545.0) / 36525.0;

    // 1. Mittlere Elongation des Mondes (D) in Grad
    double D = 297.8501921 + 445267.1114034 * T - 0.0018819 * T * T + (T * T * T) / 545868.0 - (T * T * T * T) / 113065000.0;
    // 2. Mittlere Anomalie der Sonne (M) in Grad
    double M = 357.5291092 + 35999.0502909 * T - 0.0001536 * T * T + (T * T * T) / 24490000.0;
    // 3. Mittlere Anomalie des Mondes (M') in Grad
    double MPrime = 134.9633964 + 477198.8675055 * T + 0.0087414 * T * T + (T * T * T) / 69699.0 - (T * T * T * T) / 14712000.0;

    double dRad = toRad(D);
    double mRad = toRad(M);
    double mPrimeRad = toRad(MPrime);

    // Periodische Hauptstörungen der Mondbahn nach Meeus (in Grad)
    double correction = 6.289 * sin(mPrimeRad) - 
                        1.274 * sin(mPrimeRad - 2.0 * dRad) + 
                        0.658 * sin(2.0 * dRad) + 
                        0.214 * sin(2.0 * mPrimeRad) - 
                        0.186 * sin(mRad);

    // Tatsächlicher Phasenwinkel in Grad (Elongation)
    double correctedPhaseAngle = fmod(D + correction, 360.0);
    if (correctedPhaseAngle < 0) {
        correctedPhaseAngle += 360.0;
    }

    // Umrechnung des Winkels in die Synodische Periode (0 - 29.53059 Tage)
    double synodicMonth = 29.530588853;
    double ageInDays = (correctedPhaseAngle / 360.0) * synodicMonth;

    return ageInDays;
}

void moonAgeToPhase(double age) {
    if (age < 1.5 || age > 28.0) {
        printf("Phase: Neumond 🌑\n");
    } else if (age >= 1.5 && age < 6.0) {
        printf("Phase: Erstes Viertel (Zunehmend) 🌒\n");
    } else if (age >= 6.0 && age < 9.0) {
        printf("Phase: Zunehmender Halbmond 🌓\n");
    } else if (age >= 9.0 && age < 13.5) {
        printf("Phase: Zunehmender Dreiviertelmond 🌔\n");
    } else if (age >= 13.5 && age < 16.0) {
        printf("Phase: Vollmond 🌕\n");
    } else if (age >= 16.0 && age < 20.5) {
        printf("Phase: Abnehmender Dreiviertelmond 🌖\n");
    } else if (age >= 20.5 && age < 23.5) {
        printf("Phase: Abnehmender Halbmond 🌗\n");
    } else {
        printf("Phase: Letztes Viertel (Abnehmend) 🌘\n");
    }
}

// waning=abnehmend , waxing=zunehmend
// Crescent=Sichel, Gibbous=Dreiviertelmond
// related to light part of moon
char *pngFiles[] = {
    "questionmark.png",
    "moon-1-full.png",    /* Vollmond */
    "moon-2-waxgib.png",  /* zunehmender Dreiviertelmond */
    "moon-3-1st-q.png",   /* Erstes Viertel */
    "moon-4-waxcres.png", /* Zunehmender Sichelmond */
    "moon-5-new.png",     /* Neumomd */
    "moon-6-wancres.png", /* Abnehmender Sichelmond */
    "moon-7-last-q.png",  /* Letztes Viertel */
    "moon-8-wangib.png",  /* Abnehmender 3/4 Mond */
};

char *moonAgeToPixmapName(double age) {
    int i=0;
    if (age < 1.5 || age > 28.0) {
        i=5;
    } else if (age >= 1.5 && age < 6.0) {
        i=6;
    } else if (age >= 6.0 && age < 9.0) {
        i=7;
    } else if (age >= 9.0 && age < 13.5) {
        i=8;
    } else if (age >= 13.5 && age < 16.0) {
        i=1;
    } else if (age >= 16.0 && age < 20.5) {
        i=2;
    } else if (age >= 20.5 && age < 23.5) {
        i=3;
    } else {
        i=4;
    }
    return pngFiles[i];
}

int test_main(int argc, char *argv[]) {
    // 1. Prüfen, ob ein Argument übergeben wurde
    if (argc < 2) {
        printf("Fehler: Bitte ein Datum im Format DD.MM.YYYY angeben.\n");
        printf("Beispiel: %s 25.09.2026\n", argv[0]);
        return 1;
    }

    DigitStruct d;
    // 2. Den String parsen (entspricht fmt.Sscanf aus Go)
    if (sscanf(argv[1], "%d.%d.%d", &d.day, &d.month, &d.year) != 3) {
        printf("Fehler: Ungültiges Datumsformat. Erwartet wird DD.MM.YYYY\n");
        return 1;
    }

    // 3. Einfache Validierung der Werte
    if (d.month < 1 || d.month > 12 || d.day < 1 || d.day > 31) {
        printf("Fehler: Ungültige Werte für Tag oder Monat.\n");
        return 1;
    }

    // 4. Berechnung ausführen
    double age = moonAge(&d);

    printf("Mondalter am %02d.%02d.%d: %.2f Tage\n", d.day, d.month, d.year, age);

    moonAgeToPhase(age);
    return 0;
}

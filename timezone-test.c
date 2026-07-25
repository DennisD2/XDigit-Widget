/* test time zone manipulation */
/* TZ environment variable is manipulated */

/* if TZ is not set, the zone is usually read from /etc/localtime */
/* /etc/localtime is a link to a real timezone info file, located in /usr/share/zoneinfo */
/* /usr/share/zoneinfo contains files like "GMT" and "MET", but also like "America/New_York" and "Europe/Berlin" */

#include <time.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char **argv) {
	time_t t;

	// Get UTC value
	time( &t );
	struct tm * tt;
	tt = gmtime(&t);
	char *buf = asctime(tt);
	printf("GMT/UTC: %s\n", buf);

	// Get vanilla local time
	time( &t );
	tt = localtime(&t);
	buf = asctime(tt);
	printf("LOCAL: %s\n", buf);

	// print time zone value
	char *e = getenv("TZ");
	if (e != NULL) {
		printf("TZ=%s\n", e);
	} else {
		printf("TZ not set.\n");
	}

	// manipulate time zone value via environment variable change/setting
	//putenv("TZ=MST");
	//putenv("TZ=EST");
	putenv("TZ=America/New_York");
	e = getenv("TZ");
	if (e != NULL) {
		printf("TZ=%s\n", e);
	} else {
		printf("TZ not set.\n");
	}

	// Get "remote" local time
	tt = localtime(&t);
	buf = asctime(tt);
	printf("REMOTE: %s\n", buf);
}

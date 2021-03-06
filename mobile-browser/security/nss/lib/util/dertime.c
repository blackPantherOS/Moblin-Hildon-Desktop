/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Netscape security libraries.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1994-2000
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "prtypes.h"
#include "prtime.h"
#include "secder.h"
#include "prlong.h"
#include "secerr.h"

#define HIDIGIT(v) (((v) / 10) + '0')
#define LODIGIT(v) (((v) % 10) + '0')

#define C_SINGLE_QUOTE '\047'

#define DIGITHI(dig) (((dig) - '0') * 10)
#define DIGITLO(dig) ((dig) - '0')
#define ISDIGIT(dig) (((dig) >= '0') && ((dig) <= '9'))
#define CAPTURE(var,p,label)				  \
{							  \
    if (!ISDIGIT((p)[0]) || !ISDIGIT((p)[1])) goto label; \
    (var) = ((p)[0] - '0') * 10 + ((p)[1] - '0');	  \
}

#define SECMIN  60L            /* seconds in a minute        */
#define SECHOUR (60L*SECMIN)   /* seconds in an hour         */
#define SECDAY  (24L*SECHOUR)  /* seconds in a day           */
#define SECYEAR (365L*SECDAY)  /* seconds in a non-leap year */

static long monthToDayInYear[12] = {
    0,
    31,
    31+28,
    31+28+31,
    31+28+31+30,
    31+28+31+30+31,
    31+28+31+30+31+30,
    31+28+31+30+31+30+31,
    31+28+31+30+31+30+31+31,
    31+28+31+30+31+30+31+31+30,
    31+28+31+30+31+30+31+31+30+31,
    31+28+31+30+31+30+31+31+30+31+30,
};

static const PRTime January1st1     = (PRTime) LL_INIT(0xff234001U, 0x00d44000U);
static const PRTime January1st1950  = (PRTime) LL_INIT(0xfffdc1f8U, 0x793da000U);
static const PRTime January1st2050  = LL_INIT(0x0008f81e, 0x1b098000);
static const PRTime January1st10000 = LL_INIT(0x0384440c, 0xcc736000);

/* gmttime must contains UTC time in micro-seconds unit */
SECStatus
DER_TimeToUTCTimeArena(PRArenaPool* arenaOpt, SECItem *dst, int64 gmttime)
{
    PRExplodedTime printableTime;
    unsigned char *d;

    if ( (gmttime < January1st1950) || (gmttime >= January1st2050) ) {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return SECFailure;
    }

    dst->len = 13;
    if (arenaOpt) {
        dst->data = d = (unsigned char*) PORT_ArenaAlloc(arenaOpt, dst->len);
    } else {
        dst->data = d = (unsigned char*) PORT_Alloc(dst->len);
    }
    dst->type = siUTCTime;
    if (!d) {
	return SECFailure;
    }

    /* Convert an int64 time to a printable format.  */
    PR_ExplodeTime(gmttime, PR_GMTParameters, &printableTime);

    /* The month in UTC time is base one */
    printableTime.tm_month++;

    /* remove the century since it's added to the tm_year by the 
       PR_ExplodeTime routine, but is not needed for UTC time */
    printableTime.tm_year %= 100; 

    d[0] = HIDIGIT(printableTime.tm_year);
    d[1] = LODIGIT(printableTime.tm_year);
    d[2] = HIDIGIT(printableTime.tm_month);
    d[3] = LODIGIT(printableTime.tm_month);
    d[4] = HIDIGIT(printableTime.tm_mday);
    d[5] = LODIGIT(printableTime.tm_mday);
    d[6] = HIDIGIT(printableTime.tm_hour);
    d[7] = LODIGIT(printableTime.tm_hour);
    d[8] = HIDIGIT(printableTime.tm_min);
    d[9] = LODIGIT(printableTime.tm_min);
    d[10] = HIDIGIT(printableTime.tm_sec);
    d[11] = LODIGIT(printableTime.tm_sec);
    d[12] = 'Z';
    return SECSuccess;
}

SECStatus
DER_TimeToUTCTime(SECItem *dst, int64 gmttime)
{
    return DER_TimeToUTCTimeArena(NULL, dst, gmttime);
}

/* The caller of DER_AsciiToItem MUST ENSURE that either
** a) "string" points to a null-terminated ASCII string, or
** b) "string" points to a buffer containing a valid UTCTime, 
**     whether null terminated or not.
** otherwise, this function may UMR and/or crash.
** It suffices to ensure that the input "string" is at least 17 bytes long.
*/
SECStatus
DER_AsciiToTime(int64 *dst, const char *string)
{
    long year, month, mday, hour, minute, second, hourOff, minOff, days;
    int64 result, tmp1, tmp2;

    if (string == NULL) {
	goto loser;
    }
    
    /* Verify time is formatted properly and capture information */
    second = 0;
    hourOff = 0;
    minOff = 0;
    CAPTURE(year,string+0,loser);
    if (year < 50) {
	/* ASSUME that year # is in the 2000's, not the 1900's */
	year += 100;
    }
    CAPTURE(month,string+2,loser);
    if ((month == 0) || (month > 12)) goto loser;
    CAPTURE(mday,string+4,loser);
    if ((mday == 0) || (mday > 31)) goto loser;
    CAPTURE(hour,string+6,loser);
    if (hour > 23) goto loser;
    CAPTURE(minute,string+8,loser);
    if (minute > 59) goto loser;
    if (ISDIGIT(string[10])) {
	CAPTURE(second,string+10,loser);
	if (second > 59) goto loser;
	string += 2;
    }
    if (string[10] == '+') {
	CAPTURE(hourOff,string+11,loser);
	if (hourOff > 23) goto loser;
	CAPTURE(minOff,string+13,loser);
	if (minOff > 59) goto loser;
    } else if (string[10] == '-') {
	CAPTURE(hourOff,string+11,loser);
	if (hourOff > 23) goto loser;
	hourOff = -hourOff;
	CAPTURE(minOff,string+13,loser);
	if (minOff > 59) goto loser;
	minOff = -minOff;
    } else if (string[10] != 'Z') {
	goto loser;
    }
    
    
    /* Compute the number of seconds in the years elapsed since 1970 */
    LL_I2L(tmp1, (year-70L));   /* ignores leap days (see below) */
    LL_I2L(tmp2, SECYEAR);
    LL_MUL(result, tmp1, tmp2);
    /* compute number of seconds since beginning of the given month */
    LL_I2L(tmp1, ( (mday-1L)*SECDAY + hour*SECHOUR + minute*SECMIN -
		  hourOff*SECHOUR - minOff*SECMIN + second ) );
    LL_ADD(result, result, tmp1);
    /* compute days for elapsed months in the target year */
    days = monthToDayInYear[month-1];  /* ignoring leap days */

    /*
    ** Account for leap days. The return time value is in
    ** microseconds since January 1st, 12:00am 1970 and may be negative.
    ** Using two digit years, we can only represent dates from 1950
    ** to 2049.  All years in that span of time that are divisible
    ** by 4 are leap years.
    **/
    /* compute number of elapsed leap days since 1970 */
    days += (year - 68)/4;  
    if (((year % 4) == 0) && (month < 3)) {
	days--;
    }

    LL_I2L(tmp1, (days * SECDAY) );
    LL_ADD(result, result, tmp1 );

    /* convert to micro seconds */
    LL_I2L(tmp1, PR_USEC_PER_SEC);
    LL_MUL(result, result, tmp1);

    *dst = result;
    return SECSuccess;

  loser:
    PORT_SetError(SEC_ERROR_INVALID_TIME);
    return SECFailure;
	
}

SECStatus
DER_UTCTimeToTime(int64 *dst, const SECItem *time)
{
    const char * string;
    char localBuf[20]; 

    /* Minimum valid UTCTime is yymmddhhmmZ       which is 11 bytes. 
    ** Maximum valid UTCTime is yymmddhhmmss+0000 which is 17 bytes.
    ** 20 should be large enough for all valid encoded times. 
    */
    if (!time || !time->data || time->len < 11) {
	PORT_SetError(SEC_ERROR_INVALID_TIME);
	return SECFailure;
    }
    if (time->len >= sizeof localBuf) { 
	string = (const char *)time->data;
    } else {
	memset(localBuf, 0, sizeof localBuf);
	memcpy(localBuf, time->data, time->len);
        string = (const char *)localBuf;
    }
    return DER_AsciiToTime(dst, string);
}

/*
   gmttime must contains UTC time in micro-seconds unit.
   Note: the caller should make sure that Generalized time
   should only be used for certifiate validities after the
   year 2049.  Otherwise, UTC time should be used.  This routine
   does not check this case, since it can be used to encode
   certificate extension, which does not have this restriction. 
 */
SECStatus
DER_TimeToGeneralizedTimeArena(PRArenaPool* arenaOpt, SECItem *dst, int64 gmttime)
{
    PRExplodedTime printableTime;
    unsigned char *d;

    if ( (gmttime<January1st1) || (gmttime>=January1st10000) ) {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return SECFailure;
    }
    dst->len = 15;
    if (arenaOpt) {
        dst->data = d = (unsigned char*) PORT_ArenaAlloc(arenaOpt, dst->len);
    } else {
        dst->data = d = (unsigned char*) PORT_Alloc(dst->len);
    }
    dst->type = siGeneralizedTime;
    if (!d) {
	return SECFailure;
    }

    /* Convert an int64 time to a printable format.  */
    PR_ExplodeTime(gmttime, PR_GMTParameters, &printableTime);

    /* The month in Generalized time is base one */
    printableTime.tm_month++;

    d[0] = (printableTime.tm_year /1000) + '0';
    d[1] = ((printableTime.tm_year % 1000) / 100) + '0';
    d[2] = ((printableTime.tm_year % 100) / 10) + '0';
    d[3] = (printableTime.tm_year % 10) + '0';
    d[4] = HIDIGIT(printableTime.tm_month);
    d[5] = LODIGIT(printableTime.tm_month);
    d[6] = HIDIGIT(printableTime.tm_mday);
    d[7] = LODIGIT(printableTime.tm_mday);
    d[8] = HIDIGIT(printableTime.tm_hour);
    d[9] = LODIGIT(printableTime.tm_hour);
    d[10] = HIDIGIT(printableTime.tm_min);
    d[11] = LODIGIT(printableTime.tm_min);
    d[12] = HIDIGIT(printableTime.tm_sec);
    d[13] = LODIGIT(printableTime.tm_sec);
    d[14] = 'Z';
    return SECSuccess;
}

SECStatus
DER_TimeToGeneralizedTime(SECItem *dst, int64 gmttime)
{
    return DER_TimeToGeneralizedTimeArena(NULL, dst, gmttime);
}


/*
    The caller should make sure that the generalized time should only
    be used for the certificate validity after the year 2051; otherwise,
    the certificate should be consider invalid!?
 */
SECStatus
DER_GeneralizedTimeToTime(int64 *dst, const SECItem *time)
{
    PRExplodedTime genTime;
    const char *string;
    long hourOff, minOff;
    uint16 century;
    char localBuf[20];

    /* Minimum valid GeneralizedTime is ccyymmddhhmmZ       which is 13 bytes.
    ** Maximum valid GeneralizedTime is ccyymmddhhmmss+0000 which is 19 bytes.
    ** 20 should be large enough for all valid encoded times. 
    */
    if (!time || !time->data || time->len < 13)
        goto loser;
    if (time->len >= sizeof localBuf) {
        string = (const char *)time->data;
    } else {
	memset(localBuf, 0, sizeof localBuf);
        memcpy(localBuf, time->data, time->len);
	string = (const char *)localBuf;
    }

    memset(&genTime, 0, sizeof genTime);

    /* Verify time is formatted properly and capture information */
    hourOff = 0;
    minOff = 0;

    CAPTURE(century, string+0, loser);
    century *= 100;
    CAPTURE(genTime.tm_year,string+2,loser);
    genTime.tm_year += century;

    CAPTURE(genTime.tm_month,string+4,loser);
    if ((genTime.tm_month == 0) || (genTime.tm_month > 12)) goto loser;

    /* NSPR month base is 0 */
    --genTime.tm_month;
    
    CAPTURE(genTime.tm_mday,string+6,loser);
    if ((genTime.tm_mday == 0) || (genTime.tm_mday > 31)) goto loser;
    
    CAPTURE(genTime.tm_hour,string+8,loser);
    if (genTime.tm_hour > 23) goto loser;
    
    CAPTURE(genTime.tm_min,string+10,loser);
    if (genTime.tm_min > 59) goto loser;
    
    if (ISDIGIT(string[12])) {
	CAPTURE(genTime.tm_sec,string+12,loser);
	if (genTime.tm_sec > 59) goto loser;
	string += 2;
    }
    if (string[12] == '+') {
	CAPTURE(hourOff,string+13,loser);
	if (hourOff > 23) goto loser;
	CAPTURE(minOff,string+15,loser);
	if (minOff > 59) goto loser;
    } else if (string[12] == '-') {
	CAPTURE(hourOff,string+13,loser);
	if (hourOff > 23) goto loser;
	hourOff = -hourOff;
	CAPTURE(minOff,string+15,loser);
	if (minOff > 59) goto loser;
	minOff = -minOff;
    } else if (string[12] != 'Z') {
	goto loser;
    }

    /* Since the values of hourOff and minOff are small, there will
       be no loss of data by the conversion to int8 */
    /* Convert the GMT offset to seconds and save it it genTime
       for the implode time process */
    genTime.tm_params.tp_gmt_offset = (PRInt32)((hourOff * 60L + minOff) * 60L);
    *dst = PR_ImplodeTime (&genTime);
    return SECSuccess;

  loser:
    PORT_SetError(SEC_ERROR_INVALID_TIME);
    return SECFailure;
	
}

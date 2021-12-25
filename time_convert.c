#include <time.h>
#include <stdio.h>

int get_timezone()
{
    time_t time_utc = 0;
    struct tm tm_time;
    int time_zone = 0;

    localtime_r( &time_utc, &tm_time);
    time_zone = ( tm_time.tm_hour > 12 ) ?   ( tm_time.tm_hour-=  24 )  :  tm_time.tm_hour;

    printf("time_zone: %d\n", time_zone);
    return time_zone;
}

int main()
{
    char time_str[] = "03:00|17:00";
    struct tm start_time, end_time;
    time_t utc_time = time(NULL);
    printf("utc: %d\n", utc_time);

    localtime_r(&utc_time, &start_time);
    localtime_r(&utc_time, &end_time);

    time_t local_time = mktime(&start_time);
    printf("local: %d\n", local_time);

    sscanf(time_str, "%d:%d|%d:%d", &start_time.tm_hour, &start_time.tm_min, &end_time.tm_hour, &end_time.tm_min);
    printf("start_time: %d/%d/%d %d:%d:%d\n", start_time.tm_year+1900, start_time.tm_mon+1, start_time.tm_mday, start_time.tm_hour, start_time.tm_min, start_time.tm_sec);
    printf("end_time: %d/%d/%d %d:%d:%d\n", end_time.tm_year+1900, end_time.tm_mon+1, end_time.tm_mday, end_time.tm_hour, end_time.tm_min, end_time.tm_sec);

    get_timezone();

    return 0;
}


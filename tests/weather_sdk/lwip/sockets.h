#pragma once
#include <sys/socket.h>
#include <sys/select.h>
int weather_test_select(int, fd_set*, fd_set*, fd_set*, timeval*);
int weather_test_getsockopt(int, int, int, void*, socklen_t*);
#define select weather_test_select
#define getsockopt weather_test_getsockopt

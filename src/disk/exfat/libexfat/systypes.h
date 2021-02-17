#pragma once

#ifndef _OFF_T_DEFINED
#define _OFF_T_DEFINED

typedef long long _off_t;

#if (defined _CRT_DECLARE_NONSTDC_NAMES && _CRT_DECLARE_NONSTDC_NAMES) || (!defined _CRT_DECLARE_NONSTDC_NAMES && !__STDC__)
typedef _off_t off_t;
#endif
#endif

#include <sys/types.h>

#ifndef _UID_T_DEFINED
#define _UID_T_DEFINED

typedef unsigned int _uid_t;

#if (defined _CRT_DECLARE_NONSTDC_NAMES && _CRT_DECLARE_NONSTDC_NAMES) || (!defined _CRT_DECLARE_NONSTDC_NAMES && !__STDC__)
typedef _uid_t uid_t;
#endif
#endif



#ifndef _GID_T_DEFINED
#define _GID_T_DEFINED

typedef unsigned int _gid_t;

#if (defined _CRT_DECLARE_NONSTDC_NAMES && _CRT_DECLARE_NONSTDC_NAMES) || (!defined _CRT_DECLARE_NONSTDC_NAMES && !__STDC__)
typedef _gid_t gid_t;
#endif
#endif



#ifndef _SSIZE_T_DEFINED
#define _SSIZE_T_DEFINED

typedef long long int _ssize_t;

#if (defined _CRT_DECLARE_NONSTDC_NAMES && _CRT_DECLARE_NONSTDC_NAMES) || (!defined _CRT_DECLARE_NONSTDC_NAMES && !__STDC__)
typedef _ssize_t ssize_t;
#endif
#endif



#ifndef _UID_T_DEFINED
#define _UID_T_DEFINED

typedef unsigned short _uid_t;

#if (defined _CRT_DECLARE_NONSTDC_NAMES && _CRT_DECLARE_NONSTDC_NAMES) || (!defined _CRT_DECLARE_NONSTDC_NAMES && !__STDC__)
typedef _uid_t uid_t;
#endif
#endif

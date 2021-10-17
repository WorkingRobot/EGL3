#pragma once

#define IDI_ICON                        100

#define IDD_LICENSE                     102
#define IDD_DIR                         103
#define IDD_INST                        105
#define IDD_INSTFILES                   106

#define IDC_BACK                        3
#define IDC_EDIT                        1000
#define IDC_BROWSE                      1001
#define IDC_PROGRESS                    1004
#define IDC_INTROTEXT                   1006
#define IDC_LIST1                       1016
#define IDC_CHILDRECT                   1018
#define IDC_DIR                         1019
#define IDC_SELDIRTEXT                  1020
#define IDC_SPACEREQUIRED               1023
#define IDC_SPACEAVAILABLE              1024
#define IDC_SHOWDETAILS                 1027
#define IDC_VERSTR                      1028
#define IDC_HEADER                      1034
#define IDC_TITLE                       1037
#define IDC_DESCRIPTION                 1038
#define IDC_ICO                         1039
#define IDC_TOPTEXT                     1040

// Sent to page that it can start executing code
#define WM_NOTIFY_START (WM_USER + 0x5)

// Sent to outer window to change to next page
#define WM_NOTIFY_OUTER_NEXT (WM_USER + 0x8)

// Sent to page that it will close soon
#define WM_NOTIFY_CLOSE_WARN (WM_USER + 0xb)

// Sent to page that the install thread aborted
#define WM_IN_INSTALL_ABORT (WM_USER + 0xe)

// Sent to page that it should update its space availability fields
#define WM_IN_UPDATEMSG (WM_USER + 0xf)

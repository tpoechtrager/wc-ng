//************************************************************************
//  The Logitech LCD SDK, including all acompanying documentation,
//  is protected by intellectual property laws.  All use of the Logitech
//  LCD SDK is subject to the License Agreement found in the
//  "Logitech LCD SDK License Agreement" file and in the Reference Manual.  
//  All rights not expressly granted by Logitech are reserved.
//************************************************************************

// Copyright 2010 Logitech Inc.

#ifndef EZLCD_H_DEFINES_INCLUDED_
#define EZLCD_H_DEFINES_INCLUDED_

#include <TChar.h>

// text box height for various font sizes
CONST INT LG_SMALL_FONT_TEXT_BOX_HEIGHT = 12;
CONST INT LG_MEDIUM_FONT_TEXT_BOX_HEIGHT = 13;
CONST INT LG_BIG_FONT_TEXT_BOX_HEIGHT = 20;

CONST INT LG_TINY_FONT_TEXT_BOX_HEIGHT_COLOR = 14;
CONST INT LG_SMALL_FONT_TEXT_BOX_HEIGHT_COLOR = 19;
CONST INT LG_MEDIUM_FONT_TEXT_BOX_HEIGHT_COLOR = 24;
CONST INT LG_BIG_FONT_TEXT_BOX_HEIGHT_COLOR = 37;

CONST INT LG_TINY_FONT_LOGICAL_ORIGIN_Y_COLOR = 0;
CONST INT LG_SMALL_FONT_LOGICAL_ORIGIN_Y_COLOR = 0;
CONST INT LG_MEDIUM_FONT_LOGICAL_ORIGIN_Y_COLOR = -1;
CONST INT LG_BIG_FONT_LOGICAL_ORIGIN_Y_COLOR = 0;

CONST INT LG_SMALL_FONT_LOGICAL_ORIGIN_Y = -3;
CONST INT LG_MEDIUM_FONT_LOGICAL_ORIGIN_Y = -1;
CONST INT LG_BIG_FONT_LOGICAL_ORIGIN_Y = -2;

// corresponding font size
CONST INT LG_SMALL_FONT_SIZE = 7;
CONST INT LG_MEDIUM_FONT_SIZE = 8;
CONST INT LG_BIG_FONT_SIZE = 12;

CONST INT LG_TINY_FONT_SIZE_COLOR = 8;
CONST INT LG_SMALL_FONT_SIZE_COLOR = 12;
CONST INT LG_MEDIUM_FONT_SIZE_COLOR = 16;
CONST INT LG_BIG_FONT_SIZE_COLOR = 24;

// Scrolling text parameters
CONST INT LG_SCROLLING_STEP_COLOR = 1;
CONST INT LG_SCROLLING_SPEED_TINY_COLOR = 15;
CONST INT LG_SCROLLING_SPEED_SMALL_COLOR = 22;
CONST INT LG_SCROLLING_SPEED_MEDIUM_COLOR = 30;
CONST INT LG_SCROLLING_SPEED_BIG_COLOR = 45;

CONST INT LG_SCROLLING_STEP_SMALL_MONOCHROME = 10;
CONST INT LG_SCROLLING_STEP_MEDIUM_MONOCHROME = 12;
CONST INT LG_SCROLLING_STEP_BIG_MONOCHROME = 20;
CONST INT LG_SCROLLING_SPEED_SMALL_MONOCHROME = 15;
CONST INT LG_SCROLLING_SPEED_MEDIUM_MONOCHROME = 18;
CONST INT LG_SCROLLING_SPEED_BIG_MONOCHROME = 30;

CONST INT LG_SCROLLING_DELAY_MS = 1500;
CONST TCHAR LG_SCROLLING_GAP_TEXT[] = _T("       ");

// Progress bar parameters
CONST INT LG_PROGRESS_BAR_RANGE_MIN = 0;
CONST INT LG_PROGRESS_BAR_RANGE_MAX = 100;
CONST INT LG_PROGRESS_BAR_INITIAL_WIDTH = 80;
CONST INT LG_PROGRESS_BAR_INITIAL_HEIGHT = 5;
CONST INT LG_PROGRESS_BAR_INITIAL_WIDTH_COLOR = 160;
CONST INT LG_PROGRESS_BAR_INITIAL_HEIGHT_COLOR = 10;

// Font
CONST LPCTSTR LG_FONT = _T("Microsoft Sans Serif");
CONST LPCTSTR LG_ARIAL_FONT = _T("Arial");

//#define MAX_SOFT_BUTTONS		4
//#define LCD_DEFAULT_WIDTH		160
//#define LCD_DEFAULT_HEIGHT		43

CONST DWORD LG_DEVICE_FAMILY_TIMER = 2000; // timer in ms to look for different device families

/*
typedef enum
{
    LG_SCROLLING_TEXT, LG_STATIC_TEXT, LG_ICON, LG_PROGRESS_BAR, LG_UNKNOWN
} LGObjectType;
*/
/*
typedef int LGObjectType;
#define LG_SCROLLING_TEXT   1
#define LG_STATIC_TEXT      2
#define LG_ICON             3
#define LG_PROGRESS_BAR     4
#define LG_UNKNOWN          5
*/
/*
// Soft-Button masks
#define LGLCDBUTTON_BUTTON0                 (0x00000001)
#define LGLCDBUTTON_BUTTON1                 (0x00000002)
#define LGLCDBUTTON_BUTTON2                 (0x00000004)
#define LGLCDBUTTON_BUTTON3                 (0x00000008)

// Soft-Buttons available through the SDK
#define LGLCDBUTTON_LEFT                    (0x00000001)
#define LGLCDBUTTON_RIGHT                   (0x00000002)
#define LGLCDBUTTON_OK                      (0x00000004)
#define LGLCDBUTTON_CANCEL                  (0x00000008)
#define LGLCDBUTTON_UP                      (0x00000010)
#define LGLCDBUTTON_DOWN                    (0x00000020)
#define LGLCDBUTTON_MENU                    (0x00000040)
*/
typedef enum
{
    LG_BUTTON_1, LG_BUTTON_2, LG_BUTTON_3, LG_BUTTON_4,
    LG_BUTTON_LEFT, LG_BUTTON_RIGHT, LG_BUTTON_UP, LG_BUTTON_DOWN,
    LG_BUTTON_OK, LG_BUTTON_CANCEL, LG_BUTTON_MENU,
    NUMBER_SOFT_BUTTONS
} LGSoftButton;

typedef enum
{
    LG_TINY, LG_SMALL, LG_MEDIUM, LG_BIG,
} LGTextSize;

typedef enum
{
    LG_CURSOR, LG_FILLED, LG_DOT_CURSOR
} LGProgressBarType;


#endif		// EZLCD_H_DEFINES_INCLUDED_






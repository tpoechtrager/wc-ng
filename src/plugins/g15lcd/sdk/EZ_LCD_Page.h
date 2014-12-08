//************************************************************************
//  The Logitech LCD SDK, including all acompanying documentation,
//  is protected by intellectual property laws.  All use of the Logitech
//  LCD SDK is subject to the License Agreement found in the
//  "Logitech LCD SDK License Agreement" file and in the Reference Manual.  
//  All rights not expressly granted by Logitech are reserved.
//************************************************************************

// Copyright 2010 Logitech Inc.

#ifndef EZLCD_PAGE_H_INCLUDED_
#define EZLCD_PAGE_H_INCLUDED_




#include "EZ_LCD_Defines.h"
#include "LCDPage.h"

#include "LCDUI.h"

class CEzLcd;

class CEzLcdPage : public CLCDPage
{
public:
    CEzLcdPage();
    CEzLcdPage(CEzLcd * container);
    ~CEzLcdPage();

    HANDLE AddText(LGObjectType type, LGTextSize size, INT alignment, INT maxLengthPixels, INT numberOfLines = 1);
    HRESULT SetText(HANDLE handle, LPCTSTR text, BOOL resetScrollingTextPosition = FALSE);

    HANDLE AddColorText(LGObjectType type, LGTextSize size, INT alignment, INT maxLengthPixels, INT numberOfLines = 1, LONG fontWeight = FW_DONTCARE);
    HRESULT SetTextBackground(HANDLE handle, INT backMode, COLORREF color = RGB(0,0,0));
    HRESULT SetTextFontColor(HANDLE handle, COLORREF color);

    HANDLE AddIcon(HICON hIcon, INT sizeX, INT sizeY);

    HANDLE AddProgressBar(LGProgressBarType type);
    HRESULT SetProgressBarPosition(HANDLE handle, FLOAT percentage);
    HRESULT SetProgressBarSize(HANDLE handle, INT width, INT height);

    //Need a separate function for a color progress bar
    //this is invisible on the EZLCD level
    HANDLE AddColorProgressBar(LGProgressBarType type);
    HRESULT SetProgressBarColors(HANDLE handle, COLORREF cursorcolor, COLORREF bordercolor); 
    HRESULT SetProgressBarBackgroundColor(HANDLE handle, COLORREF color);

    HANDLE AddSkinnedProgressBar(LGProgressBarType type);

    //Animated bitmaps using framestrips
    //In order to have an animated bitmap, one has to have a framestrip with several frames,
    //each having a constant width
    HANDLE AddAnimatedBitmap(HBITMAP bitmap, INT subpicWidth, INT height);
    HRESULT SetAnimatedBitmap(HANDLE handle,INT msPerPic);

    // if width and height defined, the image gets resized to those dimensions
    HANDLE AddBitmap(INT width, INT height);
    HRESULT SetBitmap(HANDLE handle, HBITMAP bitmap,float zoomLevel=1);

    HRESULT SetOrigin(HANDLE handle, INT originX, INT originY);
    HRESULT SetVisible(HANDLE handle, BOOL visible);

    VOID Update();

protected:
    CLCDBase* GetObject(HANDLE handle);
    VOID Init();

protected:
    CEzLcd *    m_container;
    BOOL        m_buttonIsPressed[NUMBER_SOFT_BUTTONS];
    BOOL        m_buttonWasPressed[NUMBER_SOFT_BUTTONS];

};

typedef std::vector <CEzLcdPage*> LCD_PAGE_LIST;
typedef LCD_PAGE_LIST::iterator LCD_PAGE_LIST_ITER;


#endif		// EZLCD_PAGE_H_INCLUDED_

//************************************************************************
//  The Logitech LCD SDK, including all acompanying documentation,
//  is protected by intellectual property laws.  All use of the Logitech
//  LCD SDK is subject to the License Agreement found in the
//  "Logitech LCD SDK License Agreement" file and in the Reference Manual.  
//  All rights not expressly granted by Logitech are reserved.
//************************************************************************

// Copyright 2010 Logitech Inc.

#ifndef EZLCD_H_INCLUDED_
#define EZLCD_H_INCLUDED_


#include "EZ_LCD_Defines.h"
#include "EZ_LCD_Page.h"
#include "LCDConnection.h"


enum AppletSupportType { LG_MONOCHROME_MODE_ONLY, LG_COLOR_MODE_ONLY, LG_DUAL_MODE, LG_NONE };
enum DisplayType { LG_MONOCHROME, LG_COLOR };

class CEzLcd
{
public:
    CEzLcd();
    ~CEzLcd();

    CEzLcd(LPCTSTR friendlyName);

    HRESULT Initialize(LPCTSTR friendlyName, 
        AppletSupportType supportType,
        BOOL isAutoStartable = FALSE,
        BOOL isPersistent = FALSE,
        lgLcdConfigureContext * configContext = NULL,
        lgLcdSoftbuttonsChangedContext * softbuttonChangedContext = NULL);

    BOOL IsDeviceAvailable(DisplayType type);
    VOID ModifyDisplay(DisplayType type);

    INT AddNewPage();
    INT RemovePage(INT pageNumber);
    INT GetPageCount();
    INT AddNumberOfPages(INT numberOfPages);
    HRESULT ModifyControlsOnPage(INT pageNumber);
    HRESULT ShowPage(INT pageNumber);
    INT GetCurrentPageNumber();

    VOID SetBackground(HBITMAP bitmap);
    VOID SetBackground(COLORREF color);

    HANDLE AddText(LGObjectType type, LGTextSize size, INT alignment, INT maxLengthPixels, INT numberOfLines = 1, LONG fontWeight = FW_DONTCARE);
    HRESULT SetText(HANDLE handle, LPCTSTR text, BOOL resetScrollingTextPosition = FALSE);

    //These functions are for color only.
    //The background can either be OPAQUE or TRANSPARENT. If transparent, there is no need to
    //set the third parameter.
    HRESULT SetTextBackground(HANDLE handle, INT backMode, COLORREF color=RGB(0,0,0));
    HRESULT SetTextFontColor(HANDLE handle, COLORREF color);

    HANDLE AddIcon(HICON hIcon, INT sizeX, INT sizeY);

    HANDLE AddProgressBar(LGProgressBarType type);
    HRESULT SetProgressBarPosition(HANDLE handle, FLOAT percentage);
    HRESULT SetProgressBarSize(HANDLE handle, INT width, INT height);

    //This one is for color displays only
    HRESULT SetProgressBarColors(HANDLE handle, COLORREF cursorcolor, COLORREF bordercolor);
    HRESULT SetProgressBarBackgroundColor(HANDLE handle, COLORREF color);

    //Animated bitmaps using framestrips
    //In order to have an animated bitmap, one has to have a framestrip with several frames,
    //each having a constant width
    HANDLE AddAnimatedBitmap(HBITMAP bitmap, INT subpicWidth, INT height);
    HRESULT SetAnimatedBitmap(HANDLE handle,INT msPerPic);

    //Skinned Progress Bar is for color only
    //This type of progress bar uses LG_CURSOR and LG_FILLED types
    //Each width and height refers to the size of the bitmap image
    //Furthermore, do not use the non-skinned function SetProgressBarSize() -- the skinned
    //  version does not scale and displays as the size of the background.
    HANDLE AddSkinnedProgressBar(LGProgressBarType type);
    HRESULT SetSkinnedProgressBarBackground(HANDLE handle, HBITMAP background, INT width, INT height);
    HRESULT SetSkinnedProgressBarFiller(HANDLE handle, HBITMAP filler, INT width, INT height);
    HRESULT SetSkinnedProgressBarCursor(HANDLE handle, HBITMAP cursor, INT width, INT height);
    HRESULT SetSkinnedProgressBarThreePieceCursor(HANDLE handle, HBITMAP left, INT bmpLeftWidth, INT bmpLeftHeight,
                             HBITMAP mid, INT bmpMidWidth, INT bmpMidHeight,
                             HBITMAP right, INT bmpRightWidth, INT bmpRightHeight);
    HRESULT SetSkinnedProgressHighlight(HANDLE handle, HBITMAP highlight, INT width, INT height);

    HANDLE AddBitmap(INT width, INT height);
    HRESULT SetBitmap(HANDLE handle, HBITMAP bitmap, float zoomLevel=1);

    HRESULT SetOrigin(HANDLE handle, INT originX, INT originY);
    HRESULT SetVisible(HANDLE handle, BOOL visible);

    HRESULT SetAsForeground(BOOL setAsForeground);

    HRESULT SetScreenPriority(DWORD priority);
    DWORD GetScreenPriority();

    BOOL ButtonTriggered(INT button);
    BOOL ButtonReleased(INT button);
    BOOL ButtonIsPressed(INT button); 

    // These functions have been deprecated.
    BOOL AnyDeviceOfThisFamilyPresent(DWORD deviceFamily);
    HRESULT SetDeviceFamilyToUse(DWORD deviceFamily);
    HRESULT SetPreferredDisplayFamily(DWORD deviceFamily);

    VOID Update();

protected:
    static DWORD WINAPI OnButtonCB(IN INT connection, IN DWORD dwButtons, IN const PVOID pContext);
    virtual VOID OnButtons(DWORD buttons);
    virtual CLCDConnection* AllocLCDConnection(void);
    CLCDOutput*             GetCurrentOutput();

    TCHAR                   m_friendlyName[MAX_PATH];
    CLCDConnection         *m_pConnection;
    AppletSupportType       m_SupportType;

    LCD_PAGE_LIST           m_LCDPageListMono, m_LCDPageListColor;
    CEzLcdPage*             m_activePageMono, *m_activePageColor;
    INT                     m_pageCountMono, m_pageCountColor;		// How many pages there are
    INT                     m_currentPageNumberShownMono, m_currentPageNumberShownColor;

    BOOL                    m_initNeeded;
    BOOL                    m_initSucceeded;
    lgLcdConfigureContext * m_configContext;
    BOOL                    m_isPersistent;
    BOOL                    m_isAutoStartable;
    DWORD                   m_currentDeviceFamily;
    DWORD                   m_preferredDeviceFamily;

    INT m_previousScreenPriorityBW;
    INT m_previousScreenPriorityColor;

    CRITICAL_SECTION        m_ButtonCS;
    DWORD                   m_ButtonCache;
    DWORD                   m_PrevButtonStatus;
    DWORD                   m_CurrButtonStatus;
    lgLcdSoftbuttonsChangedContext m_SBContext;

    inline CEzLcdPage*      GetActivePage();
    CLCDOutput*             m_pCurrentOutput;
private:
    LCD_PAGE_LIST&          GetPageList();
    inline VOID             SetActivePage(CEzLcdPage*);
    INT                     GetCurrentPageNumberShown();
    VOID                    SetCurrentPageNumberShown(INT nPage);  
};


#endif

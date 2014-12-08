//************************************************************************
//  The Logitech LCD SDK, including all acompanying documentation,
//  is protected by intellectual property laws.  All use of the Logitech
//  LCD SDK is subject to the License Agreement found in the
//  "Logitech LCD SDK License Agreement" file and in the Reference Manual.  
//  All rights not expressly granted by Logitech are reserved.
//************************************************************************

// Copyright 2010 Logitech Inc.

/****f* LCD.SDK/EzLcd
* NAME
*   LCD SDK
* COPYRIGHT
*   The Logitech LCD SDK, including all accompanying documentation, is
*   protected by intellectual property laws. All rights not expressly
*   granted by Logitech are reserved.
* PURPOSE
*   The LCD SDK is aimed at developers wanting to make use of the LCD
*   display on Logitech G-series keyboard. It comes with a very
*   intuitive and easy to use interface which enables one to easily
*   display static strings, scrolling strings, progress bars, icons
*   and bitmaps.
*   See the following to get started:
*       - readme.txt: Describes how to get started
*       - Use one of the samples included to see how things work.
* AUTHOR
*   Christophe Juncker (cj@wingmanteam.com)
* CREATION DATE
*   06/13/2005
* MODIFICATION HISTORY
*   04/25/2011 - Fixed animated bitmap bug. Updated documentation.
*   06/26/2009 - Renamed to LCD SDK. Fixed SetPriority bug that would 
*                happen when using mode other than dual.
*   09/24/2008 - Added support for color display.
*   04/30/2007 - Added string safe function "LCDUI_tcscpy", See
*                LCDBase.h for macro definition.
*   03/01/2006 - Added the concept of pages to the API. A client can
*                now have multiple pages, each with its own
*                controls. A page can be shown, while another is
*                modified.
*   11/17/2006 - Added extra AddBitmap(...) method with width and
*                height parameters. Added GetCurrentPageNumber()
*                method.
*
******
*/

#include "LCDUI.h"
#include "EZ_LCD_Defines.h"
#include "EZ_LCD_Page.h"
#include "EZ_LCD.h"

/****f* LCD.SDK/CEzLcd()
* NAME
*  CEzLcd() -- Basic constructor. The user must call the
*  InitYourself(...) method after calling this constructor.
* FUNCTION
*  Object is created.
******
*/
CEzLcd::CEzLcd()
{
    m_pCurrentOutput = NULL;
    m_activePageMono = m_activePageColor = NULL;
    m_pageCountMono = m_pageCountColor = 0;
    m_currentPageNumberShownMono = m_currentPageNumberShownColor = 0;
    m_SupportType = LG_NONE;
    m_ButtonCache = 0;
    m_CurrButtonStatus = 0;
    m_previousScreenPriorityBW = -1;
    m_previousScreenPriorityColor = -1;

    m_pConnection = NULL;

    InitializeCriticalSection(&m_ButtonCS);
}

/****f* LCD.SDK/CEzLcd(LPCTSTR.friendlyName)
* NAME
*  CEzLcd(LPCTSTR friendlyName) -- Constructor.
* FUNCTION
*  Does necessary initialization. If you are calling this constructor,
*  then you should NOT call the InitYourself(...) method.
* INPUTS
*  friendlyName - friendly name of the applet/game. This name will be
*                 displayed in the Logitech G-series LCD Manager.
******
*/
CEzLcd::CEzLcd(LPCTSTR friendlyName)
{
    CEzLcd();
    Initialize(friendlyName, LG_MONOCHROME_MODE_ONLY, FALSE, FALSE, NULL, NULL);
}

CEzLcd::~CEzLcd()
{
    // delete all the screens
    LCD_PAGE_LIST::iterator it = m_LCDPageListMono.begin();
    while(it != m_LCDPageListMono.end())
    {
        CEzLcdPage *page_ = *it;
        LCDUIASSERT(NULL != page_);

        delete page_;
        ++it;
    }

    it = m_LCDPageListColor.begin();
    while(it != m_LCDPageListColor.end())
    {
        CEzLcdPage *page_ = *it;
        LCDUIASSERT(NULL != page_);

        delete page_;
        ++it;
    }

    m_pConnection->Shutdown();

    DeleteCriticalSection(&m_ButtonCS);
    delete m_pConnection;
    m_pConnection = NULL;
}

/****f* LCD.SDK/Initialize(LPCTSTR.friendlyName,AppletSupportType.supportType,BOOL.isAutoStartable,BOOL.isPersistent,lgLcdConfigureContext*configContext,lgLcdSoftbuttonsChangedContext*softbuttonChangedContext)
* NAME
*  HRESULT Initialize(LPCTSTR friendlyName,
*  AppletSupportType supportType,
*  BOOL isAutoStartable = FALSE,
*  BOOL isPersistent = FALSE,
*  lgLcdConfigureContext* configContext = NULL,
*  lgLcdNotificationContext* notificationContext = NULL,
*  lgLcdSoftbuttonsChangedContext* softbuttonChangedContext = NULL)
* FUNCTION
*  Does necessary initialization. This method SHOULD ONLY be called if
*  the empty constructor is used: CEzLcd().
* INPUTS
*  friendlyName     - friendly name of the applet/game. This name will be
*                     displayed in the Logitech G-series LCD Manager.
*  supportType      - What type of displays the applet supports. Options
*                     are:
*                       - LG_MONOCHROME_MODE_ONLY
*                       - LG_COLOR_MODE_ONLY
*                       - LG_DUAL_MODE
*  IsAutoStartable  - Determines if the applet is to be started
*                     automatically every time by the LCD manager software
*                     (Part of G15 software package).
*  IsPersistent     - Determines if the applet's friendlyName will remain
*                     in the list of applets seen by the LCD manager
*                     software after the applet terminates.
*  configContext    - Pointer to the lgLcdConfigContext structure used
*                     during callback into the applet.
*  softbuttonChangedContext - softbutton callback (if the user does
*                             not want to use LCD SDK's native handler).
* RETURN VALUE
*  S_OK if it can connect to the LCD Manager.
*  E_FAIL otherwise.
******
*/
HRESULT CEzLcd::Initialize(LPCTSTR friendlyName,
        AppletSupportType supportType,
        BOOL isAutoStartable,
        BOOL isPersistent,
        lgLcdConfigureContext* configContext,
        lgLcdSoftbuttonsChangedContext* softbuttonChangedContext)
{
    if ( (m_pageCountMono != 0) || (m_pageCountColor != 0) )
    {
        // Maybe the user is calling the old constructor and calling InitYourself as well.
        // Alert him of the problem. If the old constructor is called, then InitYourself should
        // not be called. InitYourself should be called, when the empty parameter constructor is
        // called, CEzLcd()
        return E_FAIL;
    }

    LCDUI_tcscpy(m_friendlyName, friendlyName);

    m_initNeeded = TRUE;
    m_initSucceeded = FALSE;
    m_currentDeviceFamily = LGLCD_DEVICE_FAMILY_OTHER;
    m_preferredDeviceFamily = LGLCD_DEVICE_FAMILY_OTHER;

    m_configContext = configContext;		// Keep the context structure pointer
    m_isPersistent = isPersistent;
    m_isAutoStartable = isAutoStartable;

    // No active page to start with
    m_activePageMono = m_activePageColor = NULL;
    m_currentPageNumberShownMono = m_currentPageNumberShownColor = 0;

    // Will now connect to the real library and see it the lgLcdInit() succeeds
    lgLcdConnectContextEx   lgdConnectContextEx_;
    lgLcdConfigureContext   lgdConfigureContext_;

    if (m_configContext != NULL)
    {
        lgdConfigureContext_.configCallback = m_configContext->configCallback;
        lgdConfigureContext_.configContext = m_configContext->configContext;
    }
    else
    {
        lgdConfigureContext_.configCallback = NULL;
        lgdConfigureContext_.configContext = NULL;
    }

    lgdConnectContextEx_.appFriendlyName = m_friendlyName;
    lgdConnectContextEx_.isPersistent = m_isPersistent;
    lgdConnectContextEx_.isAutostartable = m_isAutoStartable;
    lgdConnectContextEx_.onConfigure = lgdConfigureContext_;

    switch(supportType)
    {
    case LG_MONOCHROME_MODE_ONLY:
        lgdConnectContextEx_.dwAppletCapabilitiesSupported = LGLCD_APPLET_CAP_BW;
        break;
    case LG_COLOR_MODE_ONLY:
        lgdConnectContextEx_.dwAppletCapabilitiesSupported = LGLCD_APPLET_CAP_QVGA;
        break;
    case LG_DUAL_MODE:
        lgdConnectContextEx_.dwAppletCapabilitiesSupported = LGLCD_APPLET_CAP_BW|LGLCD_APPLET_CAP_QVGA;
        break;
    default:
        break;
    }

    m_SupportType = supportType;

    lgdConnectContextEx_.dwReserved1 = NULL;
    lgdConnectContextEx_.onNotify.notificationCallback = NULL;
    lgdConnectContextEx_.onNotify.notifyContext = NULL;

    if( NULL == softbuttonChangedContext )
    {
        m_SBContext.softbuttonsChangedCallback = OnButtonCB;
        m_SBContext.softbuttonsChangedContext = this;
    }
    else
    {
        m_SBContext.softbuttonsChangedCallback = softbuttonChangedContext->softbuttonsChangedCallback;
        m_SBContext.softbuttonsChangedContext = softbuttonChangedContext->softbuttonsChangedContext;
    }

    m_pConnection = AllocLCDConnection();
    if (FALSE == m_pConnection->Initialize(lgdConnectContextEx_, &m_SBContext))
    {
        // This means the LCD SDK's lgLcdInit failed, and therefore
        // we will not be able to ever connect to the LCD, even if
        // a G-series keyboard is actually connected.
        LCDUITRACE(_T("ERROR: LCD SDK initialization failed\n"));
        m_initSucceeded = FALSE;
        return E_FAIL;
    }
    else
    {
        m_initSucceeded = TRUE;
    }

    // Add the first page(s) for backwards compatibility
    if( m_SupportType == LG_COLOR_MODE_ONLY || m_SupportType == LG_DUAL_MODE )
    {
        ModifyDisplay(LG_COLOR);
        AddNewPage();
        ModifyControlsOnPage(0);
        ShowPage(0);
    }

    if( m_SupportType == LG_MONOCHROME_MODE_ONLY || m_SupportType == LG_DUAL_MODE )
    {
        ModifyDisplay(LG_MONOCHROME);
        AddNewPage();
        ModifyControlsOnPage(0);
        ShowPage(0);
    }

    m_initNeeded = FALSE;

    // Default mode is Monochrome

    return S_OK;
}

/****f* LCD.SDK/IsDeviceAvailable(DisplayType.type)
* NAME
*  BOOL IsDeviceAvailable(DisplayType type) -- Check if a device of
*  specified type is available.
* INPUTS
*  type - type of the device. Possible values are:
*         - LG_MONOCHROME
*         - LG_COLOR
* RETURN VALUE
*  TRUE if available
*  FALSE otherwise.
******
*/
BOOL CEzLcd::IsDeviceAvailable(DisplayType type)
{
    CLCDOutput* pOutput = NULL;

    switch(type)
    {
    case LG_MONOCHROME:
        pOutput = m_pConnection->MonoOutput();
        break;
    case LG_COLOR:
        pOutput = m_pConnection->ColorOutput();
        break;
    default:
        break;
    }

    if (NULL == pOutput)
    {
        return FALSE;
    }

    return pOutput->IsOpened();
}

/****f* LCD.SDK/ModifyDisplay(DisplayType.type)
* NAME
*  VOID ModifyDisplay(DisplayType type) -- Define whether to modify
*  color or monochrome display.
* NOTES
*  If programming support for both color and monochrome displays
*  (LG_DUAL_MODE), this function needs to be called every time before
*  calling a function or a set of functions for one of the displays.
* INPUTS
*  type - type of the display. Possible values are:
*         - LG_MONOCHROME
*         - LG_COLOR
******
*/
VOID CEzLcd::ModifyDisplay(DisplayType type)
{
    switch(type)
    {
    case LG_MONOCHROME:
        m_pCurrentOutput = m_pConnection->MonoOutput();
        break;
    case LG_COLOR:
        m_pCurrentOutput = m_pConnection->ColorOutput();
        break;
    default:
        break;
    }
}

/****f* LCD.SDK/AddNewPage()
* NAME
*  INT AddNewPage() -- Call this method to create a new page to be
*  displayed on the LCD.
* RETURN VALUE
*  Current number of pages, after the page is added.
******
*/
INT CEzLcd::AddNewPage()
{
    CEzLcdPage*	page_ = NULL;

    // Create a new page and add it in
    page_ = new CEzLcdPage(this);
    page_->Initialize();
    page_->SetExpiration(INFINITE);

    if( m_pCurrentOutput == m_pConnection->ColorOutput() )
    {
        page_->SetSize(320,240);
    }

    LCD_PAGE_LIST& PageList = GetPageList();
    PageList.push_back(page_);

    return (INT)PageList.size();
}

/****f* LCD.SDK/RemovePage(INT.pageNumber)
* NAME
*  INT RemovePage(INT pageNumber) -- Call this method to remove a page
*  from the pages you've created to be displayed on the LCD.
* INPUTS
*  pageNumber - The number for the page that is to be removed.
* RETURN VALUE
*  Current number of pages, after the page is removed.
******
*/
INT CEzLcd::RemovePage(INT pageNumber)
{
    LCD_PAGE_LIST& PageList = GetPageList();

    // Do we have this page, if not return error
    if (pageNumber >= (INT)(PageList.size()))
    {
        return -1;
    }

    // find the next active screen
    LCD_PAGE_LIST::iterator it = PageList.begin();
    PageList.erase(it + pageNumber);

    // If there are any pages, the first one is the active page
    if (PageList.size())
    {
        SetActivePage(*PageList.begin());
    }
    else
    {
        SetActivePage(NULL);
    }

    return (INT)PageList.size();
}

/****f* LCD.SDK/GetPageCount()
* NAME
*  INT GetPageCount() -- Returns the current number of pages.
* RETURN VALUE
*  Current number of pages.
******
*/
INT CEzLcd::GetPageCount()
{
    return (INT)GetPageList().size();
}

/****f* LCD.SDK/AddNumberOfPages(INT.numberOfPages)
* NAME
*  INT AddNumberOfPages(INT numberOfPages) -- Adds numberOfPages to
*  the total of pages you've created.
* INPUTS
*  numberOfPages - Count of pages to add in.
* RETURN VALUE
*  Current number of pages, after the pages are added.
******
*/
INT CEzLcd::AddNumberOfPages(INT numberOfPages)
{
    for (INT iCount=0; iCount<numberOfPages; iCount++)
    {
        AddNewPage();
    }

    return GetPageCount();
}


/****f* LCD.SDK/ModifyControlsOnPage(INT.pageNumber)
* NAME
*  HRESULT ModifyControlsOnPage(INT pageNumber) -- Call this method in
*  order to modify the controls on specified page.
* NOTES
*  This method must be called first prior to any modifications on that
*  page.
* INPUTS
*  pageNumber - The page number that the controls will be modified on.
* RETURN VALUE
*  S_OK if succeeded.
*  E_FAIL otherwise.
******
*/
HRESULT CEzLcd::ModifyControlsOnPage(INT pageNumber)
{
    LCD_PAGE_LIST& PageList = GetPageList();

    // Do we have this page, if not return error
    if (pageNumber >= (INT)(PageList.size()))
    {
        return E_FAIL;
    }

    LCD_PAGE_LIST::iterator it = PageList.begin();
    SetActivePage(*(it + pageNumber));

    return S_OK;
}

/****f* LCD.SDK/ShowPage(INT.pageNumber)
* NAME
*  HRESULT ShowPage(INT pageNumber) -- Call this method in order to
*  make the page shown on the LCD.
* INPUTS
*  pageNumber - The page that will be shown among your pages on the
*  LCD.
* RETURN VALUE
*  S_OK if succeeded.
*  E_FAIL otherwise.
******
*/
HRESULT CEzLcd::ShowPage(INT pageNumber)
{
    LCD_PAGE_LIST& PageList = GetPageList();

    // Do we have this page, if not return error
    if (pageNumber >= (INT)(PageList.size()))
    {
        return E_FAIL;
    }

    LCD_PAGE_LIST::iterator it = PageList.begin();
    SetActivePage(*(it + pageNumber));

    m_pCurrentOutput->ShowPage(GetActivePage());

    SetCurrentPageNumberShown(pageNumber);

    return S_OK;
}

/****f* LCD.SDK/GetCurrentPageNumber()
* NAME
*  INT GetCurrentPageNumber() -- Get page number currently being
*  displayed.
* RETURN VALUE
*  Number of page currently being displayed. First page is number 0.
******
*/
INT CEzLcd::GetCurrentPageNumber()
{
    return GetCurrentPageNumberShown();
}

/****f* LCD.SDK/SetBackground(HBITMAP.bitmap)
* NAME
*  VOID SetBackground(HBITMAP bitmap) - Use a bitmap as a background.
* NOTES
*  To be used for color display only.
* INPUTS
*  bitmap - handle to the bitmap to be used.
******
*/
VOID CEzLcd::SetBackground(HBITMAP bitmap)
{
    if (GetActivePage() == NULL)
    {
        return;
    }

    if( m_pCurrentOutput == m_pConnection->ColorOutput() )
    {
        GetActivePage()->SetBackground(bitmap);
    }
}

/****f* LCD.SDK/SetBackground(COLORREF.color)
* NAME
*  VOID SetBackground(COLORREF color) - Use a color as a background.
* NOTES
*  To be used for color display only.
* INPUTS
*  color - color to set the background.
******
*/
VOID CEzLcd::SetBackground(COLORREF color)
{
        if (GetActivePage() == NULL)
    {
        return;
    }

    if( m_pCurrentOutput == m_pConnection->ColorOutput() )
    {
        GetActivePage()->SetBackground(color);
    }
}

/****f* LCD.SDK/AddText(LGObjectType.type,LGTextSize.size,INT.alignment,INT.maxLengthPixels,INT.numberOfLines,LONG.fontWeight)
* NAME
*  HANDLE AddText(LGObjectType type,
*   LGTextSize size,
*   INT alignment,
*   INT maxLengthPixels,
*   INT numberOfLines = 1,
*   fontWeight = FW_DONTCARE) -- Add a text object to the page being
*   worked on.
* INPUTS
*  type            - specifies whether the text is static or
*                    scrolling. Possible types are:
*                      - LG_SCROLLING_TEXT
*                      - LG_STATIC_TEXT
*  size            - size of the text. Can be any of the following:
*                      - LG_TINY (color display only)
*                      - LG_SMALL
*                      - LG_MEDIUM
*                      - LG_BIG
*  alignment       - alignment of the text. Can be any of the following:
*                      - DT_LEFT
*                      - DT_CENTER
*                      - DT_RIGHT
*  maxLengthPixels - max length in pixels of the text. If the text is
*                    longer and of type LG_STATIC_TEXT, it will be cut
*                    off. If the text is longer and of type
*                    LG_SCROLLING_TEXT, it will scroll.
*  numberOfLines   - number of lines the text can use. For static text
*                    only. If number bigger than 1 and static text is
*                    too long to fit on LCD, the text will be displayed
*                    on multiple lines as necessary.
*  fontWeight      - weight of the font. Options are:
*                  - FW_DONTCARE, FW_THIN, FW_EXTRALIGHT, FW_LIGHT,
*                    FW_NORMAL,FW_MEDIUM, FW_SEMIBOLD, FW_BOLD,
*                    FW_EXTRABOLD, FW_HEAVY
* RETURN VALUE
*  Handle for this object.
* SEE ALSO
*  AddIcon(HICON.hIcon,INT.sizeX,INT.sizeY)
*  AddProgressBar(LGProgressBarType.type)
******
*/
HANDLE CEzLcd::AddText(LGObjectType type, LGTextSize size, INT alignment, INT maxLengthPixels,
                       INT numberOfLines, LONG fontWeight)
{
    if (GetActivePage() == NULL)
    {
        return NULL;
    }

    if( m_pCurrentOutput == m_pConnection->MonoOutput() )
    {
        return GetActivePage()->AddText(type, size, alignment, maxLengthPixels, numberOfLines);
    }
    else if( m_pCurrentOutput == m_pConnection->ColorOutput() )
    {
        return GetActivePage()->AddColorText(type, size, alignment, maxLengthPixels, numberOfLines, fontWeight);
    }

    return NULL;
}

/****f* LCD.SDK/SetText(HANDLE.handle,LPCTSTR.text,BOOL.resetScrollingTextPosition)
* NAME
*  HRESULT SetText(HANDLE handle,
*   LPCTSTR text,
*   BOOL resetScrollingTextPosition = FALSE) -- Sets the text in the
*   control on the page being worked on.
* INPUTS
*  handle                     - handle to the object.
*  text                       - text string.
*  resetScrollingTextPosition - indicates if position of scrolling
*                               text needs to be reset.
* RETURN VALUE
*  S_OK if succeeded.
*  E_FAIL otherwise.
******
*/
HRESULT CEzLcd::SetText(HANDLE handle, LPCTSTR text, BOOL resetScrollingTextPosition)
{
    CLCDBase* myObject = (CLCDBase*)handle;

    if (NULL != myObject)
    {
        if (!((LG_STATIC_TEXT == myObject->GetObjectType() || LG_SCROLLING_TEXT == myObject->GetObjectType() || LG_RIGHTFOCUS_TEXT == myObject->GetObjectType())))
            return E_FAIL;

        if (LG_STATIC_TEXT == myObject->GetObjectType())
        {
            CLCDText* staticText = static_cast<CLCDText*>(myObject);
            if (NULL == staticText)
                return E_FAIL;

            staticText->SetText(text);
            return S_OK;
        }
        else if (LG_SCROLLING_TEXT == myObject->GetObjectType())
        {
            CLCDStreamingText* streamingText = static_cast<CLCDStreamingText*>(myObject);
            if (NULL == streamingText)
                return E_FAIL;

            streamingText->SetText(text);
            if (resetScrollingTextPosition)
            {
                streamingText->ResetUpdate();
            }
            return S_OK;
        }
        else if (LG_RIGHTFOCUS_TEXT == myObject->GetObjectType())
        {
            CLCDText* rightFocusText = static_cast<CLCDText*>(myObject);
            if (NULL == rightFocusText)
                return E_FAIL;
            
            rightFocusText->SetText(text);
            rightFocusText->CalculateExtent(true);
            // if out of focus, set alignment to right in order to follow what is written
            if (rightFocusText->GetHExtent().cx>=rightFocusText->GetSize().cx)
                rightFocusText->SetAlignment(DT_RIGHT);
            else
                rightFocusText->SetAlignment(DT_LEFT);
            
            return S_OK;
        }
    }

    return E_FAIL;
}



/****f* LCD.SDK/SetTextBackground(HANDLE.handle,INT.backMode,COLORREF.color)
* NAME
*  HRESULT SetTextBackground(HANDLE handle,
*   INT backMode,
*   COLORREF color=RGB(0,0,0)) -- Set whether the background is opaque
*   or transparent, and if opaque, set the color.
* NOTES
*  To be used for color display only.
* INPUTS
*  handle   - handle to the object.
*  backMode - OPAQUE or TRANSPARENT.
*  color    - color to set an opaque background.
* RETURN VALUE
*  S_OK if succeeded.
*  E_FAIL otherwise.
******
*/
HRESULT CEzLcd::SetTextBackground(HANDLE handle, INT backMode, COLORREF color)
{
    CLCDBase* myObject_ = (CLCDBase*)handle;

    if (NULL != myObject_)
    {
        CLCDText* pText_ = static_cast<CLCDText*>(myObject_);
        LCDUIASSERT(NULL != pText_);
        pText_->SetBackgroundMode(backMode);
        pText_->SetBackgroundColor(color);
        return S_OK;
    }

    return E_FAIL;
}

/****f* LCD.SDK/SetTextFontColorHANDLE.handle,COLORREF.color)
* NAME
*  HRESULT SetTextFontColor(HANDLE handle, COLORREF color) -- Sets the
*  color of the text.
* NOTES
*  To be used for color display only.
* INPUTS
*  handle - handle to the object.
*  color  - color to set the text.
* RETURN VALUE
*  S_OK if succeeded.
*  E_FAIL otherwise.
******
*/
HRESULT CEzLcd::SetTextFontColor(HANDLE handle, COLORREF color)
{
    CLCDBase* myObject_ = (CLCDBase*)handle;

    if (NULL != myObject_)
    {
        CLCDText* pText_ = static_cast<CLCDText*>(myObject_);
        LCDUIASSERT(NULL != pText_);
        pText_->SetForegroundColor(color);
        return S_OK;
    }

    return E_FAIL;
}

/****f* LCD.SDK/AddIcon(HICON.hIcon,INT.sizeX,INT.sizeY)
* NAME
*  HANDLE AddIcon(HICON hIcon, INT sizeX, INT sizeY) -- Add an icon
*  object to the page being worked on.
* INPUTS
*  hIcon - icon to be displayed on the page. Should be 1 bpp bitmap.
*  sizeX - x axis size of the bitmap.
*  sizeY - y axis size of the bitmap.
* RETURN VALUE
*  Handle for this object.
* SEE ALSO
*  AddText(LGObjectType.type,LGTextSize.size,INT.alignment,INT.maxLengthPixels,INT.numberOfLines,LONG.fontWeight)
*  AddProgressBar(LGProgressBarType.type)
******
*/
HANDLE CEzLcd::AddIcon(HICON hIcon, INT sizeX, INT sizeY)
{
    if (GetActivePage() == NULL)
    {
        return NULL;
    }

    return GetActivePage()->AddIcon(hIcon, sizeX, sizeY);
}

/****f* LCD.SDK/AddProgressBar(LGProgressBarType.type)
* NAME
*  HANDLE AddProgressBar(LGProgressBarType type) -- Add a progress bar
*  object to the page being worked on.
* INPUTS
*  type - type of the progress bar. Types are:
*         - LG_CURSOR, LG_FILLED, LG_DOT_CURSOR.
* RETURN VALUE
*  Handle for this object.
* SEE ALSO
*  AddText(LGObjectType.type,LGTextSize.size,INT.alignment,INT.maxLengthPixels,INT.numberOfLines,LONG.fontWeight)
*  AddIcon(HICON.hIcon,INT.sizeX,INT.sizeY)
******
*/
HANDLE CEzLcd::AddProgressBar(LGProgressBarType type)
{
    if( GetActivePage() == NULL ||
        m_pCurrentOutput == NULL )
    {
        return NULL;
    }

    if( m_pCurrentOutput == m_pConnection->ColorOutput() )
    {
        return GetActivePage()->AddColorProgressBar(type);
    }
    else
    {
        return GetActivePage()->AddProgressBar(type);
    }
}

/****f* LCD.SDK/SetProgressBarPosition(HANDLE.handle,FLOAT.percentage)
* NAME
*  HRESULT SetProgressBarPosition(HANDLE handle, FLOAT percentage) --
*  Set position of the progress bar's cursor.
* INPUTS
*  handle     - handle to the object.
*  percentage - percentage of progress (0 to 100).
* RETURN VALUE
*  S_OK if succeeded.
*  E_FAIL otherwise.
******
*/
HRESULT CEzLcd::SetProgressBarPosition(HANDLE handle, FLOAT percentage)
{
    CLCDBase* myObject_ = (CLCDBase*)handle;

    if (NULL != myObject_)
    {
        LCDUIASSERT(LG_PROGRESS_BAR == myObject_->GetObjectType());
// only allow this function for progress bars
        if (LG_PROGRESS_BAR == myObject_->GetObjectType())
        {
            CLCDProgressBar *progressBar_ = static_cast<CLCDProgressBar*>(myObject_);
            LCDUIASSERT(NULL != progressBar_);
            progressBar_->SetPos(percentage);
            return S_OK;
        }
    }

    return E_FAIL;
}

/****f* LCD.SDK/SetProgressBarSize(HANDLE.handle,INT.width,INT.height)
* NAME
*  HRESULT SetProgressBarSize(HANDLE handle, INT width, INT height) --
*  Set size of progress bar.
* INPUTS
*  handle - handle to the object.
*  width  - x axis part of the size.
*  height - y axis part of the size (a good default value is 5).
* RETURN VALUE
*  S_OK if succeeded.
*  E_FAIL otherwise.
******
*/
HRESULT CEzLcd::SetProgressBarSize(HANDLE handle, INT width, INT height)
{
    CLCDBase* myObject_ = (CLCDBase*)handle;
    LCDUIASSERT(NULL != myObject_);

    if (NULL != myObject_)
    {
        LCDUIASSERT(LG_PROGRESS_BAR == myObject_->GetObjectType());
// only allow this function for progress bars
        if (LG_PROGRESS_BAR == myObject_->GetObjectType())
        {
            myObject_->SetSize(width, height);
            return S_OK;
        }
    }

    return E_FAIL;
}

/****f* LCD.SDK/SetProgressBarColors(HANDLE.handle,COLORREF.cursorColor,COLORREF.borderColor)
* NAME
*  HRESULT SetProgressBarColors(HANDLE handle, COLORREF cursorColor,
*  COLORREF borderColor) -- For color only. Set the color of the
*  progress bar.
* INPUTS
*  handle      - handle to the object.
*  cursorColor - color to set the cursor.
*  borderColor - color to set the border.
* RETURN VALUE
*  S_OK if succeeded.
*  E_FAIL otherwise.
******
*/
HRESULT CEzLcd::SetProgressBarColors(HANDLE handle, COLORREF cursorColor, COLORREF borderColor)
{
    CLCDBase* myObject_ = (CLCDBase*)handle;

    if (NULL != myObject_)
    {
        LCDUIASSERT(LG_PROGRESS_BAR == myObject_->GetObjectType());
        // only allow this function for progress bars
        if (LG_PROGRESS_BAR == myObject_->GetObjectType())
        {
            CLCDColorProgressBar *progressBar_ = static_cast<CLCDColorProgressBar*>(myObject_);
            LCDUIASSERT(NULL != progressBar_);
            progressBar_->SetCursorColor(cursorColor);
            progressBar_->SetBorderColor(borderColor);
            return S_OK;
        }
    }

    return E_FAIL;
}

/****f* LCD.SDK/SetProgressBarBackgroundColor(HANDLE.handle,COLORREF.color)
* NAME
* HRESULT SetProgressBarBackgroundColor(HANDLE handle, COLORREF color)
* -- For color only. Set the backgroundcolor of the progress bar.
* INPUTS
*  handle - handle to the object.
*  color  - color to set the background.
* RETURN VALUE
*  S_OK if succeeded.
*  E_FAIL otherwise.
******
*/
HRESULT CEzLcd::SetProgressBarBackgroundColor(HANDLE handle, COLORREF color)
{
    CLCDBase* myObject_ = (CLCDBase*)handle;

    if (NULL != myObject_)
    {
        LCDUIASSERT(LG_PROGRESS_BAR == myObject_->GetObjectType());
        // only allow this function for progress bars
        if (LG_PROGRESS_BAR == myObject_->GetObjectType())
        {
            CLCDColorProgressBar *progressBar_ = static_cast<CLCDColorProgressBar*>(myObject_);
            LCDUIASSERT(NULL != progressBar_);
            progressBar_->SetBackgroundColor(color);
            return S_OK;
        }
    }

    return E_FAIL;
}

/****f* LCD.SDK/AddAnimatedBitmap(HBITMAP.bitmap,INT.subpicWidth,INT.height)
* NAME
* HANDLE AddAnimatedBitmap(HBITMAP bitmap, INT subpicWidth, INT height)
* -- Creates an Animated Bitmap. The bitmap must be composed of framestrips with constant dimensions.
* The framestrips must be aligned in the x axis
* INPUTS
*  bitmap       - the HBITMAP of the bitmap
*  subpicWidth  - width of 1 framestrip
*..height       - height of the bitmap
* RETURN VALUE
*  HANDLE of the Animated Bitmap if succeeded.
*  NULL otherwise.
******
*/
HANDLE CEzLcd::AddAnimatedBitmap(HBITMAP bitmap, INT subpicWidth, INT height)
{
    if (GetActivePage() == NULL)
    {
        return NULL;
    }

    return GetActivePage()->AddAnimatedBitmap(bitmap, subpicWidth, height);
}


/****f* LCD.SDK/SetAnimatedBitmap(HANDLE.handle,INT.msPerPic)
* NAME
* HRESULT SetAnimatedBitmap(HANDLE handle,INT msPerPic)
* -- Set options to the animated bitmap
* INPUTS
*  handle   - the HANDLE of the animated bitmap
*  msPerPic - frame rate (milliseconds per frame)
* RETURN VALUE
*  S_OK if succeeded.
*  E_FAIL otherwise.
******
*/
HRESULT CEzLcd::SetAnimatedBitmap(HANDLE handle,INT msPerPic)
{
    if (GetActivePage() == NULL)
    {
        return E_FAIL;
    }

    return GetActivePage()->SetAnimatedBitmap(handle,msPerPic);
}

/****f* LCD.SDK/AddSkinnedProgressBar(LGProgressBarType.type)
* NAME
*  HANDLE AddSkinnedProgressBar(LGProgressBarType type) -- Add a
*  skinned progress bar.
* NOTES
*  To be used for color display only.
* INPUTS
*   type - type of the progress bar. Types are: LG_CURSOR, LG_FILLED.
* RETURN VALUE
*  Handle for this object.
******
*/
HANDLE CEzLcd::AddSkinnedProgressBar(LGProgressBarType type)
{
    if( GetActivePage() == NULL ||
        m_pCurrentOutput == NULL )
    {
        return NULL;
    }

    if( m_pCurrentOutput == m_pConnection->ColorOutput() )
    {
        return GetActivePage()->AddSkinnedProgressBar(type);
    }

    return NULL;
}

/****f* LCD.SDK/SetSkinnedProgressBarBackground(HANDLE.handle,HBITMAP.background,INT.width,INT.height)
* NAME
* HRESULT SetSkinnedProgressBarBackground(HANDLE handle, HBITMAP
* background, INT width, INT height) -- Sets the background bitmap of
* the progress bar. Width and height will set the size of the progress
* bar.
* INPUTS
*   handle        - Handle to the progress bar.
*   background    - Handle to the bitmap to display.
*   width, height - Size of the image, and therefore the progress bar.
* RETURN VALUE
*  S_OK if succeeded.
*  E_FAIL otherwise.
******
*/
HRESULT CEzLcd::SetSkinnedProgressBarBackground(HANDLE handle, HBITMAP background, INT width, INT height)
{
    CLCDBase* myObject_ = (CLCDBase*)handle;

    if (NULL != myObject_)
    {
        LCDUIASSERT(LG_PROGRESS_BAR == myObject_->GetObjectType());
        // only allow this function for progress bars
        if (LG_PROGRESS_BAR == myObject_->GetObjectType())
        {
            CLCDSkinnedProgressBar *progressBar_ = static_cast<CLCDSkinnedProgressBar*>(myObject_);
            LCDUIASSERT(NULL != progressBar_);
            progressBar_->SetBackground(background, width, height);
            return S_OK;
        }
    }

    return E_FAIL;
}

/****f* LCD.SDK/SetSkinnedProgressBarFiller(HANDLE.handle,HBITMAP.filler,INT.width,INT.height)
* NAME
* HRESULT SetSkinnedProgressBarFiller(HANDLE handle, HBITMAP filler,
* INT width, INT height) -- Sets the filler bitmap of the progress
* bar, which needs to be the same size as the background.
* INPUTS
*   handle        - Handle to the progress bar.
*   filler        - Handle to the bitmap to display as the filler.
*   width, height - Size of the image (needs to be the same size as
*                   background).
* RETURN VALUE
*  S_OK if succeeded.
*  E_FAIL otherwise.
******
*/
HRESULT CEzLcd::SetSkinnedProgressBarFiller(HANDLE handle, HBITMAP filler, INT width, INT height)
{
    CLCDBase* myObject_ = (CLCDBase*)handle;

    if (NULL != myObject_)
    {
        LCDUIASSERT(LG_PROGRESS_BAR == myObject_->GetObjectType());
        // only allow this function for progress bars
        if (LG_PROGRESS_BAR == myObject_->GetObjectType())
        {
            CLCDSkinnedProgressBar *progressBar_ = static_cast<CLCDSkinnedProgressBar*>(myObject_);
            LCDUIASSERT(NULL != progressBar_);
            progressBar_->SetFiller(filler, width, height);
            return S_OK;
        }
    }

    return E_FAIL;
}

/****f* LCD.SDK/SetSkinnedProgressBarCursor(HANDLE.handle,HBITMAP.cursor,INT.width,INT.height)
* NAME
* HRESULT SetSkinnedProgressBarCursor(HANDLE handle, HBITMAP cursor,
* INT width, INT height) -- Sets the cursor bitmap of the progress
* bar. Height needs to be the same as the background.
* INPUTS
*   handle        - Handle to the progress bar.
*   filler        - Handle to the bitmap to display as the cursor.
*   width, height - Size of the image (height needs to be the same
*                   size as background).
* RETURN VALUE
*  S_OK if succeeded.
*  E_FAIL otherwise.
******
*/
HRESULT CEzLcd::SetSkinnedProgressBarCursor(HANDLE handle, HBITMAP cursor, INT width, INT height)
{
    CLCDBase* myObject_ = (CLCDBase*)handle;

    if (NULL != myObject_)
    {
        LCDUIASSERT(LG_PROGRESS_BAR == myObject_->GetObjectType());
        // only allow this function for progress bars
        if (LG_PROGRESS_BAR == myObject_->GetObjectType())
        {
            CLCDSkinnedProgressBar *progressBar_ = static_cast<CLCDSkinnedProgressBar*>(myObject_);
            LCDUIASSERT(NULL != progressBar_);
            progressBar_->SetCursor(cursor, width, height);
            return S_OK;
        }
    }

    return E_FAIL;
}

/****f* LCD.SDK/SetSkinnedProgressBarThreePieceCursor(HANDLE.handle,HBITMAP.left,INT.bmpLeftWidth,INT.bmpLeftHeight,HBITMAP.mid,INT.bmpMidWidth,INT.bmpMidHeight,HBITMAP.right,INT.bmpRightWidth,INT.bmpRightHeight )
* NAME
* HRESULT SetSkinnedProgressBarThreePieceCursor(HANDLE handle, HBITMAP
* left, INT bmpLeftWidth, INT bmpLeftHeight, HBITMAP mid, INT
* bmpMidWidth, INT bmpMidHeight, HBITMAP right, INT bmpRightWidth, INT
* bmpRightHeight) -- Sets the cursor of the progress bar if it is in 3
* pieces. This is a special case -- this is the case where the cursor
* needs to have left, middle, and right sections, such as curved
* edges.
* INPUTS
*   handle           - Handle to the progress bar.
*   left, mid, right - Handles to the bitmaps to display as the cursor.
*   widths, heights  - Size of the images.
* RETURN VALUE
*  S_OK if succeeded.
*  E_FAIL otherwise.
******
*/
HRESULT CEzLcd::SetSkinnedProgressBarThreePieceCursor(HANDLE handle, HBITMAP left, INT bmpLeftWidth, INT bmpLeftHeight,
                             HBITMAP mid, INT bmpMidWidth, INT bmpMidHeight,
                             HBITMAP right, INT bmpRightWidth, INT bmpRightHeight)
{
        CLCDBase* myObject_ = (CLCDBase*)handle;

    if (NULL != myObject_)
    {
        LCDUIASSERT(LG_PROGRESS_BAR == myObject_->GetObjectType());
        // only allow this function for progress bars
        if (LG_PROGRESS_BAR == myObject_->GetObjectType())
        {
            CLCDSkinnedProgressBar *progressBar_ = static_cast<CLCDSkinnedProgressBar*>(myObject_);
            LCDUIASSERT(NULL != progressBar_);
            progressBar_->SetThreePieceCursor(left, bmpLeftWidth, bmpLeftHeight,
                                              mid, bmpMidWidth, bmpMidHeight,
                                              right, bmpRightWidth, bmpRightHeight);
            return S_OK;
        }
    }

    return E_FAIL;
}

/****f* LCD.SDK/SetSkinnedProgressHighlight(HANDLE.handle,HBITMAP.highlight,INT.width,INT.height)
* NAME
* HRESULT SetSkinnedProgressHighlight(HANDLE handle, HBITMAP
* highlight, INT width, INT height) -- Sets the highlight bitmap of
* the progress bar. Optional.
* INPUTS
*   handle        - Handle to the progress bar.
*   highlight     - Handle to the bitmap to display as the highlight.
*   width, height - Size of the image.
* RETURN VALUE
*  S_OK if succeeded.
*  E_FAIL otherwise.
******
*/
HRESULT CEzLcd::SetSkinnedProgressHighlight(HANDLE handle, HBITMAP highlight, INT width, INT height)
{
    CLCDBase* myObject_ = (CLCDBase*)handle;

    if (NULL != myObject_)
    {
        LCDUIASSERT(LG_PROGRESS_BAR == myObject_->GetObjectType());
        // only allow this function for progress bars
        if (LG_PROGRESS_BAR == myObject_->GetObjectType())
        {
            CLCDSkinnedProgressBar *progressBar_ = static_cast<CLCDSkinnedProgressBar*>(myObject_);
            LCDUIASSERT(NULL != progressBar_);
            progressBar_->AddHighlight(highlight, width, height);
            return S_OK;
        }
    }

    return E_FAIL;
}

/****f* LCD.SDK/AddBitmap(INT.width,INT.height)
* NAME
*  HANDLE AddBitmap(INT width, INT height) -- Add a bitmap object to
*  the page being worked on.
* INPUTS
*  width - width of the invisible box around the bitmap.
*  height - height of the invisible box around the bitmap.
* RETURN VALUE
*  Handle for this object.
* NOTES
*  The width and height parameters define an invisible box that is
*  around the bitmap, starting at the upper left corner. If for
*  example a bitmap is 32x32 pixels and the width and height
*  parameters given are 10x10, only the 10 first pixels of the bitmap
*  will be displayed both in width and height.
* SEE ALSO
*  AddText(LGObjectType.type,LGTextSize.size,INT.alignment,INT.maxLengthPixels,INT.numberOfLines,LONG.fontWeight)
*  AddIcon(HICON.hIcon,INT.sizeX,INT.sizeY)
******
*/
HANDLE CEzLcd::AddBitmap(INT width, INT height)
{
    if (GetActivePage() == NULL)
    {
        return NULL;
    }

    return GetActivePage()->AddBitmap(width, height);
}

/****f* LCD.SDK/SetBitmap(HANDLE.handle,HBITMAP.bitmap)
* NAME
*  HRESULT SetBitmap(HANDLE handle, HBITMAP bitmap) -- Set the bitmap.
* INPUTS
*  handle - handle to the object.
*  bitmap - handle to the bitmap to be set. Needs to be 1bpp for
*           monochrome display.
*..zoomLevel - zooms the bitmap. Zoom level examples:
                                 1 no zoom
                                 0.5 bitmap reduced by 50%
                                 1.5 bitmap enlarged by 50%
* RETURN VALUE
*  S_OK if succeeded.
*  E_FAIL otherwise.
******
*/
HRESULT CEzLcd::SetBitmap(HANDLE handle, HBITMAP bitmap, float zoomLevel)
{
    if (NULL == bitmap)
        return E_FAIL;

    CLCDBase* myObject_ = (CLCDBase*)handle;

    if (NULL != myObject_)
    {
        CLCDBitmap* bitmap_ = static_cast<CLCDBitmap*>(myObject_);
        LCDUIASSERT(bitmap_);
        bitmap_->SetBitmap(bitmap);
        bitmap_->SetZoomLevel(zoomLevel);
        return S_OK;
    }

    return E_FAIL;
}


/****f* LCD.SDK/SetOrigin(HANDLE.handle,INT.XOrigin,INT.YOrigin)
* NAME
*  HRESULT SetOrigin(HANDLE handle, INT XOrigin, INT YOrigin) -- Set
*  the origin of an object. The origin corresponds to the furthest
*  pixel on the upper left corner of an object.
* INPUTS
*  handle  - handle to the object.
*  XOrigin - x axis part of the origin.
*  YOrigin - y axis part of the origin.
* RETURN VALUE
*  S_OK if succeeded.
*  E_FAIL otherwise.
******
*/
HRESULT CEzLcd::SetOrigin(HANDLE handle, INT XOrigin, INT YOrigin)
{
    CLCDBase* myObject_ = (CLCDBase*)handle;
    LCDUIASSERT(NULL != myObject_);
    LCDUIASSERT(NULL != myObject_);

    if (NULL != myObject_)
    {
        myObject_->SetOrigin(XOrigin, YOrigin);
        return S_OK;
    }

    return E_FAIL;
}

/****f* LCD.SDK/SetVisible(HANDLE.handle,BOOL.visible)
* NAME
*  HRESULT SetVisible(HANDLE handle, BOOL visible) -- Set
*  corresponding object to be visible or invisible on the page being
*  worked on.
* INPUTS
*  handle  - handle to the object.
*  visible - set to FALSE to make object invisible, TRUE to make it
*            visible (default).
* RETURN VALUE
*  S_OK if succeeded.
*  E_FAIL otherwise.
******
*/
HRESULT CEzLcd::SetVisible(HANDLE handle, BOOL visible)
{
    CLCDBase* myObject_ = (CLCDBase*)handle;
    LCDUIASSERT(NULL != myObject_);
    LCDUIASSERT(NULL != myObject_);

    if (NULL != myObject_)
    {
        myObject_->Show(visible);
        return S_OK;
    }

    return E_FAIL;
}

/****f* LCD.SDK/SetAsForeground(BOOL.setAsForeground)
* NAME
*  HRESULT SetAsForeground(BOOL setAsForeground) -- Become foreground
*  applet on LCD, or remove yourself as foreground applet.
* INPUTS
*  setAsForeground - Determines whether to be foreground or not.
*                    Possible values are:
*                      - TRUE implies foreground
*                      - FALSE implies no longer foreground
* RETURN VALUE
*  S_OK if succeeded.
*  E_FAIL otherwise.
******
*/
HRESULT CEzLcd::SetAsForeground(BOOL setAsForeground)
{
    if (NULL != m_pConnection->MonoOutput())
    {
        m_pConnection->MonoOutput()->SetAsForeground(setAsForeground);
    }

    if (NULL != m_pConnection->ColorOutput())
    {
        m_pConnection->ColorOutput()->SetAsForeground(setAsForeground);
    }

    return S_OK;
}

/****f* LCD.SDK/SetScreenPriority(DWORD priority)
* NAME
*  HRESULT SetScreenPriority(DWORD priority) -- Set screen priority.
* INPUTS
*  priority - priority of the screen. Possible values are:
*             - LGLCD_PRIORITY_IDLE_NO_SHOW
*             - LGLCD_PRIORITY_BACKGROUND
*             - LGLCD_PRIORITY_NORMAL
*             - LGLCD_PRIORITY_ALERT.
* NOTES
*  Default priority is LGLCD_PRIORITY_NORMAL. If setting to
*  LGLCD_PRIORITY_ALERT for too long, applet will be shut out by
*  applet manager.
* RETURN VALUE
*  S_OK if succeeded.
*  E_FAIL otherwise.
******
*/
HRESULT CEzLcd::SetScreenPriority(DWORD priority)
{
    if( m_SupportType == LG_MONOCHROME_MODE_ONLY || m_SupportType == LG_DUAL_MODE )
    {
        ModifyDisplay(LG_MONOCHROME);
        if (m_previousScreenPriorityBW != static_cast<INT>(priority))
        {
            CLCDOutput* output_ = GetCurrentOutput();
            if (NULL != output_)
            {
                output_->SetScreenPriority(priority);
                m_previousScreenPriorityBW = priority;
            }
        }
    }

    if( m_SupportType == LG_COLOR_MODE_ONLY || m_SupportType == LG_DUAL_MODE )
    {
        ModifyDisplay(LG_COLOR);
        if (m_previousScreenPriorityColor != static_cast<INT>(priority))
        {
            CLCDOutput* output_ = GetCurrentOutput();
            if (NULL != output_)
            {
                output_->SetScreenPriority(priority);
                m_previousScreenPriorityColor = priority;
            }
        }
    }

    return S_OK;
}

/****f* LCD.SDK/GetScreenPriority()
* NAME
*  DWORD GetScreenPriority() -- Get screen priority.
* RETURN VALUE
*  One of the following:
*    - LGLCD_PRIORITY_IDLE_NO_SHOW
*    - LGLCD_PRIORITY_BACKGROUND
*    - LGLCD_PRIORITY_NORMAL
*    - LGLCD_PRIORITY_ALERT
******
*/
DWORD CEzLcd::GetScreenPriority()
{
    CLCDOutput* output_ = GetCurrentOutput();

    if (NULL == output_)
    {
        return LGLCD_PRIORITY_NORMAL;
    }

    return output_->GetScreenPriority();
}

/****f* LCD.SDK/ButtonTriggered(INT.button)
* NAME
*  BOOL ButtonTriggered(INT button) -- Check if a button was
*  triggered.
* INPUTS
*  button - name of the button to be checked. Possible names are:
*           - monochrome display: LG_BUTTON_1, LG_BUTTON_2, LG_BUTTON_3,
*             LG_BUTTON_4.
*           - color display: LG_BUTTON_LEFT, LG_BUTTON_RIGHT, LG_BUTTON_UP,
*             LG_BUTTON_DOWN, LG_BUTTON_OK, LG_BUTTON_CANCEL,
*             LG_BUTTON_MENU.
* RETURN VALUE
*  TRUE if the specific button was triggered.
*  FALSE otherwise.
* SEE ALSO
*  ButtonReleased(INT.button)
*  ButtonIsPressed(INT.button)
******
*/
BOOL CEzLcd::ButtonTriggered(INT button)
{
    switch(button)
    {
    case LG_BUTTON_1:
        return ( !(m_PrevButtonStatus & LGLCDBUTTON_BUTTON0) && (m_CurrButtonStatus & LGLCDBUTTON_BUTTON0) );
    case LG_BUTTON_2:
        return ( !(m_PrevButtonStatus & LGLCDBUTTON_BUTTON1) && (m_CurrButtonStatus & LGLCDBUTTON_BUTTON1) );
    case LG_BUTTON_3:
        return ( !(m_PrevButtonStatus & LGLCDBUTTON_BUTTON2) && (m_CurrButtonStatus & LGLCDBUTTON_BUTTON2) );
    case LG_BUTTON_4:
        return ( !(m_PrevButtonStatus & LGLCDBUTTON_BUTTON3) && (m_CurrButtonStatus & LGLCDBUTTON_BUTTON3) );
    case LG_BUTTON_LEFT:
        return ( !(m_PrevButtonStatus & LGLCDBUTTON_LEFT) && (m_CurrButtonStatus & LGLCDBUTTON_LEFT) );
    case LG_BUTTON_RIGHT:
        return ( !(m_PrevButtonStatus & LGLCDBUTTON_RIGHT) && (m_CurrButtonStatus & LGLCDBUTTON_RIGHT) );
    case LG_BUTTON_UP:
        return ( !(m_PrevButtonStatus & LGLCDBUTTON_UP) && (m_CurrButtonStatus & LGLCDBUTTON_UP) );
    case LG_BUTTON_DOWN:
        return ( !(m_PrevButtonStatus & LGLCDBUTTON_DOWN) && (m_CurrButtonStatus & LGLCDBUTTON_DOWN) );
    case LG_BUTTON_OK:
        return ( !(m_PrevButtonStatus & LGLCDBUTTON_OK) && (m_CurrButtonStatus & LGLCDBUTTON_OK) );
    case LG_BUTTON_CANCEL:
        return ( !(m_PrevButtonStatus & LGLCDBUTTON_CANCEL) && (m_CurrButtonStatus & LGLCDBUTTON_CANCEL) );
    case LG_BUTTON_MENU:
        return ( !(m_PrevButtonStatus & LGLCDBUTTON_MENU) && (m_CurrButtonStatus & LGLCDBUTTON_MENU) );
    default: break;
    }

    return FALSE;
}

/****f* LCD.SDK/ButtonReleased(INT.button)
* NAME
*  BOOL ButtonReleased(INT button) -- Check if a button was released.
* INPUTS
*  button - name of the button to be checked. Possible names are:
*           - monochrome display: LG_BUTTON_1, LG_BUTTON_2, LG_BUTTON_3,
*             LG_BUTTON_4.
*           - color display: LG_BUTTON_LEFT, LG_BUTTON_RIGHT, LG_BUTTON_UP,
*             LG_BUTTON_DOWN, LG_BUTTON_OK, LG_BUTTON_CANCEL,
*             LG_BUTTON_MENU.
* RETURN VALUE
*  TRUE if the specific button was released.
*  FALSE otherwise.
* SEE ALSO
*  ButtonTriggered(INT.button)
*  ButtonIsPressed(INT.button)
******
*/
BOOL CEzLcd::ButtonReleased(INT button)
{
    switch(button)
    {
    case LG_BUTTON_1:
        return ( (m_PrevButtonStatus & LGLCDBUTTON_BUTTON0) && !(m_CurrButtonStatus & LGLCDBUTTON_BUTTON0) );
    case LG_BUTTON_2:
        return ( (m_PrevButtonStatus & LGLCDBUTTON_BUTTON1) && !(m_CurrButtonStatus & LGLCDBUTTON_BUTTON1) );
    case LG_BUTTON_3:
        return ( (m_PrevButtonStatus & LGLCDBUTTON_BUTTON2) && !(m_CurrButtonStatus & LGLCDBUTTON_BUTTON2) );
    case LG_BUTTON_4:
        return ( (m_PrevButtonStatus & LGLCDBUTTON_BUTTON3) && !(m_CurrButtonStatus & LGLCDBUTTON_BUTTON3) );
    case LG_BUTTON_LEFT:
        return ( (m_PrevButtonStatus & LGLCDBUTTON_LEFT) && !(m_CurrButtonStatus & LGLCDBUTTON_LEFT) );
    case LG_BUTTON_RIGHT:
        return ( (m_PrevButtonStatus & LGLCDBUTTON_RIGHT) && !(m_CurrButtonStatus & LGLCDBUTTON_RIGHT) );
    case LG_BUTTON_UP:
        return ( (m_PrevButtonStatus & LGLCDBUTTON_UP) && !(m_CurrButtonStatus & LGLCDBUTTON_UP) );
    case LG_BUTTON_DOWN:
        return ( (m_PrevButtonStatus & LGLCDBUTTON_DOWN) && !(m_CurrButtonStatus & LGLCDBUTTON_DOWN) );
    case LG_BUTTON_OK:
        return ( (m_PrevButtonStatus & LGLCDBUTTON_OK) && !(m_CurrButtonStatus & LGLCDBUTTON_OK) );
    case LG_BUTTON_CANCEL:
        return ( (m_PrevButtonStatus & LGLCDBUTTON_CANCEL) && !(m_CurrButtonStatus & LGLCDBUTTON_CANCEL) );
    case LG_BUTTON_MENU:
        return ( (m_PrevButtonStatus & LGLCDBUTTON_MENU) && !(m_CurrButtonStatus & LGLCDBUTTON_MENU) );
    default: break;
    }

    return FALSE;
}

/****f* LCD.SDK/ButtonIsPressed(INT.button)
* NAME
*  BOOL ButtonIsPressed(INT button) -- Check if a button is being pressed.
* INPUTS
*  button - name of the button to be checked. Possible names are:
*           - monochrome display: LG_BUTTON_1, LG_BUTTON_2, LG_BUTTON_3,
*             LG_BUTTON_4.
*           - color display: LG_BUTTON_LEFT, LG_BUTTON_RIGHT, LG_BUTTON_UP,
*             LG_BUTTON_DOWN, LG_BUTTON_OK, LG_BUTTON_CANCEL,
*             LG_BUTTON_MENU.
* RETURN VALUE
*  TRUE if the specific button is being pressed.
*  FALSE otherwise.
* SEE ALSO
*  ButtonTriggered(INT.button)
*  ButtonReleased(INT.button)
******
*/
BOOL CEzLcd::ButtonIsPressed(INT button)
{
    switch(button)
    {
    case LG_BUTTON_1:
        return ( m_CurrButtonStatus & LGLCDBUTTON_BUTTON0 );
    case LG_BUTTON_2:
        return ( m_CurrButtonStatus & LGLCDBUTTON_BUTTON1 );
    case LG_BUTTON_3:
        return ( m_CurrButtonStatus & LGLCDBUTTON_BUTTON2 );
    case LG_BUTTON_4:
        return ( m_CurrButtonStatus & LGLCDBUTTON_BUTTON3 );
    case LG_BUTTON_LEFT:
        return ( m_CurrButtonStatus & LGLCDBUTTON_LEFT );
    case LG_BUTTON_RIGHT:
        return ( m_CurrButtonStatus & LGLCDBUTTON_RIGHT );
    case LG_BUTTON_UP:
        return ( m_CurrButtonStatus & LGLCDBUTTON_UP );
    case LG_BUTTON_DOWN:
        return ( m_CurrButtonStatus & LGLCDBUTTON_DOWN );
    case LG_BUTTON_OK:
        return ( m_CurrButtonStatus & LGLCDBUTTON_OK );
    case LG_BUTTON_CANCEL:
        return ( m_CurrButtonStatus & LGLCDBUTTON_CANCEL );
    case LG_BUTTON_MENU:
        return ( m_CurrButtonStatus & LGLCDBUTTON_MENU );
    default: break;
    }

    return FALSE;
}

/****f* LCD.SDK/AnyDeviceOfThisFamilyPresent(DWORD.deviceFamily)
* NAME
*  BOOL AnyDeviceOfThisFamilyPresent(DWORD deviceFamily)
* NOTES
*  This function is no longer supported.
******
*/
BOOL CEzLcd::AnyDeviceOfThisFamilyPresent(DWORD deviceFamily)
{
    // This function is no longer supported
    UNREFERENCED_PARAMETER(deviceFamily);
    return TRUE;
}

/****f* LCD.SDK/SetDeviceFamilyToUse(DWORD.deviceFamily)
* NAME
*  HRESULT SetDeviceFamilyToUse(DWORD deviceFamily)
* NOTES
*  This function is no longer supported.
******
*/
HRESULT CEzLcd::SetDeviceFamilyToUse(DWORD deviceFamily)
{
    // This function is no longer supported
    UNREFERENCED_PARAMETER(deviceFamily);
    return S_OK;
}

/****f* LCD.SDK/SetPreferredDisplayFamily(DWORD.deviceFamily)
* NAME
*  HRESULT SetPreferredDisplayFamily(DWORD deviceFamily)
* NOTES
*  This function is no longer supported.
******
*/
HRESULT CEzLcd::SetPreferredDisplayFamily(DWORD deviceFamily)
{
    // This function is no longer supported
     UNREFERENCED_PARAMETER(deviceFamily);
   return S_OK;
}

/****f* LCD.SDK/Update()
* NAME
*  VOID Update() -- Update LCD display.
* FUNCTION
*  Updates the display. Must be called every loop.
******
*/
VOID CEzLcd::Update()
{
    // Only do stuff if initialization was successful. Otherwise
    // IsConnected will simply return false.
    if (m_initSucceeded)
    {
        m_pConnection->Update();

        m_PrevButtonStatus = m_CurrButtonStatus;
        EnterCriticalSection(&m_ButtonCS);
        m_CurrButtonStatus = m_ButtonCache;
        LeaveCriticalSection(&m_ButtonCS);
    }
}

CLCDOutput* CEzLcd::GetCurrentOutput()
{
    return m_pCurrentOutput;
}

LCD_PAGE_LIST& CEzLcd::GetPageList()
{
    return (m_pCurrentOutput == m_pConnection->MonoOutput()) ? m_LCDPageListMono : m_LCDPageListColor;
}

CEzLcdPage* CEzLcd::GetActivePage()
{
    return (m_pCurrentOutput == m_pConnection->MonoOutput()) ? m_activePageMono : m_activePageColor;
}

VOID CEzLcd::SetActivePage(CEzLcdPage* pPage)
{
    if (m_pConnection->MonoOutput() == m_pCurrentOutput)
    {
        m_activePageMono = pPage;
    }
    else
    {
        m_activePageColor = pPage;
    }
}

INT CEzLcd::GetCurrentPageNumberShown()
{
    return (m_pCurrentOutput == m_pConnection->MonoOutput()) ? m_currentPageNumberShownMono : m_currentPageNumberShownColor;
}

VOID CEzLcd::SetCurrentPageNumberShown(INT nPage)
{
    if (m_pConnection->MonoOutput() == m_pCurrentOutput)
    {
        m_currentPageNumberShownMono = nPage;
    }
    else
    {
        m_currentPageNumberShownColor = nPage;
    }
}

DWORD WINAPI CEzLcd::OnButtonCB(IN INT connection, IN DWORD dwButtons, IN const PVOID pContext)
{
    UNREFERENCED_PARAMETER(connection);
    CEzLcd* pezlcd = (CEzLcd*)pContext;
    pezlcd->OnButtons(dwButtons);
    return 0;
}

VOID CEzLcd::OnButtons(DWORD buttons)
{
    EnterCriticalSection(&m_ButtonCS);
    m_ButtonCache = buttons;
    LeaveCriticalSection(&m_ButtonCS);
}
CLCDConnection* CEzLcd::AllocLCDConnection( void )
{
    return new CLCDConnection;
}

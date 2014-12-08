//************************************************************************
//  The Logitech LCD SDK, including all acompanying documentation,
//  is protected by intellectual property laws.  All use of the Logitech
//  LCD SDK is subject to the License Agreement found in the
//  "Logitech LCD SDK License Agreement" file and in the Reference Manual.  
//  All rights not expressly granted by Logitech are reserved.
//************************************************************************

// Copyright 2010 Logitech Inc.

/****h* EZ.LCD.SDK.Wrapper/EZ_LCD_Page.cpp
 * NAME
 *   EZ_LCD_Page.cpp
 * COPYRIGHT
 *   The Logitech EZ LCD SDK Wrapper, including all accompanying
 *   documentation, is protected by intellectual property laws. All rights
 *   not expressly granted by Logitech are reserved.
 * PURPOSE
 *   Part of the SDK package.
 * AUTHOR
 *   Christophe Juncker (cj@wingmanteam.com)
 *   Vahid Afshar (Vahid_Afshar@logitech.com)
 * CREATION DATE
 *   06/13/200
 * MODIFICATION HISTORY
 *   03/01/2006 - Added the concept of pages.
 *
 *******
 */

#include "LCDUI.h"
#include "EZ_LCD.h"
#include "EZ_LCD_Page.h"

CEzLcdPage::CEzLcdPage()
{
    m_container = NULL;
}

CEzLcdPage::CEzLcdPage(CEzLcd * container)
{
    m_container = container;
    Init();
}

CEzLcdPage::~CEzLcdPage()
{
    LCD_OBJECT_LIST::iterator it_ = m_Objects.begin();
    while(it_ != m_Objects.end())
    {
        CLCDBase *pObject_ = *it_;
        LCDUIASSERT(NULL != pObject_);
        delete pObject_;

        ++it_;
    }
}

HANDLE CEzLcdPage::AddText(LGObjectType type, LGTextSize size, INT alignment, INT maxLengthPixels, INT numberOfLines)
{
    LCDUIASSERT(LG_SCROLLING_TEXT == type || LG_STATIC_TEXT == type || LG_RIGHTFOCUS_TEXT == type);
    CLCDText* pStaticText_;
    CLCDText* pRightFocusText_;
    CLCDStreamingText* pStreamingText_;

    INT iBoxHeight_ = LG_MEDIUM_FONT_TEXT_BOX_HEIGHT;
    INT iFontSize_ = LG_MEDIUM_FONT_SIZE;
    INT iLocalOriginY_ = LG_MEDIUM_FONT_LOGICAL_ORIGIN_Y;

    switch (type)
    {
    case LG_SCROLLING_TEXT:
        pStreamingText_ = new CLCDStreamingText();
        LCDUIASSERT(NULL != pStreamingText_);
        pStreamingText_->Initialize();
        pStreamingText_->SetOrigin(0, 0);
        pStreamingText_->SetFontFaceName(LG_FONT);
        pStreamingText_->SetAlignment(alignment);
        pStreamingText_->SetText(_T(" "));
        pStreamingText_->SetGapText(LG_SCROLLING_GAP_TEXT);
        pStreamingText_->SetStartDelay(LG_SCROLLING_DELAY_MS);

        if (LG_SMALL == size || LG_TINY == size)
        {
            iBoxHeight_ = LG_SMALL_FONT_TEXT_BOX_HEIGHT;
            iFontSize_ = LG_SMALL_FONT_SIZE;
            iLocalOriginY_ = LG_SMALL_FONT_LOGICAL_ORIGIN_Y;
            pStreamingText_->SetSpeed(LG_SCROLLING_SPEED_SMALL_MONOCHROME);
            pStreamingText_->SetScrollingStep(LG_SCROLLING_STEP_SMALL_MONOCHROME);
        }
        else if (LG_MEDIUM == size)
        {
            iBoxHeight_ = LG_MEDIUM_FONT_TEXT_BOX_HEIGHT;
            iFontSize_ = LG_MEDIUM_FONT_SIZE;
            iLocalOriginY_ = LG_MEDIUM_FONT_LOGICAL_ORIGIN_Y;
            pStreamingText_->SetSpeed(LG_SCROLLING_SPEED_MEDIUM_MONOCHROME);
            pStreamingText_->SetScrollingStep(LG_SCROLLING_STEP_MEDIUM_MONOCHROME);
        }
        else if (LG_BIG == size)
        {
            pStreamingText_->SetFontWeight(FW_BOLD);
            iBoxHeight_ = LG_BIG_FONT_TEXT_BOX_HEIGHT;
            iFontSize_ = LG_BIG_FONT_SIZE;
            iLocalOriginY_ = LG_BIG_FONT_LOGICAL_ORIGIN_Y;
            pStreamingText_->SetSpeed(LG_SCROLLING_SPEED_BIG_MONOCHROME);
            pStreamingText_->SetScrollingStep(LG_SCROLLING_STEP_BIG_MONOCHROME);
        }

        pStreamingText_->SetSize(maxLengthPixels, iBoxHeight_);
        pStreamingText_->SetFontPointSize(iFontSize_);
        pStreamingText_->SetLogicalOrigin(0, iLocalOriginY_);
        pStreamingText_->SetObjectType(LG_SCROLLING_TEXT);

        AddObject(pStreamingText_);

        return pStreamingText_;
        break;
    case LG_STATIC_TEXT:
        pStaticText_ = new CLCDText();
        LCDUIASSERT(NULL != pStaticText_);
        pStaticText_->Initialize();
        pStaticText_->SetOrigin(0, 0);
        pStaticText_->SetFontFaceName(LG_FONT);
        pStaticText_->SetAlignment(alignment);
        pStaticText_->SetBackgroundMode(OPAQUE);
        pStaticText_->SetText(_T(" "));

        if (LG_SMALL == size || LG_TINY == size)
        {
            iBoxHeight_ = numberOfLines * LG_SMALL_FONT_TEXT_BOX_HEIGHT;
            iFontSize_ = LG_SMALL_FONT_SIZE;
            iLocalOriginY_ = LG_SMALL_FONT_LOGICAL_ORIGIN_Y;
        }
        else if (LG_MEDIUM == size)
        {
            iBoxHeight_ = numberOfLines * LG_MEDIUM_FONT_TEXT_BOX_HEIGHT;
            iFontSize_ = LG_MEDIUM_FONT_SIZE;
            iLocalOriginY_ = LG_MEDIUM_FONT_LOGICAL_ORIGIN_Y;
        }
        else if (LG_BIG == size)
        {
            pStaticText_->SetFontWeight(FW_BOLD);
            iBoxHeight_ = numberOfLines * LG_BIG_FONT_TEXT_BOX_HEIGHT;
            iFontSize_ = LG_BIG_FONT_SIZE;
            iLocalOriginY_ = LG_BIG_FONT_LOGICAL_ORIGIN_Y;
        }

        pStaticText_->SetSize(maxLengthPixels, iBoxHeight_);
        pStaticText_->SetFontPointSize(iFontSize_);
        pStaticText_->SetLogicalOrigin(0, iLocalOriginY_);
        pStaticText_->SetObjectType(LG_STATIC_TEXT);

        if (1 < numberOfLines)
            pStaticText_->SetWordWrap(TRUE);

        AddObject(pStaticText_);

        return pStaticText_;
        break;
    case LG_RIGHTFOCUS_TEXT:
        pRightFocusText_ = new CLCDText();
        LCDUIASSERT(NULL != pRightFocusText_);
        pRightFocusText_->Initialize();
        pRightFocusText_->SetOrigin(0, 0);
        pRightFocusText_->SetFontFaceName(LG_ARIAL_FONT);
        pRightFocusText_->SetAlignment(alignment);
        pRightFocusText_->SetBackgroundMode(TRANSPARENT);
        pRightFocusText_->SetText(_T(" "));

        if (LG_SMALL == size || LG_TINY == size)
        {
            iBoxHeight_ = numberOfLines * LG_SMALL_FONT_TEXT_BOX_HEIGHT;
            iFontSize_ = LG_SMALL_FONT_SIZE;
            iLocalOriginY_ = LG_SMALL_FONT_LOGICAL_ORIGIN_Y;
        }
        else if (LG_MEDIUM == size)
        {
            iBoxHeight_ = numberOfLines * LG_MEDIUM_FONT_TEXT_BOX_HEIGHT;
            iFontSize_ = LG_MEDIUM_FONT_SIZE;
            iLocalOriginY_ = LG_MEDIUM_FONT_LOGICAL_ORIGIN_Y;
        }
        else if (LG_BIG == size)
        {
            pRightFocusText_->SetFontWeight(FW_BOLD);
            iBoxHeight_ = numberOfLines * LG_BIG_FONT_TEXT_BOX_HEIGHT;
            iFontSize_ = LG_BIG_FONT_SIZE;
            iLocalOriginY_ = LG_BIG_FONT_LOGICAL_ORIGIN_Y;
        }

        pRightFocusText_->SetSize(maxLengthPixels, iBoxHeight_);
        pRightFocusText_->SetFontPointSize(iFontSize_);
        pRightFocusText_->SetLogicalOrigin(0, iLocalOriginY_);
        pRightFocusText_->SetObjectType(LG_RIGHTFOCUS_TEXT);

        if (1 < numberOfLines)
            pRightFocusText_->SetWordWrap(TRUE);

        AddObject(pRightFocusText_);

        return pRightFocusText_;
    default:
        LCDUITRACE(_T("ERROR: trying to add text object with undefined type\n"));
    }

    return NULL;
}

HRESULT CEzLcdPage::SetText(HANDLE handle, LPCTSTR text, BOOL resetScrollingTextPosition)
{
    CLCDBase* myObject = GetObject(handle);

    if (NULL != myObject)
    {
        if (!((LG_STATIC_TEXT == myObject->GetObjectType() || LG_SCROLLING_TEXT == myObject->GetObjectType() || LG_RIGHTFOCUS_TEXT == myObject->GetObjectType() )))
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

HANDLE CEzLcdPage::AddColorText(LGObjectType type, LGTextSize size, INT alignment, INT maxLengthPixels, INT numberOfLines, LONG fontWeight)
{
    LCDUIASSERT(LG_SCROLLING_TEXT == type || LG_STATIC_TEXT == type || LG_RIGHTFOCUS_TEXT == type);
    CLCDText* pStaticText_;
    CLCDStreamingText* pStreamingText_;

    CLCDText* pRightFocusText_;

    INT iBoxHeight_ = LG_MEDIUM_FONT_TEXT_BOX_HEIGHT_COLOR;
    INT iFontSize_ = LG_MEDIUM_FONT_SIZE_COLOR;
    INT iLocalOriginY_ = LG_MEDIUM_FONT_LOGICAL_ORIGIN_Y_COLOR;

    switch (type)
    {
    case LG_SCROLLING_TEXT:
        pStreamingText_ = new CLCDStreamingText();
        LCDUIASSERT(NULL != pStreamingText_);
        pStreamingText_->Initialize();
        pStreamingText_->SetOrigin(0, 0);
        pStreamingText_->SetFontFaceName(LG_ARIAL_FONT);
        pStreamingText_->SetAlignment(alignment);
        pStreamingText_->SetText(_T(" "));
        pStreamingText_->SetGapText(LG_SCROLLING_GAP_TEXT);
        pStreamingText_->SetScrollingStep(LG_SCROLLING_STEP_COLOR);
        pStreamingText_->SetStartDelay(LG_SCROLLING_DELAY_MS);
        pStreamingText_->SetFontWeight(fontWeight);

        if (LG_TINY == size)
        {
            iBoxHeight_ = LG_TINY_FONT_TEXT_BOX_HEIGHT_COLOR;
            iFontSize_ = LG_TINY_FONT_SIZE_COLOR;
            iLocalOriginY_ = LG_TINY_FONT_LOGICAL_ORIGIN_Y_COLOR;
            pStreamingText_->SetSpeed(LG_SCROLLING_SPEED_TINY_COLOR);
        }
        else if (LG_SMALL == size)
        {
            iBoxHeight_ = LG_SMALL_FONT_TEXT_BOX_HEIGHT_COLOR;
            iFontSize_ = LG_SMALL_FONT_SIZE_COLOR;
            iLocalOriginY_ = LG_SMALL_FONT_LOGICAL_ORIGIN_Y_COLOR;
            pStreamingText_->SetSpeed(LG_SCROLLING_SPEED_SMALL_COLOR);
        }
        else if (LG_MEDIUM == size)
        {
            iBoxHeight_ = LG_MEDIUM_FONT_TEXT_BOX_HEIGHT_COLOR;
            iFontSize_ = LG_MEDIUM_FONT_SIZE_COLOR;
            iLocalOriginY_ = LG_MEDIUM_FONT_LOGICAL_ORIGIN_Y_COLOR;
            pStreamingText_->SetSpeed(LG_SCROLLING_SPEED_MEDIUM_COLOR);
        }
        else if (LG_BIG == size)
        {
            iBoxHeight_ = LG_BIG_FONT_TEXT_BOX_HEIGHT_COLOR;
            iFontSize_ = LG_BIG_FONT_SIZE_COLOR;
            iLocalOriginY_ = LG_BIG_FONT_LOGICAL_ORIGIN_Y_COLOR;
            pStreamingText_->SetSpeed(LG_SCROLLING_SPEED_BIG_COLOR);
        }

        pStreamingText_->SetSize(maxLengthPixels, iBoxHeight_);
        pStreamingText_->SetFontPointSize(iFontSize_);
        pStreamingText_->SetLogicalOrigin(0, iLocalOriginY_);
        pStreamingText_->SetObjectType(LG_SCROLLING_TEXT);

        AddObject(pStreamingText_);

        return pStreamingText_;
        break;
    case LG_STATIC_TEXT:
        pStaticText_ = new CLCDText();
        LCDUIASSERT(NULL != pStaticText_);
        pStaticText_->Initialize();
        pStaticText_->SetOrigin(0, 0);
        pStaticText_->SetFontFaceName(LG_ARIAL_FONT);
        pStaticText_->SetAlignment(alignment);
        pStaticText_->SetBackgroundMode(TRANSPARENT);
        pStaticText_->SetText(_T(" "));
        pStaticText_->SetFontWeight(fontWeight);

        if (LG_TINY == size)
        {
            iBoxHeight_ = numberOfLines * LG_TINY_FONT_TEXT_BOX_HEIGHT_COLOR;
            iFontSize_ = LG_TINY_FONT_SIZE_COLOR;
            iLocalOriginY_ = LG_TINY_FONT_LOGICAL_ORIGIN_Y_COLOR;
        }
        else if (LG_SMALL == size)
        {
            iBoxHeight_ = numberOfLines * LG_SMALL_FONT_TEXT_BOX_HEIGHT_COLOR;
            iFontSize_ = LG_SMALL_FONT_SIZE_COLOR;
            iLocalOriginY_ = LG_SMALL_FONT_LOGICAL_ORIGIN_Y_COLOR;
        }
        else if (LG_MEDIUM == size)
        {
            iBoxHeight_ = numberOfLines * LG_MEDIUM_FONT_TEXT_BOX_HEIGHT_COLOR;
            iFontSize_ = LG_MEDIUM_FONT_SIZE_COLOR;
            iLocalOriginY_ = LG_MEDIUM_FONT_LOGICAL_ORIGIN_Y_COLOR;
        }
        else if (LG_BIG == size)
        {
            iBoxHeight_ = numberOfLines * LG_BIG_FONT_TEXT_BOX_HEIGHT_COLOR;
            iFontSize_ = LG_BIG_FONT_SIZE_COLOR;
            iLocalOriginY_ = LG_BIG_FONT_LOGICAL_ORIGIN_Y_COLOR;
        }

        pStaticText_->SetSize(maxLengthPixels, iBoxHeight_);
        pStaticText_->SetFontPointSize(iFontSize_);
        pStaticText_->SetLogicalOrigin(0, iLocalOriginY_);
        pStaticText_->SetObjectType(LG_STATIC_TEXT);

        if (1 < numberOfLines)
            pStaticText_->SetWordWrap(TRUE);

        AddObject(pStaticText_);

        return pStaticText_;
        break;
    case LG_RIGHTFOCUS_TEXT:
        pRightFocusText_ = new CLCDText();
        LCDUIASSERT(NULL != pRightFocusText_);
        pRightFocusText_->Initialize();
        pRightFocusText_->SetOrigin(0, 0);
        pRightFocusText_->SetFontFaceName(LG_ARIAL_FONT);
        pRightFocusText_->SetAlignment(alignment);
        pRightFocusText_->SetBackgroundMode(TRANSPARENT);
        pRightFocusText_->SetText(_T(" "));
        pRightFocusText_->SetFontWeight(fontWeight);

        if (LG_TINY == size)
        {
            iBoxHeight_ = numberOfLines * LG_TINY_FONT_TEXT_BOX_HEIGHT_COLOR;
            iFontSize_ = LG_TINY_FONT_SIZE_COLOR;
            iLocalOriginY_ = LG_TINY_FONT_LOGICAL_ORIGIN_Y_COLOR;
        }
        else if (LG_SMALL == size)
        {
            iBoxHeight_ = numberOfLines * LG_SMALL_FONT_TEXT_BOX_HEIGHT_COLOR;
            iFontSize_ = LG_SMALL_FONT_SIZE_COLOR;
            iLocalOriginY_ = LG_SMALL_FONT_LOGICAL_ORIGIN_Y_COLOR;
        }
        else if (LG_MEDIUM == size)
        {
            iBoxHeight_ = numberOfLines * LG_MEDIUM_FONT_TEXT_BOX_HEIGHT_COLOR;
            iFontSize_ = LG_MEDIUM_FONT_SIZE_COLOR;
            iLocalOriginY_ = LG_MEDIUM_FONT_LOGICAL_ORIGIN_Y_COLOR;
        }
        else if (LG_BIG == size)
        {
            iBoxHeight_ = numberOfLines * LG_BIG_FONT_TEXT_BOX_HEIGHT_COLOR;
            iFontSize_ = LG_BIG_FONT_SIZE_COLOR;
            iLocalOriginY_ = LG_BIG_FONT_LOGICAL_ORIGIN_Y_COLOR;
        }

        pRightFocusText_->SetSize(maxLengthPixels, iBoxHeight_);
        pRightFocusText_->SetFontPointSize(iFontSize_);
        pRightFocusText_->SetLogicalOrigin(0, iLocalOriginY_);
        pRightFocusText_->SetObjectType(LG_RIGHTFOCUS_TEXT);

        if (1 < numberOfLines)
            pRightFocusText_->SetWordWrap(TRUE);

        AddObject(pRightFocusText_);

        return pRightFocusText_;
    default:
        LCDUITRACE(_T("ERROR: trying to add text object with undefined type\n"));
    }

    return NULL;
}

HRESULT CEzLcdPage::SetTextBackground(HANDLE handle, INT backMode, COLORREF color)
{
    CLCDBase* myObject_ = GetObject(handle);

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

HRESULT CEzLcdPage::SetTextFontColor(HANDLE handle, COLORREF color)
{
    CLCDBase* myObject_ = GetObject(handle);

    if (NULL != myObject_)
    {
        CLCDText* pText_ = static_cast<CLCDText*>(myObject_);
        LCDUIASSERT(NULL != pText_);
        pText_->SetForegroundColor(color);
        return S_OK;
    }

    return E_FAIL;
}

HANDLE CEzLcdPage::AddIcon(HICON hIcon, INT width, INT height)
{
    CLCDIcon* hIcon_ = new CLCDIcon();
    LCDUIASSERT(NULL != hIcon_);
    hIcon_->Initialize();
    hIcon_->SetOrigin(0, 0);
    hIcon_->SetSize(width, height);
    hIcon_->SetIcon(hIcon, width, height);
    hIcon_->SetObjectType(LG_ICON);

    AddObject(hIcon_);

    return hIcon_;
}

HANDLE CEzLcdPage::AddProgressBar(LGProgressBarType type)
{
    LCDUIASSERT(LG_FILLED == type || LG_CURSOR == type || LG_DOT_CURSOR == type);
    CLCDProgressBar *pProgressBar_ = new CLCDProgressBar();
    LCDUIASSERT(NULL != pProgressBar_);
    pProgressBar_->Initialize();
    pProgressBar_->SetOrigin(0, 0);
    pProgressBar_->SetSize(160, LG_PROGRESS_BAR_INITIAL_HEIGHT);
    pProgressBar_->SetRange(LG_PROGRESS_BAR_RANGE_MIN, LG_PROGRESS_BAR_RANGE_MAX );
    pProgressBar_->SetPos(static_cast<FLOAT>(LG_PROGRESS_BAR_RANGE_MIN));
    pProgressBar_->SetObjectType(LG_PROGRESS_BAR);

// Map the progress style into what the UI classes understand
    CLCDProgressBar::ePROGRESS_STYLE eStyle = CLCDProgressBar::STYLE_FILLED;
    switch (type)
    {
    case LG_FILLED:
        eStyle = CLCDProgressBar::STYLE_FILLED;
        break;
    case LG_CURSOR:
        eStyle = CLCDProgressBar::STYLE_CURSOR;
        break;
    case LG_DOT_CURSOR:
        eStyle = CLCDProgressBar::STYLE_DASHED_CURSOR;
        break;
    }

    pProgressBar_->SetProgressStyle(eStyle);

    AddObject(pProgressBar_);

    return pProgressBar_;
}

HRESULT CEzLcdPage::SetProgressBarPosition(HANDLE handle, FLOAT percentage)
{
    CLCDBase* myObject_ = GetObject(handle);

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

HRESULT CEzLcdPage::SetProgressBarSize(HANDLE handle, INT width, INT height)
{
    CLCDBase* myObject_ = GetObject(handle);
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

HANDLE CEzLcdPage::AddColorProgressBar(LGProgressBarType type)
{
    LCDUIASSERT(LG_FILLED == type || LG_CURSOR == type || LG_DOT_CURSOR == type);
    CLCDColorProgressBar *pProgressBar_ = new CLCDColorProgressBar();
    LCDUIASSERT(NULL != pProgressBar_);
    pProgressBar_->Initialize();
    pProgressBar_->SetOrigin(0, 0);
    pProgressBar_->SetSize(LG_PROGRESS_BAR_INITIAL_WIDTH_COLOR, LG_PROGRESS_BAR_INITIAL_HEIGHT_COLOR);
    pProgressBar_->SetRange(LG_PROGRESS_BAR_RANGE_MIN, LG_PROGRESS_BAR_RANGE_MAX );
    pProgressBar_->SetPos(static_cast<FLOAT>(LG_PROGRESS_BAR_RANGE_MIN));
    pProgressBar_->SetObjectType(LG_PROGRESS_BAR);
    pProgressBar_->EnableBorder(TRUE);

// Map the progress style into what the UI classes understand
    CLCDProgressBar::ePROGRESS_STYLE eStyle = CLCDProgressBar::STYLE_FILLED;
    switch (type)
    {
    case LG_FILLED:
        eStyle = CLCDProgressBar::STYLE_FILLED;
        break;
    //Dot cursor does not look good on color, so only using the regular cursor
    case LG_CURSOR:
    case LG_DOT_CURSOR:
        eStyle = CLCDProgressBar::STYLE_CURSOR;
        break;
    }

    pProgressBar_->SetProgressStyle(eStyle);

    AddObject(pProgressBar_);

    return pProgressBar_;
}

HRESULT CEzLcdPage::SetProgressBarColors(HANDLE handle, COLORREF cursorcolor, COLORREF bordercolor)
{
    CLCDBase* myObject_ = GetObject(handle);

    if (NULL != myObject_)
    {
        LCDUIASSERT(LG_PROGRESS_BAR == myObject_->GetObjectType());
        // only allow this function for progress bars
        if (LG_PROGRESS_BAR == myObject_->GetObjectType())
        {
            CLCDColorProgressBar *progressBar_ = static_cast<CLCDColorProgressBar*>(myObject_);
            LCDUIASSERT(NULL != progressBar_);
            progressBar_->SetCursorColor(cursorcolor);
            progressBar_->SetBorderColor(bordercolor);
            return S_OK;
        }
    }

    return E_FAIL;
}

HRESULT CEzLcdPage::SetProgressBarBackgroundColor(HANDLE handle, COLORREF color)
{
    CLCDBase* myObject_ = GetObject(handle);

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

HANDLE CEzLcdPage::AddSkinnedProgressBar(LGProgressBarType type)
{
    LCDUIASSERT(LG_FILLED == type || LG_CURSOR == type || LG_DOT_CURSOR == type);
    CLCDSkinnedProgressBar *pProgressBar_ = new CLCDSkinnedProgressBar();
    LCDUIASSERT(NULL != pProgressBar_);
    pProgressBar_->Initialize();
    pProgressBar_->SetOrigin(0, 0);
    pProgressBar_->SetSize(160, LG_PROGRESS_BAR_INITIAL_HEIGHT);
    pProgressBar_->SetRange(LG_PROGRESS_BAR_RANGE_MIN, LG_PROGRESS_BAR_RANGE_MAX );
    pProgressBar_->SetPos(static_cast<FLOAT>(LG_PROGRESS_BAR_RANGE_MIN));
    pProgressBar_->SetObjectType(LG_PROGRESS_BAR);

// Map the progress style into what the UI classes understand
    CLCDProgressBar::ePROGRESS_STYLE eStyle = CLCDProgressBar::STYLE_FILLED;
    switch (type)
    {
    case LG_FILLED:
        eStyle = CLCDProgressBar::STYLE_FILLED;
        break;
    case LG_CURSOR:
        eStyle = CLCDProgressBar::STYLE_CURSOR;
        break;
    case LG_DOT_CURSOR:
        eStyle = CLCDProgressBar::STYLE_DASHED_CURSOR;
        break;
    }

    pProgressBar_->SetProgressStyle(eStyle);

    AddObject(pProgressBar_);

    return pProgressBar_;
}

/****f* LCD.SDK/AddAnimatedBitmap(HBITMAP bitmap, INT subpicWidth, INT height)
* NAME
* HANDLE AddAnimatedBitmap(HBITMAP bitmap, INT subpicWidth, INT height)
* -- Creates an Animated Bitmap. The bitmap must be composed of framestrips with constant dimensions.
* The framestrips must be aligned in the x axis
* INPUTS
*  bitmap -.the HBITMAP of the bitmap
*  subpicWidth - width of 1 framestrip
*..height - height of the bitmap
* RETURN VALUE
*  HANDLE of the Animated Bitmap if succeeded.
*  NULL otherwise.
******
*/
HANDLE CEzLcdPage::AddAnimatedBitmap(HBITMAP bitmap, INT subpicWidth, INT height)
{
    CLCDAnimatedBitmap *animBit_ = new CLCDAnimatedBitmap();
    LCDUIASSERT(NULL != animBit_);
    animBit_->Initialize();
    animBit_->SetSize(subpicWidth, height);
    animBit_->SetBitmap(bitmap);
    animBit_->SetSubpicWidth(subpicWidth);

    AddObject(animBit_);

    return animBit_;
}

/****f* LCD.SDK/SetAnimatedBitmap(HANDLE handle,INT msPerPic)
* NAME
* HRESULT SetAnimatedBitmap(HANDLE handle,INT msPerPic)
* -- Set options to the animated bitmap
* INPUTS
*  handle -.the HANDLE of the animated bitmap
*  msPerPic - frame rate
* RETURN VALUE
*  S_OK if succeeded.
*  E_FAIL otherwise.
******
*/
HRESULT CEzLcdPage::SetAnimatedBitmap(HANDLE handle,INT msPerPic)
{
    CLCDBase* myObject_ = GetObject(handle);

    if (NULL != myObject_)
    {
        CLCDAnimatedBitmap* animBit_ = static_cast<CLCDAnimatedBitmap*>(myObject_);
        LCDUIASSERT(animBit_);
        animBit_->SetAnimationRate(msPerPic);
        return S_OK;
    }
    return E_FAIL;
}

HANDLE CEzLcdPage::AddBitmap(INT width, INT height)
{
    CLCDBitmap *bitmap_ = new CLCDBitmap();
    LCDUIASSERT(NULL != bitmap_);
    bitmap_->Initialize();
    bitmap_->SetOrigin(0, 0);
    bitmap_->SetSize(width, height);

    AddObject(bitmap_);

    return bitmap_;
}

HRESULT CEzLcdPage::SetBitmap(HANDLE handle, HBITMAP bitmap, float zoomLevel)
{
    if (NULL == bitmap)
        return E_FAIL;

    CLCDBase* myObject_ = GetObject(handle);

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

HRESULT CEzLcdPage::SetOrigin(HANDLE handle, INT originX, INT originY)
{
    CLCDBase* myObject_ = GetObject(handle);
    LCDUIASSERT(NULL != myObject_);
    LCDUIASSERT(NULL != myObject_);

    //SIZE size_ = myObject_->GetSize();

    //LGObjectType objectType_ = myObject_->GetObjectType();

    if (NULL != myObject_)
    {
        myObject_->SetOrigin(originX, originY);
        return S_OK;
    }

    return E_FAIL;
}

HRESULT CEzLcdPage::SetVisible(HANDLE handle, BOOL visible)
{
    CLCDBase* myObject_ = GetObject(handle);
    LCDUIASSERT(NULL != myObject_);
    LCDUIASSERT(NULL != myObject_);

    if (NULL != myObject_)
    {
        myObject_->Show(visible);
        return S_OK;
    }

    return E_FAIL;
}

VOID CEzLcdPage::Update()
{
// Save copy of button state
    for (INT ii = 0; ii < NUMBER_SOFT_BUTTONS; ii++)
    {
        m_buttonWasPressed[ii] = m_buttonIsPressed[ii];
    }
}

CLCDBase* CEzLcdPage::GetObject(HANDLE handle)
{
    LCD_OBJECT_LIST::iterator it_ = m_Objects.begin();
    while(it_ != m_Objects.end())
    {
        CLCDBase *pObject_ = *it_;
        LCDUIASSERT(NULL != pObject_);

        if (pObject_ == handle)
        {
            return pObject_;
        }
        ++it_;
    }

    return NULL;
}

VOID CEzLcdPage::Init()
{

    for (INT ii = 0; ii < NUMBER_SOFT_BUTTONS; ii++)
    {
        m_buttonIsPressed[ii] = FALSE;
        m_buttonWasPressed[ii] = FALSE;
    }
}


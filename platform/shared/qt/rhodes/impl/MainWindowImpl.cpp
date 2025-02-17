/*------------------------------------------------------------------------
* (The MIT License)
* 
* Copyright (c) 2008-2011 Rhomobile, Inc.
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
* 
* http://rhomobile.com
*------------------------------------------------------------------------*/

#include "MainWindowImpl.h"
#include "common/RhoStd.h"
#include "common/RhodesApp.h"
#include "common/RhoConf.h"
#include "common/StringConverter.h"
#include "common/RhoFilePath.h"
#include "rubyext/NativeToolbarExt.h"
#undef null
#include <QString>
#include <QApplication>
#include <QtGui/QAction>
#include <QHash>
#include "../QtMainWindow.h"

IMPLEMENT_LOGCLASS(CMainWindow,"MainWindow");

using namespace rho;
using namespace rho::common;

extern "C" void rho_geoimpl_turngpsoff();

int CMainWindow::m_screenWidth;
int CMainWindow::m_screenHeight;

bool CMainWindow::mainWindowClosed = false;

CMainWindow::CMainWindow():
    QObject(),
    m_started(true),
    qtMainWindow(NULL),
    qtApplication(NULL)
    //TODO: m_logView
{
}

CMainWindow::~CMainWindow()
{
    //TODO: m_logView
    LOGCONF().setLogView(NULL);

    if (qtMainWindow) delete (QtMainWindow*)qtMainWindow;
    if (qtApplication) delete (QApplication*)qtApplication;
}

CMainWindow* CMainWindow::getInstance(void)
{
    static CMainWindow* instance = 0;
    if (instance==0)
        instance = new CMainWindow();
    return instance;
}

void CMainWindow::updateSizeProperties(int width, int height)
{
    m_screenWidth = width;
    m_screenHeight = height;
    //TODO: LOGCONF().setLogView(&m_logView);
}

void CMainWindow::logEvent(const ::std::string& message)
{
    LOG(INFO) + message;
}

void CMainWindow::onWebViewUrlChanged(const ::std::string& url)
{
    RHODESAPP().keepLastVisitedUrl(url);
}

bool CMainWindow::Initialize(const wchar_t* title)
{
    bool ok = init(this, title);
    rho_rhodesapp_callUiCreatedCallback();
    return ok;
}

void CMainWindow::createCustomMenu(void)
{
    RHODESAPP().getAppMenu().copyMenuItems(m_arAppMenuItems);
#ifdef ENABLE_DYNAMIC_RHOBUNDLE
    String strIndexPage = CFilePath::join(RHODESAPP().getStartUrl(),"index"RHO_ERB_EXT);
    if ( RHODESAPP().getCurrentUrl().compare(RHODESAPP().getStartUrl()) == 0 ||
         RHODESAPP().getCurrentUrl().compare(strIndexPage) == 0 )
        m_arAppMenuItems.addElement(CAppMenuItem("Reload RhoBundle","reload_rhobundle"));
#endif //ENABLE_DYNAMIC_RHOBUNDLE

    //update UI with custom menu items
    menuClear();
    for ( unsigned int i = 0; i < m_arAppMenuItems.size(); i++)
    {
        CAppMenuItem& oItem = m_arAppMenuItems.elementAt(i);
        if (oItem.m_eType == CAppMenuItem::emtSeparator) 
            menuAddSeparator();
        else
        {
            menuAddAction((oItem.m_eType == CAppMenuItem::emtClose ? "Exit" : oItem.m_strLabel.c_str()), i);
        }
    }
}

void CMainWindow::onCustomMenuItemCommand(int nItemPos)
{    
    if ( nItemPos < 0 || nItemPos >= (int)m_arAppMenuItems.size() )
        return;

    CAppMenuItem& oMenuItem = m_arAppMenuItems.elementAt(nItemPos);
    if ( oMenuItem.m_eType == CAppMenuItem::emtUrl )
    {
        if ( oMenuItem.m_strLink == "reload_rhobundle" )
        {
        #ifdef ENABLE_DYNAMIC_RHOBUNDLE
            if ( RHODESAPP().getRhobundleReloadUrl().length()>0 ) {
                CAppManager::ReloadRhoBundle(m_hWnd,RHODESAPP().getRhobundleReloadUrl().c_str(), NULL);
            } else {
                MessageBox(_T("Path to the bundle is not defined."),_T("Information"), MB_OK | MB_ICONINFORMATION );
            }
        #endif
            return;
        }
    }

    oMenuItem.processCommand();
}

void CMainWindow::DestroyUi(void)
{
    rho_rhodesapp_callUiDestroyedCallback();
}

void CMainWindow::onWindowClose(void)
{
    mainWindowClosed = true;
}


// **************************************************************************
//
// proxy methods:
//
// **************************************************************************

void CMainWindow::navigate(const wchar_t* url, int index)
{
    LOG(INFO) + "navigate: '"+url+"'";
    ((QtMainWindow*)qtMainWindow)->navigate(QString::fromWCharArray(url), index);
}

void CMainWindow::setCallback(IMainWindowCallback* callback)
{
    ((QtMainWindow*)qtMainWindow)->setCallback(callback);
}

bool CMainWindow::init(IMainWindowCallback* callback, const wchar_t* title)
{
    int argc = 0;
	QCoreApplication::setOrganizationName("Rhomobile");
	QCoreApplication::setApplicationName("RhoSimulator");
    qtApplication = (void*)new QApplication(argc, 0);
    qtMainWindow = (void*)new QtMainWindow();
    ((QtMainWindow*)qtMainWindow)->setWindowTitle(QString::fromWCharArray(title));
    ((QtMainWindow*)qtMainWindow)->setCallback(callback);
    ((QtMainWindow*)qtMainWindow)->show();

    QObject::connect(this, SIGNAL(doExitCommand(void)),
        ((QtMainWindow*)qtMainWindow), SLOT(exitCommand(void)) );
    QObject::connect(this, SIGNAL(doNavigateBackCommand(void)),
        ((QtMainWindow*)qtMainWindow), SLOT(navigateBackCommand(void)) );
    QObject::connect(this, SIGNAL(doNavigateForwardCommand(void)),
        ((QtMainWindow*)qtMainWindow), SLOT(navigateForwardCommand(void)) );
    QObject::connect(this, SIGNAL(doLogCommand(void)),
        ((QtMainWindow*)qtMainWindow), SLOT(logCommand(void)) );
    QObject::connect(this, SIGNAL(doRefreshCommand(int)),
        ((QtMainWindow*)qtMainWindow), SLOT(refreshCommand(int)) );
    QObject::connect(this, SIGNAL(doNavigateCommand(TNavigateData*)),
        ((QtMainWindow*)qtMainWindow), SLOT(navigateCommand(TNavigateData*)) );
    QObject::connect(this, SIGNAL(doTakePicture(char*)),
        ((QtMainWindow*)qtMainWindow), SLOT(takePicture(char*)) );
    QObject::connect(this, SIGNAL(doSelectPicture(char*)),
        ((QtMainWindow*)qtMainWindow), SLOT(selectPicture(char*)) );
    QObject::connect(this, SIGNAL(doAlertShowPopup(CAlertParams*)),
        ((QtMainWindow*)qtMainWindow), SLOT(alertShowPopup(CAlertParams*)) );
    QObject::connect(this, SIGNAL(doAlertHidePopup(void)),
        ((QtMainWindow*)qtMainWindow), SLOT(alertHidePopup(void)) );
    QObject::connect(this, SIGNAL(doDateTimePicker(CDateTimeMessage*)),
        ((QtMainWindow*)qtMainWindow), SLOT(dateTimePicker(CDateTimeMessage*)) );
    QObject::connect(this, SIGNAL(doExecuteCommand(RhoNativeViewRunnable*)),
        ((QtMainWindow*)qtMainWindow), SLOT(executeCommand(RhoNativeViewRunnable*)) );
    QObject::connect(this, SIGNAL(doExecuteRunnable(rho::common::IRhoRunnable*)),
        ((QtMainWindow*)qtMainWindow), SLOT(executeRunnable(rho::common::IRhoRunnable*)) );
    QObject::connect(this, SIGNAL(doTakeSignature(void*)), //TODO: Signature::Params*
        ((QtMainWindow*)qtMainWindow), SLOT(takeSignature(void*)) );
    QObject::connect(this, SIGNAL(doFullscreenCommand(int)),
        ((QtMainWindow*)qtMainWindow), SLOT(fullscreenCommand(int)) );
    QObject::connect(this, SIGNAL(doSetCookie(const char*, const char*)),
        ((QtMainWindow*)qtMainWindow), SLOT(setCookie(const char*, const char*)) );
    return true;
}

void CMainWindow::messageLoop(void)
{
    ((QApplication*)qtApplication)->exec();
}

void CMainWindow::GoBack(void)
{
    LOG(INFO) + "back";
    ((QtMainWindow*)qtMainWindow)->GoBack();
}

void CMainWindow::GoForward(void)
{
    LOG(INFO) + "forward";
    ((QtMainWindow*)qtMainWindow)->GoForward();
}

void CMainWindow::Refresh(int index)
{
    LOG(INFO) + "refresh";
    ((QtMainWindow*)qtMainWindow)->Refresh(index);
}

int CMainWindow::getLogicalDpiX()
{
    return ((QtMainWindow*)qtMainWindow)->getLogicalDpiX();
}

int CMainWindow::getLogicalDpiY()
{
    return ((QtMainWindow*)qtMainWindow)->getLogicalDpiY();
}

bool CMainWindow::isStarted()
{
    return ((QtMainWindow*)qtMainWindow)->isStarted();
}

int CMainWindow::getToolbarHeight()
{
    return ((QtMainWindow*)qtMainWindow)->toolbarGetHeight();
}

void CMainWindow::removeToolbar()
{
    ((QtMainWindow*)qtMainWindow)->toolbarHide();
}

void CMainWindow::removeAllButtons()
{
    ((QtMainWindow*)qtMainWindow)->toolbarRemoveAllButtons();
}

static QColor getColorFromString(const char* szColor)
{
    if ( !szColor || !*szColor )
        return QColor(0, 0, 0);

    int c = atoi(szColor);

    int cR = (c & 0xFF0000) >> 16;
    int cG = (c & 0xFF00) >> 8;
    int cB = (c & 0xFF);

    return QColor(cR, cG, cB);
}

void CMainWindow::createToolbar(rho_param *p)
{
    if (!rho_rhodesapp_check_mode())
        return;

    int bar_type = TOOLBAR_TYPE;
	std::auto_ptr<QColor> m_rgbBackColor (NULL);
    std::auto_ptr<QColor> m_rgbMaskColor (NULL);
    int m_nHeight = CNativeToolbar::MIN_TOOLBAR_HEIGHT;

    rho_param *params = NULL;
    switch (p->type) 
    {
        case RHO_PARAM_ARRAY:
            params = p;
            break;
        case RHO_PARAM_HASH: 
            {
                for (int i = 0, lim = p->v.hash->size; i < lim; ++i) 
                {
                    const char *name = p->v.hash->name[i];
                    rho_param *value = p->v.hash->value[i];
                    
                    if (strcasecmp(name, "background_color") == 0) 
                        m_rgbBackColor.reset(new QColor(getColorFromString(value->v.string)));
                    else if (strcasecmp(name, "mask_color") == 0) 
                        m_rgbMaskColor.reset(new QColor(getColorFromString(value->v.string)));
                    else if (strcasecmp(name, "view_height") == 0) 
                        m_nHeight = atoi(value->v.string);
                    else if (strcasecmp(name, "buttons") == 0 || strcasecmp(name, "tabs") == 0) 
                        params = value;
                }
            }
            break;
        default: {
            LOG(ERROR) + "Unexpected parameter type for create_nativebar, should be Array or Hash";
            return;
        }
    }
    
    if (!params) {
        LOG(ERROR) + "Wrong parameters for create_nativebar";
        return;
    }

    int size = params->v.array->size;
    if ( size == 0 )
    {
        removeToolbar();
        return;
    }

    removeAllButtons();

    int nSeparators = 0;
    bool wasSeparator = false;
    for (int ipass=0; ipass < 2; ++ipass) {
        for (int i = 0; i < size; ++i) 
        {
            rho_param *hash = params->v.array->value[i];
            if (hash->type != RHO_PARAM_HASH) {
                LOG(ERROR) + "Unexpected type of array item for create_nativebar, should be Hash";
                return;
            }
            
            const char *label = NULL;
            const char *action = NULL;
            const char *icon = NULL;
            const char *colored_icon = NULL;
            int  nItemWidth = 0;

            for (int j = 0, lim = hash->v.hash->size; j < lim; ++j) 
            {
                const char *name = hash->v.hash->name[j];
                rho_param *value = hash->v.hash->value[j];
                if (value->type != RHO_PARAM_STRING) {
                    LOG(ERROR) + "Unexpected '" + name + "' type, should be String";
                    return;
                }
                
                if (strcasecmp(name, "label") == 0)
                    label = value->v.string;
                else if (strcasecmp(name, "action") == 0)
                    action = value->v.string;
                else if (strcasecmp(name, "icon") == 0)
                    icon = value->v.string;
                else if (strcasecmp(name, "colored_icon") == 0)
                    colored_icon = value->v.string;
                else if (strcasecmp(name, "width") == 0)
                    nItemWidth = atoi(value->v.string);
            }
            
            if (label == NULL && bar_type == TOOLBAR_TYPE)
                label = "";
            
            if ( label == NULL || action == NULL) {
                LOG(ERROR) + "Illegal argument for create_nativebar";
                return;
            }
            if ( strcasecmp(action, "forward") == 0 && rho_conf_getBool("jqtouch_mode") )
                continue;

            if (!action) action = "";

            if (ipass==0) {
                if (strcasecmp(action, "separator")==0)
                    ++nSeparators;
            } else {
                LOG(INFO) + "addToolbarButton: Label: '"+label+"';Action: '"+action+"'";
                if (strcasecmp(action, "separator")==0) {
                    if (nSeparators!=1)
                        ((QtMainWindow*)qtMainWindow)->toolbarAddSeparator();
                    else
                        wasSeparator = true;
                } else {
                    String strImagePath;
                    if ( icon && *icon )
                        strImagePath = rho::common::CFilePath::join( RHODESAPP().getRhoRootPath(), icon );
                    else {
#if defined(RHODES_EMULATOR)
#define RHODES_EMULATOR_PLATFORM_STR ".wm"
#elif defined(RHO_SYMBIAN)
#define RHODES_EMULATOR_PLATFORM_STR ".sym"
#else
#define RHODES_EMULATOR_PLATFORM_STR
#endif
                        if ( strcasecmp(action, "options")==0 )
                            strImagePath = "res/options_btn" RHODES_EMULATOR_PLATFORM_STR ".png";
                        else if ( strcasecmp(action, "home")==0 )
                            strImagePath = "res/home_btn" RHODES_EMULATOR_PLATFORM_STR ".png";
                        else if ( strcasecmp(action, "refresh")==0 )
                            strImagePath = "res/refresh_btn" RHODES_EMULATOR_PLATFORM_STR ".png";
                        else if ( strcasecmp(action, "back")==0 )
                            strImagePath = "res/back_btn" RHODES_EMULATOR_PLATFORM_STR ".png";
                        else if ( strcasecmp(action, "forward")==0 )
                            strImagePath = "res/forward_btn" RHODES_EMULATOR_PLATFORM_STR ".png";
#undef RHODES_EMULATOR_PLATFORM_STR
#ifdef RHODES_EMULATOR
                        strImagePath = strImagePath.length() > 0 ? CFilePath::join( RHOSIMCONF().getRhodesPath(), "lib/framework/" + strImagePath) : String();
#else
                        strImagePath = strImagePath.length() > 0 ? CFilePath::join( rho_native_rhopath() , "lib/" + strImagePath) : String();
#endif
                    }

                    ((QtMainWindow*)qtMainWindow)->toolbarAddAction(QIcon(QString(strImagePath.c_str())), QString(label), action, wasSeparator);
                }
            }
        }
    }
	((QtMainWindow*)qtMainWindow)->setToolbarStyle(false, (m_rgbBackColor.get()!=NULL ? m_rgbBackColor->name() : ""));
    ((QtMainWindow*)qtMainWindow)->toolbarShow();
    //removeTabbar();
    m_started = true;
}

bool charToBool(const char* str)
{
    return str && ((strcasecmp(str,"true")==0) || (strcasecmp(str,"yes")==0) || (atoi(str)==1));
}

void CMainWindow::createTabbar(int bar_type, rho_param *p)
{
    if (!rho_rhodesapp_check_mode())
        return;

    /*
    if (bar_type==NOBAR_TYPE) {
        removeToolbar();
		removeAllButtons();
        removeTabbar();
		removeAllTabs();
        m_started = false;
        return;
    }
    */

    std::auto_ptr<QColor> background_color (NULL);
    const char* on_change_tab_callback = NULL;
    
    rho_param *params = NULL;
    switch (p->type)
	{
        case RHO_PARAM_ARRAY:
            params = p;
            break;
        case RHO_PARAM_HASH:
			{
                for (int i = 0, lim = p->v.hash->size; i < lim; ++i)
			    {
                    const char *name = p->v.hash->name[i];
                    rho_param *value = p->v.hash->value[i];
                    if (strcasecmp(name, "background_color") == 0) {
                        background_color.reset(new QColor(getColorFromString(value->v.string)));
                    } else if (strcasecmp(name, "on_change_tab_callback") == 0) {
                        on_change_tab_callback = value->v.string;
                    } else if (strcasecmp(name, "buttons") == 0 || strcasecmp(name, "tabs") == 0) {
                        params = value;
                    }
                }
            }
            break;
        default: {
            RAWLOG_ERROR("Unexpected parameter type for create_nativebar, should be Array or Hash");
            return;
        }
    }
    
    if (!params) {
        RAWLOG_ERROR("Wrong parameters for create_tabbar");
        return;
    }
    
    ((QtMainWindow*)qtMainWindow)->tabbarInitialize();

    int size = params->v.array->size;

    for (int i = 0; i < size; ++i) {
        rho_param *hash = params->v.array->value[i];
        if (hash->type != RHO_PARAM_HASH) {
            RAWLOG_ERROR("Unexpected type of array item for create_nativebar, should be Hash");
            return;
        }
        
        const char *label = NULL;
        const char *action = NULL;
        const char *icon = NULL;
        const char *reload = NULL;
        const char *colored_icon = NULL;
        
    	std::auto_ptr<QColor> selected_color (NULL);
        const char *disabled = NULL;
		std::auto_ptr<QColor> web_bkg_color (NULL);
        const char* use_current_view_for_tab = NULL;
        
        bool skip_item = false;
        for (int j = 0, lim = hash->v.hash->size; j < lim; ++j) {
            const char *name = hash->v.hash->name[j];
            rho_param *value = hash->v.hash->value[j];
            if (value->type != RHO_PARAM_STRING) {
                RAWLOGC_ERROR("Unexpected '%s' type, should be String", name);
                return;
            }
            if (strcasecmp(name, "background_color") == 0) {
                background_color.reset(new QColor(getColorFromString(value->v.string)));
                skip_item = true;
            }
            
            if (strcasecmp(name, "label") == 0)
                label = value->v.string;
            else if (strcasecmp(name, "action") == 0)
                action = value->v.string;
            else if (strcasecmp(name, "icon") == 0)
                icon = value->v.string;
            else if (strcasecmp(name, "reload") == 0)
                reload = value->v.string;
            else if (strcasecmp(name, "colored_icon") == 0)
                colored_icon = value->v.string;
            else if (strcasecmp(name, "selected_color") == 0){
                selected_color.reset(new QColor(getColorFromString(value->v.string)));
            }    
            else if (strcasecmp(name, "disabled") == 0)
                disabled = value->v.string;
            else if (strcasecmp(name, "web_bkg_color") == 0)
                web_bkg_color.reset(new QColor(getColorFromString(value->v.string)));
            else if (strcasecmp(name, "use_current_view_for_tab") == 0) {
                use_current_view_for_tab = value->v.string;
                if (strcasecmp(use_current_view_for_tab, "true") == 0) {
                    action = "none";
                }
            }
        }
        
        if (label == NULL && bar_type == TOOLBAR_TYPE)
            label = "";
        
        if ((label == NULL || (action == NULL)) && (!skip_item)) {
            RAWLOG_ERROR("Illegal argument for create_nativebar");
            return;
        }
        if (!skip_item) {
            QtMainWindow::QTabBarRuntimeParams tbrp;
            tbrp["label"] = QString(label);
            tbrp["action"] = QString(action);
            tbrp["reload"] = charToBool(reload);
            tbrp["use_current_view_for_tab"] = charToBool(use_current_view_for_tab);
            tbrp["background_color"] = background_color.get() != NULL ? background_color->name() : QString("");
			tbrp["selected_color"] = selected_color.get() != NULL ? selected_color->name() : QString("");
            tbrp["on_change_tab_callback"] = QString(on_change_tab_callback != NULL ? on_change_tab_callback : "");
            String strIconPath = icon ? CFilePath::join( RHODESAPP().getAppRootPath(), icon) : String();
            ((QtMainWindow*)qtMainWindow)->tabbarAddTab(QString(label), icon ? strIconPath.c_str() : NULL, charToBool(disabled), web_bkg_color.get(), tbrp);
        }
    }

    ((QtMainWindow*)qtMainWindow)->tabbarShow();

    m_started = true;
}

int CMainWindow::getTabbarHeight()
{
    return ((QtMainWindow*)qtMainWindow)->tabbarGetHeight();
}

void CMainWindow::removeTabbar()
{
    ((QtMainWindow*)qtMainWindow)->tabbarHide();
}

void CMainWindow::removeAllTabs(bool restore)
{
    ((QtMainWindow*)qtMainWindow)->tabbarRemoveAllTabs(restore);
}

void CMainWindow::tabbarSwitch(int index)
{
    ((QtMainWindow*)qtMainWindow)->tabbarSwitch(index);
}

void CMainWindow::tabbarBadge(int index, char* badge)
{
    ((QtMainWindow*)qtMainWindow)->tabbarSetBadge(index, badge);
}

int CMainWindow::tabbarGetCurrent()
{
    return ((QtMainWindow*)qtMainWindow)->tabbarGetCurrent();
}

// Menu
void CMainWindow::menuClear()
{
    ((QtMainWindow*)qtMainWindow)->menuClear();
}

void CMainWindow::menuAddSeparator()
{
    ((QtMainWindow*)qtMainWindow)->menuAddSeparator();
}

void CMainWindow::menuAddAction(const char* label, int item)
{
    ((QtMainWindow*)qtMainWindow)->menuAddAction(QString(label), item);
}

void CMainWindow::onActivate(int active)
{
    rho_rhodesapp_callAppActiveCallback(active);
    if (!active)
        rho_geoimpl_turngpsoff();
}

// Commands
void CMainWindow::exitCommand()
{
    emit doExitCommand();
}

void CMainWindow::navigateBackCommand()
{
    emit doNavigateBackCommand();
}

void CMainWindow::navigateForwardCommand()
{
    emit doNavigateForwardCommand();
}

void CMainWindow::logCommand()
{
    emit doLogCommand();
}

void CMainWindow::refreshCommand(int tab_index)
{
    emit doRefreshCommand(tab_index);
}

void CMainWindow::navigateCommand(TNavigateData* nd)
{
    emit doNavigateCommand(nd);
}

void CMainWindow::takePicture(char* callbackUrl)
{
    emit doTakePicture(callbackUrl);
}

void CMainWindow::selectPicture(char* callbackUrl)
{
    emit doSelectPicture(callbackUrl);
}

void CMainWindow::alertShowPopup(CAlertParams *params)
{
    emit doAlertShowPopup(params);
}

void CMainWindow::alertHidePopup()
{
    emit doAlertHidePopup();
}

void CMainWindow::dateTimePicker(CDateTimeMessage * msg)
{
    emit doDateTimePicker(msg);
}

void CMainWindow::executeCommand(RhoNativeViewRunnable* runnable)
{
    emit doExecuteCommand(runnable);
}

void CMainWindow::executeRunnable(rho::common::IRhoRunnable* pTask)
{
    emit doExecuteRunnable(pTask);
}

void CMainWindow::takeSignature(void* params) //TODO: Signature::Params*
{
    emit doTakeSignature(params);
}

void CMainWindow::fullscreenCommand(int enable)
{
    emit doFullscreenCommand(enable);
}

void CMainWindow::setCookie(const char* url, const char* cookie)
{
    emit doSetCookie(url, cookie);
}

void CMainWindow::bringToFront()
{
    emit doBringToFront();
}

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

#pragma once

#ifdef __cplusplus

#include "logging/RhoLog.h"
#include "db/DBAdapter.h"
#include "sync/SyncEngine.h"
#include "common/ThreadQueue.h"

namespace rho {
namespace sync {

class CSyncThread : public common::CThreadQueue
{
public:
    enum ESyncCommands{ scNone = 0, scSyncAll, scSyncOne, scLogin, scSearchOne};

private:

    DEFINE_LOGCLASS;

public:
    static const unsigned int SYNC_WAIT_BEFOREKILL_SECONDS  = 3;

    class CSyncCommand : public IQueueCommand
    {
    public:
	    int m_nCmdCode;
	    int m_nCmdParam;
	    String m_strCmdParam, m_strQueryParams;
   		boolean m_bShowStatus;

	    CSyncCommand(int nCode, int nParam, boolean bShowStatus, const char * query_params)
	    {
		    m_nCmdCode = nCode;
		    m_nCmdParam = nParam;
            m_bShowStatus = bShowStatus;
            m_strQueryParams = query_params ? query_params : "";
	    }
	    CSyncCommand(int nCode, String strParam, boolean bShowStatus, const char * query_params)
	    {
		    m_nCmdCode = nCode;
		    m_strCmdParam = strParam;
            m_bShowStatus = bShowStatus;
            m_strQueryParams = query_params ? query_params : "";
	    }
	    CSyncCommand(int nCode, String strParam, int nCmdParam, boolean bShowStatus, const char * query_params)
	    {
		    m_nCmdCode = nCode;
		    m_strCmdParam = strParam;
            m_nCmdParam = nCmdParam;
            m_bShowStatus = bShowStatus;
            m_strQueryParams = query_params ? query_params : "";
	    }

	    CSyncCommand(int nCode, boolean bShowStatus, const char * query_params)
	    {
		    m_nCmdCode = nCode;
		    m_nCmdParam = 0;
            m_bShowStatus = bShowStatus;
            m_strQueryParams = query_params ? query_params : "";
	    }

	    boolean equals(const IQueueCommand& cmd)
	    {
            const CSyncCommand& oSyncCmd = (const CSyncCommand&)cmd;
		    return m_nCmdCode == oSyncCmd.m_nCmdCode && m_nCmdParam == oSyncCmd.m_nCmdParam &&
			    m_strCmdParam == oSyncCmd.m_strCmdParam &&
                m_strQueryParams == oSyncCmd.m_strQueryParams;
	    }

        virtual String toString();

    };

    class CSyncLoginCommand : public CSyncCommand
    {
    public:
	    String m_strName, m_strPassword;
        common::CAutoPtr<CSyncNotification> m_pNotify;
        CSyncLoginCommand(String name, String password, CSyncNotification* pNotify) : 
            CSyncCommand(CSyncThread::scLogin,"",false,NULL)
	    {
		    m_strName = name;
		    m_strPassword = password;
			m_pNotify = pNotify;
	    }
    };
	
    class CSyncSearchCommand : public CSyncCommand
    {
    public:
	    String m_strFrom;
        boolean m_bSyncChanges;
        rho::Vector<rho::String> m_arSources;

        CSyncSearchCommand(String from, String params, rho::Vector<rho::String>& arSources, boolean sync_changes, int nProgressStep) : CSyncCommand(CSyncThread::scSearchOne,params,nProgressStep, false, "")
	    {
		    m_strFrom = from;
            m_bSyncChanges = sync_changes;
            m_arSources = arSources;
	    }
    };

private:
    static CSyncThread* m_pInstance;

    CSyncEngine     m_oSyncEngine;
public:
    ~CSyncThread(void);

    static CSyncThread* Create();
    static void Destroy();
    static CSyncThread* getInstance(){ return m_pInstance; }
    static CSyncEngine& getSyncEngine(){ return m_pInstance->m_oSyncEngine; }

	void setPollInterval(int nInterval);
    
    void stopAll();

    unsigned long getRetValue();
private:
    CSyncThread();

    virtual int getLastPollInterval();
    virtual void processCommand(IQueueCommand* pCmd);
    virtual boolean isSkipDuplicateCmd() { return true; }

    virtual void onTimeout();

    void checkShowStatus(CSyncCommand& oSyncCmd);
};

}
}
#endif //__cplusplus

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus
	
unsigned long rho_sync_doSyncAllSources(int show_status_popup, const char * query_params);
unsigned long rho_sync_doSyncSource(unsigned long nSrcID,int show_status_popup, const char * query_params);
unsigned long rho_sync_doSyncSourceByID(int nSrcID);
unsigned long rho_sync_doSyncSourceByName(const char* szSrcName);

unsigned long rho_sync_doSearch(unsigned long ar_sources, const char *from, const char *params, bool sync_changes, int nProgressStep, const char* callback, const char* callback_params);

unsigned long rho_sync_login(const char *login, const char *password, const char* callback);
int rho_sync_logged_in();
void rho_sync_logout();
void rho_sync_set_notification(int source_id, const char *url, char* params);
void rho_sync_clear_notification(int source_id);
int rho_sync_set_pollinterval(int nInterval);
int rho_sync_get_pollinterval();
void rho_sync_set_syncserver(const char* syncserver);
void rho_sync_setobjectnotify_url(const char* szUrl);
void rho_sync_clear_object_notification();    
void rho_sync_addobjectnotify(int nSrcID, const char* szObject);
void rho_sync_cleanobjectnotify();
int rho_sync_get_pagesize();
void rho_sync_set_pagesize(int nPageSize);

unsigned long rho_sync_get_attrs(const char* szPartition, int nSrcID);
unsigned long rho_sync_is_blob_attr(const char* szPartition, int source_id, const char* szAttrName);
int rho_sync_get_lastsync_objectcount(int nSrcID);

void rho_sync_set_threaded_mode(int b);
char* rho_sync_create_string(const char* szStr);
void rho_sync_free_string(char* szStr);

unsigned long rho_sync_login_c(const char *name, const char *password, /*RHOC_CALLBACK*/void* callback, void* callback_data);
unsigned long rho_sync_doSearchByNames(unsigned long ar_sources, const char *from, const char *params, bool sync_changes, int nProgressStep, /*RHOC_CALLBACK*/void* callback, void* callback_data);
void rho_sync_set_notification_c(int source_id, /*RHOC_CALLBACK*/void* callback, void* callback_data);

void rho_sync_setobjectnotify_url_c(/*RHOC_CALLBACK*/void* callback, void* callback_data);
    
void rho_sync_stop();
void rho_sync_set_source_property(int nSrcID, const char* szPropName, const char* szPropValue);
unsigned long rho_sync_get_source_property(int nSrcID, const char* szPropName);
int  rho_sync_issyncing();

#ifdef __cplusplus
};
#endif //__cplusplus

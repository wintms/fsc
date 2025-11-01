/****************************************************************
 ****************************************************************
 **                                                            **
 **    (C)Copyright 2006-2020, American Megatrends Inc.        **
 **                                                            **
 **            All Rights Reserved.                            **
 **                                                            **
 **        5555 Oakbrook Parkway, Norcross,                    **
 **                                                            **
 **        Georgia - 30093, USA. Phone-(770)-246-8600.         **
 **                                                            **
 ****************************************************************
 ****************************************************************
 *
 * PDKAlert.c
 *
 * @brief Handles all the OEM defined Alerts
 *
 *****************************************************************/
#define ENABLE_DEBUG_MACROS 0


#include "PDKAlert.h"


#include <stdio.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/select.h>

#include "dbgout.h"
#include "smtpclient.h"
#include <arpa/inet.h>
#include "IPMIConf.h"
#include <PDKAccess.h>
#include <dlfcn.h>
#include <Ethaddr.h>

#define PRIMARY_SERVER "GetSMTP_PrimaryServer"
#define SECONDARY_SERVER "GetSMTP_SecondaryServer"
#define  MSGHNDLR_LIB "/usr/local/lib/libipmimsghndlr.so"

int Get_SMTPSever(SMTP_STRUCT *pmail, INT8U SetSelector, INT8U EthIndex,int i, int BMCInst );
void FillFormat(void* pEventRecord,SMTP_STRUCT *pmail,char *useremailformat,int BMCInst);

int
PDK_Alert ( void* pEventRecord,  int SetSelector ,INT8U  EthIndex, int BMCInst) 
{

/* We are setting the set selector as  zero based in libipmi it self  and No need to do it  again */

   /* Send a email As the Oem Alert  Action */
    return PDK_FrameAndSendMail (pEventRecord, SetSelector,EthIndex , BMCInst);
   /* Other   Oem Alerts go here */
}

int
PDK_AfterSNMPTrap ( void* pEventRecord,  int SetSelector ,int BMCInst)
{
    /* Add any OEM Alert after SNMP Trap */
    if(0)
    {
	pEventRecord=pEventRecord;  /*  -Wextra. fix for unused parameters  */
	SetSelector=SetSelector;
	BMCInst=BMCInst;
    }
    return 0;
}

int
PDK_FrameAndSendMail ( void* pEventRecord, INT8U SetSelector, INT8U EthIndex, int BMCInst )
{

    char ifname [16] = {0};
    int ret = 0;
    SMTP_STRUCT mail;
    int status = 0,i;
    int smtpServerFlag=0;//0 - Primary, 1 - Secondary
    _FAR_ UserInfo_T* pUserTable = (UserInfo_T *) GetNVRUsrCfgAddr (NVRH_USERCONFIG, BMCInst);
    _FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];

    memset(&mail,0,sizeof(SMTP_STRUCT));

    for(i=0;i<pBMCInfo->IpmiConfig.MaxUsers;i++) 
	{
		if(Get_SMTPSever(&mail,SetSelector,EthIndex,0,BMCInst) < 0)//PRIMARY Server
		{
			smtpServerFlag=1;
			if(Get_SMTPSever(&mail,SetSelector,EthIndex,1,BMCInst) < 0)
			return 0;
		}
		status = 0;
		if(pUserTable[i].UserId != mail.UserID)
		{
			continue;
		}
		if(!pUserTable[i].UserStatus) return USER_DISABLED;
		if( SetSelector > 	MAX_EMAIL_DESTINATIONS ) { return 0 ; }
		
		if( pUserTable[i].UserEMailID[0] != 0 )
		{
			if(EmailformatCount != 0)
				FillFormat(pEventRecord,&mail,(char *)pUserTable[i].EmailFormat,BMCInst);
			strncpy((char *)mail.to_addr,(char *)pUserTable[i].UserEMailID,EMAIL_ADDR_SIZE);
			if ( GetIfcName(EthIndex, ifname, BMCInst) < 0 ) {
					TCRIT("Cannot get ifname\n");
					return 0;
			}

			ret = snprintf((char *)mail.from_ifname, sizeof(mail.from_ifname), "%s", ifname);
			if (ret < 0 || ret >= (int)sizeof(mail.from_ifname)) {
					TCRIT("Buffer overflow\n");
					return 0;
			}

			status = smtp_mail(&mail);
			if(status == EMAIL_NO_CONNECT)
			{
				if(smtpServerFlag == 1) return status;
				if(Get_SMTPSever(&mail,SetSelector,EthIndex,1,BMCInst) < 0)
					return 0;
				if(strlen(mail.smtp_server) != 0)
				{
				    if(EmailformatCount != 0)
					    FillFormat(pEventRecord,&mail,(char *)pUserTable[i].EmailFormat,BMCInst);
				    strncpy((char *)mail.to_addr,(char *)pUserTable[i].UserEMailID,EMAIL_ADDR_SIZE);
				    ret = snprintf((char *)mail.from_ifname, sizeof(mail.from_ifname), "%s", ifname);
				    if (ret < 0 || ret >= (int)sizeof(mail.from_ifname)) {
						    TCRIT("Buffer overflow\n");
						    return 0;
				    }

				    status = smtp_mail(&mail);
				}
			}
			return status;
		}
		else
		return EMAILID_NOT_CONFIGURED;
    }
    if(i == pBMCInfo->IpmiConfig.MaxUsers) return USER_NOT_FOUND;
    return status;
}

int Get_SMTPSever(SMTP_STRUCT *pmail, INT8U SetSelector, INT8U EthIndex,int i, int BMCInst )
{
    void *dl_handle;
    int (*getsmtpmail) (SMTP_STRUCT *pmail, INT8U SetSelector, INT8U EthIndex, int BMCInst );
    dl_handle = dlopen((char *)MSGHNDLR_LIB,RTLD_NOW);
    int ret;
    getsmtpmail  = NULL;
    if(!dl_handle)
    {
        IPMI_ERROR("Error in loading IPMI library %s\n",dlerror());
        return -1;
    }

	if(i==0)
	{
		getsmtpmail=dlsym(dl_handle, PRIMARY_SERVER);
		if(getsmtpmail  == NULL)
		{
			dlclose(dl_handle);   
			return -1;
		}
	}
	else
	{
		getsmtpmail=dlsym(dl_handle, SECONDARY_SERVER);
		if(getsmtpmail  == NULL)
		{
			dlclose(dl_handle);   
			return -1;
		}
	}
	ret=getsmtpmail(pmail,SetSelector,EthIndex,BMCInst);

	dlclose(dl_handle);
	return ret;
}


void FillFormat(void *pEventRecord,SMTP_STRUCT *pmail,char *useremailformat,int BMCInst)
{
	INT32 count = 0;
	
	for(count = 0; count < EmailformatCount; ++count)
	{
		if(g_PDKEmailHandle[count] == NULL) continue;
		if(strncmp((char *)g_PDKEmailFormat[count].EmailFormat,useremailformat,EMAIL_FORMAT_SIZE)!=0)
		{
			continue;
		}
		((void(*)(void *,SMTP_STRUCT *,int))g_PDKEmailHandle[count]) (pEventRecord,pmail,BMCInst);
	}
}

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
 * pdkhw.c
 * Hardware configuration specific to a platform.
 *
 *  Author: Govind Kothandapani <govindk@ami.com>
 ******************************************************************/
#define ENABLE_DEBUG_MACROS     0
#include "Types.h"
#include "OSPort.h"
#include "Debug.h"
#include "PDKHooks.h"
#include "PDKHW.h"
#include "API.h"
#include "SharedMem.h"
#include "Indicators.h"
#include "LEDMap.h"
#include "PDKEEPROM.h"
#include "mmap.h"
#include "PDKDefs.h"
#include "intrsensor.h"
#include "IPMIConf.h"
#include "Platform.h"
#include "SSIAccess.h"
#include "featuredef.h"
#include <unistd.h>
#include <sys/reboot.h>
#include <linux/reboot.h>
#include <dlfcn.h>
#include "flashlib.h"
#include "PendTask.h"
#include "drvload.h"

/* AMI DC and AC cycle stress test with AMI 876 platform*/
#include "hal_hw.h"
#include "gpioifc.h"

#include "OEMPLD.h"
#include "OEMSysInfo.h"

#define GPIO_BMC_RSTBTN_OUT_N 175  //GPIOV7
#define GPIO_BMC_PWBTN_OUT_N 174  //GPIOV6
#define GPIO_SLP_S3_N 173         //GPIOV5
#define GPIO_PS_PWROK 172         //GPIOV4

#define IO1	1
#define IO2 2
#define IO3	3
#define IO4	4
#define UART1	7
#define UART2	8
#define UART3	9
#define UART4	10
#define SET_MUX_TO_SYS_MASK	0xFE
#define SET_MUX_TO_BMC_MASK	0x1

#define LIBTELCOHELPER_LIB_PATH		"/usr/local/lib/libtelcohelper.so"
/**
 * @def Parameters to controle the Chassis actions
 *
**/
#define TYPE_CHASSIS 0x01

#define CHASSIS_POWER_OFF               0x00
#define CHASSIS_POWER_ON                0x01
#define CHASSIS_POWER_CYCLE             0x02
#define CHASSIS_POWER_RESET             0x03
#define CHASSIS_DIAGNOSTIC_INTERRUPT    0x04
#define CHASSIS_SOFT_OFF                0x05

#define CMD_FAIL               0x00
#define CMD_PASS               0x01
#define CMD_WAIT               0x02



extern int g_SMItoutEnable;
extern int g_SMItoutInterval;
extern int g_lpcreset;
int g_chassisCtrlCmdStatus = 0;
int g_UIDStatus=0;

#define AST_SCU_REG_BASE	0x1E6E2000
#define AST_SCU_SYSRST_CTRLSTATUS_REG_OFFSET 	0x64

/*-----------------------------------------------------------------
 * @fn chassis_telco_log
 * @brief Chassis log sent to TELCO helper package
 *-----------------------------------------------------------------*/
void telco_chassis_alert(int BMCInst, INT8U Action, INT8U result)
{
    char *(*chassis_command_helper)() = NULL;
	void *telco_helper = NULL;
	telco_helper = dlopen(LIBTELCOHELPER_LIB_PATH, RTLD_NOW | RTLD_NODELETE | RTLD_GLOBAL);
	if(telco_helper != NULL)
	{
		chassis_command_helper = dlsym(telco_helper,"TelcoChassisHelper");
		if(chassis_command_helper)
		{
			chassis_command_helper(Action,result,BMCInst,TYPE_CHASSIS);
		}
		dlclose(telco_helper);
	}
}
/*----------------------------------------------------------------------------
 * @fn PDK_PlatformInit
 *
 * @brief This function should be used to perform any platform specific
 * initialization. This function is called before any other initialization
 * (SEL, SDR, and Platform Configuration etc.) is done. OS Queues are not
 * available and cannot be used.
 * @return	0  if success
 *			-1 if error
 *---------------------------------------------------------------------------*/
int
PDK_PlatformInit (void)
{

	/** AMI DC and AC cycle stress test begin **/	
	u8 val;
	hal_t hal;
  char *PreLoadDrv[]={
    "gpio",
    "gpio_hw",
    };

    PlatformDriverInit(&PreLoadDrv[0], sizeof(PreLoadDrv)/sizeof(PreLoadDrv[0]));
		
	/* GPIO_PS_PWROK INPUT */
	hal.func = HAL_SET_GPIO_DIR;
	hal.pwrite_buf = &val;
	val = 0;
	hal.gpio.pin = GPIO_PS_PWROK;
	gpio_write(&hal);

	/* GPIO_BMC_PWBTN_OUT_N OUTPUT */
	//hal.func = HAL_SET_GPIO_DIR;
	//hal.pwrite_buf = &val;
	//val = 1;
	//hal.gpio.pin = GPIO_BMC_PWBTN_OUT_N;
	//gpio_write(&hal);

	/* Test for gpio 8 as OUTPUT ad set high */
        hal.func = HAL_SET_GPIO_DIR_OUTPUT_REG;
        hal.pwrite_buf = &val;
        val = 1;
        hal.gpio.pin = 8;
        gpio_write(&hal);

	
	/* GPIO_BMC_PWBTN_OUT_N HIGH */
	//val = 1;
	//hal.func = HAL_DEVICE_WRITE;
	//hal.pwrite_buf = &val;
	//hal.gpio.pin = GPIO_BMC_PWBTN_OUT_N;
	//gpio_write(&hal);
	
	/** test end**/

    // Initializes the global system information.
    OEM_InitSysInfo();

   return 0;
}


/*-----------------------------------------------------------------
 * @fn PDK_ColdResetBMC
 * @brief Cold Resets the BMC
 * @return	0  if success
 *			-1 if error
 * CHECK_POINT:	01
 *-----------------------------------------------------------------*/
int
PDK_ColdResetBMC (int BMCInst)
{
	UN_USED(BMCInst);
 	//reboot (LINUX_REBOOT_CMD_RESTART);
	return 0;
}

/*-----------------------------------------------------------------
 * @fn PDK_warmResetBMC
 * @brief Warm Reset BMC
 * @return	0  if success
 *			-1 if error
 * CHECK_POINT:	01
 *-----------------------------------------------------------------*/
int
PDK_WarmResetBMC (int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameter  */
    }
    if(system ("/sbin/reboot") == -1)
    {
    	TCRIT("Unable to Reboot\n");
    	return -1;
    }
    return 0;
}


/*------------------------------------------------------------------
 * @fn PDK_GetSystemState
 * @brief This function should return the current status of the
 * system. This will be polled every 10 milliseconds.
 * @return 	Bitmap of the current state
 *			PS_PWRGD                        BIT 0
 *			FP_PWR_BTN_ACTIVE               BIT 1
 *			FP_RST_BTN_ACTIVE               BIT 2
 *			FP_NMI_BTN_ACTIVE               BIT 3
 *			FP_SLEEP_BTN_ACTIVE             BIT 4
 *			S5_SIGNAL_ACTIVE                BIT 7
 *			S1_SIGANL_ACTIVE                BIT 8
 *			PROC_HOT_ACTIVE                 BIT 9
 *			THERMAL_TRIP_ACTIVE             BIT 10
 *			FRB3_TIMER_HALT                 BIT 11
 *			CHIPSET_ERR2                    BIT 12
 *			CHIPSET_ERR1                    BIT 13
 *			CHIPSET_ERR0                    BIT 14
 *			PCI_RAS_ERR                     BIT 15
 *			PCI_SYS_SERR                    BIT 16
 *			WAKE_ON_LAN                     BIT 17
 *			SECURE_MODE_KB                  BIT 18
 *			CPU0_SKTOCC                     BIT 19
 *			CPU1_SKTOCC                     BIT 20
 *
 *          BIT 24 - 31     For OEM State
 *------------------------------------------------------------------*/
// GetSystemState and HP LP Signals - refer PDKHooks.
INT32U
PDK_GetSystemState (int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameter  */
    }
    return 0;
}

/*---------------------------------------------------------------------
 * @fn PDK_OnSystemEventDetected
 * @brief	This control function is called by the core when it detecte's any
 *			of the below event.
 * @param   Bitmap of the State
 *			PS_PWRGD                        BIT 0
 *			FP_PWR_BTN_ACTIVE               BIT 1
 *			FP_RST_BTN_ACTIVE               BIT 2
 *			FP_NMI_BTN_ACTIVE               BIT 3
 *			FP_SLEEP_BTN_ACTIVE             BIT 4
 *			S5_SIGNAL_ACTIVE                BIT 7
 *			S1_SIGANL_ACTIVE                BIT 8
 *			PROC_HOT_ACTIVE                 BIT 9
 *			THERMAL_TRIP_ACTIVE             BIT 10
 *			FRB3_TIMER_HALT                 BIT 11
 *			CHIPSET_ERR2                    BIT 12
 *			CHIPSET_ERR1                    BIT 13
 *			CHIPSET_ERR0                    BIT 14
 *			PCI_RAS_ERR                     BIT 15
 *			PCI_SYS_SERR                    BIT 16
 *			WAKE_ON_LAN                     BIT 17
 *			SECURE_MODE_KB                  BIT 18
 *			CPU0_SKTOCC                     BIT 19
 *			CPU1_SKTOCC                     BIT 20
 *
 *          BIT 24 - 31     For OEM State
 * @return  0x00	Success
 *			0xff	Failure
 *-----------------------------------------------------------------------*/
int
PDK_OnSystemEventDetected (INT32U State, int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameters  */
	State=State;
    }
    return 0;
}

/*---------------------------------------------------------------------------
* @fn PDK_IsACPowerOn
*
* @brief This function returns the AC power on status. That is, it returns
*        true if the power-on is due to the AC-on. Make sure call this
*        function after the PDK_PostInit().
*---------------------------------------------------------------------------*/
bool PDK_IsACPowerOn(int BMCInst)
{
	unsigned long value = 0;
	UN_USED(BMCInst);
	mmap_read32(AST_SCU_REG_BASE + AST_SCU_SYSRST_CTRLSTATUS_REG_OFFSET, &value);
	if((value & 1) == 0x00000001)
	{
		mmap_write32(AST_SCU_REG_BASE + AST_SCU_SYSRST_CTRLSTATUS_REG_OFFSET, 0x00000001); //Clearing the Power on reset flag to avoid value being set in the next reboot.
		return TRUE;
	}
	return FALSE;
}

static void ChassisPowerControl(INT8U gpio_pin, INT8U delay)
{
    u8 val = 0;
    hal_t hal;

    hal.func = HAL_SET_GPIO_DIR;
    hal.pwrite_buf = &val;
    val = 1;    // Set Power Button Pin direction as Output.
    hal.gpio.pin = gpio_pin;
    gpio_write(&hal);

    val = 0;
    hal.func = HAL_DEVICE_WRITE;
    gpio_write(&hal);

    sleep(delay);

    val = 1;
    hal.func = HAL_DEVICE_WRITE;
    gpio_write(&hal);

}

#ifdef CONFIG_SPX_FEATURE_GPGPU_SUPPORT
//Updating GPGPU Host status 
int UpdateGPGPUHostStatus(int status)
{
	int pipe_fd=0;
        char pipe_name[] ="/var/GPGPUHostResetStatus";
        struct stat buf={0};
        if(stat(pipe_name,&buf)==-1)
        {
                TCRIT("/var/GPGPUHostResetStatus not found\n");
                return -1;
        }
        pipe_fd = sigwrap_open(pipe_name, O_WRONLY);
        if(pipe_fd<0)
        {
                TCRIT("/var/GPGPUHostResetStatus opened failed\n");
                return -1;
        }
        if(sigwrap_write (pipe_fd,&status, sizeof(int))==-1)
        {
                TCRIT("failed to write /var/GPGPUHostResetStatus \n");
        }
        if(sigwrap_close(pipe_fd)==-1)
        {
                TCRIT("failed to close\n");
        }
        return 0;
}
#endif

/*---------------------------------------------------------------------------
 * @fn PDK_PowerOnChassis
 *
 * @brief This function is invoked to power on the chassis. The default
 * implementation links to the soft processor code (PMCP). You can override
 * with your own C code implementation.
 *
 *---------------------------------------------------------------------------*/
int 
PDK_PowerOnChassis (int BMCInst)
{
	/** AMI DC and AC cycle stress test begin **/	
	u8 val;
	hal_t hal;
    
	hal.func = HAL_SET_GPIO_DIR;
	hal.pwrite_buf = &val;
    val = 1;
	hal.gpio.pin = GPIO_BMC_PWBTN_OUT_N;
	gpio_write(&hal);
	
    val = 0;
	hal.func = HAL_DEVICE_WRITE;
	gpio_write(&hal);
	sleep(1);
	val = 1;	
	gpio_write(&hal);
	/** test end**/

    if((PDK_GetPSGood(BMCInst)) == 0)
    {
        if(IsFeatureEnabled("CONFIG_SPX_FEATURE_TELCO_RSYSLOG_SUPPORT") == 1 || IsFeatureEnabled("CONFIG_SPX_FEATURE_SNMPTRAP_SUPPORT")== 1)
        {
            telco_chassis_alert(BMCInst,CHASSIS_POWER_ON,CMD_FAIL);
        }
    }
    else
    {    
        if(IsFeatureEnabled("CONFIG_SPX_FEATURE_TELCO_RSYSLOG_SUPPORT") == 1 || IsFeatureEnabled("CONFIG_SPX_FEATURE_SNMPTRAP_SUPPORT")== 1)
        {
            telco_chassis_alert(BMCInst,CHASSIS_POWER_ON,CMD_PASS);
        }
    }

	printf("POWER ON CHASSIS\n");
#ifdef CONFIG_SPX_FEATURE_GPGPU_SUPPORT
    int status=1;
    if( UpdateGPGPUHostStatus(status)==0)
    {
	    TDBG("Updated GPGPUHostStatus host sucessfully\n");
    }
#endif

    if(g_corefeatures.ssi_support == ENABLED)
    {
        /* Do queue Operational State condition. */
        if (g_SSIHandle[SSICB_QUEUECOND] != NULL)
        {
           ((STATUS(*)(INT8U, INT8U, INT8U, int))g_SSIHandle[SSICB_QUEUECOND]) (DEFAULT_FRU_DEV_ID, COND_POWER_ON, 1, BMCInst);
        }

        LOCK_BMC_SHARED_MEM(BMCInst);
        BMC_GET_SHARED_MEM(BMCInst)->PowerActionInProgress = FALSE;
        UNLOCK_BMC_SHARED_MEM(BMCInst);
    }
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameter  */
    }
    return 0;
}


/*---------------------------------------------------------------------------
 * @fn PDK_PowerOffChassis
 *
 * @brief This function is invoked to power off the chassis. The default
 * implementation links to the soft processor code (PMCP). You can override
 * with your own C code implementation.
 *---------------------------------------------------------------------------*/
int
PDK_PowerOffChassis (int BMCInst)
{
	/** AMI DC and AC cycle stress test begin **/	
    u8 retry = 0, ChassisOFFFlag = 0, delay = 5;

    while((PDK_GetPSGood(BMCInst))==1)
    {
        if ((retry < 3) && ((PDK_GetPSGood(BMCInst)) != 0))
        {
            ChassisPowerControl(GPIO_BMC_PWBTN_OUT_N, delay);
            sleep(2);
        }
        else
        {
            if(delay <= 8)
            {
                delay++;
                ChassisPowerControl(GPIO_BMC_PWBTN_OUT_N, delay);
                sleep(2);
            }
            else
            {
                break;
            }
        }
        if((PDK_GetPSGood(BMCInst)) == 0)
        {
            ChassisOFFFlag = 1;
            break;
        }
        retry++;
    }
	/** test end**/

    if((retry > 5) && (!ChassisOFFFlag))
    {
        if(IsFeatureEnabled("CONFIG_SPX_FEATURE_TELCO_RSYSLOG_SUPPORT") == 1|| IsFeatureEnabled("CONFIG_SPX_FEATURE_SNMPTRAP_SUPPORT")== 1)
        {
            telco_chassis_alert(BMCInst,CHASSIS_POWER_OFF,CMD_FAIL);
        }
    }
    else
    {
        if(IsFeatureEnabled("CONFIG_SPX_FEATURE_TELCO_RSYSLOG_SUPPORT") == 1|| IsFeatureEnabled("CONFIG_SPX_FEATURE_SNMPTRAP_SUPPORT")== 1)
        {
            telco_chassis_alert(BMCInst,CHASSIS_POWER_OFF,CMD_PASS);
        }
    }

	printf("POWER OFF CHASSIS\n");
#ifdef CONFIG_SPX_FEATURE_GPGPU_SUPPORT
    int status=0;
    if(UpdateGPGPUHostStatus(status)==0)
    {
	    TDBG("update GPGPUHostStatus sucessfully\n");
    }
#endif
    if(g_corefeatures.ssi_support == ENABLED)
    {
        /* Do queue Operational State condition. */
        if (g_SSIHandle[SSICB_QUEUECOND] != NULL)
        {
            ((STATUS(*)(INT8U, INT8U, INT8U, int))g_SSIHandle[SSICB_QUEUECOND]) (DEFAULT_FRU_DEV_ID, COND_POWER_ON, 0, BMCInst);
        }

        LOCK_BMC_SHARED_MEM(BMCInst);
        BMC_GET_SHARED_MEM(BMCInst)->PowerActionInProgress = FALSE;
        UNLOCK_BMC_SHARED_MEM(BMCInst);
    }

    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameter  */
    }
    return 0;
}


void
PDK_SoftOffChassis (int BMCInst)
{
    u8 val;
    hal_t hal;

    //  Below code is to power off
    val = 0;
    hal.func = HAL_DEVICE_WRITE;
    hal.pwrite_buf = &val;
    hal.gpio.pin = GPIO_BMC_PWBTN_OUT_N;
    gpio_write(&hal);
    sleep(5);
    val = 1;
    gpio_write(&hal);

    printf("SOFT POWER OFF CHASSIS\n");

    if((PDK_GetPSGood(BMCInst)) != 0)
    {
        if(IsFeatureEnabled("CONFIG_SPX_FEATURE_TELCO_RSYSLOG_SUPPORT") == 1|| IsFeatureEnabled("CONFIG_SPX_FEATURE_SNMPTRAP_SUPPORT")== 1)
        {
            telco_chassis_alert(BMCInst,CHASSIS_POWER_OFF,CMD_FAIL);
        }
    }
    else
    {
        if(IsFeatureEnabled("CONFIG_SPX_FEATURE_TELCO_RSYSLOG_SUPPORT") == 1|| IsFeatureEnabled("CONFIG_SPX_FEATURE_SNMPTRAP_SUPPORT")== 1)
        {
            telco_chassis_alert(BMCInst,CHASSIS_POWER_OFF,CMD_PASS);
        }
    }

	printf("POWER OFF CHASSIS\n");
#ifdef CONFIG_SPX_FEATURE_GPGPU_SUPPORT
    int status=0;
    if(UpdateGPGPUHostStatus(status)==0)
    {
        TDBG("Update GPGPUHostStatus sucessfully\n");
    }
#endif	
    if(g_corefeatures.ssi_support == ENABLED)
    {
        /* Do queue Operational State condition. */
        if (g_SSIHandle[SSICB_QUEUECOND] != NULL)
        {
           ((STATUS(*)(INT8U, INT8U, INT8U, int))g_SSIHandle[SSICB_QUEUECOND]) (DEFAULT_FRU_DEV_ID, COND_POWER_ON, 0, BMCInst);
        }

        LOCK_BMC_SHARED_MEM(BMCInst);
        BMC_GET_SHARED_MEM(BMCInst)->PowerActionInProgress = FALSE;
        UNLOCK_BMC_SHARED_MEM(BMCInst);
   }

    return;
}
/*--------------------------------------------------------------------
 * @fn PDK_PowerCycleChassis
 * @brief Power Cycle the chassis.
 * return 	0  Success
 *			-1 if Error
 *--------------------------------------------------------------------*/
int
PDK_PowerCycleChassis (int BMCInst)
{
	int ret = 0;
    int retries = 3;
    BMCInfo_t *pBMCInfo = &g_BMCInfo[BMCInst];
    printf("POWER CYCLE CHASSIS\n");

   	IPMI_DBG_PRINT ("Power Cycle Chassis\n");

    if(IsFeatureEnabled("CONFIG_SPX_FEATURE_TELCO_RSYSLOG_SUPPORT") == 1 || IsFeatureEnabled("CONFIG_SPX_FEATURE_SNMPTRAP_SUPPORT")== 1)
    {
        telco_chassis_alert(BMCInst,CHASSIS_POWER_CYCLE,CMD_WAIT);
    }
    /* Try to power down the host. We continue whether or not it succeeds */
    /* in case the failure actually ends with the system off or otherwise */
    /* stuck.                                                             */
    PDK_PowerOffChassis(BMCInst);

    /* This is for AMI automation Test only. If needed, please remove     */ 
    /*  those and implement for OEM/ODM platform porting.                 */
    retries = 50;
    do
    {
        ret = PDK_GetPSGood(BMCInst);
        if( ret == 1 )
        {
            /* Wait for some time */
            sleep((int)(pBMCInfo->ChassisConfig.PowerCycleInterval));
        }
    } while ((ret == 1) && (retries-- > 0));


    retries = 3;
    /* Try to power up the host.  Try several times if necessary to make */
    /* sure the host comes back up.                                      */
    do
    {
        ret = PDK_PowerOnChassis(BMCInst);
        if( ret < 0 )
            sleep(1);
    } while((ret < 0) && (retries-- > 0));

    if(IsFeatureEnabled("CONFIG_SPX_FEATURE_TELCO_RSYSLOG_SUPPORT") == 1 || IsFeatureEnabled("CONFIG_SPX_FEATURE_SNMPTRAP_SUPPORT")== 1)
    {
        telco_chassis_alert(BMCInst,CHASSIS_POWER_CYCLE,CMD_PASS);
    }
    return ret;

}

/*--------------------------------------------------------------------
 * @fn PDK_ResetChassis
 * @brief Reset the chassis.
 * return 	0  Success
 *			-1 if Error
 *--------------------------------------------------------------------*/
int
PDK_ResetChassis (int BMCInst)
{
    //u8 val;
    u8 delay = 1;
    //hal_t hal;

    UN_USED(BMCInst);  /*  -Wextra, fix for unused parameter  */
   
    ChassisPowerControl(GPIO_BMC_RSTBTN_OUT_N, delay);

    if(IsFeatureEnabled("CONFIG_SPX_FEATURE_TELCO_RSYSLOG_SUPPORT") == 1 || IsFeatureEnabled("CONFIG_SPX_FEATURE_SNMPTRAP_SUPPORT")== 1)
    {
        telco_chassis_alert(BMCInst,CHASSIS_POWER_RESET,CMD_PASS);
    }
    printf("RESET CHASSIS\n");
    return 0;
}

/*--------------------------------------------------------------------
 * @fn PDK_DiagInterruptChassis
 * @brief Raises the diag interrupt of the chassis.
 * return 	0  Success
 *			-1 if Error
 *--------------------------------------------------------------------*/
int
PDK_DiagInterruptChassis (int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameter  */
    }
    return 0;
}

/*--------------------------------------------------------------------
 * @fn PDK_SMIInterruptChassis
 * @brief Raises the SMI interrupt of the chassis.
 * return 	0  Success
 *			-1 if Error
 *--------------------------------------------------------------------*/
int
PDK_SMIInterruptChassis (int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameter  */
    }
    return 0;
}

/*---------------------------------------------------------------------------
 * @fn PDK_ChassisIdentify
 *
 * @brief This function is invoked in response to chassis identify command.
 * The default implementation turns on / off the chassis identify LED.
 *
 * @param Timeout 	- Chassis Identify timeout
 *---------------------------------------------------------------------------*/
void
PDK_ChassisIdentify (INT8U Force, INT8U Timeout, int BMCInst)
{
	 void *dl_handle = NULL;
	  INT8U (*dl_func)(INT8U,INT16U,INT16U,int);
    	  if(0)
    	  {
              Force=Force;  /*  -Wextra, fix for unused parameter  */
    	  }

	  dl_handle = NULL;
	  dl_handle = dlopen("/usr/local/lib/libipmistack.so", RTLD_LAZY);
	  if(NULL == dl_handle)
	  {
	    IPMI_ERROR("Error in loading libipmistack library %s\n",dlerror());
	    return;
	  }
	  dl_func =  dlsym(dl_handle,"GlowLED");
	  if(NULL == dl_func)
	  {
	    IPMI_ERROR("Error in getting symbol %s \n", dlerror());
	    dlclose(dl_handle);
	    return;
	  }

	  if(Timeout != 0)
	    dl_func(0,0xFF,Timeout,BMCInst);
	  else
	    dl_func(0,0,Timeout,BMCInst);

	if(dl_handle)
	{
		dlclose(dl_handle);
		dl_handle = NULL;
	}
    return;
}

/*---------------------------------------------------------------------
 * @fn PDK_FPEnable
 *
 * @brief This control function is called to enable disable front panel buttons
 *
 * @param   FPEnable - Front Panel enable
 *
 * @return  0x00        - Success
 *          0xff        - Failed.
 *----------------------------------------------------------------------*/
int
PDK_FPEnable (INT8U FPEnable,int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameters  */
	FPEnable=FPEnable;
    }
    return 0;
}

/*--------------------------------------------------------------------
 * @fn PDK_GetPSGood
 * @brief Returns the current PS_GOOD status of the power supply.
 * return 	1  PS_GOOD is asserted.
 *			0 PS_GOOD is not asserted.
 *--------------------------------------------------------------------*/
int
PDK_GetPSGood (int BMCInst)
{
	/** AMI DC and AC cycle stress test begin **/	
	u8 val_GPIO_PS_PWROK;
	hal_t hal;
    	if(0)
    	{
            BMCInst=BMCInst;  /*  -Wextra, fix for unused parameter  */
    	}
	val_GPIO_PS_PWROK = 0;
	hal.func = HAL_DEVICE_READ;
	hal.pread_buf = &val_GPIO_PS_PWROK;
	hal.gpio.pin = GPIO_PS_PWROK;
	if( !IsCardInFlashMode() || ( g_corefeatures.online_flashing_support && IsCardInFlashMode() ))
	    gpio_read(&hal);
	
	if(val_GPIO_PS_PWROK){
	  return 1;
	}
	else{
	  return 0;
	  
	}
	/** test end**/
}

/*------------------------------------------------------------------------
 * @fn PDK_SwitchEMPMux
 * @brief Switch the EMP port MUX.
 * @param Direction
 *					MUX_HOST	Switch the mux to Host.
 *					MUX_BMC		Switch the mux to BMC.
 *					MUX_SOL		Switch the mux for Serial Over LAN.
 * @return 	0	Success
 *			-1  Failed.
 *------------------------------------------------------------------------*/
void
PDK_SwitchEMPMux (INT8U Direction,int BMCInst)
{
	_FAR_ BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
	INT8U TSettings=0x00;
	
	TSettings = BMC_GET_SHARED_MEM (BMCInst)->SerialMuxSetting;
	if(Direction==pBMCInfo->SerialConfig.CurSwitchDir)return;
		
	switch(Direction)
	{
#if defined CONFIG_SPX_FEATURE_SOL_EXTERNAL_SUPERIO
		hal_t hal;
		case MUX_2_BMC:
		{
			pBMCInfo->SerialConfig.CurSwitchDir=MUX_2_BMC;
			TSettings = (TSettings | SET_MUX_TO_BMC_MASK);
			goto default_routing;
		}
		
		case MUX_2_SYS:
		{
			// disable SOL
			OEM_ReadWritePLD(PLD_ID_BaseBoard, CPLD_B_SOL_CTRL, 1, NULL, PLD_WriteRegister);
			pBMCInfo->SerialConfig.CurSwitchDir=MUX_2_SYS;
			TSettings = (TSettings & SET_MUX_TO_SYS_MASK);
			goto default_routing;
		}
			
		case MUX_2_SOL:
		{
			// enable SOL
			OEM_ReadWritePLD(PLD_ID_BaseBoard, CPLD_B_SOL_CTRL, 0, NULL, PLD_WriteRegister);
default_routing:
			//UART1 TO COM1, and COM1 TO UART1
			hal.lpcuart.source_port_type = HAL_SOURCE_PORT_UART;
			hal.lpcuart.source_port = UART1;
			hal.lpcuart.destination_port = IO1;
			lpcuart_route_set(&hal);
	
			hal.lpcuart.source_port_type = HAL_SOURCE_PORT_COM;
			hal.lpcuart.source_port = IO1;
			hal.lpcuart.destination_port = UART1;
			lpcuart_route_set(&hal);
			
			//UART2 TO COM2, and COM2 TO UART2
			hal.lpcuart.source_port_type = HAL_SOURCE_PORT_UART;
			hal.lpcuart.source_port = UART2;
			hal.lpcuart.destination_port = IO2;
			lpcuart_route_set(&hal);
	
			hal.lpcuart.source_port_type = HAL_SOURCE_PORT_COM;
			hal.lpcuart.source_port = IO2;
			hal.lpcuart.destination_port = UART2;
			lpcuart_route_set(&hal);
			
			//UART3 TO COM3, and COM3 TO UART3
			hal.lpcuart.source_port_type = HAL_SOURCE_PORT_UART;
			hal.lpcuart.source_port = UART3;
			hal.lpcuart.destination_port = IO3;
			lpcuart_route_set(&hal);
	
			hal.lpcuart.source_port_type = HAL_SOURCE_PORT_COM;
			hal.lpcuart.source_port = IO3;
			hal.lpcuart.destination_port = UART3;
			lpcuart_route_set(&hal);
			
			//UART4 TO COM4, and COM4 TO UART4
			hal.lpcuart.source_port_type = HAL_SOURCE_PORT_UART;
			hal.lpcuart.source_port = UART4;
			hal.lpcuart.destination_port = IO4;
			lpcuart_route_set(&hal);
	
			hal.lpcuart.source_port_type = HAL_SOURCE_PORT_COM;
			hal.lpcuart.source_port = IO4;
			hal.lpcuart.destination_port = UART4;
			lpcuart_route_set(&hal);
		}
		break;
#elif defined CONFIG_SPX_FEATURE_SOL_INTERNAL_SUPERIO
		hal_t hal;
		case MUX_2_BMC:
		{
			//Not support in this situation.
			printf("[ERROR]%d: Current SOL/Serial Port Sharing does not support MUX to BMC.\n", __LINE__);
		}
		break;
		
		case MUX_2_SYS:
		{
			//UART1 TO COM1, and COM1 TO UART1
			hal.lpcuart.source_port_type = HAL_SOURCE_PORT_UART;
			hal.lpcuart.source_port = UART1;
			hal.lpcuart.destination_port = IO1;
			lpcuart_route_set(&hal);
	
			hal.lpcuart.source_port_type = HAL_SOURCE_PORT_COM;
			hal.lpcuart.source_port = IO1;
			hal.lpcuart.destination_port = UART1;
			lpcuart_route_set(&hal);
			
			pBMCInfo->SerialConfig.CurSwitchDir=MUX_2_SYS;
			TSettings = (TSettings & SET_MUX_TO_SYS_MASK);
		}
		break;
		
		case MUX_2_SOL:
		{
			//UART3 TO COM2, and COM2 TO UART3
			hal.lpcuart.source_port_type = HAL_SOURCE_PORT_BOTH;
			hal.lpcuart.source_port = UART3;
			hal.lpcuart.destination_port = UART2;
			lpcuart_route_set(&hal);

                        hal.lpcuart.source_port_type = HAL_SOURCE_PORT_BOTH;
                        hal.lpcuart.source_port = UART2;
                        hal.lpcuart.destination_port = UART3;
                        lpcuart_route_set(&hal);
			
			//UART4 TO UART1, and UART1 TO UART4			
			hal.lpcuart.source_port_type = HAL_SOURCE_PORT_BOTH;
			hal.lpcuart.source_port = UART4;
			hal.lpcuart.destination_port = UART1;
			lpcuart_route_set(&hal);

                        hal.lpcuart.source_port_type = HAL_SOURCE_PORT_BOTH;
                        hal.lpcuart.source_port = UART1;
                        hal.lpcuart.destination_port = UART4;
                        lpcuart_route_set(&hal);
		}
		break;
#elif defined CONFIG_SPX_FEATURE_SOL_INTERNAL_SUPERIO_SERIAL_PORT_SHARING
		hal_t hal;
		
		case MUX_2_BMC:
		{
			//UART3 TO COM3, and COM3 TO UART3
			hal.lpcuart.source_port_type = HAL_SOURCE_PORT_UART;
			hal.lpcuart.source_port = UART3;
			hal.lpcuart.destination_port = IO3;
			lpcuart_route_set(&hal);

			hal.lpcuart.source_port_type = HAL_SOURCE_PORT_COM;
			hal.lpcuart.source_port = IO3;
			hal.lpcuart.destination_port = UART3;
			lpcuart_route_set(&hal);
			
			//UART1 TO COM1, and COM1 TO UART1
			hal.lpcuart.source_port_type = HAL_SOURCE_PORT_UART;
			hal.lpcuart.source_port = UART1;
			hal.lpcuart.destination_port = IO1;
			lpcuart_route_set(&hal);
	
			hal.lpcuart.source_port_type = HAL_SOURCE_PORT_COM;
			hal.lpcuart.source_port = IO1;
			hal.lpcuart.destination_port = UART1;
			lpcuart_route_set(&hal);
			
			pBMCInfo->SerialConfig.CurSwitchDir=MUX_2_BMC;
			TSettings = (TSettings | SET_MUX_TO_BMC_MASK);
		}
		break;
		
		case MUX_2_SYS:
		{
			//COM3  TO UART4
			hal.lpcuart.source_port_type = HAL_SOURCE_PORT_COM;
			hal.lpcuart.source_port = IO3;
			hal.lpcuart.destination_port = UART4;
			lpcuart_route_set(&hal);
			
			//UART4 TO UART1
			hal.lpcuart.source_port_type = HAL_SOURCE_PORT_BOTH;
			hal.lpcuart.source_port = UART4;
			hal.lpcuart.destination_port = UART1;
			lpcuart_route_set(&hal);
			
			//UART1 TO COM3
			hal.lpcuart.source_port_type = HAL_SOURCE_PORT_UART;
			hal.lpcuart.source_port = UART1;
			hal.lpcuart.destination_port = IO3;
			lpcuart_route_set(&hal);
			
			pBMCInfo->SerialConfig.CurSwitchDir=MUX_2_SYS;
			TSettings = (TSettings & SET_MUX_TO_SYS_MASK);
		}
		break;
		
		case MUX_2_SOL:
		{
			//UART2 TO UART4, and UART4 TO UART2
			hal.lpcuart.source_port_type = HAL_SOURCE_PORT_BOTH;
			hal.lpcuart.source_port = UART2;
			hal.lpcuart.destination_port = UART4;
			lpcuart_route_set(&hal);
						
			hal.lpcuart.source_port_type = HAL_SOURCE_PORT_BOTH;
			hal.lpcuart.source_port = UART4;
			hal.lpcuart.destination_port = UART2;
			lpcuart_route_set(&hal);
		}
		break;
#else
		hal_t hal;
		case MUX_2_BMC: // terminal mode started
		{
			hal.lpcuart.source_port_type = HAL_SOURCE_PORT_UART;
			hal.lpcuart.source_port = UART1;
			hal.lpcuart.destination_port = IO1;
			lpcuart_route_set(&hal);
	
			hal.lpcuart.source_port_type = HAL_SOURCE_PORT_COM;
			hal.lpcuart.source_port = IO1;
			hal.lpcuart.destination_port = UART1;
			lpcuart_route_set(&hal);
	
			hal.lpcuart.source_port_type = HAL_SOURCE_PORT_UART;
			hal.lpcuart.source_port = UART3;
			hal.lpcuart.destination_port = IO3;
			lpcuart_route_set(&hal);
	
			hal.lpcuart.source_port_type = HAL_SOURCE_PORT_COM;
			hal.lpcuart.source_port = IO3;
			hal.lpcuart.destination_port = UART3;
			lpcuart_route_set(&hal);
	
			pBMCInfo->SerialConfig.CurSwitchDir=MUX_2_BMC;
			TSettings = (TSettings | SET_MUX_TO_BMC_MASK);
	
		}
		break;
	
		case MUX_2_SYS: // terminal mode stopped
		{
			hal.lpcuart.source_port_type = HAL_SOURCE_PORT_UART;
			hal.lpcuart.source_port = UART1;
			hal.lpcuart.destination_port = IO3;
			lpcuart_route_set(&hal);
	
			hal.lpcuart.source_port_type = HAL_SOURCE_PORT_COM;
			hal.lpcuart.source_port = IO3;
			hal.lpcuart.destination_port = UART3;
			lpcuart_route_set(&hal);
	
			hal.lpcuart.source_port_type = HAL_SOURCE_PORT_BOTH;
			hal.lpcuart.source_port = UART3;
			hal.lpcuart.destination_port = UART1;
			lpcuart_route_set(&hal);
	
			pBMCInfo->SerialConfig.CurSwitchDir=MUX_2_SYS;
			TSettings = (TSettings & SET_MUX_TO_SYS_MASK);
		}
		break;
		
		case MUX_2_SOL:
		{
			//TODO
		}
		break;
#endif		
		default:
		break;
	}
	
	BMC_GET_SHARED_MEM (BMCInst)->SerialMuxSetting = TSettings;
	return;
}

/*---------------------------------------------------------------------------
 * @fn PDK_AssertSOLCTS
 *
 * @brief This function is invoked to assert SOL serial port CTS signal.
 *
 *---------------------------------------------------------------------------*/
void
PDK_AssertSOLCTS (int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameter  */
    }
    return;
}

/*---------------------------------------------------------------------------
 * @fn PDK_AssertSOLCTS
 *
 * @brief This function is invoked to deassert SOL serial port CTS signal.
 *
 *---------------------------------------------------------------------------*/
void
PDK_DeAssertSOLCTS (int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameter  */
    }
     return;
}

/*--------------------------------------------------------------------------
 * @fn	PDK_GenerateBeep
 * @brief Produces a beep sound.
 * @return		0  	Success.
 *				-1	Failed.
 *--------------------------------------------------------------------------*/
void
PDK_GenerateBeep (INT8U IsOn, int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameters  */
	IsOn=IsOn;
    }
    return;
}

/*--------------------------------------------------------------------------
 * @fn	PDK_ClearCMOS
 * @brief Clears the CMOS contents
 * @return		0  	Success.
 *				-1	Failed.
 *--------------------------------------------------------------------------*/

void
PDK_ClearCMOS (int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameter  */
    }
    return;
}

void PDK_FanControl(int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameter  */
    }
    return;
}

int PDK_HandleChassisInterrupts(int fdesc,int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameters  */
	fdesc=fdesc;
    }
    return -1;
}

/* This PDK hook returns the handle to the chassis interrupt driver. 
The same handle is used for subsequent PDK calls that handle the 
chassis interrupts */
int PDK_RegisterChassisInterrupts()
{
	return 0;
}

/* This PDK hook returns 0, if success 
					    -1, if error */
int PDK_ProcessInterruptSensor(interrupt_sensor_info sensor,int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameters  */
	sensor=sensor;
    }
    return 0;
}

int PDK_HandleInterruptSensors(int fdesc,int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameters  */
	fdesc=fdesc;
    }
    return -1;
}

/* This PDK hook returns the handle to the interrupt sensors driver. 
The same handle is used for subsequent PDK calls that handle the 
interrupt sensors */
int PDK_RegisterInterruptSensors(interrupt_sensor_info *sensor_data,unsigned int num_interrupt_sensors, int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameters  */
	sensor_data=sensor_data;
	num_interrupt_sensors=num_interrupt_sensors;	
    }
    return 0;
}

/*---------------------------------------------------------------------------------------------
 * @fn	PDK_PowerManagementCapability
 * @brief Sets the Power Management Capability option
 * @params	
 *  * @return	1  	Success.
 *		0	Failed.
 *----------------------------------------------------------------------------------------------*/
int
PDK_PowerManagementCapability(int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameter  */
    }
    return 1;
}



/*---------------------------------------------------------------------------------------------
 * @fn	PDK_SetPowerLimit
 * @brief Sets the power Limit to the DCM server
 * @params	ExceptionAction 		- Action when Set power limit can not be maintained
 *	        PwrLimitInWatts 		- Power Limit in Watts
 *		CorrectionTimeLimitInMsec	- Upper Limit for correction of power   
 * @return	0  	Success.
 *		-1	Failed.
 *----------------------------------------------------------------------------------------------*/
int 
PDK_SetPowerLimit (INT8U ExceptionAction, INT16U PwrLimitInWatts, INT32U CorrectionTimeLimitInMsec,int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameters  */	
	ExceptionAction=ExceptionAction;
	PwrLimitInWatts=PwrLimitInWatts;
	CorrectionTimeLimitInMsec=CorrectionTimeLimitInMsec;
    }
    return 0;
}

/*--------------------------------------------------------------------------
 * @fn	PDK_ActivatePowerLimit
 * @brief Activates the power Limit to the DCM server
 * @params	Activate - The values 0 and 1
 *		0 to de-activate and 1 to activate the power limit to DCM server.
 * @return	0  	Success.
 *		-1	Failed.
 *--------------------------------------------------------------------------*/
int 
PDK_ActivatePowerLimit (INT8U Activate, int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameters  */
	Activate=Activate;
    }
    return 0;
}

/*--------------------------------------------------------------------------
 * @fn	PDK_ReadServerPowerUsage
 * @brief Read the current power usage 
 * @params	pCurPwrInWatts - Current Power consumption by DCM server.
 *						   
 * @return	0  	Success.
 *		-1	Failed.
 *--------------------------------------------------------------------------*/
int
PDK_ReadServerPowerUsage  (INT16U* pCurPwrInWatts, int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameters  */
	pCurPwrInWatts=pCurPwrInWatts;
    }
    return 0;
}

/*--------------------------------------------------------------------------
 * @fn    PDK_ReadSlotID
 * @brief Read the slot ID of blade.
 *						   
 * @return	0  	Success.
 *         -1   Failed.
 *--------------------------------------------------------------------------*/
INT8U
PDK_ReadSlotID(void)
{
    return 0;
}

/*--------------------------------------------------------------------------
 * @fn    PDK_GetSlotID
 * @brief Return the slot ID of blade.
 *						   
 * @return	0  	Success.
 *         -1   Failed.
 *--------------------------------------------------------------------------*/
INT8U
PDK_GetSlotID(int BMCInst)
{
    BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];
	
    return pBMCInfo->SlotID;
}

/*--------------------------------------------------------------------------
 * @fn    PDK_GetPhysicalSlotNum
 * @brief Return the physical slot number.
 *						   
 * @return	0  	Success.
 *         -1   Failed.
 *--------------------------------------------------------------------------*/
INT8U
PDK_GetPhysicalSlotNum(int BMCInst)
{
    BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];

    return (INT8U)(pBMCInfo->SlotID + 1);
}

/*--------------------------------------------------------------------------
 * @fn    PDK_GenerateSlotBasedIPMBAddr
 * @brief Generate the IPMB address to blade based on slot ID.
 *						   
 * @return	0  	Success.
 *         -1   Failed.
 *--------------------------------------------------------------------------*/
INT8U
PDK_GenerateSlotBasedIPMBAddr(int BMCInst)
{
    BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];

    /*  Read the Slot ID and calculate the IPMB address */
    pBMCInfo->SlotID = PDK_ReadSlotID();
    pBMCInfo->IPMBAddr = 0x30 + (pBMCInfo->SlotID * 2);

    return pBMCInfo->IPMBAddr;
}

/*--------------------------------------------------------------------------
 * @fn    PDK_GetSlotBasedIPMBAddr
 * @brief Return the IPMB address to blade based on slot ID.
 *						   
 * @return	0  	Success.
 *         -1   Failed.
 *--------------------------------------------------------------------------*/
INT8U
PDK_GetSlotBasedIPMBAddr(int BMCInst)
{
    BMCInfo_t* pBMCInfo = &g_BMCInfo[BMCInst];

    return pBMCInfo->IPMBAddr;
}

/*-----------------------------------------------------------------
 * @fn PDK_IsPowerOnReset
 * @brief This is provided to implement the platform specific
 *        Is Power On Reset detection for Operational State Machine.
 *
 * @return TRUE  - Success.
 *         FALSE - Failed.
 *-----------------------------------------------------------------*/
BOOL
PDK_IsPowerOnReset(int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameter  */
    }
    return FALSE;
}

/*-----------------------------------------------------------------
 * @fn PDK_SetPowerLevel
 * @brief This is provided to implement the platform specific
 *        Set Power Level procedure for Operational State Machine.
 *
 * @param pFruObj  - Pointer of Operational State FRU object. 
 * @param NewLevel - New Power Level value being granted.
 *
 * @return ST_OK            - Success.
 *         ST_INVALID_PARAM - Invalid parameter.
 *         ST_ERROR         - Error.
 *-----------------------------------------------------------------*/
STATUS
PDK_SetPowerLevel(OpStateFruObj_T *pFruObj, INT8U NewLevel, int BMCInst)
{
    STATUS Status = ST_OK;

    /* Currently only support a single discrete Power Level */

    if (pFruObj->CurrentPowerLevel == NewLevel)
    {
        IPMI_INFO("%s: Already at requested power level 0x%02X\n", __FUNCTION__, pFruObj->CurrentPowerLevel);

        if (PDK_GetPSGood(BMCInst))
        {
            /* This handles the case where if for some reason we are in M3 and already powered on.
               This will take us to M4. */
            if (g_SSIHandle[SSICB_QUEUECOND] != NULL)
            {
                ((STATUS(*)(INT8U, INT8U, INT8U, int))g_SSIHandle[SSICB_QUEUECOND]) (DEFAULT_FRU_DEV_ID, COND_POWER_ON, 1, BMCInst);
            }
        }
        else
        {
            /* To be complete, also handle the case where power is off. */
            if (g_SSIHandle[SSICB_QUEUECOND] != NULL)
            {
                ((STATUS(*)(INT8U, INT8U, INT8U, int))g_SSIHandle[SSICB_QUEUECOND]) (DEFAULT_FRU_DEV_ID, COND_POWER_ON, 0, BMCInst);
            }
        }

        return Status;  /* Doing nothing */
    }

    if (NewLevel == 0x00)
    {
        if (!PDK_GetPSGood(BMCInst)) /* OK to Power Off? */
        {
            Status = ST_ERROR;
        }
        else
        {
            LOCK_BMC_SHARED_MEM(BMCInst);
            BMC_GET_SHARED_MEM(BMCInst)->PowerActionInProgress = TRUE;
            UNLOCK_BMC_SHARED_MEM(BMCInst);

            /* Perform the same actions as Chassis Control command (CHASSIS_POWER_DOWN).
               This is not a soft shutdown via OS.  This is hard off. */
            Platform_HostPowerOff(BMCInst);

            /* Clear the power budget condition.
               This should get it to transition out of it's current state (M4 or M5) to M6. */
            pFruObj->CurrentPowerLevel = NewLevel;
        }
    }
    else if (NewLevel == GetPowerLevel())
    {
        if (PDK_GetPSGood(BMCInst)) /* OK to Power On? */
        {
            Status = ST_ERROR;
        }
        else
        {
            LOCK_BMC_SHARED_MEM(BMCInst);
            BMC_GET_SHARED_MEM(BMCInst)->PowerActionInProgress = TRUE;
            UNLOCK_BMC_SHARED_MEM(BMCInst);

            /* Perform the same actions as Chassis Control command (CHASSIS_POWER_UP). */
            Platform_HostPowerOn(BMCInst);

            pFruObj->CurrentPowerLevel = NewLevel;
        }
    }
    else
    {
        Status = ST_INVALID_PARAM; 		
    }

    return Status;
}

/*-----------------------------------------------------------------
 * @fn PDK_RenegotiationPower
 * @brief This is provided to implement the platform specific
 *        Renegotiation Power procedure for Operational State Machine.
 *
 * @param pFruObj  - Pointer of Operational State FRU object. 
 *
 * @return ST_OK    - Success.
 *         ST_ERROR - Error.
 *-----------------------------------------------------------------*/
STATUS
PDK_RenegotiationPower(OpStateFruObj_T *pFruObj, int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameters  */
	pFruObj=pFruObj;
    }
    return ST_OK;
}

/*-----------------------------------------------------------------
 * @fn PDK_PowerWakeUp
 * @brief This is provided to implement the platform specific
 *        Power Wake Up procedure for Operational State Machine.
 *
 * @return None.
 *-----------------------------------------------------------------*/
void
PDK_PowerWakeUp(int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameter  */
    }
    return;
}



/*---------------------------------------------------------------------------------------------
 * @fn	PDK_SetThermalLimit
 * @brief Sets the Thermal Limit to the DCM server
 * @params	ExceptionAction 		- Action when Set Temp limit can not be maintained
 *                  TempLimit 		- Temperature Limit
 *	              ExceptionTime 	- ExceptionTime interval   
 *                  EntityInst              - EntityInstance of Inlet Temperature Sensor
 * @return	0  	Success.
 *		-1	Failed.
 *----------------------------------------------------------------------------------------------*/
int PDK_SetThermalLimit(INT8U ExceptionAction,INT8U TempLimit,INT16U ExceptionTime,INT8U EntityInst,int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameters  */
	ExceptionAction=ExceptionAction;
	TempLimit=TempLimit;
	ExceptionTime=ExceptionTime;
	EntityInst=EntityInst;
    }
    return 0;
}

/*---------------------------------------------------------------------------------------------
 * @fn	PDK_ThermalMonitor
 * @brief Enables the Thermal monitor to the DCM server
 * @params	EnableMonitor 		- Enables the monitor task
 * @return	0  	Success.
 *		-1	Failed.
 *----------------------------------------------------------------------------------------------*/
int PDK_ThermalMonitor(INT8U EnableMonitor,int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameters  */
	EnableMonitor=EnableMonitor;
    }
    return 0;
}

/*---------------------------------------------------------------------------------------------
 * @fn	PDK_GetTempReading
 * @brief Gets the Current Temperature Reading for the given Inlet Entity Instance
 * @params	EntityInst 		- EntityInstance of Inlet Temperature sensor
 *                  CurrentTempReading  - Updates the Current rading
 * @return	0  	Success.
 *		-1	Failed.
 *----------------------------------------------------------------------------------------------*/
int PDK_GetTempReading(INT8U EntityInst, INT8U* CurrentTempReading, int BMCInst)
{
    if(0)
    {
        BMCInst=BMCInst;  /*  -Wextra, fix for unused parameters  */
	CurrentTempReading=CurrentTempReading;
	EntityInst=EntityInst;
    }
    return 0;
}


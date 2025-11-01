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
 * PDKInt.c
 *
 * @brief Handles the OEM defined Interrupts.
 *
 *****************************************************************/
#define ENABLE_DEBUG_MACROS 0


#include "PDKInt.h"
#include "PDKHW.h"
#include "hal_hw.h"
#include "API.h"
#include <dlfcn.h>
#include "gpio.h"
#include "gpioifc.h"
#include "EINTR_wrappers.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#define SYSFS_GPIO_DIR          "/sys/class/gpio"
#define MAX_BUF                 40

#ifdef CONFIG_SPX_FEATURE_GPGPU_SUPPORT
#define BMC_I2C0_FPGA_ALERT_L_I CONFIG_SPX_FEATURE_BMC_I2C0_FPGA_ALERT
#define BMC_I2C1_FPGA_ALERT_L_I CONFIG_SPX_FEATURE_BMC_I2C1_FPGA_ALERT
#define COMP_FPGA_INTERRUPT_INFO_PIPE "/var/FPGA_interrupt_info_PIPE"
void PDK_BMC_I2C0_FPGA_ALERT (IPMI_INTInfo_T *IntInfo);
void PDK_BMC_I2C1_FPGA_ALERT (IPMI_INTInfo_T *IntInfo);
void PDK_BMC_I2C1_FPGA_PGOOD (IPMI_INTInfo_T *IntInfo);
#endif

void PDK_SensorInterruptHandler (IPMI_INTInfo_T *IntInfo);

int gpio_count = 0;

/* Interrupts can be registred manually here or 
     can be done via MDS*/

#define GPIO_INT_SENSOR 0x01

#define GPIO_INT_SENSOR_TYPE    0
#define GPIO_BMC_INT_TEST1      9   //GPIOB1

IPMI_INTInfo_T m_IntInfo [] =
{
 	//{ int_hndlr, int_num, Source, SensorNum, SensorType, TriggerMethod, TriggerType, reading_on_assertion },
    {PDK_SensorInterruptHandler, GPIO_BMC_INT_TEST1, INT_REG_HNDLR, GPIO_INT_SENSOR, GPIO_INT_SENSOR_TYPE, IPMI_INT_TRIGGER_EDGE, IPMI_INT_RISING_EDGE, 0, 0, 0 ,0},
#ifdef CONFIG_SPX_FEATURE_GPGPU_SUPPORT
    {PDK_BMC_I2C0_FPGA_ALERT, BMC_I2C0_FPGA_ALERT_L_I, INT_REG_HNDLR, GPIO_INT_SENSOR,GPIO_INT_SENSOR_TYPE,IPMI_INT_TRIGGER_LEVEL, IPMI_INT_HIGH_LEVEL, 0, 0, 0 ,0},
    {PDK_BMC_I2C1_FPGA_PGOOD, BMC_I2C1_FPGA_ALERT_L_I, INT_REG_HNDLR, GPIO_INT_SENSOR,GPIO_INT_SENSOR_TYPE,IPMI_INT_TRIGGER_LEVEL, IPMI_INT_HIGH_LEVEL, 0, 0, 0 ,0},
    {PDK_BMC_I2C1_FPGA_ALERT, BMC_I2C1_FPGA_ALERT_L_I, INT_REG_HNDLR, GPIO_INT_SENSOR,GPIO_INT_SENSOR_TYPE,IPMI_INT_TRIGGER_LEVEL, IPMI_INT_LOW_LEVEL, 0, 0, 0 ,0}
#endif
};

int m_IntInfoCount = (sizeof(m_IntInfo)/sizeof(m_IntInfo[0]));

static struct pollfd 	fd_to_watch [IPMI_MAX_INT_FDS];	/* File descriptor to watch for interrupt */
static int	   			m_total_reg_fds = 0;

void PDK_SensorInterruptHandler (IPMI_INTInfo_T *IntInfo)
{
    switch(IntInfo->int_num)
    { 
		case GPIO_BMC_INT_TEST1:
			printf("GPIO_INT_SENSOR\n");
			break;
		default:
			break;
    }
    return;
}

/*-------------------------------------------------------------------------
 * @fn PDK_RegGPIOInts
 * @brief This function is called by the core Interrupt Task initialization
 *------------------------------------------------------------------------*/
int
PDK_RegGPIOInts(int fd, int BMCInst, int pinNum[IPMI_MAX_INT_FDS], int **pCount, int gpio_base)
{
    UN_USED(BMCInst);

    int i = 0, ret = 0;
    interrupt_sensor_info gpio_intr[MAX_IPMI_INT] ={0};
    int (*pRegisterInt) (interrupt_sensor_info *,unsigned int, int);
    int (*pUnRegisterInt) (int, interrupt_sensor_info *);
    char commandStrBuf[MAX_BUF] = {0};

    (*pCount) = &m_total_reg_fds;

    void *lib_handle = dlopen(GPIO_LIB, RTLD_NOW);
    if (lib_handle == NULL)
        return -1;
   
    pRegisterInt = dlsym(lib_handle,"register_sensor_interrupts");
    pUnRegisterInt = dlsym(lib_handle,"unregister_sensor_interrupts");
    if (NULL == pRegisterInt || NULL == pUnRegisterInt)
    {
        dlclose(lib_handle);
	lib_handle = NULL;
        return -1;
    }
    if (m_IntInfoCount >= MAX_IPMI_INT)
    {
        /*Not enough space */
        IPMI_WARNING ("Not enough space for INT registration\n");
        dlclose(lib_handle);
	lib_handle = NULL;
        return -1;
    }

	if(0 >= m_IntInfoCount)
	{
	    /*No Interrupts sensor in m_IntInfo table  */
        TDBG("No Interrupts sensor in m_IntInfo table\n");
        dlclose(lib_handle);
	lib_handle = NULL;
	return -1;
	}

    for (i = 0; i < m_IntInfoCount; i++)
    {
	    /* Reason for False Positive –  IPMI_INTInfo_T structure array contains one interrupt info only. So i value is always 1 as of now. if any other interruptdetails are added then i value will vary according to no of interrupts in the structure. So there is no issue with existed code. */
	    /* coverity[overrun-local : FALSE] */
	if (INT_SWC_HNDLR != m_IntInfo[i].Source)
        {
		/* Reason for False Positive –  IPMI_INTInfo_T structure array contains one interrupt info only. So i value is     always 1 as of now. if any other interruptdetails are added then i value will vary according to no of interrupts in the structure. So there is no issue with existed code. */
		/* coverity[overrun-local : FALSE] */
	       ret = snprintf(commandStrBuf, sizeof(commandStrBuf),SYSFS_GPIO_DIR"/gpio%d/value",(gpio_base+m_IntInfo[i].int_num));
            if((ret < 0)||(ret >= (signed int)sizeof(commandStrBuf)))
            {
                IPMI_WARNING ("Buffr Overflow");
                dlclose(lib_handle);
		lib_handle = NULL;
                return -1;
            }

            if ((access(commandStrBuf, F_OK) == 0) && gpio_intr[m_total_reg_fds].flag)
            {
                //Cleanup any previous interrupt registration
                if (0 != pUnRegisterInt(fd, &gpio_intr[m_total_reg_fds]))
                {
                    IPMI_WARNING ("No old interrupts need to be registered to clear\n");
                }
            }

            /* Check if this registration is for GPIO driver */
            /* Registration specific for GPIO */
/* Reason for False Positive –  IPMI_INTInfo_T structure array contains one interrupt info only. So i value is always 1 as of now. if any other interruptdetails are added then i value will vary according to no of interrupts in the structure. So there is no issue with existed code. */
            /* coverity[overrun-local : FALSE] */
            gpio_intr[m_total_reg_fds].int_num            = m_IntInfo[i].int_num;

/* Reason for False Positive –  IPMI_INTInfo_T structure array contains one interrupt info only. So i value is always 1 as of now. if any other interruptdetails are added then i value will vary according to no of interrupts in the structure. So there is no issue with existed code. */
            /* coverity[overrun-local : FALSE] */
            gpio_intr[m_total_reg_fds].gpio_number        = m_IntInfo[i].int_num;

/* Reason for False Positive –  IPMI_INTInfo_T structure array contains one interrupt info only. So i value is always 1 as of now. if any other interruptdetails are added then i value will vary according to no of interrupts in the structure. So there is no issue with existed code. */
            /* coverity[overrun-local : FALSE] */
            gpio_intr[m_total_reg_fds].sensor_number      = m_IntInfo[i].SensorNum;

/* Reason for False Positive –  IPMI_INTInfo_T structure array contains one interrupt info only. So i value is always 1 as of now. if any other interruptdetails are added then i value will vary according to no of interrupts in the structure. So there is no issue with existed code. */
            /* coverity[overrun-local : FALSE] */
            gpio_intr[m_total_reg_fds].trigger_method     = (enum _trigger_method_info) m_IntInfo[i].TriggerMethod;

/* Reason for False Positive –  IPMI_INTInfo_T structure array contains one interrupt info only. So i value is always 1 as of now. if any other interruptdetails are added then i value will vary according to no of interrupts in the structure. So there is no issue with existed code. */
            /* coverity[overrun-local : FALSE] */
            gpio_intr[m_total_reg_fds].trigger_type       = (enum _trigger_type_info) m_IntInfo[i].TriggerType;

/* Reason for False Positive –  IPMI_INTInfo_T structure array contains one interrupt info only. So i value is always 1 as of now. if any other interruptdetails are added then i value will vary according to no of interrupts in the structure. So there is no issue with existed code. */
            /* coverity[overrun-local : FALSE] */
            pinNum[m_total_reg_fds] = m_IntInfo[i].int_num;

            /* Register GPIO interrupts */
            if ( -1 == pRegisterInt (&gpio_intr[m_total_reg_fds], m_total_reg_fds, fd) )
            {
                IPMI_WARNING ("GPIO Interrupt registration failed \n");
                dlclose(lib_handle);
		        lib_handle = NULL;
                return -1;
            }
            else
            {
                TDBG("Registering sensor interrupt success\n");
            }
            gpio_intr[m_total_reg_fds].flag = 1;
            m_total_reg_fds++;
        }
    }
    dlclose(lib_handle);
    lib_handle = NULL;
    return 0;
}

/*-----------------------------------------------------------------
 * @fn PDK_BMC_I2C0_FPGA_ALERT
 * @brief This is provide I2C0 Interrupt alert to GPGPU
 *
 * @return None.
 *-----------------------------------------------------------------*/
#ifdef CONFIG_SPX_FEATURE_GPGPU_SUPPORT
void PDK_BMC_I2C0_FPGA_ALERT(IPMI_INTInfo_T *IntInfo)
{
    int comp_pipe_fd = 0;

    u8 i2c_interrupt_line = 1;

    struct stat buf;

    memset(&buf, 0, sizeof(buf));

    if (0)
    {
        IntInfo = IntInfo; /* -Wextra, fix for unused parameters */
    }

    if(stat(COMP_FPGA_INTERRUPT_INFO_PIPE, &buf) == -1)
    {
        TCRIT("%s is not available\n", COMP_FPGA_INTERRUPT_INFO_PIPE);
        return;
    }

    comp_pipe_fd = sigwrap_open(COMP_FPGA_INTERRUPT_INFO_PIPE, O_WRONLY);

    if(comp_pipe_fd == -1)
    {
        TCRIT("Opening a %s is failed\n", COMP_FPGA_INTERRUPT_INFO_PIPE);
        return;
    }

    if(sigwrap_write(comp_pipe_fd, &i2c_interrupt_line, sizeof(i2c_interrupt_line)) == -1)
    {
        TCRIT("Writing into %s is failed\n", COMP_FPGA_INTERRUPT_INFO_PIPE);
    }

    if(sigwrap_close(comp_pipe_fd) == -1)
    {
        TCRIT("Closing %s is failed\n", COMP_FPGA_INTERRUPT_INFO_PIPE);
    }
    return;
}
void PDK_BMC_I2C1_FPGA_PGOOD(IPMI_INTInfo_T *IntInfo)
{
    int comp_pipe_fd = 0;

    u8 i2c_interrupt_line = 2;

    struct stat buf;

    memset(&buf, 0, sizeof(buf));

    if (0)
    {
        IntInfo = IntInfo; /* -Wextra, fix for unused parameters */
    }

    if(stat(COMP_FPGA_INTERRUPT_INFO_PIPE, &buf) == -1)
    {
        TCRIT("%s is not available\n", COMP_FPGA_INTERRUPT_INFO_PIPE);
        return;
    }

    comp_pipe_fd = sigwrap_open(COMP_FPGA_INTERRUPT_INFO_PIPE, O_WRONLY);

    if(comp_pipe_fd == -1)
    {
        TCRIT("Opening a %s is failed\n", COMP_FPGA_INTERRUPT_INFO_PIPE);
        return;
    }

    if(sigwrap_write(comp_pipe_fd, &i2c_interrupt_line, sizeof(i2c_interrupt_line)) == -1)
    {
        TCRIT("Writing into %s is failed\n", COMP_FPGA_INTERRUPT_INFO_PIPE);
    }

    if(sigwrap_close(comp_pipe_fd) == -1)
    {
        TCRIT("Closing %s is failed\n", COMP_FPGA_INTERRUPT_INFO_PIPE);
    }
    return;
}
void PDK_BMC_I2C1_FPGA_ALERT(IPMI_INTInfo_T *IntInfo)
{
    int comp_pipe_fd = 0;

    u8 i2c_interrupt_line = 3;

    struct stat buf;

    memset(&buf, 0, sizeof(buf));

    if (0)
    {
        IntInfo = IntInfo; /* -Wextra, fix for unused parameters */
    }

    if(stat(COMP_FPGA_INTERRUPT_INFO_PIPE, &buf) == -1)
    {
        TCRIT("%s is not available\n", COMP_FPGA_INTERRUPT_INFO_PIPE);
        return;
    }

    comp_pipe_fd = sigwrap_open(COMP_FPGA_INTERRUPT_INFO_PIPE, O_WRONLY);

    if(comp_pipe_fd == -1)
    {
        TCRIT("Opening a %s is failed\n", COMP_FPGA_INTERRUPT_INFO_PIPE);
        return;
    }

    if(sigwrap_write(comp_pipe_fd, &i2c_interrupt_line, sizeof(i2c_interrupt_line)) == -1)
    {
        TCRIT("Writing into %s is failed\n", COMP_FPGA_INTERRUPT_INFO_PIPE);
    }

    if(sigwrap_close(comp_pipe_fd) == -1)
    {
        TCRIT("Closing %s is failed\n", COMP_FPGA_INTERRUPT_INFO_PIPE);
    }
    return;
}

#endif


/*-------------------------------------------------------------------------
 * @fn PDK_RegIntFDs
 * @brief This function is called by the core Interrupt Task initialization
 *------------------------------------------------------------------------*/
int
PDK_RegIntFDs ( struct pollfd **pfd, int BMCInst, int ret[IPMI_MAX_INT_FDS], int gpio_desc)
{
    UN_USED(BMCInst);
    UN_USED(gpio_desc);

    int i;

    /* Initialize count */
    (*pfd)              = &fd_to_watch [0];

    /* Initialize all FDs */
    memset (&fd_to_watch[0], 0 , (sizeof (struct pollfd) * IPMI_MAX_INT_FDS));

    for(i = 0;i < m_total_reg_fds; i++)
    {
        fd_to_watch [i].fd        = ret[i];
        fd_to_watch [i].events    = POLLPRI;
    }

    return 0;
}

/*-------------------------------------------------------------------------
 * @fn PDK_CheckForInt
 * @brief This function is called by the porting code for any
 *        interrupt initialization
 *------------------------------------------------------------------------*/
IPMI_INTInfo_T*
PDK_GetIntInfo (int BMCInst, int FdNum)
{
    UN_USED(BMCInst);

    return &m_IntInfo[FdNum];

}

#include "../../include/lib/dbt.h"
#include "../../include/driverHeaders/uart.h"
#include "../../include/driverHeaders/ssd_driver.h"
#include "../../include/string_utility.h"
#include "../../include/driverHeaders/virtIO.h"
#include<stdint.h>
#include<string.h>


void discover_ssd_device(struct hardware_info* out_info) ;


void discover_ssd_device(struct hardware_info* out_info)
{
    uart_puts("\n Hello Guys ! \n");
              //------------------SSD CONTROLLERS ARRAY ----------//
       static const char *ssd_compatibles[] = {
    /* --- Virtual / Generic --- */
    "virtio,mmio",              /* QEMU, KVM, Firecracker */
    "generic-ahci",             /* Fallback for any standard AHCI */

    /* --- NVMe (PCIe SSDs) --- */
    "nvme",                     /* Standard PCIe NVMe */
    "apple,nvme-ans2",          /* Apple Silicon */
    "qcom,ufshc",               /* Qualcomm UFS (often used as boot storage) */

    /* --- UFS (Universal Flash Storage) --- */
    "jedec,ufs-1.1",            /* UFS 1.1 standard */
    "jedec,ufs-2.0",            /* UFS 2.0+ standard (mobile/embedded) */
    "qcom,ufshc",               /* Qualcomm UFS host */

    /* --- SATA / AHCI (Embedded & Server) --- */
    "snps,dwc-ahci",            /* Synopsys DesignWare (most common IP) */
    "snps,spear-ahci",          /* ST Spear */
    "marvell,armada-3700-ahci", /* Marvell Armada */
    "marvell,armada-8k-ahci",   /* Marvell Armada 8K */
    "brcm,sata3-ahci",          /* Broadcom SATA3 */
    "hisilicon,hisi-ahci",      /* HiSilicon */
    "cavium,octeon-7130-ahci",  /* Cavium Octeon */

    /* --- SD/MMC / eMMC (Most common on small RISC chips) --- */
    "sdhci",                    /* Standard SD Host Controller Interface */
    "snps,dw-mshc",             /* Synopsys DesignWare Mobile Storage */
    "arm,pl180",                /* ARM PrimeCell (older) */
    "mediatek,mt8173-mmc",      /* MediaTek */

    NULL
};


uint8_t found_flag = 0 ;
uint8_t index = -1 ;
uint8_t devices_array_iterator = 0 ;
uart_puthex(out_info->device_count);
while(devices_array_iterator<out_info->device_count)
{

       uint8_t ssd_compatibles_iterator = 0 ;
       while(ssd_compatibles[ssd_compatibles_iterator]!=NULL)
       {

       

   found_flag =      str_eq((out_info->devices[devices_array_iterator].compatible ), (ssd_compatibles[ssd_compatibles_iterator]));
   if(found_flag)
   {
    uart_puts("\n We found the contyroller");
    uart_puts(ssd_compatibles[ssd_compatibles_iterator]);
    index = ssd_compatibles_iterator ;
    uart_puts("\n");
      break;
   }
        ssd_compatibles_iterator++ ;
       }


    devices_array_iterator++ ;
}


// Now Doing Binding ! 

  if(str_eq(ssd_compatibles[index],"virtio,mmio"))
  {
       ssd_driver = virtio_ssd_driver ;
  }

  ssd_driver(1,2,3,4,5);

}







#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/usb/usb-bus-stats.h>

struct usb_bus_stat_s usb_bus_stat[USB_MAXBUS];
EXPORT_SYMBOL(usb_bus_stat);


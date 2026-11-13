// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Reference module for the private-driver introduction checklist.
 * Intentionally does no hardware access, registers no notifier and exports
 * no symbol: it exists to validate the packaging/configuration chain.
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

static int __init mtk_driver_template_init(void)
{
#ifdef CONFIG_MTK_DRIVER_TEMPLATE_PROBE
	pr_info("mtk-driver-template: probe gate enabled (6.12 external module)\n");
#endif
	return 0;
}

static void __exit mtk_driver_template_exit(void)
{
}

module_init(mtk_driver_template_init);
module_exit(mtk_driver_template_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MediaTek private driver introduction template");
MODULE_AUTHOR("openwrt-25.12-6.12");

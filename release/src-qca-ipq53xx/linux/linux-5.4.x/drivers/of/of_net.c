// SPDX-License-Identifier: GPL-2.0-only
/*
 * OF helpers for network devices.
 *
 * Initially copied out of arch/powerpc/kernel/prom_parse.c
 */
#include <linux/etherdevice.h>
#include <linux/kernel.h>
#include <linux/of_net.h>
#include <linux/of_platform.h>
#include <linux/phy.h>
#include <linux/export.h>
#include <linux/device.h>
#include <linux/mtd/mtd.h>

/**
 * of_get_phy_mode - Get phy mode for given device_node
 * @np:	Pointer to the given device_node
 *
 * The function gets phy interface string from property 'phy-mode' or
 * 'phy-connection-type', and return its index in phy_modes table, or errno in
 * error case.
 */
int of_get_phy_mode(struct device_node *np)
{
	const char *pm;
	int err, i;

	err = of_property_read_string(np, "phy-mode", &pm);
	if (err < 0)
		err = of_property_read_string(np, "phy-connection-type", &pm);
	if (err < 0)
		return err;

	for (i = 0; i < PHY_INTERFACE_MODE_MAX; i++)
		if (!strcasecmp(pm, phy_modes(i)))
			return i;

	return -ENODEV;
}
EXPORT_SYMBOL_GPL(of_get_phy_mode);

static void *of_get_mac_addr(struct device_node *np, const char *name)
{
	struct property *pp = of_find_property(np, name, NULL);

	if (pp && pp->length == ETH_ALEN && is_valid_ether_addr(pp->value))
		return pp->value;
	return NULL;
}

static const void *of_get_mac_addr_nvmem(struct device_node *np)
{
	int ret;
	const void *mac;
	u8 nvmem_mac[ETH_ALEN];
	struct platform_device *pdev = of_find_device_by_node(np);

	if (!pdev)
		return ERR_PTR(-ENODEV);

	ret = nvmem_get_mac_address(&pdev->dev, &nvmem_mac);
	if (ret) {
		put_device(&pdev->dev);
		return ERR_PTR(ret);
	}

	mac = devm_kmemdup(&pdev->dev, nvmem_mac, ETH_ALEN, GFP_KERNEL);
	put_device(&pdev->dev);
	if (!mac)
		return ERR_PTR(-ENOMEM);

	return mac;
}

static const void *of_get_mac_address_mtd(struct device_node *np)
{
#ifdef CONFIG_MTD
	struct device_node *mtd_np = NULL;
	struct property *prop;
	size_t retlen;
	int size, ret;
	struct mtd_info *mtd;
	const char *part;
	const __be32 *list;
	phandle phandle;
	u32 mac_inc = 0;
	u8 mac[ETH_ALEN];
	void *addr;
	u32 inc_idx;

	list = of_get_property(np, "mtd-mac-address", &size);
	if (!list || (size != (2 * sizeof(*list))))
		return NULL;

	phandle = be32_to_cpup(list++);
	if (phandle)
		mtd_np = of_find_node_by_phandle(phandle);

	if (!mtd_np)
		return NULL;

	part = of_get_property(mtd_np, "label", NULL);
	if (!part)
		part = mtd_np->name;

	mtd = get_mtd_device_nm(part);
	if (IS_ERR(mtd))
		return NULL;

	ret = mtd_read(mtd, be32_to_cpup(list), 6, &retlen, mac);
	put_mtd_device(mtd);

	if (of_property_read_u32(np, "mtd-mac-address-increment-byte", &inc_idx))
		inc_idx = 5;
	if (inc_idx > 5)
		return NULL;

	if (!of_property_read_u32(np, "mtd-mac-address-increment", &mac_inc))
		mac[inc_idx] += mac_inc;

	if (!is_valid_ether_addr(mac))
		return NULL;

	addr = of_get_mac_addr(np, "mac-address");
	if (addr) {
		memcpy(addr, mac, ETH_ALEN);
		return addr;
	}

	prop = kzalloc(sizeof(*prop), GFP_KERNEL);
	if (!prop)
		return NULL;

	prop->name = "mac-address";
	prop->length = ETH_ALEN;
	prop->value = kmemdup(mac, ETH_ALEN, GFP_KERNEL);
	if (!prop->value || of_add_property(np, prop))
		goto free;

	return prop->value;
free:
	kfree(prop->value);
	kfree(prop);
#endif
	return NULL;
}

/**
 * Search the device tree for the best MAC address to use.  'mac-address' is
 * checked first, because that is supposed to contain to "most recent" MAC
 * address. If that isn't set, then 'local-mac-address' is checked next,
 * because that is the default address. If that isn't set, then the obsolete
 * 'address' is checked, just in case we're using an old device tree. If any
 * of the above isn't set, then try to get MAC address from nvmem cell named
 * 'mac-address'.
 *
 * Note that the 'address' property is supposed to contain a virtual address of
 * the register set, but some DTS files have redefined that property to be the
 * MAC address.
 *
 * All-zero MAC addresses are rejected, because those could be properties that
 * exist in the device tree, but were not set by U-Boot.  For example, the
 * DTS could define 'mac-address' and 'local-mac-address', with zero MAC
 * addresses.  Some older U-Boots only initialized 'local-mac-address'.  In
 * this case, the real MAC is in 'local-mac-address', and 'mac-address' exists
 * but is all zeros.
 *
 *
 * If a mtd-mac-address property exists, try to fetch the MAC address from the
 * specified mtd device, and store it as a 'mac-address' property
 *
 * Return: Will be a valid pointer on success and ERR_PTR in case of error.
*/
const void *of_get_mac_address(struct device_node *np)
{
	const void *addr;

	addr = of_get_mac_address_mtd(np);
	if (addr)
		return addr;

	addr = of_get_mac_addr(np, "mac-address");
	if (addr)
		return addr;

	addr = of_get_mac_addr(np, "local-mac-address");
	if (addr)
		return addr;

	addr = of_get_mac_addr(np, "address");
	if (addr)
		return addr;

	return of_get_mac_addr_nvmem(np);
}
EXPORT_SYMBOL(of_get_mac_address);

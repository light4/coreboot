/* SPDX-License-Identifier: GPL-2.0-only */

#include <arch/io.h>
#include <device/device.h>
#include <device/path.h>
#include <device/smbus.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include <device/pci_ops.h>
#include <soc/iomap.h>
#include <soc/ramstage.h>
#include <soc/smbus.h>
#include <device/smbus_host.h>

static void pch_smbus_init(struct device *dev)
{
	struct resource *res;
	u16 reg16;

	/* Enable clock gating */
	reg16 = pci_read_config32(dev, 0x80);
	reg16 &= ~((1 << 8)|(1 << 10)|(1 << 12)|(1 << 14));
	pci_write_config32(dev, 0x80, reg16);

	/* Set Receive Slave Address */
	res = find_resource(dev, PCI_BASE_ADDRESS_4);
	if (res)
		smbus_set_slave_addr(res->base, SMBUS_SLAVE_ADDR);
}

static int lsmbus_read_byte(struct device *dev, u8 address)
{
	u16 device;
	struct resource *res;
	struct bus *pbus;

	device = dev->path.i2c.device;
	pbus = get_pbus_smbus(dev);
	res = find_resource(pbus->dev, PCI_BASE_ADDRESS_4);

	return do_smbus_read_byte(res->base, device, address);
}

static int lsmbus_write_byte(struct device *dev, u8 address, u8 data)
{
	u16 device;
	struct resource *res;
	struct bus *pbus;

	device = dev->path.i2c.device;
	pbus = get_pbus_smbus(dev);
	res = find_resource(pbus->dev, PCI_BASE_ADDRESS_4);
	return do_smbus_write_byte(res->base, device, address, data);
}

static struct smbus_bus_operations lops_smbus_bus = {
	.read_byte	= lsmbus_read_byte,
	.write_byte	= lsmbus_write_byte,
};

static void smbus_read_resources(struct device *dev)
{
	struct resource *res = new_resource(dev, PCI_BASE_ADDRESS_4);
	res->base = SMBUS_BASE_ADDRESS;
	res->size = 32;
	res->limit = res->base + res->size - 1;
	res->flags = IORESOURCE_IO | IORESOURCE_FIXED | IORESOURCE_RESERVE |
		     IORESOURCE_STORED | IORESOURCE_ASSIGNED;

	/* Also add MMIO resource */
	res = pci_get_resource(dev, PCI_BASE_ADDRESS_0);
}

static struct device_operations smbus_ops = {
	.read_resources		= smbus_read_resources,
	.set_resources		= pci_dev_set_resources,
	.enable_resources	= pci_dev_enable_resources,
	.scan_bus		= scan_smbus,
	.init			= pch_smbus_init,
	.ops_smbus_bus		= &lops_smbus_bus,
	.ops_pci		= &broadwell_pci_ops,
};

static const unsigned short pci_device_ids[] = {
	0x9c22, /* LynxPoint */
	0x9ca2, /* WildcatPoint */
	0
};

static const struct pci_driver pch_smbus __pci_driver = {
	.ops	 = &smbus_ops,
	.vendor	 = PCI_VENDOR_ID_INTEL,
	.devices = pci_device_ids,
};

#ifndef __FAKE_DRV_H__
#define __FAKE_DRV_H__

#include <drm/drm_device.h>
#include <drm/drm_drv.h>
#include <drm/drm_managed.h>
#include <drm/drm_mm.h>
#include <drm/drm_print.h>

#include <linux/pci.h>
#include <linux/xarray.h>
#include "uapi_drm_fake_accel.h"

#define DRIVER_NAME "FAKE_ACCEL"
#define DRIVER_DESC "Driver for fake accelerator"
#define DRIVER_DATE "19700101"

#define drm_device_to_fake_device(target) \
	container_of(target, struct fake_device, drm)

struct fake_device;

struct fake_config {
	/* only set when instantiated */
	struct fake_device *dev;
};

struct fake_file_priv {
        struct kref ref;
        struct fake_device *fdev;
        struct mutex lock; /* Protects cmdq */
        u32 priority;
        bool has_mmu_faults;
};

struct fake_device {
	struct drm_device drm;
	struct platform_device *platform;
	const struct fake_config *config;

	void *memory;
	struct drm_mm mm;
	u64 range_start;
	u64 range_end;
        struct mutex bo_list_lock; /* Protects bo_list */
        struct list_head bo_list;
};

#endif // __FAKE_DRV_H__

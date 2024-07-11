#include <linux/firmware.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>

#include <drm/drm_gem.h>
#include <drm/drm_atomic.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_drv.h>
#include <drm/drm_fbdev_generic.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drm_ioctl.h>
#include <drm/drm_managed.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_gem_shmem_helper.h>
#include <drm/drm_accel.h>
#include <drm/drm_file.h>
#include <drm/drm_prime.h>

#include "fake_drv.h"
#include "fake_gem.h"

#include <drm/drm_print.h>
#include <drm/drm_debugfs.h>

#ifndef DRIVER_VERSION_STR
#define DRIVER_VERSION_STR                                  \
	__stringify(DRM_FAKE_DRIVER_MAJOR) "." __stringify( \
		DRM_FAKE_DRIVER_MINOR)
#endif

static struct fake_config *default_config;

static const struct drm_ioctl_desc fake_drm_ioctls[] = {
        DRM_IOCTL_DEF_DRV(FAKE_BO_CREATE, fake_bo_create_ioctl, 0),
        DRM_IOCTL_DEF_DRV(FAKE_BO_INFO, fake_bo_info_ioctl, 0),
        // DRM_IOCTL_DEF_DRV(FAKE_SUBMIT, fake_submit_ioctl, 0),
        // DRM_IOCTL_DEF_DRV(FAKE_BO_WAIT, fake_bo_wait_ioctl, 0),
};


// driver module release
static void fake_release(struct drm_device *dev)
{
	struct fake_device *fake = drm_device_to_fake_device(dev);

	printk("fake release\n");
	// if (fake->output.composer_workq)
	//         destroy_workqueue(fake->output.composer_workq);
}

static int fake_config_show(struct seq_file *m, void *data)
{
	struct drm_debugfs_entry *entry = m->private;
	struct drm_device *dev = entry->dev;
	struct fake_device *fakedev = drm_device_to_fake_device(dev);

	seq_printf(m, "%s\n", "helloworld");

	return 0;
}

static const struct drm_debugfs_info fake_config_debugfs_list[] = {
	{ "fake_config", fake_config_show, 0 },
};

static const struct file_operations fake_driver_fops = {
	.owner = THIS_MODULE,
	DRM_ACCEL_FOPS,
};

static int fake_open(struct drm_device *dev, struct drm_file *file)
{
	printk("fake open\n");
	struct fake_device *fdev = drm_device_to_fake_device(dev);
	struct fake_file_priv *file_priv;
	int ret;
	file_priv = kzalloc(sizeof(*file_priv), GFP_KERNEL);
	
	if (!file_priv) {
                ret = -ENOMEM;
		return ret;
        }


        file_priv->fdev = fdev;
        kref_init(&file_priv->ref);
        mutex_init(&file_priv->lock);
        file->driver_priv = file_priv;
        return 0;

}

static void fake_postclose(struct drm_device *dev, struct drm_file *file)
{
	printk("fake postclose\n");
	return;
}

static const struct drm_driver fake_driver = {
	.driver_features = DRIVER_COMPUTE_ACCEL | DRIVER_GEM,
	.open = fake_open,
	.postclose = fake_postclose,
	.fops = &fake_driver_fops,
	DRM_GEM_SHMEM_DRIVER_OPS,
	.gem_create_object = fake_gem_create_object,

	.ioctls = fake_drm_ioctls,
	.num_ioctls = ARRAY_SIZE(fake_drm_ioctls),

	.name = DRIVER_NAME,
	.desc = DRIVER_DESC,
	.date = DRIVER_DATE,
	.major = DRM_FAKE_DRIVER_MAJOR,
	.minor = DRM_FAKE_DRIVER_MINOR,
};

static int fake_create(struct fake_config *config)
{
	int ret;
	struct platform_device *pdev;
	struct fake_device *fake_device;

	pdev = platform_device_register_simple(DRIVER_NAME, -1, NULL, 0);
	if (IS_ERR(pdev))
		return PTR_ERR(pdev);

	if (!devres_open_group(&pdev->dev, NULL, GFP_KERNEL)) {
		ret = -ENOMEM;
		goto out_unregister;
	}

	fake_device = devm_drm_dev_alloc(&pdev->dev, &fake_driver,
					 struct fake_device, drm);
	if (IS_ERR(fake_device)) {
		ret = PTR_ERR(fake_device);
		goto out_devres;
	}
	fake_device->platform = pdev;
	fake_device->config = config;
	config->dev = fake_device;

	ret = dma_coerce_mask_and_coherent(fake_device->drm.dev,
					   DMA_BIT_MASK(64));

	if (ret) {
		DRM_ERROR("Could not initialize DMA support\n");
		goto out_devres;
	}
	drm_debugfs_add_files(&fake_device->drm, fake_config_debugfs_list,
			      ARRAY_SIZE(fake_config_debugfs_list));

	ret = drm_dev_register(&fake_device->drm, 0);
	if (ret)
		goto out_devres;

	// init
	drmm_mutex_init(&fake_device->drm, &fake_device->bo_list_lock);
	INIT_LIST_HEAD(&fake_device->bo_list);

	// init mm
	u64 size = 0x80000000;
	printk("size is %llx\n", size);
	// drm_mm_init
	fake_device->range_start = 0x80000000;
	fake_device->range_end = 0x80000000 + size;
	printk("fake device init drm mm start %llx end %llx\n", fake_device->range_start, fake_device->range_end);
	drm_mm_init(&fake_device->mm, fake_device->range_start, size);
	DRM_INFO("DRM dev registerd\n");
	return 0;

out_devres:
	DRM_ERROR("FBI WARNING!\n");
	devres_release_group(&pdev->dev, NULL);
out_unregister:
	platform_device_unregister(pdev);
	return ret;
}

static int __init fake_init(void)
{
	int ret;
	struct fake_config *config;

	config = kmalloc(sizeof(*config), GFP_KERNEL);
	if (!config)
		return -ENOMEM;

	default_config = config;
	ret = fake_create(config);
	if (ret)
		kfree(config);

	return ret;
}

static void fake_destroy(struct fake_config *config)
{
	printk("fake destroy!\n");
	struct platform_device *pdev;

	if (!config->dev) {
		DRM_INFO("fake_device is NULL.\n");
		return;
	}

	drm_mm_takedown(&config->dev->mm);
	pdev = config->dev->platform;

	drm_dev_unregister(&config->dev->drm);
	//drm_atomic_helper_shutdown(&config->dev->drm);
	devres_release_group(&pdev->dev, NULL);
	platform_device_unregister(pdev);

	config->dev = NULL;
}

static void __exit fake_exit(void)
{
	if (default_config->dev)
		fake_destroy(default_config);

	kfree(default_config);
}

module_init(fake_init);
module_exit(fake_exit);

MODULE_AUTHOR("DonaldDuck");
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL v2");
MODULE_VERSION(DRIVER_VERSION_STR);

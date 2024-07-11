#ifndef __FAKE_GEM_H__
#define __FAKE_GEM_H__

#include <drm/drm_gem.h>
#include <drm/drm_gem_shmem_helper.h>
#include <drm/drm_mm.h>
#include "fake_drv.h"

int fake_bo_create_ioctl(struct drm_device *dev, void *data, struct drm_file *file);
int fake_bo_info_ioctl(struct drm_device *dev, void *data, struct drm_file *file);

struct fake_bo {
	struct drm_gem_shmem_object base;
	struct list_head bo_list_node;
	struct drm_mm_node mm_node;

	struct mutex lock; /* Protects: ctx, mmu_mapped, fake_addr */
	u64 fake_addr;
	u32 handle;
	u32 flags;
	u32 job_status; /* Valid only for command buffer */
	bool mmu_mapped;
};

struct drm_gem_object *fake_gem_create_object(struct drm_device *dev, size_t size);
static struct fake_bo *fake_bo_create(struct fake_device *fdev, u64 size, u32 flags);

static inline struct fake_bo *to_fake_bo(struct drm_gem_object *obj)
{
        return container_of(obj, struct fake_bo, base.base);
}


static inline struct fake_device *fake_bo_to_fdev(struct fake_bo *bo)
{
        return drm_device_to_fake_device(bo->base.base.dev);
}


#endif

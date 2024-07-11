#include <linux/dma-buf.h>
#include <linux/highmem.h>
#include <linux/module.h>
#include <linux/set_memory.h>
#include <linux/xarray.h>

#include <drm/drm_cache.h>
#include <drm/drm_debugfs.h>
#include <drm/drm_file.h>
#include <drm/drm_utils.h>

#include "fake_drv.h"
#include "fake_gem.h"

static const struct drm_gem_object_funcs fake_gem_funcs;

static int
bo_alloc_fake_addr(struct fake_bo *bo)
{               
        struct fake_device *fdev = fake_bo_to_fdev(bo);
        int ret;

        mutex_lock(&bo->lock);

	printk("bo fake bo alloc size %d\n", bo->base.base.size);
	printk("fdev range start %p end %p size %d \n", fdev->range_start, fdev->range_end, fdev->range_end - fdev->range_start);
	printk("fdev mm head size %d\n", fdev->mm.head_node.size);
        ret = drm_mm_insert_node_in_range(&fdev->mm, &bo->mm_node, bo->base.base.size, 4096, 0,
                                          fdev->range_start, fdev->range_end, DRM_MM_INSERT_BEST);        
	if (!ret) {
		printk("alloc fake addr %p\n", bo->mm_node.start);
                bo->fake_addr = bo->mm_node.start;
 	}
        mutex_unlock(&bo->lock);

        return ret;
}



struct drm_gem_object *fake_gem_create_object(struct drm_device *dev,
					      size_t size)
{
	printk("it's fake gem create object callback\n");
	struct fake_bo *bo;

	if (size == 0 || !PAGE_ALIGNED(size))
		return ERR_PTR(-EINVAL);

	bo = kzalloc(sizeof(*bo), GFP_KERNEL);
	if (!bo)
		return ERR_PTR(-ENOMEM);

	bo->base.base.funcs = &fake_gem_funcs;
	bo->base.pages_mark_dirty_on_put = true;

	INIT_LIST_HEAD(&bo->bo_list_node);
	mutex_init(&bo->lock);

	printk("fake gem create object callback end\n");
	return &bo->base.base;
}


static struct fake_bo *fake_bo_create(struct fake_device *fdev, u64 size, u32 flags)
{
	struct drm_gem_shmem_object *shmem;
	struct fake_bo *bo;

	printk("create fake_bo, it may call fake_gem_create_object\n");
	shmem = drm_gem_shmem_create(&fdev->drm, size);
	if (IS_ERR(shmem))
		return ERR_CAST(shmem);

	printk("shmem to fakebo, shmem.base addr %p\n", &shmem->base);
	bo = to_fake_bo(&shmem->base);
	bo->base.map_wc = true;
	bo->flags = flags;

	mutex_lock(&fdev->bo_list_lock);
	list_add_tail(&bo->bo_list_node, &fdev->bo_list);
	mutex_unlock(&fdev->bo_list_lock);

	printk("fake_bo_create finish\n");
	return bo;
}

static void fake_bo_unbind_locked(struct fake_bo *bo)
{
        struct fake_device *fdev = fake_bo_to_fdev(bo);

        lockdep_assert_held(&bo->lock);
	drm_mm_remove_node(&bo->mm_node);	
}

static void fake_bo_unbind(struct fake_bo *bo)
{
        mutex_lock(&bo->lock);
        fake_bo_unbind_locked(bo);
        mutex_unlock(&bo->lock);
}


static void fake_bo_free(struct drm_gem_object *obj)
{
        struct fake_device *fdev = drm_device_to_fake_device(obj->dev);
        struct fake_bo *bo = to_fake_bo(obj);

        mutex_lock(&fdev->bo_list_lock);
        list_del(&bo->bo_list_node);
        mutex_unlock(&fdev->bo_list_lock);

        drm_WARN_ON(&fdev->drm, !dma_resv_test_signaled(obj->resv, DMA_RESV_USAGE_READ));
        
	printk("fake bo free\n");

        fake_bo_unbind(bo);
        mutex_destroy(&bo->lock);

        drm_WARN_ON(obj->dev, bo->base.pages_use_count > 1);
        drm_gem_shmem_free(&bo->base);
}  



static int fake_bo_open(struct drm_gem_object *obj, struct drm_file *file)
{
        struct fake_file_priv *file_priv = file->driver_priv;
        struct fake_device *fdev = file_priv->fdev;
        struct fake_bo *bo = to_fake_bo(obj);
        struct fake_addr_range *range;
	int ret;

	ret = bo_alloc_fake_addr(bo);
	printk("fake bo open callback end ret is %d\n", ret);
	return ret;
}

// static struct dma_buf *fake_bo_export(struct drm_gem_object *obj, int flags)
// {       
//         struct drm_device *dev = obj->dev;
//         struct dma_buf_export_info exp_info = {
//                 .exp_name = KBUILD_MODNAME,
//                 .owner = dev->driver->fops->owner,
//                 .ops = &fake_bo_dmabuf_ops,
//                 .size = obj->size,
//                 .flags = flags,
//                 .priv = obj,
//                 .resv = obj->resv,
//         };      
//         void *sgt;
// 
//         /*
//          * Make sure that pages are allocated and dma-mapped before exporting the bo.
//          * DMA-mapping is required if the bo will be imported to the same device.
//          */ 
//         sgt = drm_gem_shmem_get_pages_sgt(to_drm_gem_shmem_obj(obj));
//         if (IS_ERR(sgt))
//                 return sgt;
//   
//         return drm_gem_dmabuf_export(dev, &exp_info);
// }     


int fake_bo_create_ioctl(struct drm_device *dev, void *data, struct drm_file *file)
{
        struct fake_file_priv *file_priv = file->driver_priv;
	if (!file_priv) {
		printk("aaaaaaaaaa\n");
		return 1;
	}
        struct fake_device *fdev = file_priv->fdev;
	if (!fdev) {
		printk("bbbbbbbbbbbb\n");
		return 2;
	}
        struct drm_fake_bo_create *args = data;
        u64 size = PAGE_ALIGN(args->size);
        struct fake_bo *bo;
        int ret;

	printk("in create ioctl, before fake_bo_create\n");
        bo = fake_bo_create(fdev, size, args->flags);
        if (IS_ERR(bo)) {
                printk("Failed to create BO: %pe (size %llu flags 0x%x)",
                         bo, args->size, args->flags);
                return PTR_ERR(bo); 
        }

	printk("drm gem handle create, will call gem open to do something you want\n");
        ret = drm_gem_handle_create(file, &bo->base.base, &bo->handle);
        if (!ret) {
		printk("create success fake addr %p\n", bo->fake_addr);
                args->fake_addr = bo->fake_addr;
                args->handle = bo->handle;
        }

        drm_gem_object_put(&bo->base.base);

        return ret;
}


int fake_bo_info_ioctl(struct drm_device *dev, void *data, struct drm_file *file)
{
        struct drm_fake_bo_info *args = data;
        struct drm_gem_object *obj;
        struct fake_bo *bo;
        int ret = 0;

        obj = drm_gem_object_lookup(file, args->handle);
        if (!obj)
                return -ENOENT;

        bo = to_fake_bo(obj);

        mutex_lock(&bo->lock);
        args->flags = bo->flags;
        args->mmap_offset = drm_vma_node_offset_addr(&obj->vma_node);
        args->fake_addr = bo->fake_addr;
        args->size = obj->size;
        mutex_unlock(&bo->lock);

        drm_gem_object_put(obj);
        return ret;
}





static const struct drm_gem_object_funcs fake_gem_funcs = {
        .free = fake_bo_free,
        .open = fake_bo_open,
         // .export = fake_bo_export,
        .print_info = drm_gem_shmem_object_print_info,
        .pin = drm_gem_shmem_object_pin,
        .unpin = drm_gem_shmem_object_unpin,
        .get_sg_table = drm_gem_shmem_object_get_sg_table,
        .vmap = drm_gem_shmem_object_vmap,
        .vunmap = drm_gem_shmem_object_vunmap,
        .mmap = drm_gem_shmem_object_mmap,
        .vm_ops = &drm_gem_shmem_vm_ops,
};      

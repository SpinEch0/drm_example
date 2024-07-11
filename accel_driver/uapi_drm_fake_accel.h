#ifndef __UAPI_FAKE_DRM_H_
#define __UAPI_FAKE_DRM_H_

#include "drm/drm.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define DRM_FAKE_DRIVER_MAJOR 1
#define DRM_FAKE_DRIVER_MINOR 0

#define DRM_FAKE_GET_PARAM 0x00
#define DRM_FAKE_SET_PARAM 0x01
#define DRM_FAKE_BO_CREATE 0x02
#define DRM_FAKE_BO_INFO 0x03
#define DRM_FAKE_SUBMIT 0x04
#define DRM_FAKE_BO_WAIT 0x05

#define DRM_IOCTL_FAKE_GET_PARAM \
	DRM_IOWR(DRM_COMMAND_BASE + DRM_FAKE_GET_PARAM, struct drm_fake_param)

#define DRM_IOCTL_FAKE_SET_PARAM \
	DRM_IOW(DRM_COMMAND_BASE + DRM_FAKE_SET_PARAM, struct drm_fake_param)

#define DRM_IOCTL_FAKE_BO_CREATE                        \
	DRM_IOWR(DRM_COMMAND_BASE + DRM_FAKE_BO_CREATE, \
		 struct drm_fake_bo_create)

#define DRM_IOCTL_FAKE_BO_INFO \
	DRM_IOWR(DRM_COMMAND_BASE + DRM_FAKE_BO_INFO, struct drm_fake_bo_info)

#define DRM_IOCTL_FAKE_SUBMIT \
	DRM_IOW(DRM_COMMAND_BASE + DRM_FAKE_SUBMIT, struct drm_fake_submit)

#define DRM_IOCTL_FAKE_BO_WAIT \
	DRM_IOWR(DRM_COMMAND_BASE + DRM_FAKE_BO_WAIT, struct drm_fake_bo_wait)

/**
 * struct drm_fake_param - Get/Set FAKE parameters
 */
struct drm_fake_param {
	__u32 param;

	/** @index: Index for params that have multiple instances */
	__u32 index;

	/** @value: Param value */
	__u64 value;
};

/**
 *
 * Create GEM buffer object allocated in SHMEM memory.
 */
struct drm_fake_bo_create {
	/** @size: The size in bytes of the allocated memory */
	__u64 size;

	__u32 flags;

	/** @handle: Returned GEM object handle */
	__u32 handle;

	__u64 fake_addr;
};

struct drm_fake_bo_info {
	/** @handle: Handle of the queried BO */
	__u32 handle;

	/** @flags: Returned flags used to create the BO */
	__u32 flags;

	/** @fake_addr: Returned FAKE virtual address */
	__u64 fake_addr;

	/**
	 * @mmap_offset:
	 *
	 * Returned offset to be used in mmap(). 0 in case the BO is not mappable.
	 */
	__u64 mmap_offset;

	/** @size: Returned GEM object size, aligned to PAGE_SIZE */
	__u64 size;
};

struct drm_fake_submit {
	/**
	 * @buffers_ptr:
	 *
	 * A pointer to an u32 array of GEM handles of the BOs required for this job.
	 * The number of elements in the array must be equal to the value given by @buffer_count.
	 *
	 * The first BO is the command buffer. The rest of array has to contain all
	 * BOs referenced from the command buffer.
	 */
	__u64 buffers_ptr;

	/** @buffer_count: Number of elements in the @buffers_ptr */
	__u32 buffer_count;

	/**
	 * @engine: Select the engine this job should be executed on
	 *
	 * %DRM_FAKE_ENGINE_COMPUTE:
	 *
	 * Performs Deep Learning Neural Compute Inference Operations
	 *
	 * %DRM_FAKE_ENGINE_COPY:
	 *
	 * Performs memory copy operations to/from system memory allocated for FAKE
	 */
	__u32 engine;

	/** @flags: Reserved for future use - must be zero */
	__u32 flags;

	/**
	 * @commands_offset:
	 *
	 * Offset inside the first buffer in @buffers_ptr containing commands
	 * to be executed. The offset has to be 8-byte aligned.
	 */
	__u32 commands_offset;
};

struct drm_fake_bo_wait {
	/** @handle: Handle to the buffer object to be waited on */
	__u32 handle;

	/** @flags: Reserved for future use - must be zero */
	__u32 flags;

	/** @timeout_ns: Absolute timeout in nanoseconds (may be zero) */
	__s64 timeout_ns;

	/**
	 * @job_status:
	 *
	 * Job status code which is updated after the job is completed.
	 * &DRM_FAKE_JOB_STATUS_SUCCESS or device specific error otherwise.
	 * Valid only if @handle points to a command buffer.
	 */
	__u32 job_status;

	/** @pad: Padding - must be zero */
	__u32 pad;
};

#if defined(__cplusplus)
}
#endif

#endif // __UAPI_FAKE_DRM_H_

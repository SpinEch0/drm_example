
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include "uapi_drm_fake_accel.h"

static int dev_open(int *out, const char *node)
{
	int fd, ret;
	uint64_t has_dumb;

	fd = open(node, O_RDWR | O_CLOEXEC);
	if (fd < 0) {
		ret = -errno;
		fprintf(stderr, "cannot open '%s': %m\n", node);
		return ret;
	}

	// if (drmGetCap(fd, DRM_CAP_DUMB_BUFFER, &has_dumb) < 0 ||
	//     !has_dumb) {
	// 	fprintf(stderr, "drm device '%s' does not support dumb buffers\n",
	// 		node);
	// 	close(fd);
	// 	return -EOPNOTSUPP;
	// }

	*out = fd;
	return 0;
}

int main(int argc, char **argv)
{
	int ret, fd;
	const char *card;
	struct modeset_dev *iter;

	/* check which DRM device to open */
	if (argc > 1)
		card = argv[1];
	else
		card = "/dev/accel/accel0";

	fprintf(stderr, "using card '%s'\n", card);

	/* open the DRM device */
	ret = dev_open(&fd, card);

	struct drm_fake_bo_create args = {};

	args.size = 1024;
	args.flags = 0;

	ret = ioctl(fd, DRM_IOCTL_FAKE_BO_CREATE, &args);
	if (ret) {
		printf("bo create failed!\n");
	}

	
	printf("bo handle %d fakeaddr %p\n", args.handle, args.fake_addr);

	struct drm_fake_bo_info args2 = {};
    	args2.handle = args.handle;

        ret = ioctl(fd, DRM_IOCTL_FAKE_BO_INFO, &args2);
	if (ret) {
		printf("get bo info failed!\n");
	}
	printf("bo mmap_offset %d \n", args2.mmap_offset);
	char *ptr = mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_SHARED, fd, args2.mmap_offset);
	printf("map addr %p\n", ptr);
	printf("let see what is in the buffer:\n");
	printf("%100s\n", ptr);
	ptr[1] = 'n';
	char *ptr2 = mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_SHARED, fd, args2.mmap_offset);
	printf("%c\n", ptr2[1]);
	return ret;
}

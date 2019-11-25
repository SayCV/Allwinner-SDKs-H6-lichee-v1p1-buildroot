/*
 * Sunxi SOC ion momery utils
 *
 * Copyright (C) 2015-2018 AllwinnerTech, Inc.
 *
 * Contacts:
 * Zeng.Yajian <zengyajian@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <string.h>
#include <errno.h>
#include "log.h"
#include "ion_alloc.h"
#include "list.h"

#define ION_ALLOC_ALIGN		(SZ_1k * 4)
#define DEV_NAME		"/dev/ion"

struct buffer_node {
	struct list_head list;
	char *phy_address;
	char *vir_address;
	unsigned int size;

	int dmabuf_fd;
	ion_user_handle_t handle;
};

struct ion_alloc_context {
	int fd;
	int reference;
	struct list_head list;
};

static struct ion_alloc_context *_alloc_context = NULL;
static pthread_mutex_t _alloc_mutex = PTHREAD_MUTEX_INITIALIZER;

int ion_alloc_open(void)
{
	int ret = 0;

	pthread_mutex_lock(&_alloc_mutex);
	if (_alloc_context) {
		log_warning("ion allocator has already open\n");
		goto _out;
	}

	_alloc_context = (struct ion_alloc_context *)malloc(sizeof(*_alloc_context));
	if (!_alloc_context) {
		log_error("malloc failed, %s\n", strerror(errno));
		ret = -ENOMEM;
		goto _out;
	}

	memset((void *)_alloc_context, 0, sizeof(*_alloc_context));
	_alloc_context->fd = open(DEV_NAME, O_RDWR, 0);
	if (_alloc_context->fd <= 0) {
		log_error("open device '%s' failed, %s\n", DEV_NAME, strerror(errno));
		free(_alloc_context);
		_alloc_context = NULL;
		ret = -ENODEV;
		goto _out;
	}
	_alloc_context->reference++;
	INIT_LIST_HEAD(&_alloc_context->list);

_out:
	pthread_mutex_unlock(&_alloc_mutex);
	return ret;
}

int ion_alloc_close(void)
{
	pthread_mutex_lock(&_alloc_mutex);
	_alloc_context--;
	if (_alloc_context->reference <= 0)
		close(_alloc_context->fd);
	else
		log_error("another thread is using thise module\n");

	pthread_mutex_unlock(&_alloc_mutex);
	return 0;
}

void *ion_alloc_alloc(int size)
{
	int ret = 0;
	void *phy_address = 0;
	void *vir_address = 0;
	struct buffer_node *alloc_buffer = NULL;

	ion_allocation_data_t alloc_data;
	ion_handle_data_t handle_data;
	ion_custom_data_t custom_data;
	ion_fd_data_t fd_data;
	sunxi_phys_data phys_data;

	if (size <= 0)
		return 0;

	pthread_mutex_lock(&_alloc_mutex);

	if (_alloc_context == NULL) {
		log_error("ion_alloc do not opened\n");
		goto _out;
	}

	/* alloc buffer */
	alloc_data.len = size;
	alloc_data.align = ION_ALLOC_ALIGN;
	alloc_data.heap_id_mask = ION_HEAP_CARVEOUT_MASK;
	alloc_data.flags = ION_FLAG_CACHED | ION_FLAG_CACHED_NEEDS_SYNC;
	ret = ioctl(_alloc_context->fd, ION_IOC_ALLOC, &alloc_data);
	if (ret) {
		log_error("alloc memory error\n");
		goto _out;
	}

	/* get physical address */
	phys_data.handle = alloc_data.handle;
	custom_data.cmd = ION_IOC_SUNXI_PHYS_ADDR;
	custom_data.arg = (unsigned long)&phys_data;
	ret = ioctl(_alloc_context->fd, ION_IOC_CUSTOM, &custom_data);
	if (ret) {
		log_error("get physical address failed\n");
		goto _free_buffer;
	}
	phy_address = (void *)phys_data.phys_addr;

	/* get dma buffer fd */
	fd_data.handle = alloc_data.handle;
	ret = ioctl(_alloc_context->fd, ION_IOC_MAP, &fd_data);
	if (ret) {
		log_error("get dma buffer fd failed\n");
		goto _free_buffer;
	}

	/* mmap to user space */
	vir_address = mmap(NULL, alloc_data.len,
						PROT_READ | PROT_WRITE, MAP_SHARED,
						fd_data.fd, 0);
	if (vir_address == MAP_FAILED) {
		log_error("mmap fialed\n");
		goto _close_dmabuf_fd;
	}

	alloc_buffer = (struct buffer_node *)malloc(sizeof(struct buffer_node));
	if (alloc_buffer == NULL) {
		log_error("malloc buffer node failed\n");
		goto _unmmap_exit;
	}
	alloc_buffer->size = size;
	alloc_buffer->phy_address = phy_address;
	alloc_buffer->vir_address = vir_address;
	alloc_buffer->handle = alloc_data.handle;
	alloc_buffer->dmabuf_fd = fd_data.fd;

	log_debug("alloc succeed, addr_phy: %p, addr_vir: %p, size: %d\n",
	     phy_address, vir_address, size);

	list_add_tail(&alloc_buffer->list, &_alloc_context->list);

	goto _out;

_unmmap_exit:
	munmap(vir_address, alloc_data.len);

_close_dmabuf_fd:
	close(fd_data.fd);

_free_buffer:
	handle_data.handle = alloc_data.handle;
	ioctl(_alloc_context->fd, ION_IOC_FREE, &handle_data);

_out:
	pthread_mutex_unlock(&_alloc_mutex);
	return vir_address;
}

int ion_alloc_free(void *vaddress)
{
	int ret;
	void *vir_address = vaddress;
	struct list_head *plist;
	struct buffer_node *tmp;
	struct ion_handle_data handle_data;
	int free_size = 0;

	if (!vaddress) {
		log_error("can not free NULL buffer\n");
		return 0;
	}

	pthread_mutex_lock(&_alloc_mutex);

	if (_alloc_context == NULL) {
		log_error("ion_alloc do not opened\n");
		return 0;
	}

	list_for_each(plist, &_alloc_context->list) {
		tmp = list_entry(plist, struct buffer_node, list);
		if (tmp->vir_address == vir_address) {
			/* unmap user space */
			if (munmap(vaddress, tmp->size) < 0) {
				log_error("munmap 0x%p, size: %d failed\n",
					(void *)vir_address, tmp->size);
			}
			free_size = tmp->size;

			/* close dma buffer fd */
			close(tmp->dmabuf_fd);

			/* free memory handle */
			handle_data.handle = tmp->handle;
			ret = ioctl(_alloc_context->fd, ION_IOC_FREE, &handle_data);
			if (ret) {
				log_error("free memory handle failed\n");
			}

			log_error("free ion: %p\n", tmp->vir_address);
			list_del(&tmp->list);
			free(tmp);
			break;
		}
	}

	pthread_mutex_unlock(&_alloc_mutex);
	return free_size;
}

void *ion_alloc_vir2phy(void *vaddress)
{
	char *vir_address = vaddress;
	char *phy_address = 0;
	struct list_head *plist;
	struct buffer_node *tmp;

	if (0 == vaddress) {
		log_error("can not vir2phy a NULL buffer \n");
		return NULL;
	}

	pthread_mutex_lock(&_alloc_mutex);
	list_for_each(plist, &_alloc_context->list) {
		tmp = list_entry(plist, struct buffer_node, list);
		if (vir_address >= tmp->vir_address && vir_address < tmp->vir_address + tmp->size) {
			phy_address = tmp->phy_address + (unsigned int)(vir_address - tmp->vir_address);
			break;
		}
	}
	pthread_mutex_unlock(&_alloc_mutex);
	return phy_address;
}

void *ion_alloc_phy2vir(void *paddress)
{
	char *vir_address = 0;
	char *phy_address = paddress;
	struct list_head *plist;
	struct buffer_node *tmp;

	if (0 == paddress) {
		log_error("can not phy2vir a NULL buffer\n");
		return 0;
	}

	pthread_mutex_lock(&_alloc_mutex);
	list_for_each(plist, &_alloc_context->list) {
		tmp = list_entry(plist, struct buffer_node, list);
		if (phy_address >= tmp->phy_address && phy_address < tmp->phy_address + tmp->size) {
			vir_address = tmp->vir_address + (unsigned int)(phy_address - tmp->phy_address);
			break;
		}
	}
	pthread_mutex_unlock(&_alloc_mutex);

	return vir_address;
}

void ion_flush_cache(void *address, int size)
{
	sunxi_cache_range range;
	struct ion_custom_data custom_data;
	int ret;

	/* clean and invalid user cache */
	range.start = (unsigned long)address;
	range.end = (unsigned long)address + size;

	custom_data.cmd = ION_IOC_SUNXI_FLUSH_RANGE;
	custom_data.arg = (unsigned long)&range;

	ret = ioctl(_alloc_context->fd, ION_IOC_CUSTOM, &custom_data);
	if (ret) {
		log_error("flush cache failed\n");
	}

	return;
}

void ion_flush_cache_all()
{
	ioctl(_alloc_context->fd, ION_IOC_SUNXI_FLUSH_ALL, 0);
}


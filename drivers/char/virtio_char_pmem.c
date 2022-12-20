// SPDX-License-Identifier: GPL-2.0
/*
 * virtio_pmem.c: Virtio pmem char Driver
 *
 * Discovers persistent memory range information
 * from host and registers the virtual pmem device
 * with libnvdimm core.
 */
#include <linux/major.h>
#include <linux/ioport.h>
#include <linux/miscdevice.h>
#include "virtio_char_pmem.h"

static struct virtio_device_id id_table[] = {
	{ VIRTIO_ID_PMEM, VIRTIO_DEV_ANY_ID },
	{ 0 },
};

// TODO jk: remove
static struct virtio_pmem *vpmem = NULL;
static struct miscdevice char_pmem_miscdev;

 /* Initialize virt queue */
static int init_vq(struct virtio_pmem *vpmem)
{
	/* single vq */
	vpmem->req_vq = virtio_find_single_vq(vpmem->vdev,
					virtio_pmem_host_ack, "flush_queue");
	if (IS_ERR(vpmem->req_vq))
		return PTR_ERR(vpmem->req_vq);

	spin_lock_init(&vpmem->pmem_lock);
	INIT_LIST_HEAD(&vpmem->req_list);

	return 0;
};

static int open_mem(struct inode *inode, struct file *filp)
{
	#if 0
	int rc;

	if (!capable(CAP_SYS_RAWIO))
		return -EPERM;

	rc = security_locked_down(LOCKDOWN_DEV_MEM);
	if (rc)
		return rc;

	if (iminor(inode) != DEVMEM_MINOR)
		return 0;
	#endif
	if (!vpmem)
		return -ENODEV;
	/*
	 * Use a unified address space to have a single point to manage
	 * revocations when drivers want to take over a /dev/mem mapped
	 * range.
	 */
	// filp->f_mapping = iomem_get_mapping();
	pr_info("%s", __FUNCTION__);
	printk("vpmem->start=0x%llx vpmem->size=0x%llx", vpmem->start, vpmem->size);
	return 0;
}

static int mmap_mem(struct file *file, struct vm_area_struct *vma)
{
	pr_info("JK: %s", __FUNCTION__);
	printk(">>vpmem->start=0x%llx vpmem->size=0x%llx", vpmem->start, vpmem->size);

	pr_info(">>phys_mem_access_prot=0x%x", phys_mem_access_prot(file, vma->vm_pgoff,
						 vpmem->size,
						 vma->vm_page_prot));

	pr_info(">>pgprot_noncached=0x%x", pgprot_noncached(vma->vm_page_prot));

	pr_info(">>vma->vm_flags=0x%x", vma->vm_flags);
	vma->vm_flags |= VM_IO | VM_PFNMAP | VM_DONTEXPAND | VM_DONTDUMP | VM_MIXEDMAP | VM_READ | VM_WRITE;
	pr_info(">>added flags: vma->vm_flags=0x%x", vma->vm_flags);
	// vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	if (vm_iomap_memory(vma, vpmem->start, vpmem->size) < 0) 
	{
		pr_err("could not map the address area\n");
		return -EIO;
	}
	pr_info(">>after flags: vma->vm_flags=0x%x", vma->vm_flags);
	vma->vm_flags = VM_IO | VM_PFNMAP | VM_DONTEXPAND | VM_DONTDUMP | VM_MIXEDMAP | VM_READ | VM_WRITE;

	return 0;
}

static const struct file_operations __maybe_unused char_pmem_fops = {
#if 0
	.llseek		= memory_lseek,
	.read		= read_mem,
	.write		= write_mem,
#endif
	.mmap		= mmap_mem,
	.open		= open_mem,
};

static int virtio_pmem_probe(struct virtio_device *vdev)
{
//	struct virtio_pmem *vpmem;
	int ret;
	struct resource res, *req;
	int err = 0;

	if (!vdev->config->get) {
		dev_err(&vdev->dev, "%s failure: config access disabled\n",
			__func__);
		return -EINVAL;
	}

	vpmem = devm_kzalloc(&vdev->dev, sizeof(*vpmem), GFP_KERNEL);
	if (!vpmem) {
		err = -ENOMEM;
		goto out_err;
	}
	vpmem->vdev = vdev;
	vdev->priv = vpmem;
	err = init_vq(vpmem);
	if (err) {
		dev_err(&vdev->dev, "failed to initialize virtio pmem vq's\n");
		goto out_err;
	}

	virtio_cread_le(vpmem->vdev, struct virtio_pmem_config,
			start, &vpmem->start);
	virtio_cread_le(vpmem->vdev, struct virtio_pmem_config,
			size, &vpmem->size);

	res.start = vpmem->start;
	res.end   = vpmem->start + vpmem->size - 1;
	req = devm_request_mem_region(&vdev->dev, res.start, vpmem->size,
				dev_name(&vdev->dev));
	if (!req)
	{
		dev_warn(&vdev->dev, "could not reserve region %pR\n", res);
	}
	else
	{
		dev_info(&vdev->dev, "reserved region %pR\n", req);
	}

	return misc_register(&char_pmem_miscdev);

out_err:
	return err;
}

static void virtio_pmem_remove(struct virtio_device *vdev)
{
	vdev->config->del_vqs(vdev);
	virtio_reset_device(vdev);
}

// TODO jk
static struct virtio_driver virtio_char_pmem_driver = {
	.driver.name	= KBUILD_MODNAME,
	.driver.owner	= THIS_MODULE,
	.id_table		= id_table,
	.probe			= virtio_pmem_probe,
	.remove			= virtio_pmem_remove,
};

static struct miscdevice char_pmem_miscdev = {
	.minor		= MISC_DYNAMIC_MINOR,
	.name		= "char_pmem",
	.fops		= &char_pmem_fops,
};

module_virtio_driver(virtio_char_pmem_driver);

MODULE_DEVICE_TABLE(virtio, id_table);
MODULE_DESCRIPTION("Virtio pmem char driver");
MODULE_LICENSE("GPL");

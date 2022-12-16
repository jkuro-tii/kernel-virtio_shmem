#include <linux/init.h> 
#include <linux/module.h> 
#include <linux/kernel.h> 
 
static int __init virtio_shmem_init(void) { 
    pr_info("virtio shmem memory init!\n"); 
    return 0; 
} 
 
static void __exit virtio_shmem_init(void) { 
    pr_info("virtio shmem memory exit\n"); 
} 
 
module_init(virtio_shmem_init);
module_exit(virtio_shmem_exit);
MODULE_AUTHOR("Jaroslaw Kurowski <jaroslaw.kurowski@tii.ae>"); 
MODULE_LICENSE("GPL"); 
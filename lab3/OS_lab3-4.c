#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/version.h>

#define PROCFS_NAME "tsulab"

static struct proc_dir_entry *our_proc_file = NULL;

// Параметры задачи
#define EARTH_EQUATOR_KM    40075L
#define HINDENBURG_SPEED_KMH 126L

static ssize_t procfile_read(struct file *file, char __user *buffer,
                             size_t len, loff_t *offset)
{
    // Вычисление в минутах для точности
    const long total_minutes = (EARTH_EQUATOR_KM * 60L) / HINDENBURG_SPEED_KMH;

    const long days = total_minutes / (60L * 24L);
    const long hours = (total_minutes % (60L * 24L)) / 60L;
    const long minutes = total_minutes % 60L;

    char msg[128];
    int msg_len = scnprintf(msg, sizeof(msg),
        "Hindenburg will return in %ld days %ld hours %ld minutes\n",
        days, hours, minutes);

    if (*offset >= msg_len)
        return 0;

    if (len > (size_t)(msg_len - *offset))
        len = msg_len - *offset;

    if (copy_to_user(buffer, msg + *offset, len))
        return -EFAULT;

    *offset += len;
    return len;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
static const struct proc_ops proc_file_fops = {
    .proc_read = procfile_read,
};
#else
static const struct file_operations proc_file_fops = {
    .read = procfile_read,
};
#endif

static int __init tsulab_init(void)
{
    pr_info("Welcome to the Tomsk State University\n");
    our_proc_file = proc_create(PROCFS_NAME, 0644, NULL, &proc_file_fops);
    if (!our_proc_file) {
        pr_err("Failed to create /proc/%s\n", PROCFS_NAME);
        return -ENOMEM;
    }
    return 0;
}

static void __exit tsulab_exit(void)
{
    proc_remove(our_proc_file);
    pr_info("Tomsk State University forever!\n");
}

module_init(tsulab_init);
module_exit(tsulab_exit);
MODULE_LICENSE("GPL");

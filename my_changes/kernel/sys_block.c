#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/syscall.h>
#include <asm/uaccess.h>
#include <linux/list.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/init.h>




// Define the limited file structure
struct limited_file {
    struct list_head list;
    char *filename;
};

// Global list of limited files (protected by a copy-update mechanism)
static LIST_HEAD(limited_file_list);

// Add a file to the limited files list
int sys_block_add_file(const char *filename) {
    struct limited_file *new_file, *iter;
    struct list_head *pos;
    char *kfilename;


    // Check for NULL filename
    if (!filename) {
        return -EFAULT; // Invalid argument
    }


    if (!current->is_privileged) {
        return -EPERM; // Operation not permitted
    }

    // Allocate memory for the filename
    kfilename = kmalloc(PATH_MAX, GFP_KERNEL);
    if (!kfilename) {
        return -ENOMEM; // Out of memory
    }

    if (copy_from_user(kfilename, filename, PATH_MAX)) {
        kfree(kfilename);
        return -EFAULT; // Bad address
    }

    kfilename[PATH_MAX - 1] = '\0';

    list_for_each(pos, &limited_file_list) {
        iter = list_entry(pos, struct limited_file, list);
        if (strcmp(iter->filename, kfilename) == 0) {
            kfree(kfilename);
            return 0; // File already exists
        }
    }

    new_file = kmalloc(sizeof(*new_file), GFP_KERNEL);
    if (!new_file) {
        kfree(kfilename);
        return -ENOMEM; // Out of memory
    }

    new_file->filename = kfilename;
    list_add_tail(&new_file->list, &limited_file_list);

    return 0; // Success
}


// Clear the limited files list
int sys_block_clear(void) {
    struct list_head *pos, *tmp;
    struct limited_file *entry;

    if (!current->is_privileged) {
        return -EPERM; // Operation not permitted
    }

    list_for_each_safe(pos, tmp, &limited_file_list) {
        entry = list_entry(pos, struct limited_file, list);
        list_del(&entry->list);
        kfree(entry->filename); // Free memory for filename
        kfree(entry); // Free memory for the limited_file structure
    }

    return 0; // Success
}


// Check if a file is in the limited files list
int sys_block_query(const char *filename) {
    struct list_head *pos;
    struct limited_file *file;
    char *kfilename;

    // Check for NULL filename
    if (!filename) {
        return -EINVAL; // Invalid argument
    }

    // Allocate memory for the filename in kernel space
    kfilename = kmalloc(PATH_MAX, GFP_KERNEL);
    if (!kfilename) {
        return -ENOMEM; // Out of memory
    }

    // Copy the filename from user space
    if (copy_from_user(kfilename, filename, PATH_MAX)) {
        kfree(kfilename); // Free memory if copy fails
        return -EFAULT; // Bad address
    }

    // Ensure filename is null-terminated
    kfilename[PATH_MAX - 1] = '\0';

    // Search the blocked files list for the given filename
    list_for_each(pos, &limited_file_list) {
        file = list_entry(pos, struct limited_file, list);
        if (strcmp(file->filename, kfilename) == 0) {
            kfree(kfilename);
            return 1; // File is restricted
        }
    }

    // File not found in the list
    kfree(kfilename);
    return 0; // File is not restricted
}


static int system_initialized = 0;


static int __init block_files_init(void) {

    INIT_LIST_HEAD(&limited_file_list);

    /* Verify lists are empty */
    if (!list_empty(&limited_file_list)) {
        return -EINVAL;
    }

    // Set the initialized flag
    return 0;
}

static void __exit block_files_exit(void) {

    // Clear the list and reset initialization flag
    sys_block_clear();
}


// Implementing new_open with restrictions
long sys_new_open(const char *filename, int flags, int mode) {
    long ret;

    // Check if the file is restricted
    struct limited_file *iter;
    struct list_head *pos;
    list_for_each(pos, &limited_file_list) {
        iter = list_entry(pos, struct limited_file, list);

        if (strcmp(iter->filename, filename) == 0) {
            if (!current->is_privileged) {
                return -EPERM; // Permission denied
            }
        }
    }

    // If not restricted, proceed with sys_open
    ret = sys_open(filename, flags, mode);

    return ret;
}

// Define a new field in task_struct for privileged processes (this requires kernel modification).
// Assuming is_privileged is added to task_struct.

int sys_block_add_process(pid_t pid) {
    struct task_struct *current_process = current; // The current process
    struct task_struct *target_process;           // The target process
    struct task_struct *p;                        // Iterator for process list

    int privileged_exists = 0;

    // Step 1: Check if the current process is privileged
    if (!current_process->is_privileged) {
        // Search for any existing privileged process in the system
        for_each_task(p) {
            if (p->is_privileged) {
                privileged_exists = 1;
                break;
            }
        }

        if (privileged_exists) {
            // If a privileged process exists and the current process is not privileged
            return -EPERM;
        }
    }

    // Step 2: Locate the target process by PID
    target_process = find_task_by_pid(pid);
    if (!target_process) {
        // If no process with the given PID is found
        return -ESRCH;
    }

    // Step 3: Mark the target process as privileged
    target_process->is_privileged = 1;



    return 0; // Success
}
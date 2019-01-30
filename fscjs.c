#include <linux/init.h>
#include<linux/module.h>
#include <linux/fs.h>
#include <linux/pagemap.h>
#include <linux/slab.h>

#define FSCJS_MAGIC		0x858458f6
static const struct super_operations fscjs_super_ops;
static const struct inode_operations fscjs_dir_inode_operations;
static const struct file_operations fscjs_file_operations;
static const struct inode_operations fscjs_file_inode_operations;

static const struct super_operations fscjs_super_ops= {
	.statfs		= simple_statfs,
	.drop_inode	= generic_delete_inode,
};

struct inode *fscjs_get_inode(struct super_block *sb,
				const struct inode *dir, umode_t mode, dev_t dev)
{
	struct inode * inode = new_inode(sb);

	if (inode) {
		inode->i_ino = get_next_ino();
		inode_init_owner(inode, dir, mode);
		inode->i_atime = inode->i_mtime = inode->i_ctime = current_time(inode);
		switch (mode & S_IFMT) {
		default:
			init_special_inode(inode, mode, dev);
			break;
		case S_IFREG:
			inode->i_op = &fscjs_file_inode_operations;
			inode->i_fop = &fscjs_file_operations;
			break;
		case S_IFDIR:
			inode->i_op = &fscjs_dir_inode_operations;
			inode->i_fop = &simple_dir_operations;

			/* directory inodes start off with i_nlink == 2 (for "." entry) */
			inc_nlink(inode);
			break;
		case S_IFLNK:
			inode->i_op = &page_symlink_inode_operations;
			inode_nohighmem(inode);
			break;
		}
	}
	return inode;
}

fscjs_mknod(struct inode *dir, struct dentry *dentry, umode_t mode, dev_t dev)
{
	struct inode * inode = fscjs_get_inode(dir->i_sb, dir, mode, dev);
	int error = -ENOSPC;

	if (inode) {
		d_instantiate(dentry, inode);
		dget(dentry);	/* Extra count - pin the dentry in core */
		error = 0;
		dir->i_mtime = dir->i_ctime = current_time(dir);
	}
	return error;
}

static int fscjs_mkdir(struct inode * dir, struct dentry * dentry, umode_t mode)
{
	int retval = fscjs_mknod(dir, dentry, mode | S_IFDIR, 0);
	if (!retval)
		inc_nlink(dir);
	return retval;
}
static const struct inode_operations fscjs_dir_inode_operations = {
	.lookup		= simple_lookup,
	.link		= simple_link,
	.mkdir		= fscjs_mkdir,
	.rmdir		= simple_rmdir,
};

static const struct file_operations fscjs_file_operations = {
	.open		= generic_file_open,
	.read_iter	= generic_file_read_iter,
	.write_iter	= generic_file_write_iter,
	.splice_read	= generic_file_splice_read,
	.splice_write	= iter_file_splice_write,
	.llseek		= generic_file_llseek,
};

static const struct inode_operations fscjs_file_inode_operations = {
	.setattr	= simple_setattr,
	.getattr	= simple_getattr,
};



struct fscjs_mount_opts {
	umode_t mode;
};

struct fscjs_fs_info {
	struct fscjs_mount_opts mount_opts;
};

static int fscjs_fill_sb(struct super_block *sb, void *data, int silent)
  {
      struct fscjs_fs_info *fsi = kzalloc(sizeof(*fsi), GFP_KERNEL);;
	struct inode *inode;

	sb->s_fs_info = fsi;
	if (!fsi)
		return -ENOMEM;
	sb->s_maxbytes		= MAX_LFS_FILESIZE;
	sb->s_blocksize		= PAGE_SIZE;
	sb->s_blocksize_bits	= PAGE_SHIFT;
	sb->s_magic		= FSCJS_MAGIC;
	sb->s_op		= &fscjs_super_ops;
	sb->s_time_gran		= 1;

	inode = fscjs_get_inode(sb, NULL, S_IFDIR | fsi->mount_opts.mode, 0);
	sb->s_root = d_make_root(inode);
	if (!sb->s_root)
		return -ENOMEM;

	return 0;
 }

/* fscjs_mount function should return dentry, which represents the root catalogue of our file system. fscjs_fill_sb function will create it. */

static struct dentry *fscjs_mount(struct file_system_type *type, int flags,
                                      char const *dev, void *data)
  {
      return mount_nodev(type, flags, data, fscjs_fill_sb);
  }


static struct file_system_type fscjs_type = {
.owner = THIS_MODULE,
.name = "fscjs",
.mount = &fscjs_mount,
.kill_sb = kill_block_super,
.fs_flags	= FS_REQUIRES_DEV, FS_USERNS_MOUNT
};

static int __init fscjs_init(void)
{
register_filesystem(&fscjs_type);
printk(KERN_ALERT "File System Loaded\n");
return 0;
}

static void __exit fscjs_exit(void)
{
printk(KERN_ALERT "File System Unloaded\n");
}

module_init(fscjs_init);
module_exit(fscjs_exit);

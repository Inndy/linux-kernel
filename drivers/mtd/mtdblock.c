/*
 * Direct MTD block device access
 *
 * $Id: mtdblock.c,v 1.68 2005/11/07 11:14:20 gleixner Exp $
 *
 * (C) 2000-2003 Nicolas Pitre <nico@cam.org>
 * (C) 1999-2003 David Woodhouse <dwmw2@infradead.org>
 */

#include <linux/config.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/vmalloc.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/blktrans.h>

static struct mtdblk_dev {
	struct mtd_info *mtd;
	int count;
	struct semaphore cache_sem;
	unsigned char *cache_data;
	unsigned long cache_offset;
	unsigned int cache_size;
	enum { STATE_EMPTY, STATE_CLEAN, STATE_DIRTY } cache_state;
} *mtdblks[MAX_MTD_DEVICES];

#if (HUMAX_MODIFY == y)
typedef struct _BlkInfo
{
	unsigned long ulAddr;
	unsigned long ulEraseSize;
	unsigned long ulNumBlocks;
} BlkInfo;

typedef struct _FlashBlkInfo
{
	unsigned long ulNumRegions;
	BlkInfo		EraseRegions[10]; //max supported region cnt
} FlashBlkInfo;
#endif

/*
 * Cache stuff...
 *
 * Since typical flash erasable sectors are much larger than what Linux's
 * buffer cache can handle, we must implement read-modify-write on flash
 * sectors for each block write requests.  To avoid over-erasing flash sectors
 * and to speed things up, we locally cache a whole flash sector while it is
 * being written to until a different sector is required.
 */

static void erase_callback(struct erase_info *done)
{
	wait_queue_head_t *wait_q = (wait_queue_head_t *)done->priv;
	wake_up(wait_q);
}


int erase_block (struct mtd_info *mtd, unsigned long pos)
{
	struct erase_info erase;
	DECLARE_WAITQUEUE(wait, current);
	wait_queue_head_t wait_q;
	size_t retlen;
	int ret;

	/*
	 * First, let's erase the flash block.
	 */
	init_waitqueue_head(&wait_q);
	erase.mtd = mtd;
	erase.callback = erase_callback;
#if (HUMAX_MODIFY == y)
	erase.addr = (pos & 0x7fffffff);
	erase.len = ((pos & 0x80000000)?0x8000:0x20000);
#else
	erase.addr = pos;
	erase.len = 0x20000;
#endif
	erase.priv = (u_long)&wait_q;

	set_current_state(TASK_INTERRUPTIBLE);
	add_wait_queue(&wait_q, &wait);

	ret = MTD_ERASE(mtd, &erase);
	if (ret) {
		set_current_state(TASK_RUNNING);
		remove_wait_queue(&wait_q, &wait);
		printk (KERN_WARNING "mtdblock: erase of region [0x%lx, 0x%x] "
				     "on \"%s\" failed\n",
			pos, 0x20000, mtd->name);
		return ret;
	}

	schedule();  /* Wait for erase to finish. */
	remove_wait_queue(&wait_q, &wait);

	return 0;
}


static int erase_write (struct mtd_info *mtd, unsigned long pos,
			int len, const char *buf)
{
	size_t retlen;
	int ret;

	/*
	 *  write data to flash.
	 */

	ret = MTD_WRITE (mtd, pos, len, &retlen, buf);
	if (ret)
		return ret;
	if (retlen != len)
		return -EIO;
	return 0;
}


static int write_cached_data (struct mtdblk_dev *mtdblk)
{
	struct mtd_info *mtd = mtdblk->mtd;
	int ret;

	if (mtdblk->cache_state != STATE_DIRTY)
		return 0;

	DEBUG(MTD_DEBUG_LEVEL2, "mtdblock: writing cached data for \"%s\" "
			"at 0x%lx, size 0x%x\n", mtd->name,
			mtdblk->cache_offset, mtdblk->cache_size);

	ret = erase_write (mtd, mtdblk->cache_offset,
			   mtdblk->cache_size, mtdblk->cache_data);
	if (ret)
		return ret;

	/*
	 * Here we could argubly set the cache state to STATE_CLEAN.
	 * However this could lead to inconsistency since we will not
	 * be notified if this content is altered on the flash by other
	 * means.  Let's declare it empty and leave buffering tasks to
	 * the buffer cache instead.
	 */
	mtdblk->cache_state = STATE_EMPTY;
	return 0;
}



static int do_block_erase (struct mtdblk_dev *mtdblk, unsigned long pos)
{
	struct mtd_info *mtd = mtdblk->mtd;
	unsigned int sect_size = mtdblk->cache_size;
	size_t retlen;
	int ret;

#if (HUMAX_MODIFY == y)
	DEBUG(MTD_DEBUG_LEVEL2, "mtdblock: erase on \"%s\" at 0x%lx\n",
		mtd->name, pos);
#else
	DEBUG(MTD_DEBUG_LEVEL2, "mtdblock: write on \"%s\" at 0x%lx, size 0x%x\n",
		mtd->name, pos);
#endif

	ret = erase_block (mtd, pos);
	mtdblk->cache_state = STATE_EMPTY;
	if (ret)
		return ret;
	return 0;
}
// JHK 08.03.09
static int do_get_cfiinfo (struct mtdblk_dev *mtdblk, unsigned char ids)
{
	struct mtd_info *mtd = mtdblk->mtd;

	DEBUG(MTD_DEBUG_LEVEL2, "~~~ do_get_cfiinfo : %s_ID ~~~\n",(ids ==0)?"m":"d");
	
	return MTD_GETCFIIDS( mtd, ids);
}


static int do_cached_write (struct mtdblk_dev *mtdblk, unsigned long pos,
			    int len, const char *buf)
{
	struct mtd_info *mtd = mtdblk->mtd;
	unsigned int sect_size = mtdblk->cache_size;
	size_t retlen;
	int ret;

	DEBUG(MTD_DEBUG_LEVEL2, "mtdblock: write on \"%s\" at 0x%lx, size 0x%x\n",
		mtd->name, pos, len);

	if (!sect_size)
		return MTD_WRITE (mtd, pos, len, &retlen, buf);

	while (len > 0) {
		unsigned long sect_start = (pos/sect_size)*sect_size;
		unsigned int offset = pos - sect_start;
		unsigned int size = sect_size - offset;
		if( size > len )
			size = len;

		if (1) // (size == sect_size) 
		{
			/*
			 * We are covering a whole sector.  Thus there is no
			 * need to bother with the cache while it may still be
			 * useful for other partial writes.
			 */
			ret = erase_write (mtd, pos, size, buf);
			mtdblk->cache_state = STATE_EMPTY;
			if (ret)
				return ret;
		} else {
			/* Partial sector: need to use the cache */

			if (mtdblk->cache_state == STATE_DIRTY &&
			    mtdblk->cache_offset != sect_start) {
				ret = write_cached_data(mtdblk);
				if (ret)
					return ret;
			}

			if (mtdblk->cache_state == STATE_EMPTY ||
			    mtdblk->cache_offset != sect_start) {
				/* fill the cache with the current sector */
				mtdblk->cache_state = STATE_EMPTY;
				ret = MTD_READ(mtd, sect_start, sect_size, &retlen, mtdblk->cache_data);
				if (ret)
					return ret;
				if (retlen != sect_size)
					return -EIO;

				mtdblk->cache_offset = sect_start;
				mtdblk->cache_size = sect_size;
				mtdblk->cache_state = STATE_CLEAN;
			}

			/* write data to our local cache */
			memcpy (mtdblk->cache_data + offset, buf, size);
			mtdblk->cache_state = STATE_DIRTY;
		}

		buf += size;
		pos += size;
		len -= size;
	}

	return 0;
}


static int do_cached_read (struct mtdblk_dev *mtdblk, unsigned long pos,
			   int len, char *buf)
{
	struct mtd_info *mtd = mtdblk->mtd;
	unsigned int sect_size = mtdblk->cache_size;
	size_t retlen;
	int ret;

	DEBUG(MTD_DEBUG_LEVEL2, "mtdblock: read on \"%s\" at 0x%lx, size 0x%x\n",
			mtd->name, pos, len);

	if (!sect_size)
		return MTD_READ (mtd, pos, len, &retlen, buf);

	while (len > 0) {
		unsigned long sect_start = (pos/sect_size)*sect_size;
		unsigned int offset = pos - sect_start;
		unsigned int size = sect_size - offset;
		if (size > len)
			size = len;

		/*
		 * Check if the requested data is already cached
		 * Read the requested amount of data from our internal cache if it
		 * contains what we want, otherwise we read the data directly
		 * from flash.
		 */
		if (mtdblk->cache_state != STATE_EMPTY &&
		    mtdblk->cache_offset == sect_start) {
			memcpy (buf, mtdblk->cache_data + offset, size);
		} else {
			ret = MTD_READ (mtd, pos, size, &retlen, buf);
			if (ret)
				return ret;
			if (retlen != size)
				return -EIO;
		}

		buf += size;
		pos += size;
		len -= size;
	}

	return 0;
}

// JHK 08.03.09
int mtd_getcfiids(struct mtd_blktrans_dev *dev, unsigned char ids)
{
	struct mtdblk_dev *mtdblk = mtdblks[dev->devnum];

	return do_get_cfiinfo(mtdblk, ids);
}

static int mtdblock_readsect(struct mtd_blktrans_dev *dev,
			      unsigned long block, char *buf)
{
	struct mtdblk_dev *mtdblk = mtdblks[dev->devnum];
	return do_cached_read(mtdblk, block<<9, 512, buf);
}

static int mtdblock_writesect(struct mtd_blktrans_dev *dev,
			      unsigned long block, char *buf)
{
	struct mtdblk_dev *mtdblk = mtdblks[dev->devnum];
	if (unlikely(!mtdblk->cache_data && mtdblk->cache_size)) {
		mtdblk->cache_data = vmalloc(mtdblk->mtd->erasesize);
		if (!mtdblk->cache_data)
			return -EINTR;
		/* -EINTR is not really correct, but it is the best match
		 * documented in man 2 write for all cases.  We could also
		 * return -EAGAIN sometimes, but why bother?
		 */
	}
	return do_cached_write(mtdblk, block<<9, 512, buf);
}

#if (HUMAX_MODIFY == y)
int mtdblock_readsect2(struct mtd_blktrans_dev *dev,
			      unsigned long startaddr, unsigned long len, char *buf)
{
	struct mtdblk_dev *mtdblk = mtdblks[dev->devnum];
	return do_cached_read(mtdblk, startaddr, len, buf);
}
#endif

int mtdblock_writesect2(struct mtd_blktrans_dev *dev,
			      unsigned long startaddr, unsigned long len, char *buf)
{
	struct mtdblk_dev *mtdblk = mtdblks[dev->devnum];

	if (unlikely(!mtdblk->cache_data && mtdblk->cache_size)) {
		mtdblk->cache_data = vmalloc(mtdblk->mtd->erasesize);
		if (!mtdblk->cache_data)
			return -EINTR;
		/* -EINTR is not really correct, but it is the best match
		 * documented in man 2 write for all cases.  We could also
		 * return -EAGAIN sometimes, but why bother?
		 */
	}
	return do_cached_write(mtdblk, startaddr,(int)len, buf);
}

int mtdblock_erasesect(struct mtd_blktrans_dev *dev,unsigned long block)
{
	struct mtdblk_dev *mtdblk = mtdblks[dev->devnum];
	return do_block_erase(mtdblk, block);
}

#if (HUMAX_MODIFY == y)
#ifdef CONFIG_MTD_OTP
static int do_read_protregs (struct mtdblk_dev *mtdblk, unsigned long pos,
			    int len, const char *buf)
{
	struct mtd_info *mtd = mtdblk->mtd;
	size_t retlen;
	int ret;

	ret = mtd->read_user_prot_reg(mtd,pos,len,&retlen,buf);
	if(ret != 0)
		return -EIO;
	return 0;
}

int mtdblock_readprotregs(struct mtd_blktrans_dev *dev,
			      unsigned long startaddr, unsigned long len, char *buf)
{
	struct mtdblk_dev *mtdblk = mtdblks[dev->devnum];
	return do_read_protregs(mtdblk, startaddr, len, buf);
}

static int do_write_protregs (struct mtdblk_dev *mtdblk, unsigned long pos,
			    int len, const char *buf)
{
	struct mtd_info *mtd = mtdblk->mtd;
	size_t retlen;
	int ret;

	ret = mtd->write_user_prot_reg(mtd,pos,len,&retlen,buf);
	if(ret != 0)
		return -EIO;
	return 0;
}

int mtdblock_writeprotregs(struct mtd_blktrans_dev *dev,
			      unsigned long startaddr, unsigned long len, char *buf)
{
	struct mtdblk_dev *mtdblk = mtdblks[dev->devnum];
	return do_write_protregs(mtdblk, startaddr, len, buf);
}

static int do_lock_protregs (struct mtdblk_dev *mtdblk, unsigned long pos,
			    int len)
{
	struct mtd_info *mtd = mtdblk->mtd;
	int ret;

	ret = mtd->lock_user_prot_reg(mtd,pos,len);
	if(ret != 0)
		return -EIO;
	return 0;
}

int mtdblock_lockprotregs(struct mtd_blktrans_dev *dev,
			      unsigned long startaddr, unsigned long len)
{
	struct mtdblk_dev *mtdblk = mtdblks[dev->devnum];
	return do_lock_protregs(mtdblk, startaddr, len);
}
#endif
int mtdblock_getblockinfo(struct mtd_blktrans_dev *dev,unsigned long id, unsigned char *buf)
{
	struct mtdblk_dev *mtdblk;
	struct mtd_info *mtd;
	int i;
	int region_cnt=0;
	FlashBlkInfo *pFBI;

	mtdblk = mtdblks[dev->devnum];
	mtd = mtdblk->mtd;

	if(id == 0)
	{
		pFBI = (FlashBlkInfo *)buf;	
		pFBI->ulNumRegions= mtd->numeraseregions;
		region_cnt = ((mtd->numeraseregions>=10)?10:mtd->numeraseregions);
		for(i=0; i<region_cnt; i++)
		{	
			pFBI->EraseRegions[i].ulAddr = mtd->eraseregions[i].offset;
			pFBI->EraseRegions[i].ulEraseSize= mtd->eraseregions[i].erasesize;
			pFBI->EraseRegions[i].ulNumBlocks= mtd->eraseregions[i].numblocks;
		}			
	}
		
	printk("numeraseregions = %d\n",mtd->numeraseregions);
	printk("size of mtd_erase_region_info = %d\n",sizeof(struct mtd_erase_region_info));
	for(i=0;i<mtd->numeraseregions;i++)
	{
		printk("offset = 0x%lx\n",mtd->eraseregions[i].offset);
		printk("erase size = 0x%lx\n",mtd->eraseregions[i].erasesize);
		printk("numblocks = %ld\n",mtd->eraseregions[i].numblocks);
	}
	return 0;
}
#endif

static int mtdblock_open(struct mtd_blktrans_dev *mbd)
{
	struct mtdblk_dev *mtdblk;
	struct mtd_info *mtd = mbd->mtd;
	int dev = mbd->devnum;

	DEBUG(MTD_DEBUG_LEVEL1,"mtdblock_open\n");

	if (mtdblks[dev]) {
		mtdblks[dev]->count++;
		return 0;
	}

	/* OK, it's not open. Create cache info for it */
	mtdblk = kmalloc(sizeof(struct mtdblk_dev), GFP_KERNEL);
	if (!mtdblk)
		return -ENOMEM;

	memset(mtdblk, 0, sizeof(*mtdblk));
	mtdblk->count = 1;
	mtdblk->mtd = mtd;

	init_MUTEX (&mtdblk->cache_sem);
	mtdblk->cache_state = STATE_EMPTY;
	if ((mtdblk->mtd->flags & MTD_CAP_RAM) != MTD_CAP_RAM &&
	    mtdblk->mtd->erasesize) {
		mtdblk->cache_size = mtdblk->mtd->erasesize;
		mtdblk->cache_data = NULL;
	}

	mtdblks[dev] = mtdblk;

	DEBUG(MTD_DEBUG_LEVEL1, "ok\n");

	return 0;
}

static int mtdblock_release(struct mtd_blktrans_dev *mbd)
{
	int dev = mbd->devnum;
	struct mtdblk_dev *mtdblk = mtdblks[dev];

   	DEBUG(MTD_DEBUG_LEVEL1, "mtdblock_release\n");

	down(&mtdblk->cache_sem);
	write_cached_data(mtdblk);
	up(&mtdblk->cache_sem);

	if (!--mtdblk->count) {
		/* It was the last usage. Free the device */
		mtdblks[dev] = NULL;
		if (mtdblk->mtd->sync)
			mtdblk->mtd->sync(mtdblk->mtd);
		vfree(mtdblk->cache_data);
		kfree(mtdblk);
	}
	DEBUG(MTD_DEBUG_LEVEL1, "ok\n");

	return 0;
}

static int mtdblock_flush(struct mtd_blktrans_dev *dev)
{
	struct mtdblk_dev *mtdblk = mtdblks[dev->devnum];

	down(&mtdblk->cache_sem);
	write_cached_data(mtdblk);
	up(&mtdblk->cache_sem);

	if (mtdblk->mtd->sync)
		mtdblk->mtd->sync(mtdblk->mtd);
	return 0;
}

static void mtdblock_add_mtd(struct mtd_blktrans_ops *tr, struct mtd_info *mtd)
{
	struct mtd_blktrans_dev *dev = kmalloc(sizeof(*dev), GFP_KERNEL);

	if (!dev)
		return;

	memset(dev, 0, sizeof(*dev));

	dev->mtd = mtd;
	dev->devnum = mtd->index;
	dev->blksize = 512;
	dev->size = mtd->size >> 9;
	dev->tr = tr;

	if (!(mtd->flags & MTD_WRITEABLE))
		dev->readonly = 1;

	add_mtd_blktrans_dev(dev);
}

static void mtdblock_remove_dev(struct mtd_blktrans_dev *dev)
{
	del_mtd_blktrans_dev(dev);
	kfree(dev);
}

static struct mtd_blktrans_ops mtdblock_tr = {
	.name		= "mtdblock",
	.major		= 31,
	.part_bits	= 0,
	.open		= mtdblock_open,
	.flush		= mtdblock_flush,
	.release	= mtdblock_release,
	.readsect	= mtdblock_readsect,
	.writesect	= mtdblock_writesect,
	.add_mtd	= mtdblock_add_mtd,
	.remove_dev	= mtdblock_remove_dev,
	.owner		= THIS_MODULE,
};

static int __init init_mtdblock(void)
{
	return register_mtd_blktrans(&mtdblock_tr);
}

static void __exit cleanup_mtdblock(void)
{
	deregister_mtd_blktrans(&mtdblock_tr);
}

module_init(init_mtdblock);
module_exit(cleanup_mtdblock);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nicolas Pitre <nico@cam.org> et al.");
MODULE_DESCRIPTION("Caching read/erase/writeback block device emulation access to MTD devices");

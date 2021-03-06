/*MIT License

Copyright (c) 2018 Ievgen Korokyi

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.*/

#include "curl_drv.h"

static int curl_ctrl_open(struct inode *inode, struct file *filp)
{
    printk(KERN_DEBUG "curl_ctrl_open\n");
	return 0;
}

static int curl_ctrl_release(struct inode *inode, struct file *filp)
{
    printk(KERN_DEBUG "curl_ctrl_release\n");
    return 0;
}

static ssize_t curl_ctrl_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    struct fpga_curl_dev *curl_dev;
    int ret;
    
    u32 ctrl_reg_dat = 0;
    u32 hash_cnt = 0;
    u32 tick_cnt_l = 0;
    u32 tick_cnt_h = 0;
    u64 tick_cnt = 0;

    curl_dev = container_of(filp->private_data, struct fpga_curl_dev, ctrl_dev);

    curl_dev->write_done = 0;

    printk(KERN_DEBUG "curl_ctrl_read begin\n");

    ctrl_reg_dat = 2;

    //printk(KERN_DEBUG "ctrl_reg_dat = %d\n", ctrl_reg_dat);

    iowrite32(ctrl_reg_dat, curl_dev->regs + MAIN_CTRL_REG_OFFSET);
    
    ret = wait_event_interruptible(curl_dev->data_queue, curl_dev->write_done == 1);
    
    if( ret ) {
        return ret;
    }

    hash_cnt = ioread32(curl_dev->regs + HASH_CNT_REG_OFFSET);
    tick_cnt_l = ioread32(curl_dev->regs + TICK_CNT_LOW_REG_OFFSET);
    tick_cnt_h = ioread32(curl_dev->regs + TICK_CNT_HI_REG_OFFSET);
    tick_cnt = tick_cnt_h;
    tick_cnt = (tick_cnt << 32) | tick_cnt_l;
      
    printk(KERN_DEBUG "hash_cnt = %d\n", hash_cnt);
    printk(KERN_DEBUG "tick_cnt = %lld\n", tick_cnt);

    printk(KERN_DEBUG "curl_ctrl_read end\n");

    return 0;        
}

static ssize_t curl_ctrl_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    struct fpga_curl_dev *curl_dev;

    loff_t maxpos;

    u8 mwm;
    u32 mwm_mask;

    curl_dev = container_of(filp->private_data, struct fpga_curl_dev, ctrl_dev);
        
    maxpos = 1;

    printk(KERN_DEBUG "curl_ctrl_dev_write begin\n");

    if (*f_pos > maxpos)
        return -EINVAL;
    
    if (*f_pos == maxpos)
        return -ENOSPC;

    if (*f_pos + count > maxpos)
        count = maxpos - *f_pos;

    if (copy_from_user(&mwm + *f_pos, buf, count)) {
        printk(KERN_ERR "Fail copy_from_user in curl_idata_dev_write :(\n");
        return -EFAULT;
    }

    mwm_mask = (1 << mwm) - 1;

    iowrite32(mwm_mask, curl_dev->regs + MWM_MASK_REG_OFFSET);

    printk(KERN_DEBUG "curl_ctrl_dev_write end\n");

    return count;
}

const struct file_operations curl_ctrl_dev_fops = {
	.owner   = THIS_MODULE,
	.open    = curl_ctrl_open,
	.release = curl_ctrl_release,
    .write   = curl_ctrl_write,
	.read    = curl_ctrl_read,    
};


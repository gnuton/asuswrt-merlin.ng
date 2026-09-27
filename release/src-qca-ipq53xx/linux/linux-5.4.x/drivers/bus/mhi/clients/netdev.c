// SPDX-License-Identifier: GPL-2.0-only
/* Copyright (c) 2018-2020, The Linux Foundation. All rights reserved.*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/msm_rmnet.h>
#include <linux/if_arp.h>
#include <linux/dma-mapping.h>
#include <linux/debugfs.h>
#include <linux/ipc_logging.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/of_device.h>
#include <linux/rtnetlink.h>
#include <linux/mhi.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/jiffies.h>
#include <linux/types.h>
#include <asm/arch_timer.h>

#define MHI_RMNET_IF_NAME "rmnet_mhi"
#define MHI_NETDEV_DRIVER_NAME "mhi_netdev"
#define WATCHDOG_TIMEOUT (30 * HZ)
#define IPC_LOG_PAGES (100)
#define MAX_NETBUF_SIZE (128)

static bool mhi_rate_control;
module_param(mhi_rate_control, bool, 0644);

static unsigned long long mhi_byte_ceiling = 21200000;
module_param(mhi_byte_ceiling, ullong, 0644);

static unsigned long long mhi_sample_time_ceiling = 100;
module_param(mhi_sample_time_ceiling, ullong, 0644);

static unsigned long long mhi_drop_count;
module_param(mhi_drop_count, ullong, 0444);

static unsigned long long mhi_sample_time;
module_param(mhi_sample_time, ullong, 0444);

struct mhi_net_chain {
	struct sk_buff *head, *tail; /* chained skb */
};

struct mhi_netdev {
	int alias;
	struct mhi_device *mhi_dev;
	struct mhi_netdev *rsc_dev; /* rsc linked node */
	bool is_rsc_dev;
	int wake;

	u32 mru;
	u32 order;
	const char *interface_name;
	struct napi_struct *napi;
	struct net_device *ndev;
	bool ethernet_interface;

	struct mhi_netbuf **netbuf_pool;
	int pool_size; /* must be power of 2 */
	int current_index;
	bool chain_skb;
	struct mhi_net_chain *chain;
	bool is_rmnet;

	struct dentry *dentry;
	ktime_t first_jiffy;
	u64 bytes_received_1;
};

struct mhi_netdev_priv {
	struct mhi_netdev *mhi_netdev;
};

/* Try not to make this structure bigger than 128 bytes, since this take space
 * in payload packet.
 * Example: If MRU = 16K, effective MRU = 16K - sizeof(mhi_netbuf)
 */
struct mhi_netbuf {
	struct mhi_buf mhi_buf; /* this must be first element */
	void (*unmap)(struct device *dev, dma_addr_t addr, size_t size,
		      enum dma_data_direction dir);
};

static struct mhi_driver mhi_netdev_driver;
static void mhi_netdev_create_debugfs(struct mhi_netdev *mhi_netdev);
static u32 napi_poll_weight = NAPI_POLL_WEIGHT;
static u32 drop_at_mhi;

static unsigned long long nr_mhi_buffer;
static unsigned long long mhi_data_len;

u32 drop_at_rmnet;
unsigned long long nr_rmnet_pkts;
unsigned long long rmnet_rx_bytes;
EXPORT_SYMBOL(drop_at_rmnet);
EXPORT_SYMBOL(nr_rmnet_pkts);
EXPORT_SYMBOL(rmnet_rx_bytes);

static __be16 mhi_netdev_ip_type_trans(u8 data, bool is_rmnet)
{
	__be16 protocol = 0;

	if (is_rmnet)
		return htons(ETH_P_MAP);

	/* determine L3 protocol */
	switch (data & 0xf0) {
	case 0x40:
		protocol = htons(ETH_P_IP);
		break;
	case 0x60:
		protocol = htons(ETH_P_IPV6);
		break;
	default:
		/* default is QMAP */
		protocol = htons(ETH_P_MAP);
		break;
	}
	return protocol;
}

static struct mhi_netbuf *mhi_netdev_alloc(struct device *dev,
					   gfp_t gfp,
					   unsigned int order)
{
	struct page *page;
	struct mhi_netbuf *netbuf;
	struct mhi_buf *mhi_buf;
	void *vaddr;

	page = __dev_alloc_pages(gfp | __GFP_NOMEMALLOC, order);
	if (!page)
		return NULL;

	vaddr = page_address(page);

	/* we going to use the end of page to store cached data */
	netbuf = vaddr + (PAGE_SIZE << order) - sizeof(*netbuf);

	mhi_buf = (struct mhi_buf *)netbuf;
	mhi_buf->page = page;
	mhi_buf->buf = vaddr;
	mhi_buf->len = (void *)netbuf - vaddr;
	mhi_buf->dma_addr = dma_map_page(dev, page, 0, mhi_buf->len,
					 DMA_FROM_DEVICE);
	if (dma_mapping_error(dev, mhi_buf->dma_addr)) {
		__free_pages(mhi_buf->page, order);
		return NULL;
	}

	return netbuf;
}

static void mhi_netdev_unmap_page(struct device *dev,
				  dma_addr_t dma_addr,
				  size_t len,
				  enum dma_data_direction dir)
{
	dma_unmap_page(dev, dma_addr, len, dir);
}

static int mhi_netdev_tmp_alloc(struct mhi_netdev *mhi_netdev, int nr_tre)
{
	struct mhi_device *mhi_dev = mhi_netdev->mhi_dev;
	struct device *dev = mhi_dev->dev.parent->parent;
	struct device *dbg_dev = &mhi_dev->dev;
	const u32 order = mhi_netdev->order;
	int i, ret;

	for (i = 0; i < nr_tre; i++) {
		struct mhi_buf *mhi_buf;
		struct mhi_netbuf *netbuf = mhi_netdev_alloc(dev, GFP_ATOMIC,
							     order);
		if (!netbuf)
			return -ENOMEM;

		mhi_buf = (struct mhi_buf *)netbuf;
		netbuf->unmap = mhi_netdev_unmap_page;

		ret = mhi_queue_dma(mhi_dev, DMA_FROM_DEVICE, mhi_buf,
					 mhi_buf->len, MHI_EOT);
		if (unlikely(ret)) {
			dev_err(dbg_dev, "Failed to queue transfer, ret:%d\n", ret);
			mhi_netdev_unmap_page(dev, mhi_buf->dma_addr,
					      mhi_buf->len, DMA_FROM_DEVICE);
			__free_pages(mhi_buf->page, order);
			return ret;
		}
	}

	return 0;
}

static void mhi_netdev_queue(struct mhi_netdev *mhi_netdev)
{
	struct mhi_device *mhi_dev = mhi_netdev->mhi_dev;
	struct device *dev = mhi_dev->dev.parent->parent;
	struct device *dbg_dev = &mhi_dev->dev;
	struct mhi_netbuf *netbuf;
	struct mhi_buf *mhi_buf;
	struct mhi_netbuf **netbuf_pool = mhi_netdev->netbuf_pool;
	int nr_tre = mhi_get_no_free_descriptors(mhi_dev, DMA_FROM_DEVICE);
	int i, peak, cur_index, ret;
	const int pool_size = mhi_netdev->pool_size - 1, max_peak = 4;

	dev_dbg(dbg_dev, "Enter free_desc:%d\n", nr_tre);

	if (!nr_tre)
		return;

	/* try going thru reclaim pool first */
	for (i = 0; i < nr_tre; i++) {
		/* peak for the next buffer, we going to peak several times,
		 * and we going to give up if buffers are not yet free
		 */
		cur_index = mhi_netdev->current_index;
		netbuf = NULL;
		for (peak = 0; peak < max_peak; peak++) {
			struct mhi_netbuf *tmp = netbuf_pool[cur_index];

			mhi_buf = &tmp->mhi_buf;

			cur_index = (cur_index + 1) & pool_size;

			/* page == 1 idle, buffer is free to reclaim */
			if (atomic_read(&mhi_buf->page->_refcount) == 1) {
				netbuf = tmp;
				break;
			}
		}

		/* could not find a free buffer */
		if (!netbuf)
			break;

		/* increment reference count so when network stack is done
		 * with buffer, the buffer won't be freed
		 */
		atomic_inc(&mhi_buf->page->_refcount);
		ret = mhi_queue_dma(mhi_dev, DMA_FROM_DEVICE, mhi_buf,
					 mhi_buf->len, MHI_EOT);
		if (unlikely(ret)) {
			dev_err(dbg_dev, "Failed to queue buffer, ret:%d\n", ret);
			netbuf->unmap(dev, mhi_buf->dma_addr, mhi_buf->len,
				      DMA_FROM_DEVICE);
			atomic_dec(&mhi_buf->page->_refcount);
			return;
		}
		mhi_netdev->current_index = cur_index;
	}

	/* recyling did not work, buffers are still busy allocate temp pkts */
	if (i < nr_tre)
		mhi_netdev_tmp_alloc(mhi_netdev, nr_tre - i);
}

/* allocating pool of memory */
static int mhi_netdev_alloc_pool(struct mhi_netdev *mhi_netdev)
{
	int i;
	struct mhi_netbuf *netbuf, **netbuf_pool;
	struct mhi_buf *mhi_buf;
	const u32 order = mhi_netdev->order;
	struct device *dev = mhi_netdev->mhi_dev->dev.parent->parent;

	netbuf_pool = kmalloc_array(mhi_netdev->pool_size, sizeof(*netbuf_pool),
				    GFP_KERNEL);
	if (!netbuf_pool)
		return -ENOMEM;

	for (i = 0; i < mhi_netdev->pool_size; i++) {
		/* allocate paged data */
		netbuf = mhi_netdev_alloc(dev, GFP_KERNEL, order);
		if (!netbuf)
			goto error_alloc_page;

		netbuf->unmap = dma_sync_single_for_cpu;
		netbuf_pool[i] = netbuf;
	}

	mhi_netdev->netbuf_pool = netbuf_pool;

	return 0;

error_alloc_page:
	for (--i; i >= 0; i--) {
		netbuf = netbuf_pool[i];
		mhi_buf = &netbuf->mhi_buf;
		dma_unmap_page(dev, mhi_buf->dma_addr, mhi_buf->len,
			       DMA_FROM_DEVICE);
		__free_pages(mhi_buf->page, order);
	}

	kfree(netbuf_pool);
	return -ENOMEM;
}

static void mhi_netdev_free_pool(struct mhi_netdev *mhi_netdev)
{
	int i;
	struct mhi_netbuf *netbuf, **netbuf_pool = mhi_netdev->netbuf_pool;
	struct device *dev = mhi_netdev->mhi_dev->dev.parent->parent;
	struct mhi_buf *mhi_buf;

	for (i = 0; i < mhi_netdev->pool_size; i++) {
		netbuf = netbuf_pool[i];
		mhi_buf = &netbuf->mhi_buf;
		dma_unmap_page(dev, mhi_buf->dma_addr, mhi_buf->len,
			       DMA_FROM_DEVICE);
		__free_pages(mhi_buf->page, mhi_netdev->order);
	}

	kfree(mhi_netdev->netbuf_pool);
	mhi_netdev->netbuf_pool = NULL;
}

static int mhi_netdev_poll(struct napi_struct *napi, int budget)
{
	struct net_device *dev = napi->dev;
	struct mhi_netdev_priv *mhi_netdev_priv = netdev_priv(dev);
	struct mhi_netdev *mhi_netdev = mhi_netdev_priv->mhi_netdev;
	struct mhi_device *mhi_dev = mhi_netdev->mhi_dev;
	struct device *dbg_dev = &mhi_dev->dev;
	struct mhi_netdev *rsc_dev = mhi_netdev->rsc_dev;
	struct mhi_net_chain *chain = mhi_netdev->chain;
	int rx_work = 0;

	dev_dbg(dbg_dev, "Entered\n");

	rx_work = mhi_poll(mhi_dev, budget);

	if (likely(drop_at_mhi == 0)) {
		/* chained skb, push it to stack */
		if (chain && chain->head) {
			netif_receive_skb(chain->head);
			chain->head = NULL;
		}
	}

	if (rx_work < 0) {
		dev_err(dbg_dev, "Error polling ret:%d\n", rx_work);
		napi_complete(napi);
		return 0;
	}

	/* queue new buffers */
	mhi_netdev_queue(mhi_netdev);

	if (rsc_dev)
		mhi_netdev_queue(rsc_dev);

	/* complete work if # of packet processed less than allocated budget */
	if (rx_work < budget)
		napi_complete(napi);

	dev_dbg(dbg_dev, "polled %d pkts\n", rx_work);

	return rx_work;
}

static int mhi_netdev_open(struct net_device *dev)
{
	struct mhi_netdev_priv *mhi_netdev_priv = netdev_priv(dev);
	struct mhi_netdev *mhi_netdev = mhi_netdev_priv->mhi_netdev;
	struct mhi_device *mhi_dev = mhi_netdev->mhi_dev;
	struct device *dbg_dev = &mhi_dev->dev;

	dev_dbg(dbg_dev, "Opened net dev interface\n");

	/* tx queue may not necessarily be stopped already
	 * so stop the queue if tx path is not enabled
	 */
	if (!mhi_dev->ul_chan)
		netif_stop_queue(dev);
	else
		netif_start_queue(dev);

	return 0;
}

static int mhi_netdev_change_mtu(struct net_device *dev, int new_mtu)
{
	struct mhi_netdev_priv *mhi_netdev_priv = netdev_priv(dev);
	struct mhi_netdev *mhi_netdev = mhi_netdev_priv->mhi_netdev;
	struct mhi_device *mhi_dev = mhi_netdev->mhi_dev;

	if (new_mtu < 0 || mhi_dev->mtu < new_mtu)
		return -EINVAL;

	dev->mtu = new_mtu;
	return 0;
}

static int mhi_netdev_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct mhi_netdev_priv *mhi_netdev_priv = netdev_priv(dev);
	struct mhi_netdev *mhi_netdev = mhi_netdev_priv->mhi_netdev;
	struct mhi_device *mhi_dev = mhi_netdev->mhi_dev;
	struct device *dbg_dev = &mhi_dev->dev;
	int res = 0;

	dev_dbg(dbg_dev, "Entered\n");

	res = mhi_queue_skb(mhi_dev, DMA_TO_DEVICE, skb, skb->len,
				 MHI_EOT);
	if (res) {
		dev_dbg(dbg_dev, "Failed to queue with reason:%d\n", res);
		netif_stop_queue(dev);
		res = NETDEV_TX_BUSY;
	}

	dev_dbg(dbg_dev, "Exited\n");

	return res;
}

static int mhi_netdev_ioctl_extended(struct net_device *dev, struct ifreq *ifr)
{
	struct rmnet_ioctl_extended_s ext_cmd;
	int rc = 0;
	struct mhi_netdev_priv *mhi_netdev_priv = netdev_priv(dev);
	struct mhi_netdev *mhi_netdev = mhi_netdev_priv->mhi_netdev;
	struct mhi_device *mhi_dev = mhi_netdev->mhi_dev;
	struct device *dbg_dev = &mhi_dev->dev;

	rc = copy_from_user(&ext_cmd, ifr->ifr_ifru.ifru_data,
			    sizeof(struct rmnet_ioctl_extended_s));
	if (rc)
		return rc;

	switch (ext_cmd.extended_ioctl) {
	case RMNET_IOCTL_GET_SUPPORTED_FEATURES:
		ext_cmd.u.data = 0;
		break;
	case RMNET_IOCTL_GET_DRIVER_NAME:
		strlcpy(ext_cmd.u.if_name, mhi_netdev->interface_name,
			sizeof(ext_cmd.u.if_name));
		break;
	case RMNET_IOCTL_SET_SLEEP_STATE:
		if (ext_cmd.u.data && mhi_netdev->wake) {
			/* Request to enable LPM */
			dev_dbg(dbg_dev, "Enable MHI LPM");
			mhi_netdev->wake--;
			mhi_device_put(mhi_dev);
		} else if (!ext_cmd.u.data && !mhi_netdev->wake) {
			/* Request to disable LPM */
			dev_dbg(dbg_dev, "Disable MHI LPM");
			mhi_netdev->wake++;
			mhi_device_get(mhi_dev);
		}
		break;
	default:
		rc = -EINVAL;
		break;
	}

	rc = copy_to_user(ifr->ifr_ifru.ifru_data, &ext_cmd,
			  sizeof(struct rmnet_ioctl_extended_s));
	return rc;
}

static int mhi_netdev_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{
	int rc = 0;
	struct rmnet_ioctl_data_s ioctl_data;

	switch (cmd) {
	case RMNET_IOCTL_SET_LLP_IP: /* set RAWIP protocol */
		break;
	case RMNET_IOCTL_GET_LLP: /* get link protocol state */
		ioctl_data.u.operation_mode = RMNET_MODE_LLP_IP;
		if (copy_to_user(ifr->ifr_ifru.ifru_data, &ioctl_data,
		    sizeof(struct rmnet_ioctl_data_s)))
			rc = -EFAULT;
		break;
	case RMNET_IOCTL_GET_OPMODE: /* get operation mode */
		ioctl_data.u.operation_mode = RMNET_MODE_LLP_IP;
		if (copy_to_user(ifr->ifr_ifru.ifru_data, &ioctl_data,
		    sizeof(struct rmnet_ioctl_data_s)))
			rc = -EFAULT;
		break;
	case RMNET_IOCTL_SET_QOS_ENABLE:
		rc = -EINVAL;
		break;
	case RMNET_IOCTL_SET_QOS_DISABLE:
		rc = 0;
		break;
	case RMNET_IOCTL_OPEN:
	case RMNET_IOCTL_CLOSE:
		/* we just ignore them and return success */
		rc = 0;
		break;
	case RMNET_IOCTL_EXTENDED:
		rc = mhi_netdev_ioctl_extended(dev, ifr);
		break;
	default:
		/* don't fail any IOCTL right now */
		rc = 0;
		break;
	}

	return rc;
}

static const struct net_device_ops mhi_netdev_ops_ip = {
	.ndo_open = mhi_netdev_open,
	.ndo_start_xmit = mhi_netdev_xmit,
	.ndo_do_ioctl = mhi_netdev_ioctl,
	.ndo_change_mtu = mhi_netdev_change_mtu,
	.ndo_set_mac_address = 0,
	.ndo_validate_addr = 0,
};

static void mhi_netdev_setup(struct net_device *dev)
{
	dev->netdev_ops = &mhi_netdev_ops_ip;
	ether_setup(dev);

	/* set this after calling ether_setup */
	dev->header_ops = 0;  /* No header */
	dev->type = ARPHRD_RAWIP;
	dev->hard_header_len = 0;
	dev->addr_len = 0;
	dev->flags &= ~(IFF_BROADCAST | IFF_MULTICAST);
	dev->watchdog_timeo = WATCHDOG_TIMEOUT;
}

/* enable mhi_netdev netdev, call only after grabbing mhi_netdev.mutex */
static int mhi_netdev_enable_iface(struct mhi_netdev *mhi_netdev)
{
	int ret = 0;
	char ifalias[IFALIASZ];
	char ifname[IFNAMSIZ];
	struct mhi_device *mhi_dev = mhi_netdev->mhi_dev;
	struct device_node *of_node = mhi_dev->dev.of_node;
	struct mhi_netdev_priv *mhi_netdev_priv;
	struct device *dbg_dev = &mhi_dev->dev;
	struct mhi_controller *mhi_cntrl = mhi_dev->mhi_cntrl;

	mhi_netdev->alias = of_alias_get_id(of_node, "mhi-netdev");
	if (mhi_netdev->alias < 0)
		mhi_netdev->alias = 0;

	ret = of_property_read_string(of_node, "mhi,interface-name",
				      &mhi_netdev->interface_name);
	if (ret)
		mhi_netdev->interface_name = mhi_netdev_driver.driver.name;

	if (!strcmp(mhi_netdev->interface_name, MHI_RMNET_IF_NAME))
		mhi_netdev->is_rmnet = true;

	snprintf(ifalias, sizeof(ifalias), "%s_%s_%u",
		 mhi_netdev->interface_name,
		 dev_name(mhi_cntrl->cntrl_dev), mhi_netdev->alias);

	snprintf(ifname, sizeof(ifname), "%s%%d", mhi_netdev->interface_name);

	mhi_netdev->ethernet_interface = of_property_read_bool(of_node,
			"mhi,ethernet-interface");
	rtnl_lock();
	mhi_netdev->ndev = alloc_netdev(sizeof(*mhi_netdev_priv),
					ifname, NET_NAME_PREDICTABLE,
					mhi_netdev_setup);
	if (!mhi_netdev->ndev) {
		rtnl_unlock();
		return -ENOMEM;
	}

	mhi_netdev->ndev->mtu = mhi_dev->mtu;
	SET_NETDEV_DEV(mhi_netdev->ndev, &mhi_dev->dev);
	dev_set_alias(mhi_netdev->ndev, ifalias, strlen(ifalias));
	mhi_netdev_priv = netdev_priv(mhi_netdev->ndev);
	mhi_netdev_priv->mhi_netdev = mhi_netdev;
	rtnl_unlock();

	mhi_netdev->napi = devm_kzalloc(&mhi_dev->dev,
					sizeof(*mhi_netdev->napi), GFP_KERNEL);
	if (!mhi_netdev->napi) {
		ret = -ENOMEM;
		goto napi_alloc_fail;
	}

	netif_napi_add(mhi_netdev->ndev, mhi_netdev->napi,
		       mhi_netdev_poll, napi_poll_weight);
	ret = register_netdev(mhi_netdev->ndev);
	if (ret) {
		dev_err(dbg_dev, "Network device registration failed\n");
		goto net_dev_reg_fail;
	}

	napi_enable(mhi_netdev->napi);

	dev_dbg(dbg_dev, "Exited.\n");

	return 0;

net_dev_reg_fail:
	netif_napi_del(mhi_netdev->napi);

napi_alloc_fail:
	free_netdev(mhi_netdev->ndev);
	mhi_netdev->ndev = NULL;

	return ret;
}

static void mhi_netdev_xfer_ul_cb(struct mhi_device *mhi_dev,
				  struct mhi_result *mhi_result)
{
	struct mhi_netdev *mhi_netdev = dev_get_drvdata(&mhi_dev->dev);
	struct sk_buff *skb = mhi_result->buf_addr;
	struct net_device *ndev = mhi_netdev->ndev;

	ndev->stats.tx_packets++;
	ndev->stats.tx_bytes += skb->len;
	dev_kfree_skb(skb);

	if (netif_queue_stopped(ndev))
		netif_wake_queue(ndev);
}

static void mhi_netdev_push_skb(struct mhi_netdev *mhi_netdev,
				struct mhi_buf *mhi_buf,
				struct mhi_result *mhi_result)
{
	struct sk_buff *skb;

	if (unlikely(drop_at_mhi)) {
		if (unlikely(put_page_testzero(mhi_buf->page)))
			free_compound_page(mhi_buf->page);
	} else {
		skb = alloc_skb(0, GFP_ATOMIC);
		if (!skb) {
			__free_pages(mhi_buf->page, mhi_netdev->order);
			return;
		}

		if (!mhi_netdev->ethernet_interface) {
			skb_add_rx_frag(skb, 0, mhi_buf->page, 0,
					mhi_result->bytes_xferd, mhi_netdev->mru);
			skb->dev = mhi_netdev->ndev;
			skb->protocol = mhi_netdev_ip_type_trans(*(u8 *)mhi_buf->buf,
					mhi_netdev->is_rmnet);
		} else {
			skb_add_rx_frag(skb, 0, mhi_buf->page, ETH_HLEN,
					mhi_result->bytes_xferd - ETH_HLEN,
					mhi_netdev->mru);
			skb->dev = mhi_netdev->ndev;
			skb->protocol = mhi_netdev_ip_type_trans(((u8 *)mhi_buf->buf)[ETH_HLEN],
					mhi_netdev->is_rmnet);
		}
		netif_receive_skb(skb);
	}
}

static void mhi_netdev_xfer_dl_cb(struct mhi_device *mhi_dev,
				  struct mhi_result *mhi_result)
{
	struct mhi_netdev *mhi_netdev = dev_get_drvdata(&mhi_dev->dev);
	struct mhi_netbuf *netbuf = mhi_result->buf_addr;
	struct mhi_buf *mhi_buf = &netbuf->mhi_buf;
	struct sk_buff *skb;
	struct net_device *ndev = mhi_netdev->ndev;
	struct device *dev = mhi_dev->dev.parent->parent;
	struct mhi_net_chain *chain = mhi_netdev->chain;
	s64 time_difference = 0;
	ktime_t second_jiffy;
	u64 bytes_received_2;

	netbuf->unmap(dev, mhi_buf->dma_addr, mhi_buf->len, DMA_FROM_DEVICE);

	/* modem is down, drop the buffer */
	if (mhi_result->transaction_status == -ENOTCONN) {
		__free_pages(mhi_buf->page, mhi_netdev->order);
		return;
	}

	nr_mhi_buffer++;
	mhi_data_len += mhi_result->bytes_xferd;
	ndev->stats.rx_packets++;
	ndev->stats.rx_bytes += mhi_result->bytes_xferd;

	if (mhi_rate_control) {
		if (mhi_netdev->first_jiffy) {
			second_jiffy = ktime_get();
			bytes_received_2 = ndev->stats.rx_bytes;
			if ((second_jiffy > mhi_netdev->first_jiffy) &&
					(bytes_received_2 > mhi_netdev->bytes_received_1)) {

				time_difference = ktime_to_ms(second_jiffy) - ktime_to_ms(mhi_netdev->first_jiffy);

				if (time_difference < mhi_sample_time_ceiling) {
					if ((bytes_received_2 - mhi_netdev->bytes_received_1) > mhi_byte_ceiling) {
						ndev->stats.rx_dropped ++;
						mhi_sample_time = time_difference;
						mhi_drop_count += mhi_result->bytes_xferd;
						__free_pages(mhi_buf->page, mhi_netdev->order);
						return;
					}
				} else {
					mhi_netdev->first_jiffy = second_jiffy;
					mhi_netdev->bytes_received_1 = bytes_received_2;
				}
			} else {
				mhi_netdev->first_jiffy = second_jiffy;
				mhi_netdev->bytes_received_1 = bytes_received_2;
			}
		} else {
			mhi_netdev->first_jiffy = ktime_get();
		}
	}

	if (unlikely(!chain)) {
		mhi_netdev_push_skb(mhi_netdev, mhi_buf, mhi_result);
		return;
	}

	if (unlikely(drop_at_mhi)) {
		if (unlikely(put_page_testzero(mhi_buf->page)))
			free_compound_page(mhi_buf->page);
	} else {
		/* we support chaining */
		skb = alloc_skb(0, GFP_ATOMIC);
		if (likely(skb)) {
			if (!mhi_netdev->ethernet_interface) {
				skb_add_rx_frag(skb, 0, mhi_buf->page, 0,
						mhi_result->bytes_xferd, mhi_netdev->mru);
			} else {
				skb_add_rx_frag(skb, 0, mhi_buf->page, ETH_HLEN,
						mhi_result->bytes_xferd - ETH_HLEN,
						mhi_netdev->mru);
			}

			/* this is first on list */
			if (!chain->head) {
				skb->dev = ndev;
				if (!mhi_netdev->ethernet_interface) {
					skb->protocol =
						mhi_netdev_ip_type_trans(*(u8 *)mhi_buf->buf,
								mhi_netdev->is_rmnet);
				} else {
					skb->protocol =
						mhi_netdev_ip_type_trans(((u8 *)mhi_buf->buf)[ETH_HLEN],
								mhi_netdev->is_rmnet);
				}
				chain->head = skb;
			} else {
				skb_shinfo(chain->tail)->frag_list = skb;
			}

			chain->tail = skb;
		} else {
			__free_pages(mhi_buf->page, mhi_netdev->order);
		}
	}
}

static void mhi_netdev_status_cb(struct mhi_device *mhi_dev, enum mhi_callback mhi_cb)
{
	struct mhi_netdev *mhi_netdev = dev_get_drvdata(&mhi_dev->dev);

	if (mhi_cb != MHI_CB_PENDING_DATA)
		return;

	napi_schedule(mhi_netdev->napi);
}

#ifdef CONFIG_DEBUG_FS

struct dentry *dentry;

static void mhi_netdev_create_debugfs(struct mhi_netdev *mhi_netdev)
{
	char node_name[32];
	struct mhi_device *mhi_dev = mhi_netdev->mhi_dev;
	struct mhi_controller *mhi_cntrl = mhi_dev->mhi_cntrl;

	/* Both tx & rx client handle contain same device info */
	snprintf(node_name, sizeof(node_name), "%s_%s_%u",
		 mhi_netdev->interface_name, dev_name(mhi_cntrl->cntrl_dev),
		 mhi_netdev->alias);

	if (IS_ERR_OR_NULL(dentry))
		return;

	mhi_netdev->dentry = debugfs_create_dir(node_name, dentry);
	if (IS_ERR_OR_NULL(mhi_netdev->dentry))
		return;
}

static void mhi_netdev_create_debugfs_dir(void)
{
	dentry = debugfs_create_dir(MHI_NETDEV_DRIVER_NAME, 0);
}

#else

static void mhi_netdev_create_debugfs(struct mhi_netdev_private *mhi_netdev)
{
}

static void mhi_netdev_create_debugfs_dir(void)
{
}

#endif

static void mhi_netdev_remove(struct mhi_device *mhi_dev)
{
	struct mhi_netdev *mhi_netdev = dev_get_drvdata(&mhi_dev->dev);
	struct device *dbg_dev = &mhi_dev->dev;

	dev_dbg(dbg_dev, "Remove notification received\n");

	/* rsc parent takes cares of the cleanup */
	if (mhi_netdev->is_rsc_dev) {
		mhi_netdev_free_pool(mhi_netdev);
		return;
	}

	netif_stop_queue(mhi_netdev->ndev);
	napi_disable(mhi_netdev->napi);
	unregister_netdev(mhi_netdev->ndev);
	netif_napi_del(mhi_netdev->napi);
	free_netdev(mhi_netdev->ndev);
	mhi_netdev_free_pool(mhi_netdev);

	if (!IS_ERR_OR_NULL(mhi_netdev->dentry))
		debugfs_remove_recursive(mhi_netdev->dentry);
}

static int mhi_netdev_match(struct device *dev, const void *data)
{
	/* if phandle dt == device dt, we found a match */
	return (dev->of_node == data);
}

static void mhi_netdev_clone_dev(struct mhi_netdev *mhi_netdev,
				 struct mhi_netdev *parent)
{
	mhi_netdev->ndev = parent->ndev;
	mhi_netdev->napi = parent->napi;
	mhi_netdev->is_rsc_dev = true;
	mhi_netdev->chain = parent->chain;
}

static int __init config_napi_poll(char *str)
{
	u32 poll_weight;
	 if (get_option(&str, &poll_weight)) {
		napi_poll_weight = poll_weight;
		pr_emerg("MHI napi_poll_weight %u\n", napi_poll_weight);
        }
        return 0;
}

early_param("napi_poll_weight", config_napi_poll);

static int __init config_drop_at_mhi(char *str)
{
	u32 val;
	 if (get_option(&str, &val)) {
		drop_at_mhi = val;
		pr_emerg("MHI drop_at_mhi %u\n", drop_at_mhi);
        }
        return 0;
}

early_param("drop_at_mhi", config_drop_at_mhi);

static int __init config_drop_at_rmnet(char *str)
{
	u32 val;
	 if (get_option(&str, &val)) {
		drop_at_rmnet = val;
		pr_emerg("MHI drop_at_rmnet %u\n", drop_at_rmnet);
        }
        return 0;
}

early_param("drop_at_rmnet", config_drop_at_rmnet);

static int fwainfo_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m,
		"napi_poll_weight	: %u\n"
		"drop_at_mhi		: %u\n"
		"drop_at_rmnet		: %u\n"
		"nr_rmnet_pkts		: %llu\n"
		"rmnet_rx_bytes		: %llu\n"
		"nr_mhi_buffer		: %llu\n"
		"mhi_data_len		: %llu\n",
		napi_poll_weight,
		drop_at_mhi,
		drop_at_rmnet,
		nr_rmnet_pkts,
		rmnet_rx_bytes,
		nr_mhi_buffer,
		mhi_data_len);

	return 0;
}

static int fwainfo_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, fwainfo_proc_show, NULL);
}

static const struct file_operations fwainfo_proc_fops = {
	.open		= fwainfo_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int mhi_netdev_probe(struct mhi_device *mhi_dev,
			    const struct mhi_device_id *id)
{
	int ret;
	struct mhi_netdev *mhi_netdev, *p_netdev = NULL;
	struct device_node *of_node = mhi_dev->dev.of_node;
	struct device *dbg_dev = &mhi_dev->dev;
	int nr_tre;
	struct device_node *phandle;
	bool no_chain;
	static int fwa_proc_created;

	if (!of_node)
		return -ENODEV;

	mhi_netdev = devm_kzalloc(&mhi_dev->dev, sizeof(*mhi_netdev),
				  GFP_KERNEL);
	if (!mhi_netdev)
		return -ENOMEM;

	mhi_netdev->mhi_dev = mhi_dev;
	dev_set_drvdata(&mhi_dev->dev, mhi_netdev);

	ret = of_property_read_u32(of_node, "mhi,mru", &mhi_netdev->mru);
	if (ret)
		return -ENODEV;

	/* MRU must be multiplication of page size */
	mhi_netdev->order = __ilog2_u32(mhi_netdev->mru / PAGE_SIZE);
	if ((PAGE_SIZE << mhi_netdev->order) < mhi_netdev->mru)
		return -EINVAL;

	mhi_rate_control = of_property_read_bool(of_node,
						 "mhi,enable-rate-control");

	/* check if this device shared by a parent device */
	phandle = of_parse_phandle(of_node, "mhi,rsc-parent", 0);
	if (phandle) {
		struct device *dev;
		struct mhi_device *pdev;
		/* find the parent device */
		dev = driver_find_device(mhi_dev->dev.driver, NULL, phandle,
					 mhi_netdev_match);
		if (!dev)
			return -ENODEV;

		/* this device is shared with parent device. so we won't be
		 * creating a new network interface. Clone parent
		 * information to child node
		 */
		pdev = to_mhi_device(dev);
		p_netdev = dev_get_drvdata(&mhi_dev->dev);
		mhi_netdev_clone_dev(mhi_netdev, p_netdev);
		put_device(dev);
	} else {
		no_chain = of_property_read_bool(of_node,
						 "mhi,disable-chain-skb");
		if (!no_chain) {
			mhi_netdev->chain = devm_kzalloc(&mhi_dev->dev,
						sizeof(*mhi_netdev->chain),
						GFP_KERNEL);
			if (!mhi_netdev->chain)
				return -ENOMEM;
		}

		ret = mhi_netdev_enable_iface(mhi_netdev);
		if (ret)
			return ret;

		mhi_netdev_create_debugfs(mhi_netdev);
	}

	/* move mhi channels to start state */
	ret = mhi_prepare_for_transfer(mhi_dev);
	if (ret) {
		dev_err(dbg_dev, "Failed to start channels ret %d\n", ret);
		goto error_start;
	}

	/* setup pool size ~2x ring length*/
	nr_tre = mhi_get_no_free_descriptors(mhi_dev, DMA_FROM_DEVICE);
	mhi_netdev->pool_size = 1 << __ilog2_u32(nr_tre);
	if (nr_tre > mhi_netdev->pool_size)
		mhi_netdev->pool_size <<= 1;
	mhi_netdev->pool_size <<= 1;

	dev_info(dbg_dev, "MHI netdev pool size: 0x%x \n",
				(mhi_netdev->pool_size * mhi_netdev->mru));
	dev_info(dbg_dev, "Note: To increase static netdev pool allocation use"
				" mhi_nr_tre_update bootargs\n");
	/* allocate memory pool */
	ret = mhi_netdev_alloc_pool(mhi_netdev);

	if (ret)
		goto error_start;

	/* link child node with parent node if it's children dev */
	if (p_netdev)
		p_netdev->rsc_dev = mhi_netdev;

	/* now we have a pool of buffers allocated, queue to hardware
	 * by triggering a napi_poll
	 */
	napi_schedule(mhi_netdev->napi);

	if (!fwa_proc_created) {
		proc_create("fwainfo", 0, NULL, &fwainfo_proc_fops);
		fwa_proc_created = 1;
	}

	return 0;

error_start:
	if (phandle)
		return ret;

	netif_stop_queue(mhi_netdev->ndev);
	napi_disable(mhi_netdev->napi);
	unregister_netdev(mhi_netdev->ndev);
	netif_napi_del(mhi_netdev->napi);
	free_netdev(mhi_netdev->ndev);

	return ret;
}

static const struct mhi_device_id mhi_netdev_match_table[] = {
	{ .chan = "IP_HW0" },
	{ .chan = "IP_HW_ADPL" },
	{ .chan = "IP_HW0_RSC" },
	{ .chan = "IP_SW0" },
	{},
};

static struct mhi_driver mhi_netdev_driver = {
	.id_table = mhi_netdev_match_table,
	.probe = mhi_netdev_probe,
	.remove = mhi_netdev_remove,
	.ul_xfer_cb = mhi_netdev_xfer_ul_cb,
	.dl_xfer_cb = mhi_netdev_xfer_dl_cb,
	.status_cb = mhi_netdev_status_cb,
	.driver = {
		.name = "mhi_netdev",
		.owner = THIS_MODULE,
	}
};

static int __init mhi_netdev_init(void)
{
	BUILD_BUG_ON(sizeof(struct mhi_netbuf) > MAX_NETBUF_SIZE);
	mhi_netdev_create_debugfs_dir();

	return mhi_driver_register(&mhi_netdev_driver);
}
module_init(mhi_netdev_init);

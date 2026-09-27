/*
	nvram list:
	nvram name				default value	comment
	sh_algo_type			2				1: static anchor link, 2: dynamic anchor link, 0: disable smarthaul
	sh_algo_txop_hi			60
	sh_algo_txop_lo			40
	sh_algo_anchor_band		0				link index(0~2), not band(2/5/6)
	sh_algo_cycle_ms		3000
	sh_algo_min_cnt			0				minimum cycle in a row to change tidmap
	sh_algo_disable_band	N/A				valuse: 2/5/6, froced to set tidmap of specific band as 0x00
	sh_algo_stop			N/A				1: pause algorithm in next cycle and reset tidmap
	sh_algo_tx_sum			20				only allow high retry link be added back when sum of all link's tx and inbss above this value
	sh_algo_obss			N/A				the function of this nvram is replaced by the two nvram below
	sh_algo_obss_6G			3				the obss multiplier of 6G band (on BQ16 pro, this default value is 6)
	sh_algo_obss_5G			3				the obss multiplier of 5G and 2G band
	sh_algo_retry			0				if the retry of 2G link above this threshold, remove it and mark as high retry link
	smarthaul_msglevel		0x01			set to 0xff to enable all smarthaul debug log
 */
#include <stdio.h>
#include <stdlib.h>
#include <rc.h>
#if defined(RTCONFIG_RALINK)
#include <netinet/ether.h> //brcm original: ethernet.h
#else
#include <ethernet.h>
#endif
#include <sys/time.h>
#include "smarthaul.h"
#include <json.h>
#include <sys/un.h>
#include <pthread.h>


#define SH_ROUNDUP(x, y)		((((x) + ((y) - 1)) / (y)) * (y))
#define MAP_MAX_AGENT_COUNT 6u
#define ETHER_ADDR_STR_LEN	18
#define NBBY    8
#define SH_IDX_MASK_LEN	(SH_ROUNDUP(MAP_MAX_AGENT_COUNT, NBBY) / NBBY)
#define SETBIT(a, i)    (((uint8 *)a)[(i) / NBBY] |= 1 << ((i) % NBBY))
#define CLRBIT(a, i)    (((uint8 *)a)[(i) / NBBY] &= ~(1 << ((i) % NBBY)))
#define ISSET(a, i)     (((const uint8 *)a)[(i) / NBBY] & (1 << ((i) % NBBY)))
#define ISCLR(a, i)     ((((const uint8 *)a)[(i) / NBBY] & (1 << ((i) % NBBY))) == 0)

#define SH_STATS_FLAG_SCB (1 << 1)
#define SH_STATS_FLAG_CHANIM (1 << 0)

#define SH_IPC_SOCKET_PATH "/var/run/smarthaul_ipc_socket" 
#define SH_IPC_MAX_CONNECTION 128
#define EID_SH_STA_ADD 1
#define EID_SH_STA_UPD 2
#define EID_SH_STA_DEL 3
#define CFGMNT_IPC_SOCKET_PATH	"/var/run/cfgmnt_ipc_socket"

#define MIN_LIVE_CYCLE 10

uint8 sh_idx_bitvec[SH_IDX_MASK_LEN];

sh_repeater_node_t	*active_bh;
smarthaul_t			g_smarthaul;

static int thread_term = 0;

static bool timer_running = FALSE;
static bool sh_stopped = FALSE;
sh_algo_info_t *algo_info;

static struct sh_algo g_algo;

static int sh_start_ipc_socket(void* arg);
static void sh_ipc_receive(void* arg, int sockfd);
static int sh_sta_add(void *arg, char *data);
static int sh_sta_upd(void *arg, char *data);
static int sh_sta_del(void *arg, char *data);
static void sh_add_dev2list(smarthaul_t *smarthaul, sh_dev_info_t *new_dev);
static void dump_sh_dev_info(sh_dev_info_t *dev);
static void sh_dbglog_dump_chanim(struct sh_mbh *mbh);
static void sh_mbh_get_tidmap(struct sh_mbh *mbh, uint8 linkset, sh_tidmap_t *map);
static int sh_tidmap_set(sh_tidmap_t *tm);
static int sh_activate_bh(smarthaul_t *smarthaul, sh_repeater_node_t *new_dev, sh_dev_info_t *local_dev);
static void sh_deactivate_bh(smarthaul_t *smarthaul, sh_repeater_node_t *rpt_node);
static void sh_mbh_free(struct sh_mbh *mbh);

struct eventHandler
{
    int event_id;
    int (*func)(void *arg, char *data);
};

struct eventHandler SH_CFG_EVENTS[] = {
	{ EID_SH_STA_ADD, sh_sta_add },
    { EID_SH_STA_UPD, sh_sta_upd },
    { EID_SH_STA_DEL, sh_sta_del }
};

static void sh_ipc_socket_thread(smarthaul_t *smarthaul)
{
    pthread_t thread;
    pthread_attr_t attr;

    SMARTHAUL_TRACE("Start ipc socket thread.\n");

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&thread, NULL, (void *)&sh_start_ipc_socket, (void*)smarthaul);
    pthread_attr_destroy(&attr);
}

static int sh_start_ipc_socket(void* arg)
{
    smarthaul_t *smarthaul = (smarthaul_t *)arg;

    struct sockaddr_un addr;
    int sockfd, newsockfd;

    if ( (sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
	SMARTHAUL_ERROR("ipc create socket error!\n");
        exit(-1);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SH_IPC_SOCKET_PATH, sizeof(addr.sun_path)-1);

    unlink(SH_IPC_SOCKET_PATH);

    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        SMARTHAUL_ERROR("ipc bind socket error!\n");
		exit(-1);
    }

    if (listen(sockfd, SH_IPC_MAX_CONNECTION) == -1) {
        SMARTHAUL_ERROR("ipc listen socket error!\n");
		exit(-1);
    }

    while (!thread_term) {
	    SMARTHAUL_TRACE("smarthaul ipc accept socket...\n");
        if ( (newsockfd = accept(sockfd, NULL, NULL)) == -1) {
		    SMARTHAUL_ERROR("ipc accept socket error!\n");
            continue;
        }

        sh_ipc_receive(smarthaul, newsockfd);
        close(newsockfd);

	}

	return 0;
}

static void sh_ipc_receive(void* arg, int sockfd)
{
    smarthaul_t *smarthaul = (smarthaul_t *)arg;

	int length = 0;
	char buf[2048];
	memset(buf, 0, sizeof(buf));
	if ((length = read(sockfd, buf, sizeof(buf))) <= 0)
	{
		SMARTHAUL_ERROR("ipc read socket error!\n");
		return;
	}

	SMARTHAUL_TRACE("IPC Receive: %s <<< RCV EVENT >>>\n", buf);

	json_object *rootObj = json_tokener_parse(buf);
	json_object *cfgObj = NULL;
	json_object *eidObj = NULL;
	json_object_object_get_ex(rootObj, "CFG", &cfgObj);
	json_object_object_get_ex(cfgObj, "eid", &eidObj);

	int EID = 0;
	struct eventHandler *handler = NULL;

	if(eidObj) {
		EID = atoi(json_object_get_string(eidObj));
		for(handler = &SH_CFG_EVENTS[0]; handler->event_id > 0; handler++)
		{
			if (handler->event_id == EID)
			break;
		}

		if (handler == NULL || handler->event_id < 0)
			SMARTHAUL_TRACE("no corresponding function pointer(%d)", EID);
		else
		{
			SMARTHAUL_TRACE("process event (%d)\n", EID);
 			if (!handler->func(smarthaul, buf)) {
				SMARTHAUL_ERROR("fail to process event(%d)\n", EID);
			}
		}
	}

	json_object_put(rootObj);
}

static int sh_ipc_send_event(const char *ipc_path, char *data)
{
    int fd, length;
	int ret = -1;
	struct sockaddr_un addr_un;

	if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
		SMARTHAUL_ERROR("ipc socket error!\n");
		goto error;
	}

	memset(&addr_un, 0, sizeof(addr_un));
	addr_un.sun_family = AF_UNIX;
	snprintf(addr_un.sun_path, sizeof(addr_un.sun_path), ipc_path);
	if (connect(fd, (struct sockaddr *)&addr_un, sizeof(addr_un)) < 0) {
		SMARTHAUL_ERROR("ipc connect error\n");
		goto error;
	}

	SMARTHAUL_TRACE("IPC Send: %s  <<< SEND EVENT >>>\n", data);

	length = write(fd, data, strlen(data));

        if(length < 0) {
                SMARTHAUL_ERROR("[%s:(%d)] ERROR writing:%s.\n", __FUNCTION__, __LINE__, strerror(errno));
                goto error;
        }

	ret = SH_OK;

error:
        close(fd);
        return ret;
}

static int sh_process_ipc_data(sh_dev_info_t *new_dev, char *data)
{
    int cnt = 0;
    char cap_bssid[18], mld_mac[18], re_mac[18];

    json_object *rootObj = json_tokener_parse(data);
	json_object *cfgObj = NULL;
    json_object *re_macObj = NULL;
    json_object *dev_infoObj = NULL;
    json_object *mld_macObj = NULL;
    json_object *map_unitObj = NULL;
    json_object *mlo_link_infoObj = NULL;
    json_object *pap_bssidObj = NULL;

    if(rootObj == NULL){
        SMARTHAUL_ERROR("fail to parsing json");
        return 0;
    }

    json_object_object_get_ex(rootObj, "CFG", &cfgObj);
    json_object_object_get_ex(cfgObj, "re_mac", &re_macObj);
    json_object_object_get_ex(cfgObj, "device_info", &dev_infoObj);
    json_object_object_get_ex(dev_infoObj, "mld_mac", &mld_macObj);
    json_object_object_get_ex(dev_infoObj, "map_unit", &map_unitObj);
    json_object_object_get_ex(dev_infoObj, "mlo_link_info", &mlo_link_infoObj);

    snprintf(re_mac, sizeof(re_mac), json_object_get_string(re_macObj));
    ether_atoe(re_mac, SH_ETHER_ADDR(new_dev->self_mac));
    snprintf(mld_mac, sizeof(mld_mac), json_object_get_string(mld_macObj));
    ether_atoe(mld_mac, SH_ETHER_ADDR(new_dev->mlo_info.self_mld_addr));

    json_object_object_foreach(mlo_link_infoObj, key, value){
        json_object_object_get_ex(value, "pap_bssid", &pap_bssidObj);
        SMARTHAUL_TRACE("pap :%s\n", json_object_to_json_string(pap_bssidObj));
        snprintf(cap_bssid, sizeof(cap_bssid), json_object_get_string(pap_bssidObj));
        ether_atoe(cap_bssid, SH_ETHER_ADDR(new_dev->mlo_info.mlo_link_info[cnt].pap_bssid));

        new_dev->mlo_info.mlo_link_info[cnt].wlc_unit = atoi(key);
        new_dev->mlo_info.mlo_link_info[cnt].link_id = cnt; 
        cnt++;    
    }

    new_dev->mlo_info.mlo_num_links = cnt;

    return cnt;
}

static int sh_sta_add(void *arg, char *data)
{
    smarthaul_t *smarthaul = (smarthaul_t *)arg;
    sh_repeater_node_t *curr;
    sh_root_info_t* root_info = (sh_root_info_t *)smarthaul->dev_info;
    sh_dev_info_t *new_dev;
    new_dev = calloc(1, sizeof(sh_dev_info_t));
    char new_mac[18], list_mac[18];
 
    if(!sh_process_ipc_data(new_dev, data)){
        SMARTHAUL_ERROR("porcess ipc data failed!\n");
        return 0;
    }
    ether_etoa(SH_ETHER_ADDR(new_dev->self_mac), new_mac);
    curr = root_info->rpt_list;
    while(curr){
	ether_etoa(SH_ETHER_ADDR(curr->self_mac), list_mac);
        if(!strcmp(new_mac, list_mac)){
            SMARTHAUL_TRACE("Node %s existed, skip adding\n", new_mac);
            free(new_dev);
            return 0;
        }
        else {
            curr = curr->next;
        }
    }
    sh_add_dev2list(smarthaul, new_dev);
 
    free(new_dev);
    return 1;
}

static int sh_sta_upd(void *arg, char *data)
{ 
    smarthaul_t *smarthaul = (smarthaul_t *)arg;
    sh_repeater_node_t *curr;
    sh_root_info_t* root_info = (sh_root_info_t *)smarthaul->dev_info;
    sh_dev_info_t *new_dev;
    new_dev = calloc(1, sizeof(sh_dev_info_t));
    char new_mac[18], list_mac[18];
    sh_process_ipc_data(new_dev, data);
    ether_etoa(SH_ETHER_ADDR(new_dev->self_mac), new_mac);
    curr = root_info->rpt_list;
    while(curr){
        ether_etoa(SH_ETHER_ADDR(curr->self_mac), list_mac);
        if(!strcmp(new_mac, list_mac)){
            SMARTHAUL_TRACE("Update mlo info of %s\n", new_mac);
            if(!memcmp(&curr->mlo_info, &new_dev->mlo_info, sizeof(sh_mlo_info_t))){
                SMARTHAUL_TRACE("No info of %s need to update\n", new_mac);
                return 1;
            }
            if (curr == active_bh){
                sh_deactivate_bh(smarthaul, curr);
                memcpy(&curr->mlo_info, &new_dev->mlo_info, sizeof(sh_mlo_info_t));
				sh_activate_bh(smarthaul, curr, &root_info->local_dev);
            }
            else    
                memcpy(&curr->mlo_info, &new_dev->mlo_info, sizeof(sh_mlo_info_t));

            free(new_dev);
            return 1;
        }
        else{
            curr = curr->next;
        }    
    }

    SMARTHAUL_TRACE("No match node in rpt list, adding dev to list instead of updating\n");
    sh_add_dev2list(smarthaul, new_dev);

    free(new_dev);
    return 1;
}

static int sh_sta_del(void *arg, char *data)
{
    smarthaul_t *smarthaul = (smarthaul_t *)arg;
    sh_root_info_t* root_info = (sh_root_info_t *)smarthaul->dev_info;
    sh_repeater_node_t *curr, *pre = NULL;
    char del_mac[18], list_mac[18];

    json_object *rootObj = json_tokener_parse(data);
    json_object *cfgObj = NULL;
    json_object *re_macObj = NULL;
    
    if(rootObj == NULL){
        SMARTHAUL_ERROR("fail to parsing json");
        return 0;
    }

    json_object_object_get_ex(rootObj, "CFG", &cfgObj);
    json_object_object_get_ex(cfgObj, "re_mac", &re_macObj);
    snprintf(del_mac, sizeof(del_mac), json_object_get_string(re_macObj));
    json_object_put(rootObj);

    curr = root_info->rpt_list;
    while(curr){
        ether_etoa(SH_ETHER_ADDR(curr->self_mac), list_mac);
        if(!strcmp(del_mac, list_mac)){
            SMARTHAUL_TRACE("Delete all info of %s\n", del_mac);
            //Delete the node from list
            if(curr == root_info->rpt_list)
                root_info->rpt_list = curr -> next;
            else 
                pre -> next = curr->next;

            root_info->num_repeaters--;
            CLRBIT(sh_idx_bitvec, curr->sh_idx);

			sh_deactivate_bh(smarthaul, curr);

            free(curr);
            return 1;
        }
        else{
            pre = curr;
            curr = curr->next;
        }    
    }
    SMARTHAUL_TRACE("%s not found. No RE be deleted\n", del_mac);
    return 0;
}

static void sh_add_dev2list(smarthaul_t *smarthaul, sh_dev_info_t *new_dev)
{
	sh_root_info_t* root_info = (sh_root_info_t *)smarthaul->dev_info;
	sh_repeater_node_t *new_node;
	int i,j;

	SMARTHAUL_TRACE("New added device info\n");
	dump_sh_dev_info(new_dev);

    new_node = calloc(1, sizeof(sh_repeater_node_t));
	memcpy(&new_node->mlo_info, &new_dev->mlo_info, sizeof(sh_mlo_info_t));
	eacopy(SH_ETHER_ADDR(new_dev->self_mac), SH_ETHER_ADDR(new_node->self_mac));
	for(i = 0; i < root_info->local_dev.mlo_info.mlo_num_links; i++){
		for(j = 0; j < root_info->local_dev.mlo_info.mlo_num_links; j++){
			if(!eacmp(&root_info->local_dev.mlo_info.mlo_link_info[i].pap_bssid, &new_dev->mlo_info.mlo_link_info[j].pap_bssid)){
				memcpy(&new_node->mlo_info.mlo_link_info[i], &new_dev->mlo_info.mlo_link_info[j], sizeof(sh_mlo_link_info_t));
				new_node->mlo_info.mlo_link_info[i].link_id = i;
				continue;
			}
		}
	}
	for (i = 0; i < SH_IDX_MASK_LEN * NBBY; i++) {
		if (ISCLR(sh_idx_bitvec, i)) {
			new_node->sh_idx = i;
			SETBIT(sh_idx_bitvec, i);
			break;
		}
	}

	if (root_info->rpt_list == NULL) {
        root_info->rpt_list = new_node;
    } else {
        new_node->next = root_info->rpt_list;
        root_info->rpt_list = new_node;
    }
    root_info->num_repeaters++;

	SMARTHAUL_TRACE("Activate SH bSta with MLD: " MACF "\n",
		ETHER_TO_MACF(new_node->mlo_info.self_mld_addr));
	sh_activate_bh(smarthaul, new_node, &root_info->local_dev);
}

static int sh_send_tidmap2re(sh_mbh_t *dev_info, sh_tidmap_t *tm)
{
    int i,j;
    char re_mac[18], wlc_unitStr[2], json_data[256];

    json_object *root = json_object_new_object();
    json_object *shObj = json_object_new_object();
    json_object *mapObj = json_object_new_object();

    for(i = 0; i < dev_info->mlo_info->mlo_num_links; i++){
        for(j = 0; j < dev_info->mlo_info->mlo_num_links; j++){
            if(dev_info->mlo_info->mlo_link_info[j].link_id == i){
                snprintf(wlc_unitStr, sizeof(wlc_unitStr), "%d", dev_info->mlo_info->mlo_link_info[j].wlc_unit);
                if(tm->tids[j] == 0xff)
                    json_object_object_add(mapObj, wlc_unitStr, json_object_new_int(256));
                else   
                    json_object_object_add(mapObj, wlc_unitStr, json_object_new_int(192));
            }
        }
        
        snprintf(wlc_unitStr, sizeof(wlc_unitStr), "%d", dev_info->mlo_info->mlo_link_info[i].wlc_unit);
        if(tm->tids[i] == 0xff)
            json_object_object_add(mapObj, wlc_unitStr, json_object_new_int(256));
        else   
            json_object_object_add(mapObj, wlc_unitStr, json_object_new_int(192));
    }
    ether_etoa(SH_ETHER_ADDR(dev_info->mlo_info->self_mld_addr), re_mac);

    json_object_object_add(shObj, "eid", json_object_new_int(4));
    json_object_object_add(shObj, "re_mac", json_object_new_string(re_mac));
    json_object_object_add(shObj, "tidmap_info", mapObj);
    json_object_object_add(root, "SMARTHAUL", shObj);    

    memset(json_data, 0, sizeof(json_data));
	snprintf(json_data, sizeof(json_data), "%s", json_object_to_json_string(root));
	json_object_put(root);

    return sh_ipc_send_event(CFGMNT_IPC_SOCKET_PATH, &json_data[0]);
}

static struct sh_mbh *sh_algo_find_mbh_by_ix(int mbh_ix)
{
	struct sh_algo *algo = &g_algo;
	struct sh_mbh *mbh = algo->mbh;

	while (mbh) {
		if (mbh->ix == mbh_ix)
			return mbh;
		mbh = mbh->next;
	}

	return NULL;
}

static int sh_bh_add_links(struct sh_mbh *mbh)
{
	const sh_mlo_info_t *mi = mbh->mlo_info;
	const sh_mlo_link_info_t *li;
	struct sh_link *sl;
	int i;

	for (i = 0; i < mi->mlo_num_links; i++) {
		li = &mi->mlo_link_info[i];
		sl = &mbh->link[i];
		sl->link_id = i;
		sl->wlc_unit = li->wlc_unit;
		sl->band_ix = li->bandidx;
		sl->mbh = mbh;

		mbh->cfg_linkset |= (1 << i);
		mbh->link_cnt++;

		if (!mbh->is_bap) {
			SMARTHAUL_TRACE("mbh %d add link %u, wl %u, band_ix%u\n", mbh->ix, i,
			sl->wlc_unit, sl->band_ix);
		}
	}

	mbh->cur_linkset = mbh->cfg_linkset;
	mbh->can_linkset = mbh->cfg_linkset;
	if (!mbh->is_bap) {
		SMARTHAUL_TRACE("mbh %d added %d links, linkset %x\n", mbh->ix,
			mbh->link_cnt, mbh->cfg_linkset);
	}
	return SH_OK;
}

static int sh_add_bh(uint32 sh_idx, const sh_mlo_info_t *bap, const sh_mlo_info_t *bsta)
{
	struct sh_algo *algo = &g_algo;
	struct sh_mbh *ap_bh =  NULL, *sta_bh = NULL;
	int err = SH_OK;
	int i;

	if (sh_algo_find_mbh_by_ix(sh_idx)) {
		SMARTHAUL_ERROR("duplicate sh_idx %d\n", sh_idx);
		return SH_ERROR;
	}

	ap_bh = calloc(1, sizeof(*ap_bh));
	if (!ap_bh)
		return SH_ERROR;

	if (bsta) {
		sta_bh = calloc(1, sizeof(*sta_bh));
		if (!sta_bh) {
			free(ap_bh);
			return SH_ERROR;
		}
	}

	ap_bh->algo = algo;
	ap_bh->ix = sh_idx;
	ap_bh->mlo_info = bap;
	ap_bh->is_bap = TRUE;
	err = sh_bh_add_links(ap_bh);
	if (err)
		goto exit;

	if (sta_bh) {
		sta_bh->algo = algo;
		sta_bh->ix = sh_idx;
		sta_bh->mlo_info = bsta;
		sta_bh->is_bap = FALSE;
		err = sh_bh_add_links(sta_bh);
		if (err)
			goto exit;
	}
	ap_bh->bsta = sta_bh;

	/* anchor initialization */
	ap_bh->anchor_link = algo->info.anchor_band;

	/* set up the bap lists */
	ap_bh->next = algo->mbh;
	algo->mbh = ap_bh;
	algo->mbh_cnt++;
	SMARTHAUL_TRACE("mbh_cnt %d\n", algo->mbh_cnt);

	for (i = 0; i < algo->mbh->mlo_info->mlo_num_links; i++) {
		if (algo->mbh->link[i].band_ix == 2) {
			sh_tidmap_t tm;
			algo->mbh->cur_linkset &= ~(1 << i);
			SMARTHAUL_TRACE("mbh %d has 2G link, set linkset to %x\n",
				algo->mbh->ix, algo->mbh->cur_linkset);
			sh_mbh_get_tidmap(algo->mbh, algo->mbh->cur_linkset, &tm);
			sh_tidmap_set(&tm);
		}
	}
	return SH_OK;

exit:
	if (ap_bh) {
		sh_mbh_free(ap_bh);
	}

	if (sta_bh)
		sh_mbh_free(sta_bh);

	SMARTHAUL_ERROR("failed to add bh %d, err %d\n", sh_idx, err);
	return err;
}

static int sh_del_bh(uint32 sh_idx)
{
	struct sh_algo *algo = &g_algo;
	struct sh_mbh *mbh, *nlist;

	mbh = sh_algo_find_mbh_by_ix(sh_idx);
	if (!mbh) {
		return SH_ERROR;
	}

	/* detach mbh from the list */
	nlist = mbh->next;
	mbh->next = NULL;
	while (algo->mbh) {
		struct sh_mbh *cur = algo->mbh;
		algo->mbh = cur->next;

		/* take the head of algo->mbh, insert into head of nlist */
		if (cur != mbh) {
			cur->next = nlist;
			nlist = cur;
		} else
			break;
	}
	/* now algo->mbh list only has the mbh to be deleted */
	algo->mbh = nlist;

	sh_mbh_free(mbh);
	algo->mbh_cnt--;
	return SH_OK;
}

static void sh_mbh_free(struct sh_mbh *mbh)
{
	char mld_str[ETHER_ADDR_STR_LEN];
	ether_etoa((uint8 *)&mbh->mlo_info->self_mld_addr, mld_str);
	SMARTHAUL_TRACE("%p, ix=%d, mld=%s\n", mbh, mbh->ix, mld_str);

	if (mbh->bsta) {
		sh_mbh_free(mbh->bsta);
		mbh->bsta = NULL;
	}

	free(mbh);
}

static int sh_activate_bh(smarthaul_t *smarthaul, sh_repeater_node_t *new_dev,
	sh_dev_info_t *local_dev)
{
	int ret = SH_OK;

	SMARTHAUL_SYSLOG("Add bSta: " MACF ", smart haul index : %d\n",
		ETHER_TO_MACF(new_dev->self_mac), new_dev->sh_idx);

	ret = sh_add_bh(new_dev->sh_idx, &local_dev->mlo_info, &new_dev->mlo_info);

	return ret;
}

static void sh_deactivate_bh(smarthaul_t *smarthaul, sh_repeater_node_t *rpt_node)
{
	SMARTHAUL_SYSLOG("Del bSta: " MACF " smart haul index : %d\n",
		ETHER_TO_MACF(rpt_node->self_mac), rpt_node->sh_idx);

	sh_del_bh(rpt_node->sh_idx);
}

static int sh_add_new_mbh(sh_mlo_info_t* new_mbh_mlo_info)
{
	smarthaul_t *smarthaul = &g_smarthaul;
	sh_root_info_t* root_info = (sh_root_info_t *)smarthaul->dev_info;
	sh_mlo_info_t* root_mlo_info = &root_info->local_dev.mlo_info;
	int i;
	int ret = SH_OK;

	for (i = 0; i < root_mlo_info->mlo_num_links; i++) {
		new_mbh_mlo_info->mlo_link_info[i].wlc_unit = root_mlo_info->mlo_link_info[i].wlc_unit;
		new_mbh_mlo_info->mlo_link_info[i].link_id = root_mlo_info->mlo_link_info[i].link_id;
		new_mbh_mlo_info->mlo_link_info[i].bandidx = root_mlo_info->mlo_link_info[i].bandidx;
		new_mbh_mlo_info->mlo_num_links = root_mlo_info->mlo_num_links;
	}

	for (i = 0; i < SH_IDX_MASK_LEN * NBBY; i++) {
		if (ISCLR(sh_idx_bitvec, i)) {
			SMARTHAUL_TRACE("Adding new mbh %d\n", i);
			ret = sh_add_bh(i, root_mlo_info, new_mbh_mlo_info);		
			break;
		}
	}

	if (ret != SH_OK)
		SMARTHAUL_ERROR("Failed to add mbh %d\n", i);
	else
		SETBIT(sh_idx_bitvec, i);

	return ret;
}

static int sh_add_timer(int cycle_ms)
{
	struct itimerval timer;
	struct sh_algo *algo = &g_algo;
	struct sh_mbh *mbh = algo->mbh;
	if (!timer_running) {
		SMARTHAUL_TRACE("Start stats collection timer at intevals (us): %d\n", cycle_ms);
		timer.it_interval.tv_sec = cycle_ms / 1000;
		timer.it_interval.tv_usec = 0;
		timer.it_value.tv_sec = cycle_ms / 1000;
		timer.it_value.tv_usec = 0;
		if (setitimer(ITIMER_REAL, &timer, NULL) != SH_OK) {
			SMARTHAUL_ERROR("Failed to set timer, stop smarthaul\n");
			return SH_ERROR;
		}
		timer_running = TRUE;
		return SH_OK;
	}
	else if (mbh) {
		SMARTHAUL_TRACE("Already have active backhaul and timer is running\n");
		return SH_OK;
	}
	else {
		SMARTHAUL_ERROR("No active backhaul but timer is running!\n");
		return SH_ERROR;
	}
}

static int sh_algo_init(void *arg)
{
	struct sh_algo *algo = &g_algo;
	char *nvram_str;

	bzero(&g_algo, sizeof(g_algo));

	algo->arg = arg;

	nvram_str = nvram_get("sh_algo_txop_hi");
	algo->info.txop_hi = nvram_str ? strtoul(nvram_str, NULL, 0) : 60;

	nvram_str = nvram_get("sh_algo_txop_lo");
	algo->info.txop_lo = nvram_str ? strtoul(nvram_str, NULL, 0) : 40;

	nvram_str = nvram_get("sh_algo_anchor_band");
	algo->info.anchor_band = nvram_str ? strtoul(nvram_str, NULL, 0) : 0;

	nvram_str = nvram_get("sh_algo_cycle_ms");
	algo->info.cycle_ms = nvram_str ? strtoul(nvram_str, NULL, 0) : 3000;

	/* TBD: add sanity check, 0 <= txop <= 100, anchor = 0/1/2/3 */
	/* Mayby we should rename "anchor band" to "anchor link" */

	SMARTHAUL_TRACE("algo txop: hi %u lo %u, band %u, cycle_ms %d\n",
			algo->info.txop_hi, algo->info.txop_lo,
			algo->info.anchor_band, algo->info.cycle_ms);
	return SH_OK;
}

static int sh_init(smarthaul_t *smarthaul)
{
	int ret = SH_OK;
	sh_mlo_info_t* sh_mlo_info;
	sh_root_info_t* root_info;
	sh_dev_info_t* dev_info;
	int i;

	bzero(sh_idx_bitvec, sizeof(sh_idx_bitvec));
	active_bh = NULL;
	
	smarthaul->dev_info = calloc(1, sizeof(sh_root_info_t));
	if (smarthaul->dev_info == NULL) {
		return SH_ERROR;
	}
	root_info = (sh_root_info_t *) smarthaul->dev_info;
	sh_mlo_info = &root_info->local_dev.mlo_info;

	ret = sh_mlo_support();
	if (ret == 0) {
		SMARTHAUL_ERROR("Not a MLO device. Smart Haul is disabled\n");
		free(smarthaul->dev_info);
		smarthaul->dev_info = NULL;
		return SH_ERROR;
	}
	else
		sh_mlo_info->mlo_num_links = ret;

	SMARTHAUL_TRACE("Collecting mlo link info\n");
	uint8 link_id, wlc_unit, bandidx;
	for (i = 0; i < sh_mlo_info->mlo_num_links; i++){
		sh_mlo_link_info_collect(&link_id, &wlc_unit, &bandidx, &sh_mlo_info->mlo_link_info[i].pap_bssid, i);
		sh_mlo_info->mlo_link_info[i].link_id = link_id;
		sh_mlo_info->mlo_link_info[i].wlc_unit = wlc_unit;
		sh_mlo_info->mlo_link_info[i].bandidx = bandidx;

		SMARTHAUL_TRACE("MLO link%d info: wl%d, %dG band, mac_addr " MACF "\n", 
			sh_mlo_info->mlo_link_info[i].link_id,
			sh_mlo_info->mlo_link_info[i].wlc_unit,
			sh_mlo_info->mlo_link_info[i].bandidx,
			ETHER_TO_MACF(sh_mlo_info->mlo_link_info[i].pap_bssid));
	}
	
	SMARTHAUL_TRACE("Calling Smart Haul algorithm initialization routine\n");
	ret = sh_algo_init((void *)smarthaul);
	if (ret != SH_OK) {
		SMARTHAUL_ERROR("Smart Haul algo init failed. Disable Smart Haul");
		free(smarthaul->dev_info);
		smarthaul->dev_info = NULL;
		return ret;
	}

#if !defined(RTCONFIG_HND_ROUTER_BE_4916) && !defined(RTCONFIG_RALINK)
	SMARTHAUL_TRACE("Starting to create ipc socket\n");
	sh_ipc_socket_thread(smarthaul);
#endif

	algo_info = &g_algo.info;
	
	SMARTHAUL_SYSLOG("Smart Haul initial done, waiting for MLO connection\n");
	return SH_OK;
}

static int sh_tidmap_set(sh_tidmap_t *tm)
{
	int ret = SH_OK;
	int i;
	smarthaul_t *smarthaul = &g_smarthaul;
	sh_root_info_t *root_info = (sh_root_info_t *)smarthaul->dev_info;
	sh_repeater_node_t *rpt_list = root_info->rpt_list;
	sh_mbh_t *mbh;
	sh_mlo_info_t *mlo_info;
	while(rpt_list){
		if(rpt_list->sh_idx == tm->sh_idx)
			break;
		rpt_list = rpt_list->next;
	}

	mbh = sh_algo_find_mbh_by_ix(tm->sh_idx);
	mlo_info = mbh->bsta->mlo_info;

	SMARTHAUL_TRACE("MLD: " MACF "\n",
            ETHER_TO_MACF(mlo_info->self_mld_addr));

	/* sysdeps */
	ret = sh_set_cap_tidmap(mlo_info->mlo_num_links, mlo_info->mlo_link_info[0].wlc_unit, tm->tids, &mlo_info->self_mld_addr);
	if (ret != SH_OK){
		SMARTHAUL_ERROR("Setting tidmap on CAP failed!\n");
	}

#if !defined(RTCONFIG_HND_ROUTER_BE_4916) && !defined(RTCONFIG_RALINK)
	ret = sh_send_tidmap2re(rpt_list, tm);
	if (ret != SH_OK){
		SMARTHAUL_ERROR("Setting tidmap on Re failed!\n");
	}
#endif
	return ret;
}

static void sh_mbh_get_tidmap(struct sh_mbh *mbh, uint8 linkset, sh_tidmap_t *map)
{
	sh_link_t *sl;
	int dis_band = 0;
	dis_band = nvram_get_int("sh_algo_disable_band");
	map->sh_idx = mbh->ix;
	memset(map->tids, 0xF0, sizeof(map->tids));

	for (int i = 0; i < MAX_MLO_LINKS; i++) {
		sl = &mbh->link[i];
		if (sl->band_ix == dis_band){
			map->tids[i] = 0xf0;
			continue;
		}
		else if (sl->band_ix == 2 && !(linkset & (1 << i))) {
			map->tids[i] = 0xf0;
			continue;
		}
		else if (!(linkset & (1 << i)))
			continue;

		map->tids[i] = 0xFF; //FF ff?
	}
}

static int sh_reset_tidmap(void)
{
	struct sh_algo *algo = &g_algo;
	struct sh_mbh *mbh = algo->mbh;
	sh_tidmap_t tm;
	while (mbh) {
		mbh->cur_linkset = mbh->cfg_linkset;
		sh_mbh_get_tidmap(mbh, mbh->cfg_linkset, &tm);
		sh_tidmap_set(&tm);
		mbh = mbh->next;
	}
	return SH_OK;
}

static void sh_mbh_reset_stats_flags(struct sh_mbh *mbh)
{
	int i;
	sh_link_t *sl;

	for (i = 0; i < MAX_MLO_LINKS; i++) {
		sl = &mbh->link[i];
		sl->stats.flags = 0;
	}

	if (mbh->bsta)
		sh_mbh_reset_stats_flags(mbh->bsta);
}

static uint8 sh_get_new_linkset(struct sh_mbh *mbh)
{
	mbh->cur_linkset = mbh->can_linkset;
	mbh->change_cycle = mbh->algo->cycle;
	return mbh->can_linkset;
}

static int sh_link_reduce_check(struct sh_mbh *mbh, uint8 link_id)
{
	sh_algo_info_t *info = &mbh->algo->info;
	struct sh_link *sl = &mbh->link[link_id];
	int score = 0;
	uint32 obss_score;
	uint32 obss_multi;
	char *nvram_str;
#if defined(RTCONFIG_RALINK) // for BT8
	int algo_txrx_ratio = 2;
	nvram_str = nvram_get("sh_algo_txrx_ratio");
	algo_txrx_ratio = nvram_str ? strtoul(nvram_str, NULL, 0) : 2;
#endif
	if (!(sl->stats.flags & SH_STATS_FLAG_CHANIM)) {
		SMARTHAUL_TRACE("mbh %x link %d chanim stats not updated\n", mbh->ix, link_id);
		return score;
	}

#if defined(RTCONFIG_HND_ROUTER_BE_4916)
	nvram_str = nvram_get("sh_algo_obss");
	obss_multi = nvram_str ? strtoul(nvram_str, NULL, 0) : 0;
#else
	obss_multi = 0;
#endif

	int rssi_threshold;
	rssi_threshold = nvram_get_int("sh_algo_rssi_hi") ? nvram_get_int("sh_algo_rssi_hi") : -77;

	if (sl->band_ix == 2
#if defined(RTCONFIG_RALINK)
		&& mbh->link[1].stats.rssi > rssi_threshold
		&& mbh->link[2].stats.rssi > rssi_threshold) {
		SMARTHAUL_TRACE("rssi of 6G(%d) & 5G(%d) are higher than %d, remove 2G instantly\n",
				mbh->link[2].stats.rssi, mbh->link[1].stats.rssi, rssi_threshold);
#else
		&& mbh->link[0].stats.rssi > rssi_threshold
		&& mbh->link[1].stats.rssi > rssi_threshold) {
		SMARTHAUL_TRACE("rssi of 6G(%d) & 5G(%d) are higher than %d, remove 2G instantly\n",
				mbh->link[0].stats.rssi, mbh->link[1].stats.rssi, rssi_threshold);
#endif
		score = 100;
		return score;
	}
	else if (nvram_get_int("sh_algo_add_2g") == 1){
		return score;
	}

	/* if obss is low, removing this link from BH will not help */
	if (sl->band_ix == 6){
		nvram_str = nvram_get("sh_algo_obss_6G");
#if defined(BQ16_PRO) 
		obss_multi = nvram_str ? strtoul(nvram_str, NULL, 0) : 6;
#else
		obss_multi = nvram_str ? strtoul(nvram_str, NULL, 0) : 3;
#endif
		if (sl->stats.obss * obss_multi < sl->stats.tx + sl->stats.inbss){
			SMARTHAUL_TRACE("link %d obss is too low with multiplier %d\n", link_id, obss_multi);
			return score;
		}
	}	
	else {
		nvram_str = nvram_get("sh_algo_obss_5G");
		obss_multi = nvram_str ? strtoul(nvram_str, NULL, 0) : 3;
		if (sl->stats.obss * obss_multi < sl->stats.tx + sl->stats.inbss){
			SMARTHAUL_TRACE("link %d obss is too low with multiplier %d\n", link_id, obss_multi);
			return score;
		}
#if defined(RTCONFIG_RALINK)
			if (sl->stats.inbss > sl->stats.tx * algo_txrx_ratio){
				SMARTHAUL_TRACE("inbss > tx * n [%d > %d * %d]\n", sl->stats.inbss, sl->stats.tx, algo_txrx_ratio);
				SMARTHAUL_TRACE("link %d mostly RX, skip txop checking\n", link_id);
				score = 0;
				return score;
			}
#endif
	}

	if (sl->stats.txop < info->txop_lo)
		score = info->txop_lo - sl->stats.txop;

	return score;
}

static int sh_mbh_reduce_check(struct sh_mbh *mbh)
{
	struct sh_algo *algo = &g_algo;
	uint8 busy_linkset = mbh->cur_linkset;
	int reduced = 0;

	/* never exclude the anchor link */
	busy_linkset &= ~(1 << mbh->anchor_link);

	mbh->can_linkset = mbh->cur_linkset;
	for (int i = 0; i < MAX_MLO_LINKS; i++) {
		int score = 0;
		uint8 b = 1 << i;

		if ((b & busy_linkset) == 0)
			continue;

		score = sh_link_reduce_check(mbh, i);
		if (score > 0) {
			mbh->can_linkset &= ~b;
			reduced = 1;
		}
	}

	return reduced;
}

static int sh_link_expand_check(struct sh_mbh *mbh, uint8 link_id)
{
	sh_algo_info_t *info = &mbh->algo->info;
	struct sh_link *sl = &mbh->link[link_id];
	int score = 0;

	uint8 tx_sum, sum;
	char *nvram_str;
	int i;

	if (!(sl->stats.flags & SH_STATS_FLAG_CHANIM)) {
		SMARTHAUL_TRACE("mbh %x link %d chanim stats not updated\n", mbh->ix, link_id);
		return score;
	}

	int rssi_threshold;
	rssi_threshold = nvram_get_int("sh_algo_rssi_lo") ? nvram_get_int("sh_algo_rssi_lo") : -82;

	if (sl->band_ix == 2
#if defined(RTCONFIG_RALINK)
		&& mbh->link[1].stats.rssi > rssi_threshold
		&& mbh->link[2].stats.rssi > rssi_threshold) {
		SMARTHAUL_TRACE("rssi of 6G(%d) & 5G(%d) are higher than %d, skip 2G expand score calculate\n",
				mbh->link[2].stats.rssi, mbh->link[1].stats.rssi, rssi_threshold);
#else
		&& mbh->link[0].stats.rssi > rssi_threshold
		&& mbh->link[1].stats.rssi > rssi_threshold) {
		SMARTHAUL_TRACE("rssi of 6G(%d) & 5G(%d) are higher than %d, skip 2G expand score calculate\n",
				mbh->link[0].stats.rssi, mbh->link[1].stats.rssi, rssi_threshold);
#endif
		return score;
	}
	else if (nvram_get_int("sh_slgo_add_2g") == 1) {
		score = 100;
		return score;
	}

	if (sl->stats.txop > info->txop_hi)
		score = sl->stats.txop - info->txop_hi;

	return score;
}

static int sh_mbh_expand_check(struct sh_mbh *mbh)
{
	struct sh_algo *algo = &g_algo;
	uint8 idle_linkset = mbh->cfg_linkset & (~mbh->cur_linkset);
	int expanded = 0;

	mbh->can_linkset = mbh->cur_linkset;
	for (int i = 0; i < MAX_MLO_LINKS; i++) {
		int score = 0;
		uint8 b = 1 << i;

		if ((b & idle_linkset) == 0)
			continue;

		score = sh_link_expand_check(mbh, i);
		if (score > 0) {
			mbh->can_linkset |= b;
			expanded = 1;
		}
	}

	return expanded;
}

static uint8 sh_ls_next_anchor_link(struct sh_mbh *mbh)
{
	struct sh_algo *algo = &g_algo;
	uint i, best_score = 0;
	uint8 best_link = 0;

#if defined (RTCONFIG_HND_ROUTER_BE_4916) || defined(RTCONFIG_RALINK)
	/* get the link with the highest score, here just use total traffic */
	for (i = 0; i < mbh->link_cnt; i++) {
		sh_link_t *sl = &mbh->link[i];
		uint score;

		if (!(sl->stats.flags & SH_STATS_FLAG_SCB)) {
			SMARTHAUL_TRACE("mbh %x link %d scb stats not updated\n", mbh->ix, i);
			goto exit;
		}

		if ((sl->stats.ntx < sl->stats.last_ntx) || (sl->stats.nrx < sl->stats.last_nrx)) {
			SMARTHAUL_TRACE("mbh %x link %d scb stats overflow\n", mbh->ix, i);
			goto exit;
		}

		/* find the highest utilized link */
		score = sl->stats.ntx - sl->stats.last_ntx + sl->stats.nrx - sl->stats.last_nrx;
		if (score > best_score) {
			best_score = score;
			best_link = i;
		}
	}

	if (best_score && (best_link != mbh->anchor_link)) {
		SMARTHAUL_TRACE("switch anchor_link from %d to %d\n", mbh->anchor_link, best_link);
		mbh->anchor_link = best_link;
	}
#else
	/* select the best link depends on models */
	mbh->anchor_link = best_link;
#endif
exit:
	return mbh->anchor_link;
}

static int sh_algo_run(void)
{
	struct sh_algo *algo = &g_algo;
	struct sh_mbh *mbh = algo->mbh;
	struct sh_mbh *best_mbh = NULL;
	sh_tidmap_t tm;
	uint8 new_linkset UNUSED_VAR;
	int score, best_score = 0, err = SH_OK;
	bool cnt_reset_flg = TRUE;
	static int stop_flg = 0;
#if defined(RTCONFIG_RALINK)
	smarthaul_t *smarthaul = &g_smarthaul;
	sh_root_info_t *root_info = (sh_root_info_t *)smarthaul->dev_info;
	int stop_when_more_re = 1;
	int stop_when_disconnect_links[MAX_MLO_LINKS] = {6,0,0,0}; // can be {5, 6} if 5g 6g breaks, stop smarthaul.
	if (nvram_get_int("sh_bypass_more_re") == 1) {
		stop_when_more_re = 0;
	}
#endif
    if (nvram_get_int("sh_algo_stop") == 1) {
        SMARTHAUL_TRACE("sh_algo_stop == 1, skip\n");
        if (stop_flg == 0){
            SMARTHAUL_TRACE("first time stop, reset tidmap\n");
            sh_reset_tidmap();
            stop_flg = 1;
        }
        goto exit;
    }
#if defined(RTCONFIG_RALINK)
    else if(root_info->num_repeaters > 1 && stop_when_more_re)
    {
        SMARTHAUL_TRACE("more than 1 repeater, skip\n");
        if (stop_flg == 0){
            SMARTHAUL_TRACE("first time stop, reset tidmap\n");
            sh_reset_tidmap();
            stop_flg = 1;
        }
        goto exit;
    }
    else if(mbh->link_cnt < 3)
    {
        SMARTHAUL_TRACE("MLO link_cnt less than 3\n");
	int found = 0;
	sh_link_t *sl;
	for (int i = 0; i < MAX_MLO_LINKS; i++)
	{
		sl = &mbh->link[i];
		int tmp_band_ix = sl->band_ix;
		for (int i = 0; i < MAX_MLO_LINKS; i++)
		{
			if (stop_when_disconnect_links[i] == tmp_band_ix)
			{
				found = 1;
				break;
			}
		}
	}
	if(!found)
	{
        	SMARTHAUL_TRACE("link not found in stop_when_disconnect_links\n");
		if (stop_flg == 0){
		    SMARTHAUL_TRACE("first time stop, reset tidmap\n");
		    sh_reset_tidmap();
		    stop_flg = 1;
		}
		goto exit;
	}
    }
#endif
    else
        stop_flg = 0;

	/* select next anchor link of each mbh */
	mbh = algo->mbh;
	while (mbh) {
		if (mbh->live_cycle < MIN_LIVE_CYCLE) {
			mbh = mbh->next;
			continue;
		}
		if (sh_ls_next_anchor_link(mbh) == -1) {
			SMARTHAUL_ERROR("mbh %d select anchor link failed\n", mbh->ix);
			goto exit;
		}
		mbh = mbh->next;
	}

	/* check if there could be a linkset expansion */
	mbh = algo->mbh;
	while (mbh) {
		if (mbh->live_cycle < MIN_LIVE_CYCLE) {
			mbh = mbh->next;
			continue;
		}
		if (sh_mbh_expand_check(mbh)) {
			new_linkset = sh_get_new_linkset(mbh);
			sh_mbh_get_tidmap(mbh, new_linkset, &tm);
			SMARTHAUL_SYSLOG("cycle %d: mbh %d linkset exp to %x, new tidmap %02x%02x%02x(L0L1L2)\n",
					algo->cycle, mbh->ix, new_linkset, tm.tids[0],
					tm.tids[1], tm.tids[2]);
			sh_dbglog_dump_chanim(mbh);
			sh_tidmap_set(&tm);
		}

		mbh = mbh->next;
	}
#if defined(RTCONFIG_RALINK)
	/* if all band's chanim stat "summation" RX > TX (inbss > tx * n)
	 * 	does not remove linkset
	 * */
	uint8 txop, obss, inbss, tx;
       	uint8 inbss_sum = 0;
	uint8 tx_sum = 0;
	char* nvram_str;
	int algo_txrx_ratio, ret, i;
	nvram_str = nvram_get("sh_algo_txrx_ratio");
	algo_txrx_ratio = nvram_str ? strtoul(nvram_str, NULL, 0) : 2; //default to 2
	
	mbh = algo->mbh;	
	
	for (i = 0; i < mbh->link_cnt; i++){
		ret = sh_get_chanim_stats(&txop, &obss, &inbss, &tx, mbh->link[i].wlc_unit);
		inbss_sum += inbss;
		tx_sum += tx;
	}

	inbss_sum /= (mbh->link_cnt);
	tx_sum /= (mbh->link_cnt);
	if (inbss_sum > tx_sum * algo_txrx_ratio)
	{
		SMARTHAUL_TRACE("[inbss > tx * n][%d > %d * %d], don't reduce linkset\n", inbss_sum, tx_sum, algo_txrx_ratio);
		err = SH_OK;
		goto exit;
	}
#endif

	/* check if there could be a linkset reduction */
	mbh = algo->mbh;
	while (mbh) {
		if (mbh->live_cycle < MIN_LIVE_CYCLE) {
			mbh = mbh->next;
			continue;
		}
		if (sh_mbh_reduce_check(mbh)) {
			new_linkset = sh_get_new_linkset(mbh);
			sh_mbh_get_tidmap(mbh, new_linkset, &tm);
			SMARTHAUL_SYSLOG("cycle %d: mbh %d linkset red to %x, new tidmap %02x%02x%02x(L0L1L2)\n",
					algo->cycle, mbh->ix, new_linkset, tm.tids[0],
					tm.tids[1], tm.tids[2]);
			sh_dbglog_dump_chanim(mbh);
			sh_tidmap_set(&tm);
		}

		mbh = mbh->next;
	}

exit:
	/* move the cycle counter */
	algo->cycle++;
	SMARTHAUL_TRACE("next cycle is %d\n\n", algo->cycle);

	mbh = algo->mbh;
	while (mbh) {
		sh_mbh_reset_stats_flags(mbh);
		mbh = mbh->next;
	}
	return err;
}

static void sh_dump_mbh_stats(uint32 idx)
{
	struct sh_mbh *mbh;
	int i;
	mbh = sh_algo_find_mbh_by_ix(idx);

	for (i = 0; i < mbh->mlo_info->mlo_num_links; i++) {
			SMARTHAUL_TRACE("mbh %d link%d wl%d %dG, txop %2d, obss %2d, tx%2d, inbss %2d, nocat %2d, nopkt %2d, rssi %4d, ntx %4" PRIu64 ", nrx %4" PRIu64 ",thp_ntx %4" PRIu64 ",thp_nrx %4" PRIu64 "\n",
							idx, i, mbh->link[i].wlc_unit, mbh->link[i].band_ix,
							mbh->link[i].stats.txop, mbh->link[i].stats.obss, mbh->link[i].stats.tx,
							mbh->link[i].stats.inbss, mbh->link[i].stats.nocat, mbh->link[i].stats.nopkt,
							mbh->link[i].stats.rssi, mbh->link[i].stats.ntx, mbh->link[i].stats.nrx,
							mbh->link[i].stats.ntx - mbh->link[i].stats.last_ntx,
							mbh->link[i].stats.nrx - mbh->link[i].stats.last_nrx);
	}
}

static int sh_mbh_scb_upd(uint32 sh_idx)
{
	struct sh_mbh *mbh;
	struct sh_link *sl;
	int i, ret = 0;
	uint64_t ntx[MAX_MLO_LINKS];
	uint64_t nrx[MAX_MLO_LINKS];
	int rssi[MAX_MLO_LINKS];
	mbh = sh_algo_find_mbh_by_ix(sh_idx);
	if (!mbh) {
		SMARTHAUL_ERROR("no bh for ix %d\n", sh_idx);
		return SH_ERROR;
	}
#if defined (RTCONFIG_HND_ROUTER_BE_4916)  || defined(RTCONFIG_RALINK)
	ret = sh_get_mlo_scb_stats(ntx, nrx, rssi, &mbh->bsta->mlo_info->self_mld_addr, mbh->link[0].wlc_unit);
	if (ret != SH_OK)
		return ret;

	for (i = 0; i < mbh->mlo_info->mlo_num_links; i++) {
		sl = &mbh->link[i];

		sl->stats.last_ntx = sl->stats.ntx;
		sl->stats.last_nrx = sl->stats.nrx;

		sl->stats.ntx = ntx[i];
		sl->stats.nrx = nrx[i];
		sl->stats.rssi = rssi[i];

		sl->stats.flags |= SH_STATS_FLAG_SCB;
	}
	return SH_OK;
#else
	/* collect throughput & rssi*/
	return SH_OK;
#endif
}

static int sh_mbh_chanim_stats_upd(uint32 sh_idx)
{
	struct sh_mbh *mbh;
	int i, ret = SH_ERROR;
	uint8 txop, obss, inbss, tx;
	uint8 nocat, nopkt;

	mbh = sh_algo_find_mbh_by_ix(sh_idx);
	if (!mbh) {
		SMARTHAUL_ERROR("no bh for ix %d\n", sh_idx);
		return ret;
	}

	for (i = 0; i < mbh->link_cnt; i++){
#if defined (RTCONFIG_HND_ROUTER_BE_4916)
		ret = sh_get_chanim_stats(&txop, &obss, &inbss, &tx, &nocat, &nopkt, mbh->link[i].wlc_unit);
#else
		ret = sh_get_chanim_stats(&txop, &obss, &inbss, &tx, mbh->link[i].wlc_unit);
		nocat = 0;
		nopkt = 0;
#endif
		if (ret == SH_OK){
			mbh->link[i].stats.txop = txop;
			mbh->link[i].stats.obss = obss;
			mbh->link[i].stats.inbss = inbss;
			mbh->link[i].stats.tx = tx;
			mbh->link[i].stats.nocat = nocat;
			mbh->link[i].stats.nopkt = nopkt;

			mbh->link[i].stats.flags |= SH_STATS_FLAG_CHANIM;
		}
	}
	return ret;
}

static void sh_collect_stats(void)
{
	struct sh_algo *algo = &g_algo;
	struct sh_mbh *mbh = algo->mbh;
	uint32 mbh_idx;

	while (mbh) {
		mbh_idx = mbh->ix;
		if (sh_mbh_chanim_stats_upd(mbh_idx) != SH_OK)
			SMARTHAUL_ERROR("mbh %d failed to update chanim_state\n", mbh_idx);
		if(sh_mbh_scb_upd(mbh_idx) != SH_OK) {
			SMARTHAUL_ERROR("mbh %d failed to update scb_stats, delete it\n", mbh_idx);
			CLRBIT(sh_idx_bitvec, mbh_idx);
			mbh = mbh->next;
			sh_del_bh(mbh_idx);
			continue;
		}
		sh_dump_mbh_stats(mbh_idx);
		if (++mbh->live_cycle < MIN_LIVE_CYCLE)
			SMARTHAUL_TRACE("mbh %d don't live enough cycle(%d/%d)\n", mbh_idx, mbh->live_cycle, MIN_LIVE_CYCLE);
		mbh = mbh->next;
	}
	mbh = algo->mbh;
	if (algo->mbh)
		sh_algo_run();
}

#if defined (RTCONFIG_HND_ROUTER_BE_4916) || defined(RTCONFIG_RALINK)
static int sh_check_dup_mbh(struct ether_addr *mld_addr)
{
	struct sh_algo *algo = &g_algo;
	struct sh_mbh *mbh = algo->mbh;
	char target_mac[18], mbh_mac[18];
	ether_etoa(SH_ETHERP_ADDR(mld_addr), target_mac);
	while (mbh) {
		ether_etoa(SH_ETHER_ADDR(mbh->bsta->mlo_info->self_mld_addr), mbh_mac);
		if (!strncmp(target_mac, mbh_mac, sizeof(target_mac))) {
			SMARTHAUL_ERROR("mld %s is exist in mbh %d (%s)\n", target_mac, mbh->ix, mbh_mac);
			return -1;
		}
		mbh = mbh->next;
	}

	return SH_OK;
}

static void sh_mbh_num_upd(void)
{
	smarthaul_t *smarthaul = &g_smarthaul;
	sh_root_info_t* root_info = (sh_root_info_t *)smarthaul->dev_info;
	sh_mlo_info_t* root_mlo_info = &root_info->local_dev.mlo_info;
	sh_mlo_info_t* new_mbh_mlo_info;
	struct sh_algo *algo = &g_algo;
	int i;

	int no_of_mlo_scb = 0;
#if defined(RTCONFIG_RALINK)
	int calloc_count = algo->mbh_cnt + 1; // assume everytime updates, there are enough room for new mld station.
	struct bmgr_mld_sta *mld_sta_buf = calloc(calloc_count, sizeof(struct bmgr_mld_sta));
#endif

#if defined (RTCONFIG_HND_ROUTER_BE_4916)
	no_of_mlo_scb = sh_get_no_of_mlo_scb(root_mlo_info->mlo_link_info[0].wlc_unit);

	if (no_of_mlo_scb > algo->mbh_cnt) {
		for (i = 0; i < no_of_mlo_scb; i++){
			new_mbh_mlo_info = calloc(1, sizeof(sh_mlo_info_t));
			sh_get_scb_mld(root_mlo_info->mlo_link_info[0].wlc_unit, i, &new_mbh_mlo_info->self_mld_addr);
			SMARTHAUL_TRACE("mld get by sh_get_scb_mld: "MACF"\n", ETHER_TO_MACF(new_mbh_mlo_info->self_mld_addr));
			if (sh_check_dup_mbh(&new_mbh_mlo_info->self_mld_addr) == SH_OK) {
				if (sh_add_new_mbh(new_mbh_mlo_info) != SH_OK && new_mbh_mlo_info) {
					free(new_mbh_mlo_info);
					SMARTHAUL_TRACE("failed to add new mbh\n");
				}
			}
			else
				free(new_mbh_mlo_info);
		}
	}
#elif defined(RTCONFIG_RALINK)
	no_of_mlo_scb = sh_get_no_of_mlo_scb(root_mlo_info->mlo_link_info[0].wlc_unit);
	sh_mtk_get_scb_mld(mld_sta_buf);
	if (no_of_mlo_scb > algo->mbh_cnt) {
		for (i = 0; i < no_of_mlo_scb; i++){
			new_mbh_mlo_info = calloc(1, sizeof(sh_mlo_info_t));
			eacopy(mld_sta_buf[i].mld_addr, &new_mbh_mlo_info->self_mld_addr);
			SMARTHAUL_TRACE("mld get by sh_get_scb_mld: "MACF"\n", ETHER_TO_MACF(new_mbh_mlo_info->self_mld_addr));
			if (sh_check_dup_mbh(&new_mbh_mlo_info->self_mld_addr) == SH_OK) {
				if (sh_add_new_mbh(new_mbh_mlo_info) != SH_OK && new_mbh_mlo_info) {
					free(new_mbh_mlo_info);
					//SMARTHAUL_TRACE("failed to add new mbh\n");
					SMARTHAUL_ERROR("failed to add new mbh\n");
				}
			}
			else
				free(new_mbh_mlo_info);
		}
	}
	if(mld_sta_buf)
		free(mld_sta_buf);
#else
#error "NEED Implementation"
#endif
}

static void timerHandler(int sig){
	sh_mbh_num_upd();
	sh_collect_stats();
}

#else
static void timerHandler(int sig){
	sh_collect_stats();
}
#endif

static void sigtermHandler(int sig)
{
	sh_reset_tidmap();
	exit(0);
}

int smarthaul_main(void)
{
	smarthaul_t *smarthaul = &g_smarthaul;
	char *nvram_val;

	sleep(15); /* sleep for a while, wait for mlo init */

	signal(SIGALRM, timerHandler);
	signal(SIGTERM, sigtermHandler);

	/* set debug log level */
	if ((nvram_val = nvram_get("smarthaul_msglevel"))) {
		smarthaul_debug_level = (uint)strtoul(nvram_val, NULL, 0);
		smarthaul_debug_level |= SMARTHAUL_DEFAULT_LEVEL;
		SMARTHAUL_TRACE("smarthaul debug level %x\n", smarthaul_debug_level);
	}

	SMARTHAUL_TRACE("Starting smarthaul!\n");
	if (sh_init(smarthaul) != SH_OK)
		exit(0);
	
	if (sh_add_timer((int)algo_info->cycle_ms) != SH_OK)
		exit(0);
#if defined(RTCONFIG_RALINK)
	/* check smart_haul_enable nvram  */
	if(nvram_get_int("smart_haul_enable") != 1){
		SMARTHAUL_ERROR("smart_haul_enable not set to 1, NOT running SMARTHAUL!\n");
		return 0;
	}
#endif

	while (1)
	{
		pause();
	}
	return 0;
}

static void sh_dbglog_dump_chanim(struct sh_mbh *mbh)
{
	struct sh_link *sl;
	int i;
	for (i = 0; i < mbh->link_cnt; i++) {
		sl = &mbh->link[i];
		SMARTHAUL_DBGLOG("mbh %d link%d wl%d %dG, txop %d, obss %d, tx%d, inbss %d, nocat %d, nopkt %d\n",
			sl->mbh->ix, sl->link_id, sl->wlc_unit, sl->band_ix,
			sl->stats.txop, sl->stats.obss, sl->stats.tx, sl->stats.inbss, sl->stats.nocat, sl->stats.nopkt);
	}
}

static void dump_sh_dev_info(sh_dev_info_t *dev)
{
	int i;

	SMARTHAUL_TRACE("Self MAC assress" MACF ", Self MLD address: " MACF " num links: %d\n",
		ETHER_TO_MACF(dev->self_mac),
		ETHER_TO_MACF(dev->mlo_info.self_mld_addr),
		dev->mlo_info.mlo_num_links);
	for (i = 0; i < dev->mlo_info.mlo_num_links; i++) {
		SMARTHAUL_TRACE("wlc_unit: %d, link %d, pap bssid: " MACF "\n",
			dev->mlo_info.mlo_link_info[i].wlc_unit, dev->mlo_info.mlo_link_info[i].link_id, ETHER_TO_MACF(dev->mlo_info.mlo_link_info[i].pap_bssid));
	}
}

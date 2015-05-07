#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/time.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include <infiniband/verbs.h>
#include <infiniband/mad.h>
#include <sys/types.h>
#include <sys/socket.h>

/* poll CQ timeout in millisec (2 seconds) */
#define MAX_POLL_CQ_TIMEOUT 2000
#define MSG                 "SEND operation      "
#define RDMAMSGR            "RDMA read operation "
#define RDMAMSGW            "RDMA write operation"
#define MSG_SIZE            (strlen(MSG) + 1)

struct config_t {
	const char  *dev_name;    /* ib device name */
	char        *server_name; /* server host name */
	u_int32_t   tcp_port;     /* server tcp port */
	int         ib_port;      /* local ib port to work with */
	int         gid_idx;      /* gid index to use */
};

/* structure to exchange data which is needed to connect the QPs */
struct cm_con_data_t {
	uint64_t addr;    /* buffer addr */ 
	uint32_t rkey;    /* remote key */ 
	uint32_t qp_num;  /* QP number */ 
	uint16_t lid;     /* lid of the ib port */ 
	uint8_t  gid[16]; /* gid */ 
} __attribute__((packed));

struct resources {
	struct ibv_device_attr  device_attr;
	struct ibv_port_attr    port_attr;
	struct cm_con_data_t    remote_props; /* values to connect to remote side */ 
	struct ibv_context      *ib_ctx;
	struct ibv_pd           *pd;
	struct ibv_cq           *cq;
	struct ibv_qp           *qp;
	struct ibv_mr           *mr;
	char                    *buf;
	int                     sockfd;
};

static struct config_t config = {
	NULL,  /* dev_name */ 
	NULL,  /* server_name */ 
	19875, /* tcp_port */ 
	1,     /* ib_port */ 
	-1     /* gid_idx */ 
};

static int sock_connect(const char *servername, int port)
{
	int sockfd = -1;
	int tmp;
	struct sockaddr_in addr;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		perror("socket");
		exit(1);
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr("10.1.1.5");
	if (servername) {
		if ((tmp = connect(sockfd, (struct sockaddr *)&addr, sizeof(addr))) < 0) {
			perror("connect");
			close(sockfd);
			return -1;
		}
		return sockfd;
	}
	if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("bind");
		close(sockfd);
		return -1;
	}
	listen(sockfd, 1);
	return accept(sockfd, NULL, 0);
}

int sock_sync_data(int sockfd, int xfer_size, char *local_data, char *remote_data)
{
	int nbytes_write = 0;
	int nbytes_read = 0;
	int tmp;

	/* send local data to remote */
	printf("send data to remote\n");
	if ((nbytes_write = write(sockfd, local_data, xfer_size)) < xfer_size) {
		fprintf(stderr, "Failed writing data during sock_sync_data\n");
		return -1;
	}

	/* receive remote message */
	printf("receive remote data\n");
	while (nbytes_read < xfer_size) {
		tmp = read(sockfd, remote_data + nbytes_read, xfer_size);
		if (tmp < 0) {
			printf("receive failed\n");
			return -1;
		}
		if (tmp == EOF)
			return 0;
		nbytes_read += tmp;
	}
	return 0;
}

static int poll_completion(struct resources *res)
{
	struct timeval cur_time;
	unsigned long  start_time_msec;
	unsigned long  cur_time_msec;
	struct ibv_wc  wc;
	int            poll_result;
	int            ret = 0;

	/* poll the completion for a while before giving up */
	gettimeofday(&cur_time, NULL);
	start_time_msec = (cur_time.tv_sec * 1000) + (cur_time.tv_usec / 1000);

	do {
		poll_result = ibv_poll_cq(res->cq, 1, &wc);
		gettimeofday(&cur_time, NULL);
		cur_time_msec = (cur_time.tv_sec * 1000) + (cur_time.tv_usec / 1000);
	} while ((poll_result == 0) && ((cur_time_msec - start_time_msec) < MAX_POLL_CQ_TIMEOUT));

	if (poll_result < 0) {
		/* poll CQ failed */
		fprintf(stderr, "poll CQ failed\n");
		ret = -1;
	} else if (poll_result == 0) {
		/* the CQ is empty */
		fprintf(stderr, "completion was not found after timeout\n");
		ret = -1;
	} else {
		/* CQE found */
		printf("completion found with status 0x%x\n", wc.status);
		if (wc.status != IBV_WC_SUCCESS) {
			fprintf(stderr, "got bad completion with status: 0x%x vendor syndrome: 0x%x\n",
					wc.status, wc.vendor_err);
			ret = -1;
		}
	}

	return ret;
}

static void resources_init(struct resources *res)
{
	memset(res, 0, sizeof(*res));
	res->sockfd = -1;
}

static int resources_create(struct resources *res)
{
	struct ibv_device **dev_list = NULL;
	struct ibv_qp_init_attr qp_init_attr;
	struct ibv_device *ibv_dev = NULL;

	size_t size;
	int    i;
	int    mr_flags = 0;
	int    cq_size = 0;
	int    num_devices;
	int    ret = 0;

	/* if client side */
	if (config.server_name) {
		res->sockfd = sock_connect(config.server_name, config.tcp_port);
		if (res->sockfd < 0) {
			fprintf(stderr, "failed to establish TCP connection to server %s, port %d\n", config.server_name, config.tcp_port);
			ret = -1;
			goto resources_create_exit;
		}
	} else {
		fprintf(stdout, "waiting on port %d for TCP connection\n", config.tcp_port);
		res->sockfd = sock_connect(NULL, config.tcp_port);
		if (res->sockfd < 0) {
			fprintf(stderr, "failed to establish TCP connection with client on port %d\n", config.tcp_port);
			ret = -1;
			goto resources_create_exit;
		}
	}

	printf("TCP connection was established\n");
	printf("searching for IB devices in host\n");

	/* get device names in the system */
	if (!(dev_list = ibv_get_device_list(&num_devices))) {
		fprintf(stderr, "failed to get IB devices list\n");
		ret = -1;
		goto resources_create_exit;
	}
	/* if there isn't any IB device in host */
	if (num_devices == 0) {
		fprintf(stderr, "no ib devices is available\n");
		ret = -1;
		goto resources_create_exit;
	}

	fprintf(stderr, "found %d device(s)\n", num_devices);
	/* ret = -1; */

	for (i = 0; i < num_devices; i++) {
		if (!config.dev_name) {
			config.dev_name = strdup(ibv_get_device_name(dev_list[i]));
			printf("device not specified, using first one found: %s\n", config.dev_name);
		}
		if (strcmp(ibv_get_device_name(dev_list[i]), config.dev_name) == 0) {
			ibv_dev = dev_list[i];
			break;
		}
	}

	if (!ibv_dev) {
		fprintf(stderr, "IB device %s is not found\n", config.dev_name);
		ret = -1;
		goto resources_create_exit;
	}

	/* get device handle */
	if (!(res->ib_ctx = ibv_open_device(ibv_dev))) {
		fprintf(stderr, "failed to open device %s\n", config.dev_name);
		ret = -1;
		goto resources_create_exit;
	}

	/* We are now done with device list, free it */
	ibv_free_device_list(dev_list);
	dev_list = NULL;
	ibv_dev = NULL;
	/* query port properties */
	if (ibv_query_port(res->ib_ctx, config.ib_port, &res->port_attr)) {
		fprintf(stderr, "ibv_query_port on port %u failed\n", config.ib_port);
		ret = -1;
		goto resources_create_exit;
	}

	/* allocate protection domain */
	if (!(res->pd = ibv_alloc_pd(res->ib_ctx))) {
		fprintf(stderr, "ibv_alloc_pd failed\n");
		ret = -1;
		goto resources_create_exit;
	}

	/* FIXME each side will send only WR, so completion queue with 1 entry */
	/* FIXME no completion channel is attached */
	cq_size = 1;
	if (!(res->cq = ibv_create_cq(res->ib_ctx, cq_size, NULL, NULL, 0))) {
		fprintf(stderr, "failed to create CQ with %u entries\n", cq_size);
		ret = -1;
		goto resources_create_exit;
	}

	/* allocate the memory buffer */
	size = MSG_SIZE;
	if (!(res->buf = (char *)malloc(size))) {
		fprintf(stderr, "failed to malloc %Zu bytes of memory buffer\n", size);
		ret = -1;
		goto resources_create_exit;
	}
	memset(res->buf, 0, size);

	/* only in the server side put the message in the memory buffer */
	if (!config.server_name) {
		strcpy(res->buf, MSG);
		printf("going to send the message: %s\n", res->buf);
	} else {
		memset(res->buf, 0, size);
	}

	mr_flags = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE;
	if (!(res->mr = ibv_reg_mr(res->pd, res->buf, size, mr_flags))) {
		fprintf(stderr, "ibv_reg_mr failed with mr_flags=0x%x\n", mr_flags);
		ret = -1;
		goto resources_create_exit;
	}
	printf("MR is registered with addr=%p lkey=0x%x rkey=0x%x flags=0x%x\n",
			res->buf, res->mr->lkey, res->mr->rkey, mr_flags);
	
	/* create the queue pair */
	memset(&qp_init_attr, 0, sizeof(qp_init_attr));
	qp_init_attr.qp_type    = IBV_QPT_RC;
	qp_init_attr.sq_sig_all = 1;
	qp_init_attr.send_cq    = res->cq;
	qp_init_attr.recv_cq    = res->cq;
	qp_init_attr.cap.max_send_wr  = 1;
	qp_init_attr.cap.max_recv_wr  = 1;
	qp_init_attr.cap.max_send_sge = 1;
	qp_init_attr.cap.max_recv_sge = 1;

	if (!(res->qp = ibv_create_qp(res->pd, &qp_init_attr))) {
		fprintf(stderr, "failed to create QP\n");
		ret = -1;
		goto resources_create_exit;
	}
	printf("QP created 0x%x\n", res->qp->qp_num);

resources_create_exit:
	if(ret) {
		/* Error encountered, cleanup */
		if(res->qp) {
			ibv_destroy_qp(res->qp);
			res->qp = NULL;
		}
		if(res->mr) {
			ibv_dereg_mr(res->mr);
			res->mr = NULL;
		}
		if(res->buf) {
			free(res->buf);
			res->buf = NULL;
		}
		if(res->cq) {
			ibv_destroy_cq(res->cq);
			res->cq = NULL;
		}
		if(res->pd) {
			ibv_dealloc_pd(res->pd);
			res->pd = NULL;
		}
		if(res->ib_ctx) {
			ibv_close_device(res->ib_ctx);
			res->ib_ctx = NULL;
		}
		if(dev_list) {
			ibv_free_device_list(dev_list);
			dev_list = NULL;
		}
		if (res->sockfd >= 0) {
			if (close(res->sockfd))
				fprintf(stderr, "failed to close socket\n");
			res->sockfd = -1;
		}
	}
	return ret;
}

/* change state of QP to INIT */
static int modify_qp_to_init(struct ibv_qp *qp)
{
	struct ibv_qp_attr attr;
	int                flags;

	memset(&attr, 0, sizeof(attr));

	attr.qp_state = IBV_QPS_INIT; /* init state */ 
	attr.port_num = config.ib_port;
	attr.pkey_index = 0;
	attr.qp_access_flags = IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ |
		IBV_ACCESS_REMOTE_WRITE;
	flags = IBV_QP_STATE | IBV_QP_PKEY_INDEX |
		IBV_QP_PORT | IBV_QP_ACCESS_FLAGS;
	if (ibv_modify_qp(qp, &attr, flags) < 0) {
		fprintf(stderr, "failed to modify QP state to INIT\n");
		return -1;
	}
	return 0;
}

static int modify_qp_to_rtr(struct ibv_qp *qp, uint32_t remote_qp_num,
		uint16_t dlid, uint8_t *dgid)
{
	struct ibv_qp_attr attr;
	int                flags;

	memset(&attr, 0, sizeof(attr));

	attr.qp_state = IBV_QPS_RTR;
	attr.path_mtu = IBV_MTU_256;
	attr.dest_qp_num = remote_qp_num;
	attr.rq_psn = 0;
	attr.max_dest_rd_atomic = 1;
	attr.min_rnr_timer = 0x12;
	attr.ah_attr.is_global = 0;
	attr.ah_attr.dlid = dlid;
	attr.ah_attr.sl = 0;
	attr.ah_attr.src_path_bits = 0;
	attr.ah_attr.port_num = config.ib_port;

	if (config.gid_idx >= 0) {
		attr.ah_attr.is_global = 1;
		attr.ah_attr.port_num = 1;
		memcpy(&attr.ah_attr.grh.dgid, dgid, 16);
		attr.ah_attr.grh.flow_label = 0;
		attr.ah_attr.grh.hop_limit = 1;
		attr.ah_attr.grh.sgid_index = config.gid_idx;
		attr.ah_attr.grh.traffic_class = 0;
	}

	flags = IBV_QP_STATE | IBV_QP_AV | IBV_QP_PATH_MTU | IBV_QP_DEST_QPN |
		IBV_QP_RQ_PSN | IBV_QP_MAX_DEST_RD_ATOMIC | IBV_QP_MIN_RNR_TIMER;

	if (ibv_modify_qp(qp, &attr, flags) < 0) {
		fprintf(stderr, "failed to modify QP state to RTR\n");
		return -1;
	}
	return 0;
}

static int modify_qp_to_rts(struct ibv_qp *qp)
{
	struct ibv_qp_attr attr;
	int                flags;

	attr.qp_state = IBV_QPS_RTS;
	attr.timeout = 0x12;
	attr.retry_cnt = 6;
	attr.rnr_retry = 0;
	attr.sq_psn = 0;
	attr.max_rd_atomic = 1;

	flags = IBV_QP_STATE | IBV_QP_TIMEOUT | IBV_QP_RETRY_CNT |
		IBV_QP_RNR_RETRY | IBV_QP_SQ_PSN | IBV_QP_MAX_QP_RD_ATOMIC;

	if (ibv_modify_qp(qp, &attr, flags) < 0) {
		fprintf(stderr, "failed to modify QP state to RTS\n");
		return -1;
	}
	return 0;
}

int post_send(struct resources *res, int opcode)
{
	struct ibv_send_wr sr;
	struct ibv_send_wr *bad_wr = NULL;
	struct ibv_sge     sge;
	int ret;

	/* prepare the sg entry */
	memset(&sge, 0, sizeof(sge));

	sge.addr = (uintptr_t)res->buf;
	sge.length = MSG_SIZE;
	sge.lkey = res->mr->lkey;

	/* prepare send work request */
	memset(&sr, 0, sizeof(sr));

	sr.next = NULL;
	sr.wr_id = 0;
	sr.sg_list = &sge;
	sr.num_sge = 1;
	sr.opcode = opcode;
	sr.send_flags = IBV_SEND_SIGNALED;

	/* FIXME why? */
	if (opcode != IBV_WR_SEND) {
		sr.wr.rdma.remote_addr = res->remote_props.addr;
		sr.wr.rdma.rkey = res->remote_props.rkey;
	}

	/* there is a Receive Request in the responder side,
	 * so we won't get andy into RNR flow */
	if ((ret = ibv_post_send(res->qp, &sr, &bad_wr)) < 0) {
		fprintf(stderr, "failed to post SR\n");
	} else {
		switch (opcode) {
			case IBV_WR_SEND:
				printf("Send Request was posted\n");
				break;
			case IBV_WR_RDMA_READ:
				printf("RDMA Read Request was posted\n");
				break;
			case IBV_WR_RDMA_WRITE:
				printf("RDMA Write Request was posted\n");
				break;
			default:
				printf("Unknown Request was posted\n");
				break;
		}
	}

	return ret;
}

static int post_receive(struct resources *res)
{
	struct ibv_recv_wr rr;
	struct ibv_sge     sge;
	struct ibv_recv_wr *bad_wr;
	int                ret;

	/* prepare the sg entry */
	memset(&sge, 0, sizeof(sge));
	sge.addr = (uintptr_t)res->buf;
	sge.length = MSG_SIZE;
	sge.lkey = res->mr->lkey;

	/* prepare the receive work request */
	memset(&rr, 0, sizeof(rr));
	rr.next = NULL;
	rr.wr_id = 0;
	rr.sg_list = &sge;
	rr.num_sge = 1;

	/* post the Receive Request to RQ */
	if ((ret = ibv_post_recv(res->qp, &rr, &bad_wr)) < 0) {
		fprintf(stderr, "failed to post RR\n");
	} else {
		printf("Receive Request was posted\n");
	}

	return ret;
}

static int connect_qp(struct resources *res)
{
	struct cm_con_data_t local_con_data;
	struct cm_con_data_t remote_con_data;
	struct cm_con_data_t tmp_con_data;
	int ret = 0;
	char tmp_char;
	union ibv_gid my_gid;

	if (config.gid_idx >= 0) {
		if ((ret = ibv_query_gid(res->ib_ctx, config.ib_port, config.gid_idx, &my_gid)) == -1) {
			fprintf(stderr, "could not get gid for port %d, idx %d\n",
					config.ib_port, config.gid_idx);
			return ret;
		}
	} else {
		memset(&my_gid, 0, sizeof(my_gid));
	}

	/* exchange using TCP sockets info required to connect QPs */
	local_con_data.addr = htonll((uintptr_t)res->buf);
	local_con_data.rkey = htonl(res->mr->rkey);
	local_con_data.qp_num = htonl(res->qp->qp_num);
	local_con_data.lid = htons(res->port_attr.lid);
	memcpy(local_con_data.gid, &my_gid, 16);
	printf("\nLocal LID = 0x%x\n", res->port_attr.lid);

	if (sock_sync_data(res->sockfd, sizeof(struct cm_con_data_t),
				(char *)&local_con_data, (char *)&tmp_con_data) == -1) {
		fprintf(stderr, "failed to exchange connection data between sides\n");
		ret = -1;
		goto connect_qp_exit;
	}

	remote_con_data.addr = ntohll(tmp_con_data.addr);
	remote_con_data.rkey = ntohl(tmp_con_data.rkey);
	remote_con_data.qp_num = ntohl(tmp_con_data.qp_num);
	remote_con_data.lid = ntohs(tmp_con_data.lid);
	memcpy(remote_con_data.gid, tmp_con_data.gid, 16);

	/* save the remote side attributes, necessary for post SR */
	res->remote_props = remote_con_data;

	printf("remote address = 0x%"PRIx64"\n", remote_con_data.addr);
	printf("remote rkey    = 0x%x\n", remote_con_data.rkey);
	printf("remote QP num  = 0x%x\n", remote_con_data.qp_num);
	printf("remote LID     = 0x%x\n", remote_con_data.lid);
	if (config.gid_idx >= 0) {
		uint8_t *p = remote_con_data.gid;
		printf("remote GID     = %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
				p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9],
				p[10], p[11], p[12], p[13], p[14], p[15]);
	}

	/* modify the QP to init */
	if ((ret = modify_qp_to_init(res->qp)) == -1) {
		fprintf(stderr, "change QP state to INIT failed\n");
		goto connect_qp_exit;
	}

	/* let the client post RR to be prepared for incomming messages */
	if (config.server_name) {
		if ((ret = post_receive(res)) == -1) {
			fprintf(stderr, "fail to post RR\n");
			goto connect_qp_exit;
		}
	}

	/* modify the QP to RTR */
	if ((ret = modify_qp_to_rtr(res->qp, remote_con_data.qp_num,
					remote_con_data.lid, remote_con_data.gid)) == -1) {
		fprintf(stderr, "failed to modify QP state to RTR\n");
		goto connect_qp_exit;
	}

	/* modify the QP to RTS */
	if ((ret = modify_qp_to_rts(res->qp)) == -1) {
		fprintf(stderr, "failed to modify QP state to RTS\n");
		goto connect_qp_exit;
	}
	printf("QP state changed to RTS\n");

	if (sock_sync_data(res->sockfd, 1, "Q", &tmp_char)) {
		fprintf(stderr, "sync error after QPs are RTS\n");
		ret = -1;
	}

connect_qp_exit:
	return ret;
}

static int resources_destroy(struct resources *res)
{
	int ret = 0;

	if (res->qp) {
		if (ibv_destroy_qp(res->qp)) {
			fprintf(stderr, "failed to destroy QP\n");
			ret = -1;
		}
	}

	if (res->mr) {
		if (ibv_dereg_mr(res->mr)) {
			fprintf(stderr, "failed to dereg mr\n");
			ret = -1;
		}
	}

	if (res->buf) {
		free(res->buf);
	}

	if (res->cq) {
		if (ibv_destroy_cq(res->cq)) {
			fprintf(stderr, "failed to destroy CQ\n");
			ret = -1;
		}
	}

	if (res->pd) {
		if (ibv_dealloc_pd(res->pd)) {
			fprintf(stderr, "failed to dealloc pd\n");
			ret = -1;
		}
	}

	if (res->ib_ctx) {
		if (ibv_close_device(res->ib_ctx)) {
			fprintf(stderr, "failed to close device context\n");
			ret = -1;
		}
	}

	if (res->sockfd >= 0) {
		if (close(res->sockfd)) {
			fprintf(stderr, "failed to close socket\n");
			ret = -1;
		}
	}

	return ret;
}

static void print_config(void)
{
	printf(" ----------------------\n");
	printf("Device name :%s\n", config.dev_name);
	printf("IB port     :%u\n", config.ib_port);
	if (config.server_name) {
		printf("IP          :%s\n", config.server_name);
		printf("TCP port    :%u\n", config.tcp_port);
	}
	if (config.gid_idx >= 0) {
		printf("GID index   :%u\n", config.gid_idx);
	}
	printf(" ----------------------\n");
}

static void usage(const char *argv0)
{
	printf("Usage:\n");
}

int main(int argc, char *argv[])
{
	struct resources res;
	int              rc = 1;
	char             temp_char;
	int              opt;

	/* parse the command line parameters */
	static struct option long_options[] =
	{
		{"port",    required_argument, 0, 'p'},
		{"ib-dev",  required_argument, 0, 'd'},
		{"ib-port", required_argument, 0, 'i'},
		{"gid-idx", required_argument, 0, 'g'},
		{NULL, 0, 0, 0}
	};

	while ((opt = getopt_long(argc, argv, "p:d:i:g:", long_options, NULL)) != -1) {
		switch (opt) {
			case 'p':
				config.tcp_port = strtoul(optarg, NULL, 0);
				break;
			case 'd':
				config.dev_name = strdup(optarg);
				break;

			case 'i':
				config.ib_port = strtoul(optarg, NULL, 0);
				if (config.ib_port < 0) {
					usage(argv[0]);
					return 1;
				}
				break;
			case 'g':
				config.gid_idx = strtol(optarg, NULL, 0);
				if (config.gid_idx < 0) {
					usage(argv[0]);
					return 1;
				}
				break;
			default:
				usage(argv[0]);
				return 1;
		}
	}
	/* parse the last parameter (if exists) as the server name */
	if (optind == argc - 1) {
		config.server_name = argv[optind];
	} else if (optind < argc) {
		usage(argv[0]);
		return 1;
	}

	/* print the used parameters for info*/
	print_config();

	/* init all of the resources, so cleanup will be easy */
	resources_init(&res);

	/* create resources before using them */
	if (resources_create(&res)) {
		fprintf(stderr, "failed to create resources\n");
		goto main_exit;
	}

	/* connect the QPs */
	if (connect_qp(&res)) {
		fprintf(stderr, "failed to connect QPs\n");
		goto main_exit;
	}

	/* let the server post the sr */
	if (!config.server_name) {
		if (post_send(&res, IBV_WR_SEND)) {
			fprintf(stderr, "failed to post sr\n");
			goto main_exit;
		}
	}

	/* in both sides we expect to get a completion */
	if (poll_completion(&res)) {
		fprintf(stderr, "poll completion failed\n"); goto main_exit;
	}

	/* after polling the completion we have the message in the client buffer too */
	if (config.server_name) {
		fprintf(stdout, "Message is: '%s'\n", res.buf);
	} else {
		strcpy(res.buf, RDMAMSGR);
	}

	/* Sync so we are sure server side has data ready before client tries to read it */
	if (sock_sync_data(res.sockfd, 1, "R", &temp_char)) { /* just send a dummy char back and forth */
		fprintf(stderr, "sync error before RDMA ops\n"); rc = 1;
		goto main_exit;
	}

	/* Now the client performs an RDMA read and then write on server. Note that the server has no idea these events have occured */
	if (config.server_name) {
		printf("client perform RDMA read and write\n");
		/* First we read contens of server's buffer */
		if (post_send(&res, IBV_WR_RDMA_READ))
		{
			fprintf(stderr, "failed to post SR 2\n");
			rc = 1;
			goto main_exit;
		}
		if (poll_completion(&res))
		{
			fprintf(stderr, "poll completion failed 2\n"); rc = 1;
			goto main_exit;
		}
		fprintf(stdout, "Contents of server's buffer: '%s'\n", res.buf);
		/* Now we replace what's in the server's buffer */
		strcpy(res.buf, RDMAMSGW);
		fprintf(stdout, "Now replacing it with: '%s'\n", res.buf);
		if (post_send(&res, IBV_WR_RDMA_WRITE))
		{
			fprintf(stderr, "failed to post SR 3\n");
			rc = 1;
			goto main_exit;
		}
		if (poll_completion(&res))
		{
			fprintf(stderr, "poll completion failed 3\n"); rc = 1;
			goto main_exit;
		}
	}

	/* Sync so server will know that client is done mucking with its memory */
	if (sock_sync_data(res.sockfd, 1, "W", &temp_char)) { /* just send a dummy char back and forth */
		fprintf(stderr, "sync error after RDMA ops\n"); rc = 1;
		goto main_exit;
	}

	if(!config.server_name)
		fprintf(stdout, "Contents of server buffer: '%s'\n", res.buf);
	rc = 0;
main_exit:
	if (resources_destroy(&res))
	{
		fprintf(stderr, "failed to destroy resources\n");
		rc = 1; }
	if(config.dev_name)
		free((char *) config.dev_name);
	fprintf(stdout, "\ntest result is %d\n", rc);
	return rc;
}

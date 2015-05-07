/* Practice establishing connection between two hosts */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <rdma/rdma_cma.h>

#define ASSERT_Z(x) do { if ( (x)) die("error: " #x " failed (returned non-zero)." ); } while (0)
#define ASSERT_NZ(x)  do { if (!(x)) die("error: " #x " failed (returned zero/null)."); } while (0)

void die(const char *reason)
{
	fprintf(stderr, "%s\n", reason);
	exit(EXIT_FAILURE);
}

struct connection {
	struct ibv_qp *qp;

	struct ibv_mr *recv_mr;
	struct ibv_mr *send_mr;

	char *recv_region;
	char *send_region;
};

/* Represent the essential information of a commucation endpoint */
struct endpoint {
	struct ibv_context *ctx;
	struct ibv_pd *pd;
	struct ibv_cq *cq;
	struct ibv_comp_channel *chan;

	struct ibv_qp *qp;

	struct ibv_mr *recv_mr;
	struct ibv_mr *send_mr;

	char *recv_region;
	char *send_region;

	pthread_t cq_poller_thread;
};

static void on_completion(struct ibv_wc *wc)
{
	if (wc->status != IBV_WC_SUCCESS)
		die("on_completion: status is not IBV_WC_SUCCESS.");

	if (wc->opcode & IBV_WC_RECV) {
		struct connection *conn = (struct connection *)(uintptr_t)wc->wr_id;

		printf("received message: %s\n", conn->recv_region);

	} else if (wc->opcode == IBV_WC_SEND) {
		printf("send completed successfully.\n");
	}
}

/* set the attribute of queue pair to be created */
void build_qp_attr(struct endpoint *ep, struct ibv_qp_init_attr *qp_attr)
{
	memset(qp_attr, 0, sizeof(*qp_attr));

	/* set the completion queue of both send and receive queue */
	qp_attr->send_cq = ep->cq;
	qp_attr->recv_cq = ep->cq;
	qp_attr->qp_type = IBV_QPT_RC;

	/* set number of send/recv requests and scatter/gather entries? */
	qp_attr->cap.max_send_wr = 10;
	qp_attr->cap.max_recv_wr = 10;
	qp_attr->cap.max_send_sge = 1;
	qp_attr->cap.max_recv_sge = 1;
}


void * poll_cq(void *ctx)
{
	struct ibv_cq *cq;
	struct ibv_wc wc;

	while (1) {
		ASSERT_Z(ibv_get_cq_event(s_ctx->comp_channel, &cq, &ctx));
		ibv_ack_cq_events(cq, 1);
		ASSERT_Z(ibv_req_notify_cq(cq, 0));

		while (ibv_poll_cq(cq, 1, &wc))
			on_completion(&wc);
	}

	return NULL;
}

#define CQ_SIZE 32
void build_endpoint(struct ibv_context *ctx, struct endpoint *ep)
{
	ep->ctx = ctx;

	/* alloc protection domain */
	ASSERT_NZ(ep->pd = ibv_alloc_pd(ep->ctx));
	/* completion channel for receive notification of cq arrival */
	ASSERT_NZ(ep->chan = ibv_create_comp_channel(ep->ctx));
	/* create completion for queueing completion notifications */
	ASSERT_NZ(ep->cq = ibv_create_cq(ep->ctx, CQ_SIZE, NULL, ep->chan, 0));

	/* request completion notifications on completion queue */
	ASSERT_Z(ibv_req_notify_cq(ep->cq, 0));

	ASSERT_Z(pthread_create(&ep->cq_poller_thread, NULL, poll_cq, NULL));
}

const int BUFFER_SIZE = 1024;
 
void register_memory(struct connection *conn)
{
	conn->send_region = malloc(BUFFER_SIZE);
	conn->recv_region = malloc(BUFFER_SIZE);

	ASSERT_NZ(conn->send_mr = ibv_reg_mr(
				s_ctx->pd, 
				conn->send_region, 
				BUFFER_SIZE, 
				IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE));

	ASSERT_NZ(conn->recv_mr = ibv_reg_mr(
				s_ctx->pd, 
				conn->recv_region, 
				BUFFER_SIZE, 
				IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE));
}


static void post_receives(struct connection *conn);

/* The peer request connection */
int on_connect_request(struct rdma_cm_id *req_id)
{
	struct ibv_qp_init_attr qp_attr;
	struct rdma_conn_param cm_params;
	struct connection *conn;
	struct endpoint ep;

	printf("received connection request.\n");

	/* create the endpoint environment */
	build_endpoint(req_req_id->verbs, &ep);

	/* prepare queue pair attributes */
	build_qp_attr(&ep, &qp_attr);
	ASSERT_NZ(ep.qp = ibv_create_qp(ep.pd, &qp_attr));

	/* ASSERT_Z(rdma_create_qp(req_id, s_ctx->pd, &qp_attr)); */
	ASSERT_NZ(req_id->qp);

	req_id->context = conn = (struct connection *)malloc(sizeof(struct connection));
	conn->qp = req_id->qp;

	register_memory(conn);
	post_receives(conn);

	memset(&cm_params, 0, sizeof(cm_params));
	ASSERT_Z(rdma_accept(req_id, &cm_params));

	return 0;
}

void post_receives(struct connection *conn)
{
	struct ibv_recv_wr wr, *bad_wr = NULL;
	struct ibv_sge sge;

	wr.wr_id = (uintptr_t)conn;
	wr.next = NULL;
	wr.sg_list = &sge;
	wr.num_sge = 1;

	sge.addr = (uintptr_t)conn->recv_region;
	sge.length = BUFFER_SIZE;
	sge.lkey = conn->recv_mr->lkey;

	ASSERT_Z(ibv_post_recv(conn->qp, &wr, &bad_wr));
}

static int on_connection(void *context)
{
	struct connection *conn = (struct connection *)context;
	struct ibv_send_wr wr, *bad_wr = NULL;
	struct ibv_sge sge;

	snprintf(conn->send_region, BUFFER_SIZE, "message from passive/server side with pid %d", getpid());

	printf("connected. posting send...\n");

	memset(&wr, 0, sizeof(wr));

	wr.opcode = IBV_WR_SEND;
	wr.sg_list = &sge;
	wr.num_sge = 1;
	wr.send_flags = IBV_SEND_SIGNALED;

	sge.addr = (uintptr_t)conn->send_region;
	sge.length = BUFFER_SIZE;
	sge.lkey = conn->send_mr->lkey;

	ASSERT_Z(ibv_post_send(conn->qp, &wr, &bad_wr));

	return 0;
}

static int on_disconnect(struct rdma_cm_id *id)
{
	struct connection *conn = (struct connection *)id->context;

	printf("peer disconnected.\n");

	rdma_destroy_qp(id);

	ibv_dereg_mr(conn->send_mr);
	ibv_dereg_mr(conn->recv_mr);

	free(conn->send_region);
	free(conn->recv_region);

	free(conn);

	rdma_destroy_id(id);

	return 0;
}

int on_event(struct rdma_cm_event *event)
{
	int r = 0;

	/* This is the communication identifier of requestor */
	printf("processing event id %p\n", event->id);

	if (event->event == RDMA_CM_EVENT_CONNECT_REQUEST)
		r = on_connect_request(event->id);
	else if (event->event == RDMA_CM_EVENT_ESTABLISHED)
		r = on_connection(event->id->context);
	else if (event->event == RDMA_CM_EVENT_DISCONNECTED)
		r = on_disconnect(event->id);
	else
		die("on_event: unknown event.");

	return r;
}

int main(int argc, char **argv)
{
	struct sockaddr_in addr;
	struct rdma_cm_event *event = NULL;
	struct rdma_cm_id *comm_id = NULL;
	struct rdma_event_channel *ec = NULL;
	uint16_t port = 0;

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;

	ASSERT_NZ(ec = rdma_create_event_channel());
	/* XXX rdma port space type, but why TCP? Available: IPOIB, TCP, UDP, IB
	 * Create a communication identifier */
	ASSERT_Z(rdma_create_id(ec, &comm_id, NULL, RDMA_PS_TCP));
	ASSERT_Z(rdma_bind_addr(comm_id, (struct sockaddr *)&addr));
	ASSERT_Z(rdma_listen(comm_id, 10)); /* backlog=10 is arbitrary */

	/* port is determined during `rdma_bind_addr` */
	port = ntohs(rdma_get_src_port(comm_id));

	printf("listening on id %p port %d.\n", comm_id, port);

	while (rdma_get_cm_event(ec, &event) == 0) {
		struct rdma_cm_event event_copy;

		/* get a copy of the event and free it */
		memcpy(&event_copy, event, sizeof(*event));
		rdma_ack_cm_event(event);

		if (on_event(&event_copy))
			break;
	}

	rdma_destroy_id(comm_id);
	rdma_destroy_event_channel(ec);

	return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <rdma/rdma_cma.h>

#define TEST_NZ(x) do { if ( (x)) die("error: " #x " failed (returned non-zero)." ); } while (0)
#define TEST_Z(x)  do { if (!(x)) die("error: " #x " failed (returned zero/null)."); } while (0)

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

struct context {
	struct ibv_context *ctx;
	struct ibv_pd *pd;
	struct ibv_cq *cq;
	struct ibv_comp_channel *comp_channel;

	pthread_t cq_poller_thread;
};

static void on_completion(struct ibv_wc *wc);

void build_qp_attr(struct ibv_qp_init_attr *qp_attr)
{
	memset(qp_attr, 0, sizeof(*qp_attr));

	qp_attr->send_cq = s_ctx->cq;
	qp_attr->recv_cq = s_ctx->cq;
	qp_attr->qp_type = IBV_QPT_RC;

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
		TEST_NZ(ibv_get_cq_event(s_ctx->comp_channel, &cq, &ctx));
		ibv_ack_cq_events(cq, 1);
		TEST_NZ(ibv_req_notify_cq(cq, 0));

		while (ibv_poll_cq(cq, 1, &wc))
			on_completion(&wc);
	}

	return NULL;
}

static struct context *s_ctx = NULL;

void build_context(struct ibv_context *verbs)
{
	if (s_ctx) {
		if (s_ctx->ctx != verbs)
			die("cannot handle events in more than one context.");

		return;
	}

	s_ctx = (struct context *)malloc(sizeof(struct context));

	s_ctx->ctx = verbs;

	TEST_Z(s_ctx->pd = ibv_alloc_pd(s_ctx->ctx));
	TEST_Z(s_ctx->comp_channel = ibv_create_comp_channel(s_ctx->ctx));
	TEST_Z(s_ctx->cq = ibv_create_cq(s_ctx->ctx, 10, NULL, s_ctx->comp_channel, 0));
	TEST_NZ(ibv_req_notify_cq(s_ctx->cq, 0));

	TEST_NZ(pthread_create(&s_ctx->cq_poller_thread, NULL, poll_cq, NULL));
}

const int BUFFER_SIZE = 1024;
 
void register_memory(struct connection *conn)
{
	conn->send_region = malloc(BUFFER_SIZE);
	conn->recv_region = malloc(BUFFER_SIZE);

	TEST_Z(conn->send_mr = ibv_reg_mr(
				s_ctx->pd, 
				conn->send_region, 
				BUFFER_SIZE, 
				IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE));

	TEST_Z(conn->recv_mr = ibv_reg_mr(
				s_ctx->pd, 
				conn->recv_region, 
				BUFFER_SIZE, 
				IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_WRITE));
}


static void post_receives(struct connection *conn);

int on_connect_request(struct rdma_cm_id *id)
{
	struct ibv_qp_init_attr qp_attr;
	struct rdma_conn_param cm_params;
	struct connection *conn;

	printf("received connection request.\n");

	build_context(id->verbs);
	build_qp_attr(&qp_attr);

	TEST_NZ(rdma_create_qp(id, s_ctx->pd, &qp_attr));

	id->context = conn = (struct connection *)malloc(sizeof(struct connection));
	conn->qp = id->qp;

	register_memory(conn);
	post_receives(conn);

	memset(&cm_params, 0, sizeof(cm_params));
	TEST_NZ(rdma_accept(id, &cm_params));

	return 0;
}
static void on_connection(void *context);
static void on_disconnect(struct rdma_cm_id *id);
static void on_event(struct rdma_cm_event *event);
 
int on_event(struct rdma_cm_event *event)
{
	int r = 0;

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
	struct rdma_cm_id *listener = NULL;
	struct rdma_event_channel *ec = NULL;
	uint16_t port = 0;

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;

	TEST_Z(ec = rdma_create_event_channel());
	TEST_NZ(rdma_create_id(ec, &listener, NULL, RDMA_PS_TCP));
	TEST_NZ(rdma_bind_addr(listener, (struct sockaddr *)&addr));
	TEST_NZ(rdma_listen(listener, 10)); /* backlog=10 is arbitrary */

	port = ntohs(rdma_get_src_port(listener));

	printf("listening on port %d.\n", port);

	while (rdma_get_cm_event(ec, &event) == 0) {
		struct rdma_cm_event event_copy;

		memcpy(&event_copy, event, sizeof(*event));
		rdma_ack_cm_event(event);

		if (on_event(&event_copy))
			break;
	}

	rdma_destroy_id(listener);
	rdma_destroy_event_channel(ec);

	return 0;
}

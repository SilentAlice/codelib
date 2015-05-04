#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include <infiniband/verbs.h>

#define MR_SIZE 4096
#define CQ_SIZE 128

int main()
{
	int i, ret;
	int num_devices;
	struct ibv_device **pdev = ibv_get_device_list(&num_devices);

	for (i = 0; i < num_devices; i++) {
		printf("dev_name: %s, dev_path: %s, ibdev_path: %s, name: %s\n",
				pdev[i]->dev_name, pdev[i]->dev_path,
				pdev[i]->ibdev_path, pdev[i]->name);
		printf("\tNmae: %s\n", ibv_get_device_name(pdev[i]));
		printf("\tGUID: 0x%"PRIx64"\n", ibv_get_device_guid(pdev[i]));
		printf("\tnode: %s\n", ibv_node_type_str(pdev[i]->node_type));
	}

	printf("======\n");
	printf("Create context from first device\n");

	struct ibv_context *ctx = ibv_open_device(pdev[0]);
	if (!ctx) {
		printf("ibv_open_device failed\n");
		exit(1);
	}
	printf("ctx: %p\n", ctx);
	printf("\tnum_comp_vectors: %d\n", ctx->num_comp_vectors);
	printf("\tasync_fd: %d\n", ctx->async_fd);

	struct ibv_device_attr device_attr;
	if ((ret = ibv_query_device(ctx, &device_attr)) < 0) {
		printf("ibv_query_device failed\n");
		exit(1);
	}
	printf("device attr:\n");
	printf("\tmax_qp: %d\n", device_attr.max_qp);
	printf("\tphys_port_cnt: %d\n", device_attr.phys_port_cnt);

	struct ibv_port_attr port_attr;
	if ((ret = ibv_query_port(ctx, 0, &port_attr)) < 0) {
		printf("ibv_query_port failed\n");
		exit(1);
	}
	printf("port[0] attr:\n");
	printf("\tactive_mtu: %u\n",   port_attr.active_mtu & 0x7);
	printf("\tactive_speed: %d\n", port_attr.active_speed);
	printf("\tactive_width: %d\n", port_attr.active_width);

	printf("======\n");
	printf("Create PD (Protecting Domain)\n");

	struct ibv_pd *pd = ibv_alloc_pd(ctx);
	if (!pd) {
		printf("ibv_alloc_pd failed\n");
		exit(1);
	}

	printf("======\n");
	printf("Reigster memory region\n");

	struct ibv_mr *mr;
	char *buf = malloc(MR_SIZE * sizeof(char));
	mr = ibv_reg_mr(pd, buf, MR_SIZE, IBV_ACCESS_LOCAL_WRITE |
			IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE);
	if (!mr) {
		printf("ibv_reg_mr failed\n");
		exit(1);
	}
	printf("memory region: 0x%p\n", mr->addr);

	printf("======\n");
	printf("Create completion event channel\n");

	struct ibv_comp_channel *chan = ibv_create_comp_channel(ctx);
	if (!chan) {
		printf("ibv_create_comp_channel failed\n");
		exit(1);
	}

	printf("======\n");
	printf("Create completion queues\n");

	struct ibv_cq *cq1 = ibv_create_cq(ctx,
			CQ_SIZE, /* queue size */
			NULL,    /* pointer to cq_context */
			chan,    /* the comp_channel bound to */
			0);      /* comp_vector */
	if (!cq1) {
		printf("ibv_create_cq failed\n");
		exit(1);
	}
	struct ibv_cq *cq2 = ibv_create_cq(ctx,
			CQ_SIZE, /* queue size */
			NULL,    /* pointer to cq_context */
			NULL,    /* the comp_channel bound to */
			0);      /* comp_vector */
	if (!cq2) {
		printf("ibv_create_cq failed\n");
		exit(1);
	}

	// delay close
	ibv_destroy_cq(cq2);
	ibv_destroy_cq(cq1);
	ibv_destroy_comp_channel(chan);
	ibv_dereg_mr(mr);
	ibv_dealloc_pd(pd);
	ibv_free_device_list(pdev);
	ibv_close_device(ctx);

	return 0;
}

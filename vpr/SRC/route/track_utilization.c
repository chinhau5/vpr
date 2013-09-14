#include <assert.h>
#include <string.h>
#include "vpr_types.h"
#include "globals.h"

void dump_raw_routing_structs()
{
	FILE *file;
	int i;
	int count;
	struct s_trace *tptr;

	file = fopen("/home/chinhau5/vbox_shared/routing_structs.dump", "wb");

	if (!file) {
		assert(0);
	}

	fwrite(&num_nets, sizeof(int), 1, file);

	for (i = 0; i < num_nets; i++) {
		//count number of rr_node used by the net
		tptr = trace_head[i];
		count = 0;
		while (tptr != NULL) {
			count++;
			tptr = tptr->next;
		}
		fwrite(&count, sizeof(int), 1, file);
		fflush(file);

		tptr = trace_head[i];
		while (tptr != NULL) {
			fwrite(tptr, sizeof(struct s_trace), 1, file);
			tptr = tptr->next;
			fflush(file);
		}
	}
	fwrite(clb_net, sizeof(struct s_net), num_nets, file);

	fwrite(&num_rr_nodes, sizeof(int), 1, file);

	for (i = 0; i < num_rr_nodes; i++) {
		fwrite(&rr_node[i], sizeof(struct s_rr_node), 1, file);
		fwrite(rr_node[i].edges, sizeof(int), rr_node[i].num_edges, file);
		fwrite(rr_node[i].switches, sizeof(int), rr_node[i].num_edges, file);
		fflush(file);
	}

	fclose(file);
}

void read_raw_routing_structs()
{
	FILE *file;
	int i, j;
	int count;
	struct s_trace *tptr;

	file = fopen("/home/chinhau5/vbox_shared/routing_structs.dump", "rb");

	if (!file) {
		assert(0);
	}

	fread(&num_nets, sizeof(int), 1, file);

	trace_head = calloc(num_nets, sizeof(struct s_trace *));

	for (i = 0; i < num_nets; i++) {
		fread(&count, sizeof(int), 1, file);

		for (j = 0; j < count; j++) {
			if (j == 0) {
				tptr = (struct s_trace *)malloc(sizeof(struct s_trace));
				trace_head[i] = tptr;
			}
			fread(tptr, sizeof(struct s_trace), 1, file);
			if (j < count-1) {
				tptr->next = (struct s_trace *)malloc(sizeof(struct s_trace));
			} else {
				tptr->next = NULL;
			}
			tptr = tptr->next;
		}
	}

	clb_net = calloc(num_nets, sizeof(struct s_net));
	fread(clb_net, sizeof(struct s_net), num_nets, file);

	fread(&num_rr_nodes, sizeof(int), 1, file);

	rr_node = (struct rr_node *)calloc(num_rr_nodes, sizeof(struct s_rr_node));

	for (i = 0; i < num_rr_nodes; i++) {
		fread(&rr_node[i], sizeof(struct s_rr_node), 1, file);

		rr_node[i].edges = malloc(sizeof(int)*rr_node[i].num_edges);
		fread(rr_node[i].edges, sizeof(int), rr_node[i].num_edges, file);

		rr_node[i].switches = malloc(sizeof(int)*rr_node[i].num_edges);
		fread(rr_node[i].switches, sizeof(int), rr_node[i].num_edges, file);
	}

	fclose(file);
}

enum { BRANCH_STRAIGHT = 0, BRANCH_LEFT, BRANCH_RIGHT };

void print_track_utilization() {
	int inet, inode, ipin, bnum, ilow, jlow, node_block_pin, iclass;
	int i, child_inode;
	t_rr_type rr_type;
	struct s_trace *tptr;
	int *total_branches, *used_branches;
	float average_utilization;
	int num_used_rr_nodes;
	FILE *file;

	used_branches = (int *)malloc(sizeof(float)*num_rr_nodes);
	total_branches = (int *)malloc(sizeof(int)*num_rr_nodes);
	memset(used_branches, 0, sizeof(float)*num_rr_nodes);
	memset(total_branches, 0, sizeof(float)*num_rr_nodes);
	num_used_rr_nodes = 0;
	average_utilization = 0;

	file = fopen("track_utilization.txt", "w");

	for (inet = 0; inet < num_nets; inet++) {
		if (clb_net[inet].is_global == FALSE) {
			if (clb_net[inet].num_sinks == FALSE) {

			} else {
				tptr = trace_head[inet];
				//fprintf(file, "Net: %d\n", inet);

				while (tptr != NULL) {
					inode = tptr->index;
					rr_type = rr_node[inode].type;
					ilow = rr_node[inode].xlow;
					jlow = rr_node[inode].ylow;


					switch (rr_type) {
					case CHANX:
					case CHANY:
						if (total_branches[inode] == 0) {
							for (i = 0; i < rr_node[inode].num_edges; i++) {
								child_inode = rr_node[inode].edges[i];
								if (rr_node[child_inode].type == CHANX || rr_node[child_inode].type == CHANY) {
									total_branches[inode]++;
								}
							}
						}
						if (tptr->next) {
							child_inode = tptr->next->index;
							if (rr_node[child_inode].type == CHANX || rr_node[child_inode].type == CHANY) {
								used_branches[inode]++;
							}
						}
						break;
					default:
						break;
					}

					tptr = tptr->next;
				}
			}
		} else { /* Global net.  Never routed. */
		}
	}

	average_utilization = 0;
	num_used_rr_nodes = 0;
	for (i = 0; i < num_rr_nodes; i++) {
		if (used_branches[i] > 0) {
			assert(used_branches[i] <= total_branches[i]);
			fprintf(file, "%d %d %.2f\n", used_branches[i], total_branches[i], (float)used_branches[i]/total_branches[i]);
			average_utilization += (float)used_branches[i]/total_branches[i];
			num_used_rr_nodes++;
		}
	}

	fprintf(file, "%.2f\n", average_utilization/num_used_rr_nodes);

	fclose(file);
	free(used_branches);
}

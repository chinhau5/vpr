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

	file = fopen("routing_structs.dump", "wb");

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

	file = fopen("routing_structs.dump", "rb");

	if (!file) {
		assert(0);
	}

	fread(&num_nets, sizeof(int), 1, file);

	trace_head = my_calloc(num_nets, sizeof(struct s_trace *));

	for (i = 0; i < num_nets; i++) {
		fread(&count, sizeof(int), 1, file);

		for (j = 0; j < count; j++) {
			if (j == 0) {
				tptr = (struct s_trace *)my_malloc(sizeof(struct s_trace));
				trace_head[i] = tptr;
			}
			fread(tptr, sizeof(struct s_trace), 1, file);
			if (j < count-1) {
				tptr->next = (struct s_trace *)my_malloc(sizeof(struct s_trace));
			} else {
				tptr->next = NULL;
			}
			tptr = tptr->next;
		}
	}

	clb_net = my_calloc(num_nets, sizeof(struct s_net));
	fread(clb_net, sizeof(struct s_net), num_nets, file);

	fread(&num_rr_nodes, sizeof(int), 1, file);

	rr_node = (struct rr_node *)my_calloc(num_rr_nodes, sizeof(struct s_rr_node));

	for (i = 0; i < num_rr_nodes; i++) {
		fread(&rr_node[i], sizeof(struct s_rr_node), 1, file);

		rr_node[i].edges = my_malloc(sizeof(int)*rr_node[i].num_edges);
		fread(rr_node[i].edges, sizeof(int), rr_node[i].num_edges, file);

		rr_node[i].switches = my_malloc(sizeof(int)*rr_node[i].num_edges);
		fread(rr_node[i].switches, sizeof(int), rr_node[i].num_edges, file);
	}

	fclose(file);
}

enum { BRANCH_STRAIGHT = 0, BRANCH_LEFT, BRANCH_RIGHT, BRANCH_ENUM_END };

void print_track_utilization() {
	int inet, inode, child_inode;
	int i, j, k;
	struct s_trace *tptr;
	int *total_branches, ***used_branches;
	float average_utilization;
	int num_used_rr_nodes;
	int *track_length;
	int length, index;
	FILE *file;
	int used;
	int branch[BRANCH_ENUM_END];
	int count[10];
	char *branch_name[BRANCH_ENUM_END] = { "STRAIGHT", "LEFT", "RIGHT" };

	used_branches = my_calloc(num_rr_nodes, sizeof(int *));
	total_branches = (int *)my_calloc(num_rr_nodes, sizeof(int));
	track_length = (int *)my_calloc(num_rr_nodes, sizeof(int));

	file = fopen("track_utilization.txt", "w");

	for (inet = 0; inet < num_nets; inet++) {
		if (clb_net[inet].is_global == FALSE) {
			if (clb_net[inet].num_sinks == FALSE) {

			} else {
				tptr = trace_head[inet];

				while (tptr != NULL) {
					inode = tptr->index;

					switch (rr_node[inode].type) {
					case CHANX:
					case CHANY:
						if (total_branches[inode] == 0) {
							for (i = 0; i < rr_node[inode].num_edges; i++) {
								child_inode = rr_node[inode].edges[i];
								if (rr_node[child_inode].type == CHANX || rr_node[child_inode].type == CHANY) {
									total_branches[inode]++;
								}
							}
							if (rr_node[inode].type == CHANX) {
								track_length[inode] = rr_node[inode].xhigh - rr_node[inode].xlow + 1;
							} else {
								assert(rr_node[inode].type == CHANY);
								track_length[inode] = rr_node[inode].yhigh - rr_node[inode].ylow + 1;
							}
							assert(track_length[inode] <= 4);

							used_branches[inode] = alloc_matrix(0, track_length[inode]-1, 0, BRANCH_ENUM_END-1, sizeof(int));
							for (i = 0; i < track_length[inode]; i++) {
								for (j = 0; j < BRANCH_ENUM_END; j++) {
									used_branches[inode][i][j] = 0;
								}
							}
						}
						//we know rr_node[inode] is either CHANX or CHANY
						if (tptr->next) {
							child_inode = tptr->next->index;
							if (rr_node[inode].type == rr_node[child_inode].type) {
								//rr_node[child_inode] is either CHANX or CHANY
								if (rr_node[inode].type == CHANX) {
									index = rr_node[inode].xhigh-rr_node[inode].xlow;

									used_branches[inode][index][BRANCH_STRAIGHT] = 1;
								} else {
									assert(rr_node[inode].type == CHANY);

									index = rr_node[inode].yhigh-rr_node[inode].ylow;

									used_branches[inode][index][BRANCH_STRAIGHT] = 1;
								}
							} else {
								if (rr_node[child_inode].type == CHANY) {
									assert(rr_node[inode].type == CHANX);

									length = rr_node[inode].xhigh - rr_node[inode].xlow + 1;

									if (rr_node[inode].direction == INC_DIRECTION) {
										index = rr_node[child_inode].xlow - rr_node[inode].xlow;

										assert(index >= 0 && index < length);

										if (rr_node[child_inode].direction == INC_DIRECTION) {
											used_branches[inode][index][BRANCH_LEFT] = 1;
										} else {
											assert(rr_node[child_inode].direction == DEC_DIRECTION);
											used_branches[inode][index][BRANCH_RIGHT] = 1;
										}
									} else {
										index = rr_node[inode].xhigh - rr_node[child_inode].xlow - 1;

										assert(index >= 0 && index < length);

										if (rr_node[child_inode].direction == INC_DIRECTION) {
											used_branches[inode][index][BRANCH_RIGHT] = 1;
										} else {
											assert(rr_node[child_inode].direction == DEC_DIRECTION);
											used_branches[inode][index][BRANCH_LEFT] = 1;
										}
									}
								} else if (rr_node[child_inode].type == CHANX) {
									assert(rr_node[inode].type == CHANY);

									length = rr_node[inode].yhigh - rr_node[inode].ylow + 1;

									if (rr_node[inode].direction == INC_DIRECTION) {
										index = rr_node[child_inode].ylow - rr_node[inode].ylow;

										assert(index >= 0 && index < length);

										if (rr_node[child_inode].direction == INC_DIRECTION) {
											used_branches[inode][index][BRANCH_RIGHT] = 1;
										} else {
											assert(rr_node[child_inode].direction == DEC_DIRECTION);
											used_branches[inode][index][BRANCH_LEFT] = 1;
										}
									} else {
										index = rr_node[inode].yhigh - rr_node[child_inode].ylow - 1;

										assert(index >= 0 && index < length);

										if (rr_node[child_inode].direction == INC_DIRECTION) {
											used_branches[inode][index][BRANCH_LEFT] = 1;
										} else {
											assert(rr_node[child_inode].direction == DEC_DIRECTION);
											used_branches[inode][index][BRANCH_RIGHT] = 1;
										}
									}
								}
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
		used = 0;
		for (j = 0; j < track_length[i]; j++) {
			for (k = 0; k < BRANCH_ENUM_END; k++) {
				if (used_branches[i][j][k] > 0) {
					assert(used_branches[i][j][k] == 1);
					used++;
				}
			}
		}
		if (used > 0) {
			num_used_rr_nodes++;
			assert(used <= total_branches[i]);
			average_utilization += (float)used/total_branches[i];
			fprintf(file, "%d %d %.2f\n", used, total_branches[i], (float)used/total_branches[i]);
		}
	}

	fprintf(file, "%.2f\n", average_utilization/num_used_rr_nodes);

	for (i = 0; i < 5; i++) {
		for (k = 0; k < BRANCH_ENUM_END; k++) {
			branch[k] = 0;
		}
		count[i] = 0;

		for (j = 0; j < num_rr_nodes; j++) {
			if (i < track_length[j]) {
				used = 0;
				for (k = 0; k < BRANCH_ENUM_END; k++) {
					if (used_branches[j][i][k] > 0) {
						assert(used_branches[j][i][k] == 1);
						branch[k]++;
						used++;
					}
				}
				if (used > 0) {
					count[i]++;
				}
			}
		}

		for (k = 0; k < BRANCH_ENUM_END; k++) {
			fprintf(file, "Percentage of nodes branching %s at [%d]: %.2f\n", branch_name[k], i, (float)branch[k]/count[i]);
		}

		fprintf(file, "\n");
	}

	fclose(file);

	free(used_branches);
	free(track_length);
}

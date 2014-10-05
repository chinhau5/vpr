#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "util.h"
#include "vpr_types.h"
#include "globals.h"
#include "print_netlist.h"
#include "read_xml_arch_file.h"

/******************** Subroutines local to this module ***********************/

static void print_pinnum(FILE * fp,
			 int pinnum);


/********************* Subroutine definitions ********************************/

void print_netlist2()
{
	FILE *fp;
	int i, j, k;
	int dblock, dport, dpin;

	//fp = fopen(foutput, "w", 0);

	for (i = 0; i < num_nets; i++) {
		for (j = 0; j <= clb_net[i].num_sinks; j++) {
			if (j == 0) {
				dblock = clb_net[i].node_block[j];
				//dport = clb_net[i].node_block_port[j];
				dpin = clb_net[i].node_block_pin[j];
				if (!strcmp(block[dblock].type->name, "clb")) {
					for (k = 0; k < block[dblock].pb->pb_graph_node->total_pb_pins; k++) {
						if (block[dblock].pb->rr_graph[k].net_num == clb_to_vpack_net_mapping[i] &&
							block[dblock].pb->rr_graph[k].pb_graph_pin->parent_node->pb_type->blif_model == NULL &&
							block[dblock].pb->rr_graph[k].pb_graph_pin->parent_node->parent_pb_graph_node == NULL &&
							block[dblock].pb->rr_graph[k].pb_graph_pin->port->type == OUT_PORT) {
							printf("%s %s.%s.%s\n", block[dblock].name, block[dblock].pb->pb_graph_node->pb_type->name,

									block[dblock].pb->rr_graph[k].pb_graph_pin->port->name);
						}
					}

				}
			}

		}
		printf("\n");
	}
}

void
print_netlist(char *foutput,
	      char *net_file)
{

/* Prints out the netlist related data structures into the file    *
 * fname.                                                          */

    int i, j, max_pin;
    int num_global_nets;
    int num_p_inputs, num_p_outputs;
    FILE *fp;
    int k;
    int dblock, dport, dpin;

    num_global_nets = 0;
    num_p_inputs = 0;
    num_p_outputs = 0;

    /* Count number of global nets */
    for(i = 0; i < num_nets; i++)
	{
	    if(clb_net[i].is_global)
		{
		    num_global_nets++;
		}
	}

    /* Count I/O input and output pads */
    for(i = 0; i < num_blocks; i++)
	{
	    if(block[i].type == IO_TYPE)
		{
		    for(j = 0; j < IO_TYPE->num_pins; j++)
			{
			    if(block[i].nets[j] != OPEN)
				{
				    if(IO_TYPE->
				       class_inf[IO_TYPE->pin_class[j]].
				       type == DRIVER)
					{
					    num_p_inputs++;
					}
				    else
					{
					    assert(IO_TYPE->
						   class_inf[IO_TYPE->
							     pin_class[j]].
						   type == RECEIVER);
					    num_p_outputs++;
					}
				}
			}
		}
	}


    fp = my_fopen(foutput, "w", 0);

    fprintf(fp, "Input netlist file: %s\n", net_file);
    fprintf(fp, "num_p_inputs: %d, num_p_outputs: %d, num_clbs: %d\n",
	    num_p_inputs, num_p_outputs, num_blocks);
    fprintf(fp, "num_blocks: %d, num_nets: %d, num_globals: %d\n",
	    num_blocks, num_nets, num_global_nets);
    fprintf(fp, "\nNet\tName\t\t#Pins\tDriver\t\tRecvs. (block, pin)\n");

    dport = 0;
    for(i = 0; i < num_nets; i++)
	{
	    fprintf(fp, "\n%d\t%s\t", i, clb_net[i].name);
	    if(strlen(clb_net[i].name) < 8)
		fprintf(fp, "\t");	/* Name field is 16 chars wide */
	    fprintf(fp, "%d ", clb_net[i].num_sinks + 1);
	    for(j = 0; j <= clb_net[i].num_sinks; j++) {
	    	if (j == 0) { /* driver */
	    		dblock = clb_net[i].node_block[j];
	    		dport = 0;
	    		for (k = 0; k < block[dblock].pb->pb_graph_node->total_pb_pins; k++) {
	    			if (block[dblock].pb->rr_graph[k].net_num != OPEN && vpack_to_clb_net_mapping[block[dblock].pb->rr_graph[k].net_num] == i &&
	    				block[dblock].pb->rr_graph[k].pb_graph_pin->port->type == OUT_PORT && block[dblock].pb->rr_graph[k].pb_graph_pin->parent_node->pb_type->num_modes == 0) {
//	    				if (block[dblock].pb->rr_graph[k].net_num > dport) {
//	    					dport = block[dblock].pb->rr_graph[k].net_num;
//	    				}
	    				dport++;
	    				if (!strcmp(block[dblock].pb->rr_graph[k].pb_graph_pin->parent_node->pb_type->blif_model, ".latch")) {
	    					fprintf(fp, "registered ");
	    				} else {
	    					assert(!strcmp(block[dblock].pb->rr_graph[k].pb_graph_pin->parent_node->pb_type->blif_model, ".names") || !strcmp(block[dblock].pb->rr_graph[k].pb_graph_pin->parent_node->pb_type->blif_model, ".input"));
	    					fprintf(fp, "combinational ");
	    				}
	    			}
	    		}
	    		assert(dport == 1);
	    		/*dblock = clb_net[i].node_block[j];
	    		dport = clb_net[i].node_block_port[j];
	    		dpin = clb_net[i].node_block_pin[j];
	    		if (block[dblock].pb->rr_graph[block[dblock].pb->pb_graph_node->input_pins[dport][dpin].pin_count_in_cluster].pb_graph_pin->type == PB_PIN_SEQUENTIAL) {
	    			fprintf(fp, "registered ");
	    		} else {
	    			fprintf(fp, "non-registered ");
	    		}*/
	    	}
	    	fprintf(fp, "\t(%4d,%4d)", clb_net[i].node_block[j],
	    				clb_net[i].node_block_pin[j]);
	    }

	}

    fprintf(fp, "\nBlock\tName\t\tType\tPin Connections\n\n");

    for(i = 0; i < num_blocks; i++)
	{
	    fprintf(fp, "\n%d\t%s\t", i, block[i].name);
	    if(strlen(block[i].name) < 8)
		fprintf(fp, "\t");	/* Name field is 16 chars wide */
	    fprintf(fp, "%s", block[i].type->name);

	    max_pin = block[i].type->num_pins;

	    for(j = 0; j < max_pin; j++)
		print_pinnum(fp, block[i].nets[j]);
	}

    fprintf(fp, "\n");

/* TODO: Print out pb info */

    fclose(fp);
}


static void
print_pinnum(FILE * fp,
	     int pinnum)
{

/* This routine prints out either OPEN or the pin number, to file fp. */

    if(pinnum == OPEN)
	fprintf(fp, "\tOPEN");
    else
	fprintf(fp, "\t%d", pinnum);
}

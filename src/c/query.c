#include <immintrin.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include "genotq.h"
#include "timer.h"
#include "quick_file.h"
#include "output_buffer.h"

int query_help();

void print_query_result(unsigned int *mask,
                        unsigned int mask_len,
                        struct gqt_query *q,
                        unsigned int **counts,
                        unsigned int *id_lens,
                        unsigned int num_qs,
                        unsigned int num_fields,
                        char *bim);

int query_cmp(uint32_t value,
              int op_condition,
              int condition_value)
{
    switch(op_condition) {
        case p_equal:
            return value == condition_value;
        case p_not_equal:
            return value != condition_value;
        case p_less_than:
            return value < condition_value;
        case p_greater_than:
            return value > condition_value;
        case p_less_than_equal:
            return value <= condition_value;
        case p_greater_than_equal:
            return value >= condition_value;
        default:
            return -1;
    }
}

int query(int argc, char **argv)
{
    if (argc < 2) return query_help();

    int c;
    char *wahbm_file_name=NULL,
         *id_query=NULL,
         *gt_query=NULL,
         *db_file_name=NULL,
         *bim_file_name=NULL;
    int i_is_set = 0,
        id_q_count = 0,
        gt_q_count = 0,
        d_is_set = 0,
        b_is_set = 0;

    char *id_query_list[100];
    char *gt_query_list[100];

    //{{{ parse cmd line opts
    while ((c = getopt (argc, argv, "hi:p:g:d:b:")) != -1) {
        switch (c) {
        case 'i':
            i_is_set = 1;
            wahbm_file_name = optarg;
            break;
        case 'p':
            id_query_list[id_q_count] = optarg;
            id_q_count += 1;
            break;
        case 'g':
            gt_query_list[gt_q_count] = optarg;
            gt_q_count += 1;
            break;
        case 'd':
            d_is_set = 1;
            db_file_name = optarg;
            break;
        case 'b':
            b_is_set = 1;
            bim_file_name = optarg;
            break;
        case 'h':
            return query_help();
        case '?':
            if ( (optopt == 'i') ||
                    (optopt == 'p') ||
                    (optopt == 'g') ||
                    (optopt == 'd') ||
                    (optopt == 'b') )
                fprintf (stderr, "Option -%c requires an argument.\n",
                         optopt);
            else if (isprint (optopt))
                fprintf (stderr, "Unknown option `-%c'.\n", optopt);
            else
                fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
        default:
            return query_help();
        }
    }

    if (i_is_set == 0) {
        printf("WAHBM file is not set\n");
        return query_help();
    }

    if (b_is_set == 0) {
        printf("BIM file is not set\n");
        return query_help();
    }

    if (d_is_set == 0) {
        printf("PED database file is not set\n");
        return query_help();
    }

    if (gt_q_count != id_q_count) {
        printf("Mismatched number of individual and genotype query strings\n");
        return query_help();
    }
    //}}}

    struct gqt_query q[100];
    unsigned int *gt_mask[100];
    unsigned int *counts[100];
    unsigned int id_lens[100];

    int r, i, j, k;

    for (i = 0; i < gt_q_count; ++i) {
        if (parse_q(gt_query_list[i], &(q[i]))) {
            fprintf(stderr, "in the %dth genotype query.\n", i+1);
            return 1;
        }
    }

    struct wah_file wf = init_wahbm_file(wahbm_file_name);
    unsigned int num_ints = (wf.num_fields + 32 - 1)/ 32;

    unsigned int len_ints;

    for (i = 0; i < gt_q_count; ++i) {
        unsigned int len_count_R;
        unsigned int *R;
        id_lens[i] = resolve_ind_query(&R,
                                      id_query_list[i],
                                      db_file_name);

        uint32_t low_v, high_v;

        if ( q[i].genotype_condition[0] == 1)
            low_v = 0;
        else if ( q[i].genotype_condition[1] == 1)
            low_v = 1;
        else if ( q[i].genotype_condition[2] == 1)
            low_v = 2;
        else if ( q[i].genotype_condition[3] == 1)
            low_v = 3;

        if ( q[i].genotype_condition[3] == 1)
            high_v = 4;
        else if ( q[i].genotype_condition[2] == 1)
            high_v = 3;
        else if ( q[i].genotype_condition[1] == 1)
            high_v = 2;
        else if ( q[i].genotype_condition[0] == 1)
            high_v = 1;


        if ( ( q[i].variant_op == p_count ) || ( q[i].variant_op == p_pct ) ) {
            len_count_R = sum_range_records_in_place_wahbm(wf,
                                                           R,
                                                           id_lens[i],
                                                           low_v,
                                                           high_v,
                                                           &(counts[i]));

            gt_mask[i] = (uint32_t *) malloc(num_ints * sizeof(uint32_t));

            if ( q[i].op_condition != -1) {
                // when a condition is set, we test each of the values to see
                // if they meet that condition.  those results are packed into
                // ints and c
                uint32_t v = 0, int_i = 0, bit_i = 0;
                for ( j = 0; j < len_count_R; ++j) {
                    if ( query_cmp(counts[i][j],
                                   q[i].op_condition,
                                   q[i].condition_value) ) {
                        v |= 1 << (31 - bit_i);
                    }

                    bit_i += 1;
                    if ( bit_i == 32 ) {
                        gt_mask[i][int_i] = v;
                        int_i += 1;
                        bit_i = 0;
                        v = 0;
                    }
                }
            
                if ( bit_i > 0)
                    gt_mask[i][int_i] = v;
            } else {
                // if no op is set then let everything pass
                for (j = 0; j < num_ints; ++j)
                    gt_mask[i][j] = -1; // set all the bits to 1
            }
        } else {
            unsigned int *gt_R;
            unsigned int len_wf_R = range_records_in_place_wahbm(wf,
                                                                 R,
                                                                 id_lens[i],
                                                                 low_v,
                                                                 high_v,
                                                                 &gt_R);
            len_ints = wah_to_ints(gt_R,len_wf_R,&(gt_mask[i]));
            free(gt_R);
        }

        free(R);


    }

    uint32_t *final_mask = (uint32_t *) calloc(num_ints,sizeof(uint32_t));

    // combine all of the masks to see what we need to print
    for (i = 0; i < num_ints; ++i) {
        final_mask[i] = ~0;
        for (j = 0; j < gt_q_count; ++j)
            final_mask[i] &= gt_mask[j][i];
    }

    for (j = 0; j < gt_q_count; ++j)
        free(gt_mask[j]);

    print_query_result(final_mask,
                       num_ints,
                       q,
                       counts,
                       id_lens,
                       gt_q_count,
                       wf.num_fields,
                       bim_file_name);

    fclose(wf.file);

    return 0;
}

void print_query_result(unsigned int *mask,
                        unsigned int mask_len,
                        struct gqt_query *q,
                        unsigned int **counts,
                        unsigned int *id_lens,
                        unsigned int num_qs,
                        unsigned int num_fields,
                        char *bim)
{

    fprintf(stderr, "num_qs:%u\n", num_qs);

    unsigned int i,j,k,line_idx,bytes, bit_i = 0;

    struct quick_file_info qfile;
    struct output_buffer outbuf;


    init_out_buf(&outbuf, NULL);
    quick_file_init(bim, &qfile);

    for (i=0; i < mask_len; ++i) {
        bytes = mask[i];
	if (bytes == 0)
            continue; /* skip a bunch of ops if you can */
        for (j=0; j < 32; j++) {
            if (bytes & 1 << (31 - j)) {
	        line_idx = i*32+j;
	        append_out_buf(&outbuf,
                               qfile.lines[line_idx],
                               qfile.line_lens[line_idx]);
                for (k=0; k < num_qs; k++) {

	            append_out_buf(&outbuf,"\t",1);
                        if ( q[k].variant_op == p_count ) 
                            append_integer_to_out_buf(&outbuf,
                                                      counts[k][line_idx]);
                        else
	                    append_out_buf(&outbuf,"-",1);
                }
                
	        append_out_buf(&outbuf,"\n",1);
            }
	    bit_i++;
	    if (bit_i == num_fields)
	        break;
        }

        if (bit_i == num_fields)
            break;
    }
    quick_file_delete(&qfile);
    free_out_buf(&outbuf);
}

//{{{int query_help()
int query_help()
{
    printf(
"usage:   gqt query -i <wahbm file name> \\\n"
"                   -b <bim file name> \\\n"                    
"                   -d <ped database file name> \\\n"
"                   -p <population query 1> \\\n"
"                   -g <genotype query 1> \\\n"
"                   -p <population query 2> \\\n"
"                   -g <genotype query 2> \\\n"
"\n"
"A GQT query returns a set of variants that meet some number of population \n"
"and genotype conditions.  Conditions are specified by a population query \n"
"and genotype query pair, where the population query defines the set of\n"
"individuals to consider and the genotype query defines a filter on that\n"
"population.  The result is the set of variants within that sub-population\n"
"that meet the given conditions.  For example, to find the variants that are\n"
"heterozygous in the GBR population the query pair would be:\n"
"\n"
"\t-p \"Populaiton = 'GBR'\" -g \"HET\"\n"
"\n"
"Any number of query pairs can be included, to further refine that set of\n"
"variants.  For example, to find the variants that are heterozygous in at \n"
"least 10 individuals from the GBR population, and are homozygous reference \n"
"in the TSI population the two query pairs would be:\n"
"\n"
"\t-p \"Populaiton = 'GBR'\" -g \"count(HET) >= 10\" \\\n"
"\t-p \"Populaiton = 'GBR'\" -g \"HOMO_REF\"\n"
"\n"
"Population queries are based on the PED file that is associated with the\n"
"genotypes, and any column in that PED file can be part of the query.  For\n"
"example, a PED file that includes the \"Paternal_ID\" and \"Gender\" fields\n"
"(where male = 1 and female = 2) could be queried by:\n"
"\n"
"\t-p \"Paternal_ID = 'NA12878' AND Gender = 2\"\n"
"\n"
"Genotype queries can either be direct genotype filters or count-based \n"
"filters.  To get the variants that are heterozygous in every member of the\n"
"population the query would be:\n"
"\n"
"\t-g \"HET\"\n"
"\n"
"Or to get the variants that are either heterozygous or homozygous alternate\n"
"in every member the query would be:\n"
"\n"
"\t-g \"HET HOMO_ALT\"\n"
"\n"
"Count based filters used the \"count()\" operator that takes a genotype \n"
"list as a parameter followed by some condition.  For example, to find the\n"
"variants that are either heterozygous or homozygous alternate in no more\n"
"than 10 individuals the query would be\n"
"\n"
"\t-g \"count(HET HOMO_ALT) < 10\"\n");
    return 0;
}
//}}}
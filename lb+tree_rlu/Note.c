ps aux | grep run_basic.sh


0x55cd5aa64b08
0x55cd5aa5a898






if (p_inner->level == 1) {
            printf("LOST INNER IN THE INNER SPLIT UPDATE (CHILD HEADER) parent : %d master : %d  slave : %d\n",
                p_inner->min_key, p_data->p_smo->p_master_header->item.key, p_data->p_smo->p_slave_header->item.key);
        } else {
            printf("LOST INNER IN THE INNER SPLIT UPDATE (CHILD INNER) parent : %d master : %d  slave : %d\n", 
                p_inner->min_key, p_data->p_smo->p_master_inner->min_key, p_data->p_smo->p_slave_inner->min_key);
        }
        usleep(1);
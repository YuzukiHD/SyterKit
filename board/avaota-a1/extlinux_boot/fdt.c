/* SPDX-License-Identifier: Apache-2.0 */

#ifdef SYTERKIT_CLI_CMD_FDT
msh_declare_command(fdt);
msh_define_help(fdt, "flattened device tree utility commands",
                "fdt print  <path> [<prop>]          - Recursive print starting at <path>\n"
                "fdt list   <path> [<prop>]          - Print one level starting at <path>\n"
                "fdt set    <path> <prop> [<val>]    - Set <property> [to <val>]\n"
                "fdt mknode <path> <node>            - Create a new node after <path>\n"
                "fdt rm     <path> [<prop>]          - Delete the node or <property>\n"
                "fdt header                          - Display header info\n"
                "fdt rsvmem print                    - Show current mem reserves\n"
                "fdt rsvmem add <addr> <size>        - Add a mem reserve\n"
                "fdt rsvmem delete <index>           - Delete a mem reserves\n"
                "NOTE: Dereference aliases by omitting the leading '/', "
                "e.g. fdt print ethernet0.\n\n");
int cmd_fdt(int argc, const char **argv) {
    if (argc < 2) {
        uart_puts(cmd_fdt_usage);
        return 0;
    }
    if (strncmp(argv[1], "mk", 2) == 0) {
        char *pathp;    /* path */
        char *nodep;    /* new node to add */
        int nodeoffset; /* node offset from libfdt */
        int err;

        /*
		 * Parameters: Node path, new node to be appended to the path.
		 */
        if (argc < 4) {
            uart_puts(cmd_fdt_usage);
            return 0;
        }

        pathp = argv[2];
        nodep = argv[3];

        nodeoffset = fdt_path_offset(image.of_dest, pathp);
        if (nodeoffset < 0) {
            /*
			 * Not found or something else bad happened.
			 */
            printk(LOG_LEVEL_MUTE, "libfdt fdt_path_offset() returned %s\n", fdt_strerror(nodeoffset));
            return 1;
        }
        err = fdt_add_subnode(image.of_dest, nodeoffset, nodep);
        if (err < 0) {
            printk(LOG_LEVEL_MUTE, "libfdt fdt_add_subnode(): %s\n", fdt_strerror(err));
            return 1;
        }
    } else if (strncmp(argv[1], "set", 3) == 0) {
        char *pathp;                               /* path */
        char *prop;                                /* property */
        int nodeoffset;                            /* node offset from libfdt */
        static char data[SCRATCHPAD] __aligned(4); /* property storage */
        const void *ptmp;
        int len; /* new length of the property */
        int ret; /* return value */

        /*
		 * Parameters: Node path, property, optional value.
		 */
        if (argc < 4) {
            uart_puts(cmd_fdt_usage);
            return 0;
        }

        pathp = argv[2];
        prop = argv[3];

        nodeoffset = fdt_path_offset(image.of_dest, pathp);
        if (nodeoffset < 0) {
            /*
			 * Not found or something else bad happened.
			 */
            printk(LOG_LEVEL_MUTE, "libfdt fdt_path_offset() returned %s\n", fdt_strerror(nodeoffset));
            return 1;
        }

        if (argc == 4) {
            len = 0;
        } else {
            ptmp = fdt_getprop(image.of_dest, nodeoffset, prop, &len);
            if (len > SCRATCHPAD) {
                printk(LOG_LEVEL_MUTE, "prop (%d) doesn't fit in scratchpad!\n", len);
                return 1;
            }
            if (ptmp != NULL)
                memcpy(data, ptmp, len);

            ret = fdt_parse_prop(&argv[4], argc - 4, data, &len);
            if (ret != 0)
                return ret;
        }

        ret = fdt_setprop(image.of_dest, nodeoffset, prop, data, len);
        if (ret < 0) {
            printk(LOG_LEVEL_MUTE, "libfdt fdt_setprop(): %s\n", fdt_strerror(ret));
            return 1;
        }
    } else if ((argv[1][0] == 'p') || (argv[1][0] == 'l')) {
        int depth = MAX_LEVEL; /* how deep to print */
        char *pathp;           /* path */
        char *prop;            /* property */
        int ret;               /* return value */
        static char root[2] = "/";

        /*
		 * list is an alias for print, but limited to 1 level
		 */
        if (argv[1][0] == 'l') {
            depth = 1;
        }

        /*
		 * Get the starting path.  The root node is an oddball,
		 * the offset is zero and has no name.
		 */
        if (argc == 2)
            pathp = root;
        else
            pathp = argv[2];
        if (argc > 3)
            prop = argv[3];
        else
            prop = NULL;

        fdt_print(image.of_dest, pathp, prop, depth);
    } else if (strncmp(argv[1], "rm", 2) == 0) {
        int nodeoffset; /* node offset from libfdt */
        int err;

        /*
		 * Get the path.  The root node is an oddball, the offset
		 * is zero and has no name.
		 */
        nodeoffset = fdt_path_offset(image.of_dest, argv[2]);
        if (nodeoffset < 0) {
            /*
			 * Not found or something else bad happened.
			 */
            printk(LOG_LEVEL_MUTE, "libfdt fdt_path_offset() returned %s\n", fdt_strerror(nodeoffset));
            return 1;
        }
        /*
		 * Do the delete.  A fourth parameter means delete a property,
		 * otherwise delete the node.
		 */
        if (argc > 3) {
            err = fdt_delprop(image.of_dest, nodeoffset, argv[3]);
            if (err < 0) {
                printk(LOG_LEVEL_MUTE, "libfdt fdt_delprop():  %s\n", fdt_strerror(err));
                return 0;
            }
        } else {
            err = fdt_del_node(image.of_dest, nodeoffset);
            if (err < 0) {
                printk(LOG_LEVEL_MUTE, "libfdt fdt_del_node():  %s\n", fdt_strerror(err));
                return 0;
            }
        }
    } else if (argv[1][0] == 'h') {
        u32 version = fdt_version(image.of_dest);
        printk(LOG_LEVEL_MUTE, "magic:\t\t\t0x%x\n", fdt_magic(image.of_dest));
        printk(LOG_LEVEL_MUTE, "totalsize:\t\t0x%x (%d)\n", fdt_totalsize(image.of_dest),
               fdt_totalsize(image.of_dest));
        printk(LOG_LEVEL_MUTE, "off_dt_struct:\t\t0x%x\n",
               fdt_off_dt_struct(image.of_dest));
        printk(LOG_LEVEL_MUTE, "off_dt_strings:\t\t0x%x\n",
               fdt_off_dt_strings(image.of_dest));
        printk(LOG_LEVEL_MUTE, "off_mem_rsvmap:\t\t0x%x\n",
               fdt_off_mem_rsvmap(image.of_dest));
        printk(LOG_LEVEL_MUTE, "version:\t\t%d\n", version);
        printk(LOG_LEVEL_MUTE, "last_comp_version:\t%d\n",
               fdt_last_comp_version(image.of_dest));
        if (version >= 2)
            printk(LOG_LEVEL_MUTE, "boot_cpuid_phys:\t0x%x\n",
                   fdt_boot_cpuid_phys(image.of_dest));
        if (version >= 3)
            printk(LOG_LEVEL_MUTE, "size_dt_strings:\t0x%x\n",
                   fdt_size_dt_strings(image.of_dest));
        if (version >= 17)
            printk(LOG_LEVEL_MUTE, "size_dt_struct:\t\t0x%x\n",
                   fdt_size_dt_struct(image.of_dest));
        printk(LOG_LEVEL_MUTE, "number mem_rsv:\t\t0x%x\n",
               fdt_num_mem_rsv(image.of_dest));
        printk(LOG_LEVEL_MUTE, "\n");
    } else if (strncmp(argv[1], "rs", 2) == 0) {
        if (argv[2][0] == 'p') {
            uint64_t addr, size;
            int total = fdt_num_mem_rsv(image.of_dest);
            int j, err;
            printk(LOG_LEVEL_MUTE, "index\t\t   start\t\t    size\n");
            printk(LOG_LEVEL_MUTE, "-------------------------------"
                                   "-----------------\n");
            for (j = 0; j < total; j++) {
                err = fdt_get_mem_rsv(image.of_dest, j, &addr, &size);
                if (err < 0) {
                    printk(LOG_LEVEL_MUTE, "libfdt fdt_get_mem_rsv():  %s\n", fdt_strerror(err));
                    return 0;
                }
                printk(LOG_LEVEL_MUTE, "    %x\t%08x%08x\t%08x%08x\n", j,
                       (u32) (addr >> 32),
                       (u32) (addr & 0xffffffff),
                       (u32) (size >> 32),
                       (u32) (size & 0xffffffff));
            }
        } else if (argv[2][0] == 'a') {
            uint64_t addr, size;
            int err;
            addr = simple_strtoull(argv[3], NULL, 16);
            size = simple_strtoull(argv[4], NULL, 16);
            err = fdt_add_mem_rsv(image.of_dest, addr, size);

            if (err < 0) {
                printk(LOG_LEVEL_MUTE, "libfdt fdt_add_mem_rsv():  %s\n", fdt_strerror(err));
                return 0;
            }
        } else if (argv[2][0] == 'd') {
            unsigned long idx = simple_strtoul(argv[3], NULL, 16);
            int err = fdt_del_mem_rsv(image.of_dest, idx);

            if (err < 0) {
                printk(LOG_LEVEL_MUTE, "libfdt fdt_del_mem_rsv():  %s\n", fdt_strerror(err));
                return 0;
            }
        } else {
            uart_puts(cmd_fdt_usage);
            return 0;
        }
    } else {
        uart_puts(cmd_fdt_usage);
        return 0;
    }
    return 0;
}
#endif // SYTERKIT_CLI_CMD_FDT
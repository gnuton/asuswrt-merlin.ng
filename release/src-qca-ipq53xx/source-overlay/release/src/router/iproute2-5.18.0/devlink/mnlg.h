/*
 *   mnlg.h	Generic Netlink helpers for libmnl
 *
 *              This program is free software; you can redistribute it and/or
 *              modify it under the terms of the GNU General Public License
 *              as published by the Free Software Foundation; either version
 *              2 of the License, or (at your option) any later version.
 *
 * Authors:     Jiri Pirko <jiri@mellanox.com>
 */

#ifndef _MNLG_H_
#define _MNLG_H_

#include <libmnl/libmnl.h>

struct mnlu_gen_socket;

int mnlg_socket_send(struct mnlu_gen_socket *nlg, const struct nlmsghdr *nlh);
int mnlg_socket_group_add(struct mnlu_gen_socket *nlg, const char *group_name);
int mnlg_socket_get_fd(struct mnlu_gen_socket *nlg);

#endif /* _MNLG_H_ */

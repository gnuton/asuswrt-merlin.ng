/* Inspect the packaged ARM ip parser's real request before QEMU netlink I/O. */
#include <errno.h>
#include <stdio.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/fib_rules.h>

ssize_t sendmsg(int fd, const struct msghdr *message, int flags)
{
	const struct nlmsghdr *header;
	const struct fib_rule_hdr *rule;
	struct rtattr *attribute;
	int length, found = 0;
	(void)fd; (void)flags;
	if (message->msg_iovlen != 1 || message->msg_iov[0].iov_len < NLMSG_LENGTH(sizeof(*rule)))
		goto reject;
	header = message->msg_iov[0].iov_base;
	if (header->nlmsg_type != RTM_NEWRULE && header->nlmsg_type != RTM_DELRULE)
		goto reject;
	rule = NLMSG_DATA(header);
	length = header->nlmsg_len - NLMSG_LENGTH(sizeof(*rule));
	attribute = (struct rtattr *)((char *)rule + NLMSG_ALIGN(sizeof(*rule)));
	for (; RTA_OK(attribute, length); attribute = RTA_NEXT(attribute, length)) {
		if (attribute->rta_type == FRA_PROTOCOL && RTA_PAYLOAD(attribute) == 1) {
			printf("PACKAGED-FRA-PROTOCOL %u %u\n", header->nlmsg_type,
			       *(const unsigned char *)RTA_DATA(attribute));
			found = 1;
		}
	}
	if (found && !length) fflush(stdout);
reject:
	/* This probe validates serialization only. Packet tests separately use
	 * the real host kernel, without substituting any routing operation. */
	errno = ECANCELED;
	return -1;
}

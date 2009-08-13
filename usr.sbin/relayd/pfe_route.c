/*	$OpenBSD: pfe_route.c,v 1.1 2009/08/13 13:51:21 reyk Exp $	*/

/*
 * Copyright (c) 2009 Reyk Floeter <reyk@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>
#include <sys/queue.h>
#include <sys/socket.h>

#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/route.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <event.h>
#include <string.h>
#include <errno.h>

#include <openssl/ssl.h>

#include "relayd.h"

extern struct imsgev	*iev_main;

struct relay_rtmsg {
	struct rt_msghdr	rm_hdr;
	union {
		struct {
			struct sockaddr_in	rm_dst;
			struct sockaddr_in	rm_gateway;
			struct sockaddr_in	rm_netmask;
			struct sockaddr_rtlabel	rm_label;
		}		 u4;
		struct {
			struct sockaddr_in6	rm_dst;
			struct sockaddr_in6	rm_gateway;
			struct sockaddr_in6	rm_netmask;
			struct sockaddr_rtlabel	rm_label;
		}		 u6;
	}			 rm_u;
};

void
init_routes(struct relayd *env)
{
	u_int	 rtfilter;

	if (!(env->sc_flags & F_NEEDRT))
		return;

	if ((env->sc_rtsock = socket(AF_ROUTE, SOCK_RAW, 0)) == -1)
		fatal("init_routes: failed to open routing socket");

	rtfilter = ROUTE_FILTER(0);
	if (setsockopt(env->sc_rtsock, AF_ROUTE, ROUTE_MSGFILTER,
	    &rtfilter, sizeof(rtfilter)) == -1)
		fatal("init_routes: ROUTE_MSGFILTER");
}

void
sync_routes(struct relayd *env, struct router *rt)
{
	struct netroute		*nr;
	struct host		*host;
	char			 buf[MAXHOSTNAMELEN];
	struct ctl_netroute	 crt;

	if (!(env->sc_flags & F_NEEDRT))
		return;

	TAILQ_FOREACH(nr, &rt->rt_netroutes, nr_entry) {
		print_host(&nr->nr_conf.ss, buf, sizeof(buf));
		TAILQ_FOREACH(host, &rt->rt_gwtable->hosts, entry) {
			if (host->up == HOST_UNKNOWN)
				continue;

			log_debug("sync_routes: "
			    "router %s route %s/%d gateway %s %s",
			    rt->rt_conf.name, buf, nr->nr_conf.prefixlen,
			    host->conf.name,
			    HOST_ISUP(host->up) ? "up" : "down");

			crt.id = nr->nr_conf.id;
			crt.hostid = host->conf.id;
			crt.up = host->up;

			imsg_compose_event(iev_main, IMSG_RTMSG,
			    0, 0, -1, &crt, sizeof(crt));
		}
	}
}

int
pfe_route(struct relayd *env, struct ctl_netroute *crt)
{
	struct relay_rtmsg		 rm;
	struct sockaddr_rtlabel		 sr;
	struct sockaddr_storage		*gw;
	struct sockaddr_in		*s4;
	struct sockaddr_in6		*s6;
	size_t				 len = 0;
	struct netroute			 *nr;
	struct host			*host;
	char				*gwname;
	int				 i = 0;

	if ((nr = route_find(env, crt->id)) == NULL ||
	    (host = host_find(env, crt->hostid)) == NULL) {
		log_debug("pfe_route: invalid host or route id");
		return (-1);
	}

	gw = &host->conf.ss;
	gwname = host->conf.name;

	bzero(&rm, sizeof(rm));
	bzero(&sr, sizeof(sr));

	rm.rm_hdr.rtm_msglen = len;
	rm.rm_hdr.rtm_version = RTM_VERSION;
	rm.rm_hdr.rtm_type = HOST_ISUP(crt->up) ? RTM_ADD : RTM_DELETE;
	rm.rm_hdr.rtm_flags = RTF_STATIC | RTF_GATEWAY | RTF_MPATH;
	rm.rm_hdr.rtm_seq = env->sc_rtseq++;
	rm.rm_hdr.rtm_addrs = RTA_DST | RTA_GATEWAY;
	rm.rm_hdr.rtm_tableid = nr->nr_router->rt_conf.rtable;

	if (strlen(nr->nr_router->rt_conf.label)) {
		rm.rm_hdr.rtm_addrs |= RTA_LABEL;
		sr.sr_len = sizeof(sr);
		if (snprintf(sr.sr_label, sizeof(sr.sr_label),
		    "%s", nr->nr_router->rt_conf.label) == -1)
			goto bad;
	}

	if (nr->nr_conf.ss.ss_family == AF_INET) {
		rm.rm_hdr.rtm_msglen = len =
		    sizeof(rm.rm_hdr) + sizeof(rm.rm_u.u4);

		bcopy(&sr, &rm.rm_u.u4.rm_label, sizeof(sr));

		s4 = &rm.rm_u.u4.rm_dst;
		s4->sin_family = AF_INET;
		s4->sin_len = sizeof(rm.rm_u.u4.rm_dst);
		s4->sin_addr.s_addr =
		    ((struct sockaddr_in *)&nr->nr_conf.ss)->sin_addr.s_addr;

		s4 = &rm.rm_u.u4.rm_gateway;
		s4->sin_family = AF_INET;
		s4->sin_len = sizeof(rm.rm_u.u4.rm_gateway);
		s4->sin_addr.s_addr =
		    ((struct sockaddr_in *)gw)->sin_addr.s_addr;

		rm.rm_hdr.rtm_addrs |= RTA_NETMASK;
		s4 = &rm.rm_u.u4.rm_netmask;
		s4->sin_family = AF_INET;
		s4->sin_len = sizeof(rm.rm_u.u4.rm_netmask);
		if (nr->nr_conf.prefixlen)
			s4->sin_addr.s_addr =
			    htonl(0xffffffff << (32 - nr->nr_conf.prefixlen));
		else if (nr->nr_conf.prefixlen < 0)
			rm.rm_hdr.rtm_flags |= RTF_HOST;
	} else if (nr->nr_conf.ss.ss_family == AF_INET6) {
		rm.rm_hdr.rtm_msglen = len =
		    sizeof(rm.rm_hdr) + sizeof(rm.rm_u.u6);

		bcopy(&sr, &rm.rm_u.u6.rm_label, sizeof(sr));

		s6 = &rm.rm_u.u6.rm_dst;
		bcopy(((struct sockaddr_in6 *)&nr->nr_conf.ss),
		    s6, sizeof(*s6));
		s6->sin6_family = AF_INET6;
		s6->sin6_len = sizeof(*s6);

		s6 = &rm.rm_u.u6.rm_gateway;
		bcopy(((struct sockaddr_in6 *)gw), s6, sizeof(*s6));
		s6->sin6_family = AF_INET6;
		s6->sin6_len = sizeof(*s6);

		rm.rm_hdr.rtm_addrs |= RTA_NETMASK;
		s6 = &rm.rm_u.u6.rm_netmask;
		s6->sin6_family = AF_INET6;
		s6->sin6_len = sizeof(*s6);
		if (nr->nr_conf.prefixlen) {
			for (i = 0; i < nr->nr_conf.prefixlen / 8; i++)
				s6->sin6_addr.s6_addr[i] = 0xff;
			i = nr->nr_conf.prefixlen % 8;
			if (i)
				s6->sin6_addr.s6_addr[nr->nr_conf.prefixlen
				    / 8] = 0xff00 >> i;
		} else if (nr->nr_conf.prefixlen < 0)
			rm.rm_hdr.rtm_flags |= RTF_HOST;
	} else
		fatal("pfe_route: invalid address family");

 retry:
	if (write(env->sc_rtsock, &rm, len) == -1) {
		switch (errno) {
		case EEXIST:
		case ESRCH:
			if (rm.rm_hdr.rtm_type == RTM_ADD) {
				rm.rm_hdr.rtm_type = RTM_CHANGE;
				goto retry;
			} else if (rm.rm_hdr.rtm_type == RTM_DELETE) {
				/* Ignore */
				break;
			}
			/* FALLTHROUGH */
		default:
			goto bad;
		}
	}

	log_debug("pfe_route: gateway %s %s", gwname,
	    HOST_ISUP(crt->up) ? "added" : "deleted");

	return (0);

 bad:
	log_debug("pfe_route: failed to %s gateway %s: %d %s",
	    HOST_ISUP(crt->up) ? "add" : "delete", gwname,
	    errno, strerror(errno));

	return (-1);
}

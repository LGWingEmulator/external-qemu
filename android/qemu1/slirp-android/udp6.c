/*
 * Copyright (c) 2013
 * Guillaume Subiron
 */

#include "qemu/osdep.h"
#include "qemu-common.h"
#include "slirp.h"
#include "udp.h"
#include "dhcpv6.h"

void udp6_input(struct mbuf *m)
{
    struct ip6 *ip, save_ip;
    struct udphdr *uh;
    int iphlen = sizeof(struct ip6);
    int len;
    struct socket *so;
    struct sockaddr_in6 lhost;

    DEBUG_CALL("udp6_input");
    DEBUG_ARG("m = %lx", (long)m);

    if (slirp_restrict) {
        goto bad;
    }

    ip = mtod(m, struct ip6 *);
    m->m_len -= iphlen;
    m->m_data += iphlen;
    uh = mtod(m, struct udphdr *);
    m->m_len += iphlen;
    m->m_data -= iphlen;

    if (ip6_cksum(m)) {
        goto bad;
    }

    len = ntohs((uint16_t)uh->uh_ulen);

    /*
     * Make mbuf data length reflect UDP length.
     * If not enough data to reflect UDP length, drop.
     */
    if (ntohs(ip->ip_pl) != len) {
        if (len > ntohs(ip->ip_pl)) {
            goto bad;
        }
        m_adj(m, len - ntohs(ip->ip_pl));
        ip->ip_pl = htons(len);
    }

    /*
     * Save a copy of the IP header in case we want restore it
     * for sending an ICMP error message in response.
     */
    save_ip = *ip;

    /* Locate pcb for datagram. */
    lhost.sin6_family = AF_INET6;
    lhost.sin6_addr = ip->ip_src;
    lhost.sin6_port = port_getn(uh->uh_sport);

    /* handle DHCPv6 */
    if (port_geth(uh->uh_dport) == DHCPV6_SERVER_PORT &&
        (in6_equal(&ip->ip_dst, &vhost_addr6) ||
         in6_equal(&ip->ip_dst, &(struct in6_addr)ALLDHCP_MULTICAST))) {
        m->m_data += iphlen;
        m->m_len -= iphlen;
        dhcpv6_input(&lhost, m);
        m->m_data -= iphlen;
        m->m_len += iphlen;
        goto bad;
    }

    /* handle TFTP */
    if (port_geth(uh->uh_dport) == TFTP_SERVER &&
        !memcmp(ip->ip_dst.s6_addr, vhost_addr6.s6_addr, 16)) {
        m->m_data += iphlen;
        m->m_len -= iphlen;
        tftp_input(/* FIXME (struct sockaddr_storage *)&lhost, */m);
        m->m_data -= iphlen;
        m->m_len += iphlen;
        goto bad;
    }

    so = solookup(&udb, 0, 0, 0, 0
                  /*(struct sockaddr_storage *) &lhost, NULL*/);
    /*FIXME*/
    goto bad;

    if (so == NULL) {
        /* If there's no socket for this packet, create one. */
        so = socreate();
        if (!so) {
            goto bad;
        }
        if (udp_attach(so/*FIXME, AF_INET6*/) == -1) {
            DEBUG_MISC((dfd, " udp6_attach errno = %d-%s\n",
                        errno, strerror(errno)));
            sofree(so);
            goto bad;
        }

        /* Setup fields */
        #if 0
        FIXME
        so->so_lfamily = AF_INET6;
        so->so_laddr6 = ip->ip_src;
        so->so_lport6 = uh->uh_sport;
        #endif
    }
    #if 0
    FIXME
    so->so_ffamily = AF_INET6;
    so->so_faddr6 = ip->ip_dst; /* XXX */
    so->so_fport6 = uh->uh_dport; /* XXX */
    #endif

    iphlen += sizeof(struct udphdr);
    m->m_len -= iphlen;
    m->m_data += iphlen;

    /*
     * Now we sendto() the packet.
     */
    if (sosendto(so, m) == -1) {
        m->m_len += iphlen;
        m->m_data -= iphlen;
        *ip = save_ip;
        DEBUG_MISC((dfd, "udp tx errno = %d-%s\n", errno, strerror(errno)));
        icmp6_send_error(m, ICMP6_UNREACH, ICMP6_UNREACH_NO_ROUTE);
        goto bad;
    }

    m_free(so->so_m);   /* used for ICMP if error on sorecvfrom */

    /* restore the orig mbuf packet */
    m->m_len += iphlen;
    m->m_data -= iphlen;
    *ip = save_ip;
    so->so_m = m;

    return;
bad:
    m_free(m);
}

int udp6_output(struct socket *so, struct mbuf *m,
        struct sockaddr_in6 *saddr, struct sockaddr_in6 *daddr)
{
    struct ip6 *ip;
    struct udphdr *uh;

    DEBUG_CALL("udp6_output");
    DEBUG_ARG("so = %lx", (long)so);
    DEBUG_ARG("m = %lx", (long)m);

    /* adjust for header */
    m->m_data -= sizeof(struct udphdr);
    m->m_len += sizeof(struct udphdr);
    uh = mtod(m, struct udphdr *);
    m->m_data -= sizeof(struct ip6);
    m->m_len += sizeof(struct ip6);
    ip = mtod(m, struct ip6 *);

    /* Build IP header */
    ip->ip_pl = htons(m->m_len - sizeof(struct ip6));
    ip->ip_nh = IPPROTO_UDP;
    ip->ip_src = saddr->sin6_addr;
    ip->ip_dst = daddr->sin6_addr;

    /* Build UDP header */
    uh->uh_sport = port_setn(saddr->sin6_port);
    uh->uh_dport = port_setn(daddr->sin6_port);
    uh->uh_ulen = ip->ip_pl;
    uh->uh_sum = 0;
    uh->uh_sum = ip6_cksum(m);
    if (uh->uh_sum == 0) {
        uh->uh_sum = 0xffff;
    }

    return ip6_output(so, m, 0);
}

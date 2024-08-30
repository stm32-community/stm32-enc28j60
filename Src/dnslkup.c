/**
 * @file dnslkup.c
 * @brief Source file implementing DNS lookup functionality for resolving hostnames to IP addresses.
 *
 * This file contains the implementation of DNS client functionality, including sending DNS queries,
 * processing DNS responses, and handling DNS errors. It provides functions for setting the DNS server,
 * sending DNS requests, and resolving hostnames to IP addresses.
 *
 * @note For more information, refer to the `license.md` file located at the root of the project.
 */

#include "defines.h"
#include "net.h"
#include "ip_arp_udp_tcp.h"

static uint8_t dnstid_l=0; // a counter for transaction ID
// src port high byte must be a a0 or higher:
uint8_t dnsip[]={8,8,8,8}; // the google public DNS. To be used if DNS server is not set by user.
static uint8_t haveDNSanswer=0;
static uint8_t dns_answerip[4];
static uint8_t dns_ansError=0;


uint8_t dnslkup_haveanswer(void) {
        return(haveDNSanswer);
}

uint8_t dnslkup_get_error_info(void) {
        return(dns_ansError);
}

uint8_t *dnslkup_getip() {
        return(dns_answerip);
}

// send a DNS udp request packet
// See http://www.ietf.org/rfc/rfc1034.txt 
// and http://www.ietf.org/rfc/rfc1035.txt
//
//void dnslkup_request(uint8_t *buf,const prog_char *progmem_hostname)
void dnslkup_request(uint8_t *buf, uint8_t  *domainName) {
        uint8_t i,lenpos,lencnt;
        char c;
        haveDNSanswer=0;
        dns_ansError=0;
        dnstid_l++; // increment for next request, finally wrap
        send_udp_prepare(buf,(DNSCLIENT_SRC_PORT_H<<8)|(dnstid_l&0xff),dnsip,53);
        // fill tid:
        //buf[UDP_DATA_P] see below
        buf[UDP_DATA_P+1]=dnstid_l;
        buf[UDP_DATA_P+2]=1; // flags, standard recursive query
        i=3;
        // most fields are zero, here we zero everything and fill later
        while(i<10){ 
                buf[UDP_DATA_P+i]=0;
                i++;
        }
        buf[UDP_DATA_P+5]=1; // 1 question
        // the structure of the domain name
        // we ask for is always length, string, length, string, ...
        // for earch portion of the name.
        // www.twitter.com would become: 3www7twitter3com
        // 
        // the first len starts at i=12
        lenpos=12;
        i=13;
        lencnt=1;
//        while ((c = pgm_read_byte(progmem_hostname++))) {
        while ((c = *domainName++)) {
                if (c=='.'){
                        buf[UDP_DATA_P+lenpos]=lencnt-1;
                        lencnt=0;
                        lenpos=i;
                }
                buf[UDP_DATA_P+i]=c;
                lencnt++;
                i++;
        }
        buf[UDP_DATA_P+lenpos]=lencnt-1;
        buf[UDP_DATA_P+i]=0; // terminate with zero, means root domain.
        i++;
        buf[UDP_DATA_P+i]=0;
        i++;
        buf[UDP_DATA_P+i]=1; // type A
        i++;
        buf[UDP_DATA_P+i]=0; 
        i++;
        buf[UDP_DATA_P+i]=1; // class IN
        i++;
        // we encode the length into the upper byte of the TID
        // this way we can easily jump over the query section
        // of the answer:
        buf[UDP_DATA_P]=i-12;
        send_udp_transmit(buf,i);
}

// process the answer from the dns server:
// return 1 on sucessful processing of answer.
// We set also the variable haveDNSanswer
uint8_t udp_client_check_for_dns_answer(uint8_t *buf,uint16_t plen) {
        uint8_t j,i;
        if (plen<70){
                return(0);
        }
        if (buf[UDP_SRC_PORT_L_P]!=53){
                // not from a DNS
                return(0);
        }
        if (buf[UDP_DST_PORT_H_P]!=DNSCLIENT_SRC_PORT_H){ 
                return(0);
        }
        // we use the TID also as port:
        if (buf[UDP_DST_PORT_L_P]!=dnstid_l){ 
                return(0);
        }
        /* we can skip this check, it is quite unlikely that we
         * get a packet with the right tid from a DNS. Save some 
         * processing:
        // is the packet for my IP:
        if(eth_type_is_ip_and_my_ip(buf,plen)==0){
                return(0);
        }
        */
        if (buf[UDP_DATA_P+1]!=dnstid_l){ 
                return(0);
        }
        // check flags lower byte:
        if ((buf[UDP_DATA_P+3]&0x8F)!=0x80){ 
                // there is an error or server does not support recursive
                // queries. We can only work with servers that support recursive
                // queries.
                dns_ansError=1;
                return(0);
        }
        // there might be multiple answers, we use only the first one
        //
        // UDP_DATA_P+12+querylen is first byte of first answer.
        // The answer contains again the domain name and we need to
        // jump over it to find the IP. This part can be abbreviated by
        // the use of 2 byte pointers. See RFC 1035.
        i=12+buf[UDP_DATA_P]; // we encoded the query len into tid
        if (buf[UDP_DATA_P+i] & 0xc0){
                // pointer
                i+=2;
        }else{
                // we just search for the first, zero=root domain
                // all other octets must be non zero
                while(i<plen-UDP_DATA_P-7){
                        i++;
                        if (buf[UDP_DATA_P+i]==0){
                                i++;
                                break;
                        }
                }
        }
        // i is now pointing to the high octet of the type field
        int numAnswers = buf[UDP_DATA_P+7];
        int ansNum = 0;

        while( buf[UDP_DATA_P+i+1] != 1 && ansNum < numAnswers ) {
                i += buf[UDP_DATA_P+i+9] + 12;
                ansNum++;
        }
        
        if ( ansNum == numAnswers ){
                dns_ansError=2; // not IPv4
                return(0);
        }
        i+=10;
        j=0;
        while(j<4){
                dns_answerip[j]=buf[UDP_DATA_P+i+j];
                j++;
        }
        haveDNSanswer=1;
        return(1);
}

// set DNS server to be used for lookups.
// defaults to Google DNS server if not called.
void dnslkup_set_dnsip(uint8_t *dnsipaddr) {
        uint8_t i=0;
        while(i<4){
                dnsip[i]=dnsipaddr[i];
                i++;
        }
}

// Perform all processing to resolve a hostname to IP address.
// Returns 1 for successful name resolution, 0 otherwise
uint8_t resolveHostname(uint8_t *buf, uint16_t buffer_size, uint8_t *hostname) {
    uint16_t dat_p;
    int plen = 0;
    long lastDnsRequest = HAL_GetTick();
    uint8_t dns_state = DNS_STATE_INIT;
    bool gotAddress = false;
    uint8_t dnsTries = 3;  // After 3 attempts fail gracefully

    while (!gotAddress) {
        plen = enc28j60PacketReceive(buffer_size, buf);
        dat_p = handle_AllPacket(buf, plen);

        if (dat_p == 0) {
            if (client_waiting_gw()) {
                continue;
            }

            if (dns_state == DNS_STATE_INIT) {
                dns_state = DNS_STATE_REQUESTED;
                lastDnsRequest = HAL_GetTick();
                dnslkup_request(buf, hostname);
                continue;
            }

            if (dns_state != DNS_STATE_ANSWER) {
                if (HAL_GetTick() > (lastDnsRequest + 60000L)) {
                    if (--dnsTries <= 0) {
                        return 0;  // Failed to resolve address
                    }
                    dns_state = DNS_STATE_INIT;
                    lastDnsRequest = HAL_GetTick();
                }
                continue;
            }
        } else {
            if (dns_state == DNS_STATE_REQUESTED && udp_client_check_for_dns_answer(buf, plen)) {
                dns_state = DNS_STATE_ANSWER;
                client_tcp_set_serverip(dnslkup_getip());
                gotAddress = true;
            }
        }
    }

    return 1;
}

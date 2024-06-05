/*
 * This file contains the implementation for establishing a full connection using the EtherShield library
 * with an ENC28J60 Ethernet module. It includes functions to initialize the connection, allocate IP addresses
 * via DHCP, resolve DNS hostnames, and request NTP time updates. Additionally, it processes incoming packets
 * for ICMP and TCP protocols.
 */

#include "EtherShield.h"

#define BUFFER_SIZE 400

uint8_t hostname[] = "www.google.com";

// Function to initialize and establish a full connection
void ES_FullConnection(SPI_HandleTypeDef *hspi) {
    // Set the SPI handle for the ENC28J60
    enc28j60_set_spi(hspi);

    // Initialize the ENC28J60 with the MAC address
    enc28j60Init(macaddrin);

    // Configure the ENC28J60 clock
    enc28j60clkout(3);

    // Configure the ENC28J60 PHY LEDs
    enc28j60PhyWrite(PHLCON, 0x3880);
    enc28j60PhyWrite(PHLCON, 0x3990);
    enc28j60PhyWrite(PHLCON, 0x3476);

    // Allocate IP address via DHCP
    uint8_t bufDHCP[BUFFER_SIZE];
    allocateIPAddress(bufDHCP, BUFFER_SIZE, macaddrin, 80, ipaddrin, maskin, gwipin, dnssvrin, dhcpsvrin);

    // Set the gateway IP for the client
    client_set_gwip(gwipin);

    // Resolve the DNS hostname
    uint8_t bufDNS[BUFFER_SIZE];
    dnslkup_request(bufDNS, domainName);
    udp_client_check_for_dns_answer(bufDNS, plen);
    dnslkup_set_dnsip(dnsipaddr);
    resolveHostname(bufDNS, BUFFER_SIZE, hostname);

    // Send NTP request to get the current time
    uint8_t bufNTP[BUFFER_SIZE];
    client_ntp_request(bufNTP, ntpip, 123);

    // Log the success of the full connection setup
    udpLog2("ES_FullConnection", "WORKS!");
}

// Function to process incoming web packets
void ES_ProcessWebPacket() {
    static uint8_t packet[500];
    packetloop_icmp_tcp(packet, enc28j60PacketReceive(500, packet));
}

/**
 * Initialise SPI, separate from main initialisation so that
 * multiple SPI devices can be used together
 */
void ES_enc28j60SpiInit(SPI_HandleTypeDef *hspi){
//  ENC28J60_SPI1_Configuration();
	enc28j60_set_spi(hspi);
}

/**
 * Initialise the ENC28J60 using default chip select pin
 * Flash the 2 MagJack LEDs
 */
void ES_enc28j60Init( uint8_t* macaddr ) {
  /*initialize enc28j60*/
  enc28j60Init( macaddr );
  enc28j60clkout(2); // change clkout from 6.25MHz to 12.5MHz
  HAL_Delay(10);

  int f;
  for( f=0; f<3; f++ ) {
  	// 0x880 is PHLCON LEDB=on, LEDA=on
  	// enc28j60PhyWrite(PHLCON,0b0011 1000 1000 00 00);
  	enc28j60PhyWrite(PHLCON,0x3880);
  	HAL_Delay(500);

  	// 0x990 is PHLCON LEDB=off, LEDA=off
  	// enc28j60PhyWrite(PHLCON,0b0011 1001 1001 00 00);
  	enc28j60PhyWrite(PHLCON,0x3990);
  	HAL_Delay(500);
  }

  // 0x476 is PHLCON LEDA=links status, LEDB=receive/transmit
  // enc28j60PhyWrite(PHLCON,0b0011 0100 0111 01 10);
  enc28j60PhyWrite(PHLCON,0x3476);
  HAL_Delay(100);
}

void ES_enc28j60clkout(uint8_t clk){
	enc28j60clkout(clk);
}

uint8_t ES_enc28j60linkup(void) {
	return enc28j60linkup();
}

void ES_enc28j60EnableBroadcast( void ) {
	enc28j60EnableBroadcast();
}

void ES_enc28j60DisableBroadcast( void ) {
	enc28j60DisableBroadcast();
}

void ES_enc28j60EnableMulticast( void ) {
	enc28j60EnableMulticast();
}

void ES_enc28j60DisableMulticast( void ) {
	enc28j60DisableMulticast();
}

uint8_t ES_enc28j60Read( uint8_t address ) {
	return enc28j60Read( address );
}

uint8_t ES_enc28j60Revision(void) {
	return enc28j60getrev();
}

void ES_enc28j60PhyWrite(uint8_t address, uint16_t data){
	enc28j60PhyWrite(address,  data);
}

uint16_t ES_enc28j60PacketReceive(uint16_t len, uint8_t* packet){
	return enc28j60PacketReceive(len, packet);
}

void ES_enc28j60PacketSend(uint16_t len, uint8_t* packet){
	enc28j60PacketSend(len, packet);
}

void ES_init_ip_arp_udp_tcp(uint8_t *mymac,uint8_t *myip,uint16_t wwwp){
	init_ip_arp_udp_tcp(mymac,myip,wwwp);
}

uint8_t ES_eth_type_is_arp_and_my_ip(uint8_t *buf,uint16_t len) {
	return eth_type_is_arp_and_my_ip(buf,len);
}

uint8_t ES_eth_type_is_ip_and_my_ip(uint8_t *buf,uint16_t len) {
	return eth_type_is_ip_and_my_ip(buf, len);
}

void ES_make_echo_reply_from_request(uint8_t *buf,uint16_t len) {
	make_echo_reply_from_request(buf,len);
}

void ES_make_tcp_synack_from_syn(uint8_t *buf) {
	make_tcp_synack_from_syn(buf);
}

void ES_make_tcp_ack_from_any(uint8_t *buf,int16_t datlentoack,uint8_t addflags ) {
	void make_tcp_ack_from_any(uint8_t *buf,int16_t datlentoack,uint8_t addflags );
}

void ES_make_tcp_ack_with_data(uint8_t *buf,uint16_t dlen ) {
	make_tcp_ack_with_data(buf, dlen);
}
		
void ES_make_tcp_ack_with_data_noflags(uint8_t *buf,uint16_t dlen ) {
	make_tcp_ack_with_data_noflags(buf, dlen);
}

uint16_t ES_build_tcp_data(uint8_t *buf, uint16_t srcPort ) {
	return build_tcp_data( buf, srcPort );
}

void ES_send_tcp_data(uint8_t *buf,uint16_t dlen ) {
	send_tcp_data(buf, dlen);
}

void ES_send_udp_data2(uint8_t *buf, uint8_t *destmac,uint16_t dlen,uint16_t source_port, uint8_t *dest_ip, uint16_t dest_port) {
	send_udp_prepare(buf,source_port, dest_ip, dest_port);
	uint8_t i;
	for(i = 0; i< 6; i++ )
		buf[ETH_DST_MAC+i] = destmac[i];
	send_udp_transmit(buf,dlen);
}

void ES_send_udp_data1(uint8_t *buf,uint16_t dlen,uint16_t source_port, uint8_t *dest_ip, uint16_t dest_port) {
	send_udp_prepare(buf,source_port, dest_ip, dest_port);
	send_udp_transmit(buf,dlen);
}

void ES_init_len_info(uint8_t *buf) {
	init_len_info(buf);
}

/*void ES_fill_buf_p(uint8_t *buf,uint16_t len, const prog_char *progmem_s) {
	fill_buf_p(buf, len, progmem_s);
}*/

uint16_t ES_checksum(uint8_t *buf, uint16_t len,uint8_t type) {
	return checksum(buf, len, type);

}

void ES_fill_ip_hdr_checksum(uint8_t *buf) {
	fill_ip_hdr_checksum(buf);
}

uint16_t ES_get_tcp_data_pointer(void) {
	return get_tcp_data_pointer();
}

uint16_t ES_packetloop_icmp_tcp(uint8_t *buf,uint16_t plen) {
	return packetloop_icmp_tcp(buf,plen);
}

/*uint16_t ES_fill_tcp_data_p(uint8_t *buf,uint16_t pos, const prog_char *progmem_s){
	return fill_tcp_data_p(buf, pos, progmem_s);
}*/

uint16_t ES_fill_tcp_data(uint8_t *buf,uint16_t pos, const char *s){
	return fill_tcp_data(buf,pos, s);
}

uint16_t ES_fill_tcp_data_len(uint8_t *buf,uint16_t pos, const char *s, uint16_t len ){
	return fill_tcp_data_len(buf,pos, s, len);
}

void ES_www_server_reply(uint8_t *buf,uint16_t dlen) {
	www_server_reply(buf,dlen);
}
	
uint8_t ES_client_store_gw_mac(uint8_t *buf) {
	return client_store_gw_mac(buf);
}

void ES_client_set_gwip(uint8_t *gwipaddr) {
	client_set_gwip(gwipaddr);
}

void ES_client_set_wwwip(uint8_t *wwwipaddr) {
	//client_set_wwwip(wwwipaddr);
	client_tcp_set_serverip(wwwipaddr);
}

void ES_client_tcp_set_serverip(uint8_t *ipaddr) {
	client_tcp_set_serverip(ipaddr);
}

void ES_client_arp_whohas(uint8_t *buf,uint8_t *ip_we_search) {
	client_arp_whohas(buf, ip_we_search);
}

#if defined (TCP_client) || defined (WWW_client) || defined (NTP_client)
uint8_t ES_client_tcp_req(uint8_t (*result_callback)(uint8_t fd,uint8_t statuscode,uint16_t data_start_pos_in_buf, uint16_t len_of_data),uint16_t (*datafill_callback)(uint8_t fd),uint16_t port ) {
	return client_tcp_req( result_callback, datafill_callback, port );
}

void ES_tcp_client_send_packet(uint8_t *buf,uint16_t dest_port, uint16_t src_port, uint8_t flags, uint8_t max_segment_size, 
	uint8_t clear_seqck, uint16_t next_ack_num, uint16_t dlength, uint8_t *dest_mac, uint8_t *dest_ip){
	
	tcp_client_send_packet(buf, dest_port, src_port, flags, max_segment_size, clear_seqck, next_ack_num, dlength,dest_mac,dest_ip);
}

uint16_t ES_tcp_get_dlength( uint8_t *buf ){
	return tcp_get_dlength(buf);
}

#endif		// TCP_client WWW_Client etc

#ifdef WWW_client

// ----- http get

void ES_client_http_get(char *urlbuf, char *urlbuf_varpart, char *hoststr,
		void (*callback)(uint8_t,uint16_t,uint16_t)) {
	client_http_get(urlbuf, urlbuf_varpart, hoststr, callback);
}

void ES_client_http_post(char *urlbuf, char *hoststr, char *additionalheaderline,
		char *method, char *postval,void (*callback)(uint8_t,uint16_t)) {
	client_http_post(urlbuf, hoststr, additionalheaderline, method, postval,callback);
}

#endif		// WWW_client

#ifdef NTP_client
void ES_client_ntp_request(uint8_t *buf,uint8_t *ntpip,uint8_t srcport) {
	client_ntp_request(buf,ntpip,srcport);
}
#endif		// NTP_client

void ES_register_ping_rec_callback(void (*callback)(uint8_t *srcip)) {
	register_ping_rec_callback(callback);
}

#ifdef PING_client
void ES_client_icmp_request(uint8_t *buf,uint8_t *destip) {
	client_icmp_request(buf,destip);
}

uint8_t ES_packetloop_icmp_checkreply(uint8_t *buf,uint8_t *ip_monitoredhost) {
	return packetloop_icmp_checkreply(buf,ip_monitoredhost);
}
#endif // PING_client

#ifdef WOL_client
void ES_send_wol(uint8_t *buf,uint8_t *wolmac) {
	send_wol(buf,wolmac);
}
#endif // WOL_client


#ifdef FROMDECODE_websrv_help
uint8_t ES_find_key_val(char *str,char *strbuf, uint16_t maxlen,char *key) {
	return find_key_val(str,strbuf, maxlen,key);
}

void ES_urldecode(char *urlbuf) {
	urldecode(urlbuf);
}
#endif


#ifdef URLENCODE_websrv_help
void ES_urlencode(char *str,char *urlbuf) {
	urlencode(str,urlbuf);
}
#endif

uint8_t ES_parse_ip(uint8_t *bytestr,char *str) {
	return parse_ip(bytestr,str);
}

void ES_mk_net_str(char *resultstr,uint8_t *bytestr,uint16_t len,char separator,uint8_t base) {
	mk_net_str(resultstr,bytestr,len,separator,base);
}

uint8_t ES_client_waiting_gw() {
	return( client_waiting_gw() );
}

#ifdef DNS_client
uint8_t ES_dnslkup_haveanswer(void)
{       
        return( dnslkup_haveanswer() );
}

uint8_t ES_dnslkup_get_error_info(void)
{       
        return( dnslkup_get_error_info() );
}

uint8_t * ES_dnslkup_getip(void)
{       
        return(dnslkup_getip() );
}

void ES_dnslkup_set_dnsip(uint8_t *dnsipaddr) {
	dnslkup_set_dnsip(dnsipaddr);
}

void ES_dnslkup_request(uint8_t *buf,uint8_t *hostname) {
	return( dnslkup_request(buf, hostname) );
}

uint8_t ES_udp_client_check_for_dns_answer(uint8_t *buf,uint16_t plen){
	return( udp_client_check_for_dns_answer( buf, plen) );
}
#endif		// DNS_client

#ifdef DHCP_client
void ES_dhcp_start(uint8_t *buf, uint8_t *macaddrin, uint8_t *ipaddrin,
     uint8_t *maskin, uint8_t *gwipin, uint8_t *dhcpsvrin, uint8_t *dnssvrin ) {
	dhcp_start(buf, macaddrin, ipaddrin, maskin, gwipin, dhcpsvrin, dnssvrin );
}
uint8_t ES_dhcp_state(void)
{       
        return( dhcp_state() );
}

uint8_t ES_check_for_dhcp_answer(uint8_t *buf,uint16_t plen){
	return( check_for_dhcp_answer( buf, plen) );
}
#endif		// DHCP_client

void ES_enc28j60PowerUp(){
 enc28j60PowerUp();
}

void ES_enc28j60PowerDown(){
 enc28j60PowerDown();
}


// TCP functions broken out here for testing
uint8_t ES_nextTcpState( uint8_t *buf, uint16_t plen ) {
	return nextTcpState(buf, plen );
}

uint8_t ES_currentTcpState( ) {
	return currentTcpState( );
}

uint8_t ES_tcpActiveOpen( uint8_t *buf,uint16_t plen,
       uint8_t (*result_callback)(uint8_t fd,uint8_t statuscode,uint16_t data_start_pos_in_buf, uint16_t len_of_data),
       uint16_t (*datafill_callback)(uint8_t fd),
       uint16_t port ) {
	return tcpActiveOpen(buf, plen, result_callback, datafill_callback, port );
}

void ES_tcpPassiveOpen( uint8_t *buf,uint16_t plen ) {
	tcpPassiveOpen(buf, plen );
}

void ES_tcpClose( uint8_t *buf,uint16_t plen ) {
	tcpClose(buf, plen );
}

// End

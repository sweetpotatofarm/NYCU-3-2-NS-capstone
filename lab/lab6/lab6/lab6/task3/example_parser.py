import dpkt
import datetime
import socket
import sys
import math

def get_formatted_mac_addr(original_mac_addr):
    return ':'.join('%02x' % dpkt.compat.compat_ord(x) for x in original_mac_addr)

def print_packets(pcap):
    '''
    [TODO]: 
    1. Use MGMT_TYPE packets to calculate AP's mac addr / connection time / handoff times, and to collect beacon SNR
    2. Use DATA_TYPE packets to calculate total transmitted bytes / CDF of packets' SNR 
    3. Please do not print the SNR information in your submitted code, dump it to a file instead
    Note: As for SNR information, you only need to count downlink packets (but for all APs)
    '''
    
    # For each packet in the pcap process the contents
    for timestamp, buf in pcap:
        # radiotap -> ieee80211
        wlan_pkt = dpkt.radiotap.Radiotap(buf).data
        
        if(wlan_pkt.type == dpkt.ieee80211.MGMT_TYPE): 
            dst_mac_addr = get_formatted_mac_addr(wlan_pkt.mgmt.dst)
            src_mac_addr = get_formatted_mac_addr(wlan_pkt.mgmt.src)
            print('%8.6f WLAN-Pack-Mgmt: %s -> %s' % (timestamp, src_mac_addr, dst_mac_addr))
        
        elif(wlan_pkt.type == dpkt.ieee80211.DATA_TYPE):
            dst_mac_addr = get_formatted_mac_addr(wlan_pkt.data_frame.dst)
            src_mac_addr = get_formatted_mac_addr(wlan_pkt.data_frame.src)
            print('%8.6f WLAN-Pack-Data: %s -> %s' % (timestamp, src_mac_addr, dst_mac_addr))

            # ieee80211 -> llc
            llc_pkt = dpkt.llc.LLC(wlan_pkt.data_frame.data)
            if llc_pkt.type == dpkt.ethernet.ETH_TYPE_ARP:
                # llc -> arp
                arp_pkt = llc_pkt.data
                src_ip_addr = socket.inet_ntop(socket.AF_INET, arp_pkt.spa)
                dst_ip_addr = socket.inet_ntop(socket.AF_INET, arp_pkt.tpa)
                print('[ARP packet]: %s -> %s' % (src_ip_addr, dst_ip_addr))
            elif llc_pkt.type == dpkt.ethernet.ETH_TYPE_IP:
                # llc -> ip
                ip_pkt = llc_pkt.data
                src_ip_addr = socket.inet_ntop(socket.AF_INET, ip_pkt.src)
                dst_ip_addr = socket.inet_ntop(socket.AF_INET, ip_pkt.dst)
                src_port = ip_pkt.data.sport
                dst_port = ip_pkt.data.dport
                print('[IP packet] : %s:%s -> %s:%s' % (src_ip_addr, str(src_port), dst_ip_addr, str(dst_port)))
        
        elif(wlan_pkt.type == dpkt.ieee80211.CTL_TYPE):
            if wlan_pkt.subtype == dpkt.ieee80211.C_ACK:
                dst_mac_addr = get_formatted_mac_addr(wlan_pkt.ack.dst)
                src_mac_addr = ' '*17
            elif wlan_pkt.subtype == dpkt.ieee80211.C_CTS:
                dst_mac_addr = get_formatted_mac_addr(wlan_pkt.cts.dst)
                src_mac_addr = ' '*17
            elif wlan_pkt.subtype == dpkt.ieee80211.C_RTS:
                dst_mac_addr = get_formatted_mac_addr(wlan_pkt.rts.dst)
                src_mac_addr = get_formatted_mac_addr(wlan_pkt.rts.src)
            print('%8.6f WLAN-Pack-Ctrl: %s -> %s' % (timestamp, src_mac_addr, dst_mac_addr))

if __name__ == '__main__':
    with open(sys.argv[1], 'rb') as f:
        pcap = dpkt.pcap.Reader(f)
        print_packets(pcap)
    print('\n[Connection statistics]')
    print('- AP1')
    print('  - Mac addr: 00:00:00:00:00:01')
    print('  - Total connection duration: %.4fs' % 12.3654)
    print('  - Total transmitted bytes: %d bytes' % 1234)
    print('\n[Other statistics]')
    print('  - Number of handoff events: %d' % 2)
    print('  - Theoretical sum-rate: %d mbps' % int(math.floor(125.25)))

import dpkt
import datetime
import socket
import sys
import math

hand_count = 0
ap1_mac_addr = None
ap2_mac_addr = None
client_mac_addr = None
ap1_connect_duration = 0
ap2_connect_duration = 0
ap1_transmitted_bytes = 0
ap2_transmitted_bytes = 0
capacity_sum = 0
simulation_duration = 0
flag = True

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
    global hand_count, ap1_mac_addr, ap2_mac_addr, ap1_connect_duration, ap2_connect_duration, ap1_transmitted_bytes, ap2_transmitted_bytes, client_mac_addr, capacity_sum, simulation_duration, flag
    temp_timestamp = 0
    start_mac_addr = None
    current_ap_addr = None
    prev_ap_addr = None
    # For each packet in the pcap process the contents
    for timestamp, buf in pcap:
        # radiotap -> ieee80211
        simulation_duration = timestamp
        wlan_pkt = dpkt.radiotap.Radiotap(buf).data
        tap = dpkt.radiotap.Radiotap(buf)
        tap.unpack(buf)
        
        if(wlan_pkt.type == dpkt.ieee80211.MGMT_TYPE): 
            dst_mac_addr = get_formatted_mac_addr(wlan_pkt.mgmt.dst)
            src_mac_addr = get_formatted_mac_addr(wlan_pkt.mgmt.src)

            if(wlan_pkt.subtype == dpkt.ieee80211.M_ASSOC_REQ):
                if((not current_ap_addr) and prev_ap_addr != dst_mac_addr):
                    prev_ap_addr = dst_mac_addr
                    if(flag == False):
                        hand_count+=1
                    else:
                        flag = False
                if not client_mac_addr:
                    client_mac_addr = src_mac_addr
                if not ap1_mac_addr:
                    ap1_mac_addr = dst_mac_addr
                    current_ap_addr = dst_mac_addr
                elif not ap2_mac_addr:
                    ap2_mac_addr = dst_mac_addr 
                    current_ap_addr = dst_mac_addr

            if (wlan_pkt.subtype == dpkt.ieee80211.M_ASSOC_RESP):
                if not start_mac_addr:
                    start_mac_addr = src_mac_addr
                if not current_ap_addr:
                    current_ap_addr = src_mac_addr
                    temp_timestamp = timestamp

            if (wlan_pkt.subtype == dpkt.ieee80211.M_DISASSOC):
                start_mac_addr = None
                # prev_ap_addr = current_ap_addr
                current_ap_addr = False

            if (wlan_pkt.subtype == dpkt.ieee80211.M_PROBE_REQ):
                start_mac_addr = None
                # prev_ap_addr = current_ap_addr
                current_ap_addr = False
            
            if(wlan_pkt.subtype == dpkt.ieee80211.M_BEACON):
                if (current_ap_addr == ap1_mac_addr):
                    ap1_connect_duration += (timestamp - temp_timestamp)
                    temp_timestamp = timestamp
                elif (current_ap_addr == ap2_mac_addr):
                    ap2_connect_duration += (timestamp - temp_timestamp)
                    temp_timestamp = timestamp
                    
                if (current_ap_addr == src_mac_addr):
                    sig = tap.ant_sig.db
                    noise = tap.ant_noise.db
                    snr = 10**((sig-noise)/10)

                    fp = open("snr.txt", "a")
                    fp.write(str(snr))
                    fp.write("\n")
                    fp.close()

                    capacity = 20 * math.log(1+snr, 2)
                    capacity_sum+=capacity

            # print('%8.6f WLAN-Pack-Mgmt: %s -> %s' % (timestamp, src_mac_addr, dst_mac_addr))
        
        elif(wlan_pkt.type == dpkt.ieee80211.DATA_TYPE):
            dst_mac_addr = get_formatted_mac_addr(wlan_pkt.data_frame.dst)
            src_mac_addr = get_formatted_mac_addr(wlan_pkt.data_frame.src)
            # print('%8.6f WLAN-Pack-Data: %s -> %s' % (timestamp, src_mac_addr, dst_mac_addr))

            # ieee80211 -> llc
            llc_pkt = dpkt.llc.LLC(wlan_pkt.data_frame.data)
            if llc_pkt.type == dpkt.ethernet.ETH_TYPE_ARP:
                # llc -> arp
                arp_pkt = llc_pkt.data
                src_ip_addr = socket.inet_ntop(socket.AF_INET, arp_pkt.spa)
                dst_ip_addr = socket.inet_ntop(socket.AF_INET, arp_pkt.tpa)
                # print('[ARP packet]: %s -> %s' % (src_ip_addr, dst_ip_addr))
            elif llc_pkt.type == dpkt.ethernet.ETH_TYPE_IP:
                if (start_mac_addr == ap1_mac_addr):
                    ap1_transmitted_bytes += 1032
                elif (start_mac_addr == ap2_mac_addr):
                    ap2_transmitted_bytes += 1032
                # llc -> ip
                ip_pkt = llc_pkt.data
                src_ip_addr = socket.inet_ntop(socket.AF_INET, ip_pkt.src)
                dst_ip_addr = socket.inet_ntop(socket.AF_INET, ip_pkt.dst)
                src_port = ip_pkt.data.sport
                dst_port = ip_pkt.data.dport
                # print('[IP packet] : %s:%s -> %s:%s' % (src_ip_addr, str(src_port), dst_ip_addr, str(dst_port)))
        
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
            # print('%8.6f WLAN-Pack-Ctrl: %s -> %s' % (timestamp, src_mac_addr, dst_mac_addr))

if __name__ == '__main__':
    fp = open("snr.txt", "w")
    fp.close()
    with open(sys.argv[1], 'rb') as f:
        pcap = dpkt.pcap.Reader(f)
        print_packets(pcap)
    print('\n[Connection statistics]')
    print('- AP1')
    print('  - Mac addr: %s' % ap1_mac_addr)
    print('  - Total connection duration: %.4fs' % ap1_connect_duration)
    print('  - Total transmitted bytes: %d bytes' % ap1_transmitted_bytes)
    if ap2_mac_addr:
        print('- AP2')
        print('  - Mac addr: %s' % ap2_mac_addr)
        print('  - Total connection duration: %.4fs' % ap2_connect_duration)
        print('  - Total transmitted bytes: %d bytes' % ap2_transmitted_bytes)
    print('\n[Other statistics]')
    print('  - Number of handoff events: %d' % hand_count)
    print('  - Theoretical sum-rate: %d mbps' % int((102400*10**-6/simulation_duration)*capacity_sum))

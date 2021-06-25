#include <iostream>
#include <pcap.h>
#include <vector>
#include <string>
#include <string.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <algorithm>

using namespace std;

char filter_exp[100];
pcap_t *handle;
struct bpf_program filter;
bpf_u_int32 ip;

int packetNum = 1;
int gretapNum = 1;
int bridgeNum = 1;
vector<pair<string, string>> tunnelHistory;
bool first = true;

void callback(u_char *useless, const struct pcap_pkthdr *header, const u_char *packet)
{
    struct ether_header *ep;
    struct ip *iph;
    unsigned short ether_type;
    int chcnt = 0;
    int len = header->len;
    int i;

    //printf("--------------------------------\n");
/*
//next layer protocol
    u_char protocol = *(packet + 23);
    if (protocol == IPPROTO_GRE) {
        printf("Next Layer Protocol: GRE\n");
    }
*/

    // Get Ethernet header.
    ep = (struct ether_header *)packet;
    // Get upper protocol type.
    ether_type = ntohs(ep->ether_type);
    string src;
    string dst;

    if (ether_type == ETHERTYPE_IP) {
/*
//Mac address
        printf("Source MAC: ");
        for (i=0; i<ETH_ALEN; ++i){
            if(i != 0){
                printf(":");
            }
            printf("%.2X", ep->ether_shost[i]);
        }
        printf("\n");
        printf("Destination MAC: ");
        for (i=0; i<ETH_ALEN; ++i){
            if(i != 0){
                printf(":");
            }
            printf("%d, %.2X", i,ep->ether_dhost[i]);
        }
        printf("\n");
*/


        // Move packet pointer for outer header.
        packet += sizeof(struct ether_header);
        iph = (struct ip *)packet;
        src = inet_ntoa(iph->ip_src);
        dst = inet_ntoa(iph->ip_dst);
    /*
//ip version
        if(ether_type == ETHERTYPE_IPV6){
            printf("Ether type: %s%.2X%s\n", "0x", ether_type, "(IPv6)");
        }
        else if(ether_type == ETHERTYPE_IP){
            printf("Ether type: %s%.2X%s\n", "0x", ether_type, "(IPv4)");
        }
        else{
            printf("Ether type: could not determine.\n");
        }
    
//ip addr
        printf("Source IP: %s\n", inet_ntoa(iph->ip_src));
        printf("Dst IP: %s\n", inet_ntoa(iph->ip_dst));
//protocol
        printf("Protocol type: 0x");
        packet += sizeof(struct ip);
        for(int j=2; j<4; j++){
            printf("%.2x", *(packet+j));
        }
        printf("(Transparent Ethernet Bridging)\n");

        // Move packet pointer for inner header. 
        packet += 4;
        iph = (struct ip *)packet;
//Inner Mac addr
        ep = (struct ether_header *)packet;
        printf("Inner source MAC: ");
        for (i=0; i<ETH_ALEN; ++i){
            if(i != 0){
                printf(":");
            }
            printf("%.2X", ep->ether_shost[i]);
        }
        printf("\n");
        printf("Inner destination MAC: ");
        for (i=0; i<ETH_ALEN; ++i){
            if(i != 0){
                printf(":");
            }
            printf("%.2X", ep->ether_dhost[i]);
        }
        printf("\n");
        ether_type = ntohs(ep->ether_type);
        if(ether_type == ETHERTYPE_IPV6){
            printf("Inner ether type: %s%.2X%s\n", "0x", ether_type, "(IPv6)");
        }
        else if(ether_type == ETHERTYPE_IP){
            printf("Inner ether type: %s%.2X%s\n", "0x", ether_type, "(IPv4)");
        }
        else{
            printf("Inner ether type: could not determine.\n");
        }
    */
    packet += 20;
    int sport = (*packet)*16*16 + (*(packet+1));
    int dport = (*(packet+2))*16*16 + (*(packet+3));
    int header = (*(packet+10))*16*16 + (*(packet+11));
    int key = (*(packet+15));

//build tunnel
        if(header != 25944){
            return;
        }
        else{
            string cmd1 = "ip fou add port " + to_string(dport) + " ipproto 47";
            string cmd2 = "ip link add GRE" + to_string(gretapNum) + " type gretap remote "
            + src + " local 140.113.0.2 key " + to_string(key) + " encap fou encap-sport " + to_string(dport)
            + " encap-dport " + to_string(sport);
            string cmd3 = "ip link set GRE" + to_string(gretapNum) + " up";
            string cmd4 = "ip link add br" + to_string(bridgeNum) + " type bridge";
            string cmd5 = "brctl addif br" + to_string(bridgeNum) + " Veth0left";
            string cmd6 = "brctl addif br" + to_string(bridgeNum) + " GRE" + to_string(gretapNum);
            string cmd7 = "ip link set br" + to_string(bridgeNum) + " up";

            gretapNum++;
            
            pair<string, string> p;
            p.first = src;
            p.second = dst;
            //vector<pair<string, string>>::iterator it;
            //it = find(tunnelHistory.begin(), tunnelHistory.end(), p);
            //if(it == tunnelHistory.end()){
            //tunnelHistory.push_back(p);
            if(first == true){
                system(cmd1.c_str());
                system(cmd2.c_str());
                system(cmd3.c_str());
                system(cmd4.c_str());
                system(cmd5.c_str());
                system(cmd6.c_str());
                system(cmd7.c_str());
                first = false;
            }
            else{
                system(cmd1.c_str());
                system(cmd2.c_str());
                system(cmd3.c_str());
                system(cmd6.c_str());
            }
    //update filter
            string adds = " and src port not " + to_string(sport);
            strcat(filter_exp, adds.c_str());
            printf("%s", filter_exp);
            pcap_compile(handle, &filter, filter_exp, 0, ip);
            pcap_setfilter(handle, &filter);
            printf("\n");
        }
    }
}


int main()
{
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_if_t *device;
    handle = pcap_open_live("Veth1right", BUFSIZ, 1, 10000, errbuf);

//set filter
    string str = "udp and src host not 140.113.0.2";
    strcpy(filter_exp, str.c_str());
    pcap_compile(handle, &filter, filter_exp, 0, ip);
    pcap_setfilter(handle, &filter);

//capture packet
    pcap_loop(handle, 0, callback, NULL);

    return 0;
}

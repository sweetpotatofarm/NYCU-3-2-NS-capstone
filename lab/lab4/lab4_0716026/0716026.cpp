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
bool firstTimeBridge = true;

void callback(u_char *useless, const struct pcap_pkthdr *header, const u_char *packet)
{
    struct ether_header *ep;
    struct ip *iph;
    unsigned short ether_type;
    int chcnt = 0;
    int len = header->len;
    int i;

    printf("--------------------------------\n");

//next layer protocol
    u_char protocol = *(packet + 23);
    if (protocol == IPPROTO_GRE) {
        printf("Next Layer Protocol: GRE\n");
    }

    // Get Ethernet header.
    ep = (struct ether_header *)packet;
    // Get upper protocol type.
    ether_type = ntohs(ep->ether_type);
    string src;
    string dst;

    if (ether_type == ETHERTYPE_IP) {
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
            printf("%.2X", ep->ether_dhost[i]);
        }
        printf("\n");

        // Move packet pointer for outer header.
        packet += sizeof(struct ether_header);
        iph = (struct ip *)packet;
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
        src = inet_ntoa(iph->ip_src);
        printf("Dst IP: %s\n", inet_ntoa(iph->ip_dst));
        dst = inet_ntoa(iph->ip_dst);
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

//build tunnel
        string cmd1 = "ip link add GRE" + to_string(gretapNum) + " type gretap remote "
            + src + " local " + dst;
        string cmd2 = "ip link set GRE" + to_string(gretapNum) + " up";
        string cmd3 = "ip link add br0 type bridge";
        string cmd4 = "brctl addif br0 BRGr-eth0";
        string cmd5 = "brctl addif br0 GRE" + to_string(gretapNum);
        string cmd6 = "ip link set br0 up";

        gretapNum++;
        bridgeNum++;
        
        pair<string, string> p;
        p.first = src;
        p.second = dst;
        vector<pair<string, string>>::iterator it;
        it = find(tunnelHistory.begin(), tunnelHistory.end(), p);
//update filter
        if(it == tunnelHistory.end()){
            tunnelHistory.push_back(p);
            if(firstTimeBridge == true){
                system(cmd1.c_str());
                system(cmd2.c_str());
                system(cmd3.c_str());
                system(cmd4.c_str());
                system(cmd5.c_str());
                system(cmd6.c_str());
                firstTimeBridge = false;
            }
            else{
                system(cmd1.c_str());
                system(cmd2.c_str());
                system(cmd5.c_str());
            }
            printf("Tunnel finish\n");
            strcat(filter_exp, " and src host not ");
            strcat(filter_exp, src.c_str());
            if(pcap_compile(handle, &filter, filter_exp, 0, ip) == -1){
                printf("bad Filter\n");
            }
            if(pcap_setfilter(handle, &filter) == -1){
                printf("Error setting filter");
            }
        }
        printf("\n");
    }  
}

int main()
{
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_if_t *device;
    vector<char *> interfaceList;

//interface list
    pcap_findalldevs(&device, errbuf);
    int interfaceNum = 0;
    for(pcap_if_t *d = device; d; d = d->next){
        printf("%d ", interfaceNum);
        interfaceNum++;
        printf("Name: %s\n", d->name);
        interfaceList.push_back(d->name);
    }

//interface selection
    printf("%s\n", "Insert a number to seket interface");
    int selectNum;
    cin>>selectNum;

    printf("Start listening at %s\n", interfaceList[selectNum]);
    handle = pcap_open_live(interfaceList[selectNum], BUFSIZ, 1, 10000, errbuf);
    if(handle == NULL){
        printf("Could not open.\n");
        return 2;
    }

//set filter
    printf("Insert BPF filter expression:\n");
    string str;
    std::getline(cin, str);
    std::getline(cin, str);
    strcpy(filter_exp, str.c_str());
    if(pcap_compile(handle, &filter, filter_exp, 0, ip) == -1){
        printf("bad Filter\n");
        return 2;
    }
    if(pcap_setfilter(handle, &filter) == -1){
        printf("Error setting filter");
        return 2;
    }
    printf("filter: %s\n", filter_exp);

//capture packet
    pcap_loop(handle, 0, callback, NULL);

    return 0;
}
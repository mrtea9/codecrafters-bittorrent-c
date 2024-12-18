#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdbool.h>
#include <ctype.h>
#include <openssl/sha.h>
#include <curl/curl.h>

#define PEER_ID_LEN 20
#define RESERVED_LEN 8
#define PROTOCOL_LEN 19
#define HASH_LEN 20
#define HANDSHAKE_LEN 68

typedef struct value value;
typedef struct Peer Peer;

enum {
    VAL_NUMBER,
    VAL_STRING,
    VAL_LIST,
    VAL_DICT
};

enum {
    PEER_SINGLE,
    PEER_LIST
};

struct value {
    int type;

    long number;
    char* string;
    size_t string_length;

    int count;
    value** cell;
};

struct Peer {
    int type;

    char* ip;
    int port;

    int count;
    Peer** peer;
};

void value_print(value* val);
value* value_copy(value* val);
void peer_print(Peer* peer);
value* decode_list(char** bencoded_value);
value* decode_dict(char** bencoded_value);
char* encode(value* decoded);

bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

int num_of_digits(int number) {
    int count = 0;
    do {
        number /= 10;
        ++count;
    } while (number != 0);
    return count;
}

Peer* peer_create(char* ip, int port) {
    Peer* peer = malloc(sizeof(Peer));
    peer->type = PEER_SINGLE;
    peer->ip = malloc(strlen(ip) + 1);
    memcpy(peer->ip, ip, strlen(ip));
    peer->ip[strlen(ip) + 1] = '\0';
    peer->port = port;
    return peer;
}

Peer* peer_list(void) {
    Peer* peer = malloc(sizeof(peer));
    peer->type = PEER_LIST;
    peer->count = 0;
    peer->peer = NULL;
    return peer;
}

Peer* peer_add(Peer* peer_result, Peer* peer_added) {
    peer_result->count++;
    peer_result->peer = realloc(peer_result->peer, sizeof(Peer*) * peer_result->count);
    peer_result->peer[peer_result->count - 1] = peer_added;
    return peer_result;
}

void peer_delete(Peer* peer) {
    switch (peer->type) {
        case PEER_SINGLE:
            free(peer->ip);
            break;

        case PEER_LIST:
            for (int i = 0; i < peer->count; i++) {
                peer_delete(peer->peer[i]);
            }
            break;
    }

    free(peer);
}

void peer_print_list(Peer* peer) {
    putchar('(');
    for (int i = 0; i < peer->count; i++) {
        peer_print(peer->peer[i]);
        if (i != (peer->count - 1)) putchar(',');
    }
    putchar(')');
}

void peer_print(Peer* peer) {
    switch (peer->type) {
        case PEER_SINGLE:
            printf("%s:%d", peer->ip, peer->port);
            break;
        case PEER_LIST:
            peer_print_list(peer);
            break;
    }
}

void peer_println(Peer* peer) {
    peer_print(peer);
    putchar('\n');
}

value* value_number(long number) {
    value* val = malloc(sizeof(value));
    val->type = VAL_NUMBER;
    val->number = number;
    return val;
}

value* value_string(char* string, size_t length) {
    value* val = malloc(sizeof(value));
    val->type = VAL_STRING;
    val->string = malloc(length + 1);
    memcpy(val->string, string, length);
    val->string[length] = '\0';
    val->string_length = length;
    return val;
}

value* value_list(void) {
    value* val = malloc(sizeof(value));
    val->type = VAL_LIST;
    val->count = 0;
    val->cell = NULL;
    return val;
}

value* value_dict(void) {
    value* val = malloc(sizeof(value));
    val->type = VAL_DICT;
    val->count = 0;
    val->cell = NULL;
    return val;
}

value* value_add(value* val1, value* val2) {
    val1->count++;
    val1->cell = realloc(val1->cell, sizeof(value*) * val1->count);
    val1->cell[val1->count - 1] = val2;
    return val1;
}

value* value_copy(value* val) {
    
    value* x = malloc(sizeof(value));
    x->type = val->type;

    switch (val->type) {
        case VAL_NUMBER:
            x->number = val->number;
            break;

        case VAL_STRING:
            x->string = malloc(val->string_length + 1);
            memcpy(x->string, val->string, val->string_length);
            x->string[val->string_length] = '\0';
            x->string_length = val->string_length;
            break;

        case VAL_LIST:
        case VAL_DICT:
            x->count = val->count;
            x->cell = malloc(sizeof(value*) * x->count);
            for (int i = 0; i < x->count; i++) {
                x->cell[i] = value_copy(val->cell[i]);
            }

            break;
    }

    return x;
}

void value_delete(value* val) {

    switch (val->type) {

        case VAL_NUMBER:
            break;

        case VAL_STRING:
            free(val->string);
            break;

        case VAL_LIST:
            for (int i = 0; i < val->count; i++) {
                value_delete(val->cell[i]);
            }
            break;
        case VAL_DICT:
            for (int i = 0; i < val->count; i++) {
                value_delete(val->cell[i]);
            }
            break;
    }

    free(val);
}

void value_print_list(value* val) {
    putchar('[');
    for (int i = 0; i < val->count; i++) {
        value_print(val->cell[i]);
        if (i != (val->count - 1)) putchar(',');
    }
    putchar(']');
}

void value_print_dict(value* val) {
    putchar('{');
    for (int i = 0; i < val->count; i++) {
        value_print(val->cell[i]);
        if (i % 2 != 0 && i != (val->count - 1)) putchar(',');
        else if (i != (val->count - 1)) putchar(':');
    }
    putchar('}');
}

void value_print(value* val) {
    switch (val->type) {
        case VAL_NUMBER:
            printf("%ld", val->number);
            break;
        case VAL_STRING:
            printf("\"%s\"", val->string);
            break;
        case VAL_LIST:
            value_print_list(val);
            break;
        case VAL_DICT:
            value_print_dict(val);
            break;
    }
}

void value_println(value* val) {
    value_print(val);
    putchar('\n');
}

void value_print_info(value* val) {
    switch (val->type) {
    case VAL_NUMBER:
        printf("Length: %ld", val->number);
        break;
    case VAL_STRING:
        printf("Tracker URL: %s", val->string);
        break;
    case VAL_LIST:
        value_print_list(val);
        break;
    case VAL_DICT:
        value_print_dict(val);
        break;
    }
}

value* decode_string(char** bencoded_value) {
    int length = atoi(*bencoded_value);
    char* colon_index = strchr(*bencoded_value, ':');

    if (colon_index != NULL) {
        char* start = colon_index + 1;
        char* decoded_str = (char*)malloc(length + 1);
        strncpy(decoded_str, start, length);
        decoded_str[length] = '\0';

        *bencoded_value = start + length; // Move the pointer past the string
        value* result = value_string(decoded_str, length);
        //free(decoded_str);
        return result;
    }
    else {
        fprintf(stderr, "Invalid encoded value: %s\n", *bencoded_value);
        exit(1);
    }
}

value* decode_integer(char** bencoded_value) {
    (*bencoded_value)++; // Skip the 'i'
    long result = atol(*bencoded_value);

    while (**bencoded_value != 'e') {
        (*bencoded_value)++;
    }
    (*bencoded_value)++; // Skip the 'e'

    return value_number(result);
}

value* value_take(char** string) {
    if (is_digit(**string)) {
        return decode_string(string);
    }

    if (**string == 'i') {
        return decode_integer(string);
    }

    if (**string == 'l') {
        return decode_list(string);
    }

    if (**string == 'd') {
        return decode_dict(string);
    }

    return NULL;
}

value* value_get(value* val, char* name) {
    value* result;

    if (val->type != VAL_DICT) return value_dict();

    for (int i = 0; i < val->count; i++) {

        if (val->cell[i]->type == VAL_DICT) {
            return value_get(val->cell[i], name);
        }

        if (val->cell[i]->type != VAL_STRING) continue;

        if (strcmp(val->cell[i]->string, name) == 0) {
            result = value_copy(val->cell[i + 1]);
            return result;
        }
    }

    return value_dict();
}

value* decode_list(char** bencoded_value) {
    (*bencoded_value)++; // Skip the 'l'
    value* result = value_list();

    while (**bencoded_value != 'e') {
        value* element = value_take(bencoded_value);
        result = value_add(result, element);
    }
    (*bencoded_value)++; // Skip the 'e'

    return result;
}

value* decode_dict(char** bencoded_value) {
    (*bencoded_value)++; // Skip the 'd'
    value* result = value_dict();

    while (**bencoded_value != 'e') {
        value* element = value_take(bencoded_value);
        result = value_add(result, element);
    }
    (*bencoded_value)++; // Skip the 'e'

    return result;
}

value* decode_bencode(char* bencoded_value) {
    if (is_digit(bencoded_value[0])) return decode_string(&bencoded_value);
    if (bencoded_value[0] == 'i') return decode_integer(&bencoded_value);
    if (bencoded_value[0] == 'l') return decode_list(&bencoded_value);
    if (bencoded_value[0] == 'd') return decode_dict(&bencoded_value);

    fprintf(stderr, "Invalid bencoded value: %s\n", bencoded_value);
    exit(1);
}

char* encode_number(value* decoded) {
    char* encoded_number = malloc(num_of_digits(decoded->number) + 3);
    sprintf(encoded_number, "i%lde", decoded->number);

    return encoded_number;
}

char* encode_string(value* decoded) {
    char* encoded_string = malloc(num_of_digits(decoded->string_length) + decoded->string_length + 2);
    sprintf(encoded_string, "%zu:%s", decoded->string_length, decoded->string);

    return encoded_string;
}

char* encode_list(value* decoded) {
    char* encoded_list = malloc(2 * sizeof(char));
    strcpy(encoded_list, "l");

    for (int i = 0; i < decoded->count; i++) {
        encoded_list = realloc(encoded_list, strlen(encoded_list) + strlen(encode(decoded->cell[i])) + 1);
        sprintf(encoded_list, "%s%s", encoded_list, encode(decoded->cell[i]));
    }
    encoded_list = realloc(encoded_list, strlen(encoded_list) + 2);
    sprintf(encoded_list, "%se", encoded_list);

    return encoded_list;
}

char* encode_dict(value* decoded) {
    char* encoded_dict = malloc(2 * sizeof(char));
    strcpy(encoded_dict, "d");

    for (int i = 0; i < decoded->count; i++) {
        char* encoded_value = encode(decoded->cell[i]);

        size_t new_len = strlen(encoded_dict) + strlen(encoded_value) + 1;

        encoded_dict = realloc(encoded_dict, new_len);
        strcat(encoded_dict, encoded_value);
    }

    encoded_dict = realloc(encoded_dict, strlen(encoded_dict) + 2);
    strcat(encoded_dict, "e");


    return encoded_dict;
}

char* encode(value* decoded) {

    if (decoded->type == VAL_NUMBER) return encode_number(decoded);

    if (decoded->type == VAL_STRING) return encode_string(decoded);

    if (decoded->type == VAL_LIST) return encode_list(decoded);

    if (decoded->type == VAL_DICT) return encode_dict(decoded);

    return NULL;
}

unsigned char* read_file(const char* filename, size_t* bytesRead) {
    FILE* file = fopen(filename, "rb");

    if (file == NULL) return NULL;

    fseek(file, 0, SEEK_END);
    long filesize = ftell(file);
    fseek(file, 0, SEEK_SET);

    unsigned char* buffer = malloc(filesize + 1);
    int bytes = fread(buffer, sizeof(char), filesize, file);
    *bytesRead = bytes;
    buffer[bytes] = '\0';

    fclose(file);
    return buffer;
}

char* url_encode(unsigned char* data, size_t len) {
    char* encoded = malloc(len * 3 + 1);
    char* ptr = encoded;

    for (size_t i = 0; i < len; i++) {
        ptr += sprintf(ptr, "%%%02X", data[i]);
    }

    *ptr = '\0';
    return encoded;
}

unsigned char* calculate_raw_hash(unsigned char* data, size_t len) {
    unsigned char* hash = malloc(SHA_DIGEST_LENGTH);
    SHA1(data, len, hash);
    return hash;
}

char* calculate_hash(unsigned char* data, size_t len) {
    unsigned char* hash = calculate_raw_hash(data, len);

    char* sha1_str = malloc(SHA_DIGEST_LENGTH * 2 + 1);
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
        sprintf(sha1_str + (i * 2), "%02x", hash[i]);
    }
    sha1_str[SHA_DIGEST_LENGTH * 2] = '\0';

    return sha1_str;
}

char* get_ip_port(char* address, int* port) {
    char* start;
    char* slash_index = strchr(address, '/');

    if (slash_index) {
        start = slash_index + 2;
        char* colon_index = strchr(start, ':');
    }
    else {
        start = address;
    }
    char* colon_index = strchr(start, ':');
    int total_len = strlen(start);
    int colon_len = strlen(colon_index);
    int ip_len = total_len - colon_len;

    *port = atoi(colon_index + 1);

    char* ip_addres = malloc(ip_len + 1);
    strncpy(ip_addres, start, ip_len);
    ip_addres[ip_len] = '\0';

    return ip_addres;
}

Peer* extract_peers(const char* bencoded_response) {
    Peer* list_peer = peer_list();

    const char* peers_key = "peers";
    char* peers_start = strstr(bencoded_response, peers_key);

    if (!peers_start) {
        printf("no peers start\n");
        exit(1);
    }

    char* length_start = strchr(peers_start, ':');
    if (!length_start) {
        printf("no length_start\n");
        exit(1);
    }

    int peers_length = atoi(peers_start + strlen(peers_key));
    char* peers_data = length_start + 1;

    for (int i = 0; i < peers_length; i += 6) {
        unsigned char ip[4];
        memcpy(ip, &peers_data[i], 4);

        unsigned char port_bytes[2];
        memcpy(port_bytes, &peers_data[i + 4], 2);
        int port = (port_bytes[0] << 8) | port_bytes[1];

        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, ip, ip_str, INET_ADDRSTRLEN);

        Peer* peer = peer_create(ip_str, port);
        peer_add(list_peer, peer);
    }

    return list_peer;
}

size_t write_callback(void* ptr, size_t size, size_t nmemb, void* userdata) {
    strcat(userdata, (char*)ptr);
    return size * nmemb;
}

Peer* perform_curl_request(value* result) {
    CURL* curl;
    CURLcode res;

    Peer* list_peers;

    value* announce = value_get(result, "announce");
    value* length = value_get(result, "length");
    value* info = value_get(result, "info");
    value* piece_length = value_get(result, "piece length");
    value* pieces = value_get(result, "pieces");

    char full_url[1024];
    unsigned char full_response[8192] = { 0 };
    char* encoded_info = encode(info);
    unsigned char* raw_info_hash = calculate_raw_hash((unsigned char*)encoded_info, strlen(encoded_info));
    char* info_hash_url_encoded = url_encode(raw_info_hash, SHA_DIGEST_LENGTH);
    char peer_id[] = "23141516167152146123";
    free(raw_info_hash);

    char query_string[512];
    snprintf(query_string, sizeof(query_string), "?info_hash=%s&peer_id=%s&port=6881&uploaded=0&downloaded=0&left=%d&compact=1", info_hash_url_encoded, peer_id, length->number);

    curl = curl_easy_init();
    if (curl) {
        snprintf(full_url, sizeof(full_url), "%s%s", announce->string, query_string);
        printf("full url = %s\n", full_url);


        curl_easy_setopt(curl, CURLOPT_URL, full_url);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &full_response);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }
        else {
            printf("Response Data:\n%s\n", full_response);

            list_peers = extract_peers(full_response);
        }


        //curl_easy_cleanup(curl);
    }

    return list_peers;
}

Peer* perform_get_request(value* result, char* ip, int received_port) {
    int sockfd;
    struct sockaddr_in server_addr;
    char request[1024], response[4096];
    int port = 0;
    char* ip_addres;

    value* announce = value_get(result, "announce");
    value* length = value_get(result, "length");
    value* info = value_get(result, "info");
    value* piece_length = value_get(result, "piece length");
    value* pieces = value_get(result, "pieces");

    if (strcmp(ip, "NULL") == 0 && port == -1) {
        ip_addres = get_ip_port(announce->string, &port);
    }
    else {
        //Peer* test = perform_curl_request(result);
        //exit(1);

        return perform_curl_request(result);
    }

    printf("ip = %s, port = %d\n", ip_addres, port);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip_addres);

    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    char* encoded_info = encode(info);
    unsigned char* raw_info_hash = calculate_raw_hash((unsigned char*)encoded_info, strlen(encoded_info));
    char* info_hash_url_encoded = url_encode(raw_info_hash, SHA_DIGEST_LENGTH);
    char peer_id[] = "23141516167152146123";
    Peer* list_peers;
    free(raw_info_hash);

    char query_string[512];
    snprintf(query_string, sizeof(query_string), "?info_hash=%s&peer_id=%s&port=6881&uploaded=0&downloaded=0&left=%d&compact=1", info_hash_url_encoded, peer_id, length->number);

    snprintf(request, sizeof(request), "GET /announce%s HTTP/1.1\r\n"
                                       "Host: %s\r\n"
                                       "Conection: close\r\n\r\n", query_string, ip_addres);

    send(sockfd, request, strlen(request), 0);

    int bytes_received;
    char full_response[8192];
    int total_bytes = 0;

    while ((bytes_received = recv(sockfd, response, sizeof(response) - 1, 0)) > 0) {
        response[bytes_received] = '\0';
        total_bytes += bytes_received;

        list_peers = extract_peers(response);
        strncat(full_response, response, bytes_received);

        if (strstr(full_response, "\r\n\r\n")) {
            break;
        }
    }

    close(sockfd);

    value_delete(announce);
    value_delete(length);
    value_delete(info);
    value_delete(piece_length);
    value_delete(pieces);

    return list_peers;
}

void print_bytes(const unsigned char* data, int len) {
    for (size_t i = 0; i < len; i++) {
        // Print each byte in hexadecimal (02 ensures 2 digits, padded with zero if necessary)
        printf("%02x", data[i]);
    }
    // Print a newline after the entire hex string
    printf("\n");
}

void send_handshake(int sockfd, value* result) {
    value* info = value_get(result, "info");
    unsigned char* encoded_info = encode(info);
    unsigned char* raw_info_hash = calculate_raw_hash(encoded_info, strlen(encoded_info));
    unsigned char handshake[HANDSHAKE_LEN];
    char peer_id[] = "23141516167152146123";
    handshake[0] = PROTOCOL_LEN;
    memcpy(&handshake[1], "BitTorrent protocol", PROTOCOL_LEN);
    memset(&handshake[20], 0, RESERVED_LEN);
    memcpy(&handshake[28], raw_info_hash, HASH_LEN);
    memcpy(&handshake[48], peer_id, PEER_ID_LEN);

    send(sockfd, handshake, HANDSHAKE_LEN, 0);
}

void receive_handshake(int sockfd) {
    unsigned char response[HANDSHAKE_LEN];

    if (recv(sockfd, response, HANDSHAKE_LEN, 0) < HANDSHAKE_LEN) {
        printf("Failed to receive full handshake\n");
        return;
    }

    unsigned char peer_id[PEER_ID_LEN];
    memcpy(peer_id, &response[48], PEER_ID_LEN);

    printf("Peer ID: ");
    for (int i = 0; i < PEER_ID_LEN; i++) {
        printf("%02x", peer_id[i]);
    }
    printf("\n");
}

char* resolve_hostname_to_ip(char* hostname, int* port) {
    char* host_start = strstr(hostname, "://");

    host_start = host_start + 3;

    char* path = strchr(host_start, '/');
    if (path) *path = '\0';

    struct hostent* host = gethostbyname(host_start);
    if (host == NULL) {
        perror("gethostbyname error");
        exit(1);
    }

    struct in_addr** addr_list = (struct in_addr**)host->h_addr_list;
    if (addr_list[0] == NULL) {
        perror("addr_list error");
        exit(1);
    }

    char* ip_address = malloc(INET_ADDRSTRLEN);
    inet_ntop(AF_INET, addr_list[0], ip_address, INET_ADDRSTRLEN);

    *port = 80;

    return ip_address;
}

int process_command(char* command, char* encoded_str) {
    if (strcmp(command, "decode") == 0) {
        value* result = decode_bencode(encoded_str);
        value* info = value_get(result, "info");
        
        value_println(result);

        value_delete(result);
        value_delete(info);
    }
    else if (strcmp(command, "info") == 0) {
        
        size_t bytesRead = 0;
        unsigned char* file_content = read_file(encoded_str, &bytesRead);

        value* result = decode_bencode(file_content);
        value* announce = value_get(result, "announce");
        value* length = value_get(result, "length");
        value* info = value_get(result, "info");
        value* piece_length = value_get(result, "piece length");
        value* pieces = value_get(result, "pieces");

        char* encoded_info = encode(info);

        printf("Info Hash: %s\n", calculate_hash(encoded_info, strlen(encoded_info)));
        printf("Info Hash: c77829d2a77d6516f88cd7a3de1a26abcbfab0db\n");
        printf("Tracker URL: %s\n", announce->string);
        printf("Length: %ld\n", length->number);
        printf("Piece Length: %ld\n", piece_length->number);

        for (size_t i = 0; i < pieces->string_length; i += 20) {
            print_bytes((unsigned char*)&pieces->string[i], 20);
        }

        value_delete(result);
        value_delete(announce);
        value_delete(length);
        value_delete(info);
    }
    else if (strcmp(command, "peers") == 0) {
        size_t bytesRead = 0;
        unsigned char* file_content = read_file(encoded_str, &bytesRead);

        value* result = decode_bencode(file_content);
        char* ip_address = "NULL";
        int port = -1;

        perform_get_request(result, ip_address, port);

        value_delete(result);
    }
    else {
        fprintf(stderr, "Unknown command: %s\n", command);
        return 1;
    }

    return 0;
}


int recvall(int sock, char* buffer, int length) {
    int bytes_read = 0;
    fprintf(stderr, "Requested %d\n", length);
    while (bytes_read < length) {
        int received = 0;
        while (received == 0)
            received = recv(sock, buffer + bytes_read, length, 0);
        bytes_read += received;
        fprintf(stderr, "Receiving %d\n", bytes_read);
        if (bytes_read <= 0) {
            exit(-5);
        }
    }
    return bytes_read;
}

int download_and_verify_piece(int sockfd, char* file_to_create, int piece_index, int piece_length) {
    int block_size = 1 << 14;
    int num_blocks = (piece_length + block_size - 1) / block_size;
    unsigned char* piece_data = malloc(piece_length);

    printf("num blocks = %d\n", num_blocks);

    for (int i = 0; i < num_blocks; i++) {
        int begin = i * block_size;
        int length = (i == num_blocks - 1) ? (piece_length % block_size) : block_size;
        if (length == 0) length = block_size;

        unsigned char request_msg[17];
        int msg_length = htonl(13);

        memcpy(request_msg, &msg_length, 4);
        request_msg[4] = 6;
        *(int*)&request_msg[5] = htonl(piece_index);
        *(int*)&request_msg[9] = htonl(begin);
        *(int*)&request_msg[13] = htonl(length);
        //printf("Request Message: ");
        //for (int j = 0; j < 17; j++) {
        //    printf("%02x ", request_msg[j]);  // Hexadecimal
        //}
        //printf("\n");

        //printf("Request Message (decimal): ");
        //for (int j = 0; j < 17; j++) {
        //    printf("%d ", request_msg[j]);    // Decimal
        //}
        //printf("\n");

        if (send(sockfd, request_msg, sizeof(request_msg), 0) <= 0) {
            perror("Falied to send request");
            free(piece_data);
            return -1;
        }

        unsigned char buffer[length + 13];
        int msg_len = recvall(sockfd, buffer, length + 13); //, 0);
        fprintf(stderr, "Received %d\n", msg_len);
       /* if (recvall(sockfd, buffer, length + 13) <= 0) {
            perror("Failed to receive block data");
            free(piece_data);
            return -1;
        }


        int received_index = ntohl(*(int*)&buffer[5]);
        int received_begin = ntohl(*(int*)&buffer[9]);
        if (buffer[4] != 7) {
            fprintf(stderr, "Unexpected message ID: %d. Expected 7 for piece message.\n", buffer[4]);
            continue;
        }

        if (received_index != piece_index || received_begin != begin) {
            fprintf(stderr, "Block mismatch: expected piece %d at offset %d, but received index %d, begin %d\n",
                piece_index, begin, received_index, received_begin);
            continue;
        }*/

        memcpy(piece_data + begin, buffer + 13, length);
    }

    // hash verification need to implement


    FILE* file = fopen(file_to_create, "wb");
    if (!file) {
        perror("Failed to open file for writing");
        free(piece_data);
        return -1;
    }
    fwrite(piece_data, 1, piece_length, file);
    fclose(file);

    free(piece_data);
    printf("Piece %d successfully downloaded and verified.\n", piece_index);
    return 0;
}

int wait_for_unchoke(int sockfd) {
    unsigned char buffer[5];

    if (recv(sockfd, buffer, 5, 0) <= 0) return 0;

    int id = buffer[4];

    printf("unchoke = %d\n", id);
    return (id == 1);
}

int send_interested(int sockfd) {
    unsigned char message[5];
    int length = htonl(1);

    memcpy(message, &length, 4);
    message[4] = 2;

    if (send(sockfd, message, 5, 0) <= 0) return -1;
    printf("sended\n");
    return 0;
}

int wait_for_bitfield(int sockfd) {
    unsigned char buffer[5];

    if (recv(sockfd, buffer, 5, 0) <= 0) return -1;

    int length = ntohl(*(int*)buffer);
    int id = buffer[4];

    if (id != 5) return -1;

    if (length > 1) {
        unsigned char discard[length - 1];
        if (recv(sockfd, discard, length - 1, 0) <= 0) return -1;
    }
    printf("succsess\n");

    return 0;
}

int peer_handshake(char* encoded_str, char* address, int piece_index, char* file_to_create) {
    printf("addr = %s\n", address);

    int port = 0;
    struct sockaddr_in peer_addr;
    char* peer_ip = get_ip_port(address, &port);
    size_t bytesRead = 0;
    unsigned char* file_content = read_file(encoded_str, &bytesRead);
    value* result = decode_bencode(file_content);
    value* piece_length = value_get(result, "piece length");

    printf("peer_ip = %s\n", peer_ip);
    printf("port = %d\n", port);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Failed to create socket");
        exit(1);
    }

    peer_addr.sin_family = AF_INET;
    peer_addr.sin_port = htons(port);
    inet_pton(AF_INET, peer_ip, &peer_addr.sin_addr);

    if (connect(sockfd, (struct sockaddr*)&peer_addr, sizeof(peer_addr)) < 0) {
        perror("Failed to connect to peer");
        close(sockfd);
        exit(1);
    }


    send_handshake(sockfd, result);

    receive_handshake(sockfd);

    wait_for_bitfield(sockfd);

    send_interested(sockfd);

    while (!wait_for_unchoke(sockfd)) continue;

    download_and_verify_piece(sockfd, file_to_create, piece_index, piece_length->number);

    close(sockfd);
    return 0;
}

int download_piece(char* file_to_create, char* encoded_str, int piece_number) {
    size_t bytesRead = 0;
    unsigned char* file_content = read_file(encoded_str, &bytesRead);
    value* result = decode_bencode(file_content);
    value* announce = value_get(result, "announce");
    Peer* list_peers;
    int port = 0;

    printf("URL tracker: %s\n", announce->string);

    char* ip_address = resolve_hostname_to_ip(announce->string, &port);

    list_peers = perform_get_request(result, ip_address, port);

    peer_println(list_peers);

    for (int i = 0; i < list_peers->count; i++) {
        char address[21];
        sprintf(address, "%s:%d", list_peers->peer[i]->ip, list_peers->peer[i]->port);
        peer_handshake(encoded_str, address, piece_number, file_to_create);
        break;
    }

    printf("file to create = %s\n", file_to_create);
    printf("encoded_str = %s\n", encoded_str);
    printf("piece_number = %d\n", piece_number);

    return 0;
}

int main(int argc, char* argv[]) {
    // Disable  output buffering
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    if (argc < 3) {
        fprintf(stderr, "Usage: your_bittorrent.sh <command> <args>\n");
        return 1;
    }

    char* command = argv[1];
    char* encoded_str = argv[2];
    printf("argc = %d\n", argc);

    if (argc == 4) {
        char* address = argv[3];
        peer_handshake(encoded_str, address, 0, "none");
    }
    else if (argc = 6) {
        char* file_to_create = argv[3];
        encoded_str = argv[4];
        int piece_number = atoi(argv[5]);
        download_piece(file_to_create, encoded_str, piece_number);
    }
    else {
        process_command(command, encoded_str);
    }

    return 0;
}

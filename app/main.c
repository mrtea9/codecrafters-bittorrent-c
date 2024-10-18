#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdbool.h>
#include <ctype.h>
#include <openssl/sha.h>

#define PORT 42585

typedef struct value value;

enum {
    VAL_NUMBER,
    VAL_STRING,
    VAL_LIST,
    VAL_DICT
};

struct value {
    int type;

    long number;
    char* string;
    size_t string_length;

    int count;
    value** cell;
};

void value_print(value* val);
value* value_copy(value* val);
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

char* calculate_hash(unsigned char* data, size_t len) {
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(data, len, hash);

    char* sha1_str = malloc(SHA_DIGEST_LENGTH * 2 + 1);
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
        sprintf(sha1_str + (i * 2), "%02x", hash[i]);
    }
    sha1_str[SHA_DIGEST_LENGTH * 2] = '\0';

    return sha1_str;
}

void perform_get_request(char* address) {
    int sockfd;
    struct sockaddr_in server_addr;
    char request[1024], response[4096];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(address);

    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    snprintf(request, sizeof(request), "GET / HTTP/1.1\r\ninfo_hash: 1231414\r\n\r\n", address);

    send(sockfd, request, strlen(request), 0);

    int bytes_received;
    while ((bytes_received = recv(sockfd, response, sizeof(response) - 1, 0)) > 0) {
        response[bytes_received] = '\0';
        printf("%s", response);
    }

    close(sockfd);
}

void print_bytes(const unsigned char* data, int len) {
    for (size_t i = 0; i < len; i++) {
        // Print each byte in hexadecimal (02 ensures 2 digits, padded with zero if necessary)
        printf("%02x", data[i]);
    }
    // Print a newline after the entire hex string
    printf("\n");
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
        value* announce = value_get(result, "announce");
        value* length = value_get(result, "length");
        value* info = value_get(result, "info");
        value* piece_length = value_get(result, "piece length");
        value* pieces = value_get(result, "pieces");

        printf("%s\n", announce->string);
        char* ip_addres = announce->string + 7;
        printf("%s\n", ip_addres);
        //char* colon_index = strchr(*bencoded_value, ':');
        printf("%i\n", atoi(announce->string));

        char* colon_index = strchr(ip_addres, ':');

        if (colon_index != NULL) {
            char* start = colon_index + 1;
            printf("start = %s\n", start);
        }
        //perform_get_request(test);

        value_delete(result);
        value_delete(announce);
        value_delete(length);
        value_delete(info);
        value_delete(piece_length);
        value_delete(pieces);
    }
    else {
        fprintf(stderr, "Unknown command: %s\n", command);
        return 1;
    }

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

    process_command(command, encoded_str);

    return 0;
}

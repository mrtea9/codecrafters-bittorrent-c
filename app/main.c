#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

char* decode_string(const char* bencoded_value) {
    int length = atoi(bencoded_value);
    const char* colon_index = strchr(bencoded_value, ':');

    if (colon_index != NULL) {
        const char* start = colon_index + 1;
        char* decoded_str = (char*)malloc(length + 1);

        strncpy(decoded_str, start, length);
        decoded_str[length] = '\0';
        printf("\"%s\"\n", decoded_str);

        return decoded_str;
    }
    else {
        fprintf(stderr, "Invalid encoded value: %s\n", bencoded_value);
        exit(1);
    }
}

char* decode_integer(const char* bencoded_value) {
    int length = strlen(bencoded_value) - 2;
    const char* colon_index = strchr(bencoded_value, 'i');

    if (colon_index != NULL) {
        const char* start = colon_index + 1;
        char* decoded_str = (char*)malloc(length + 1);

        strncpy(decoded_str, start, length);
        decoded_str[length] = '\0';

        printf("%s\n", decoded_str);

        return decoded_str;
    }
    else {
        fprintf(stderr, "Invalid encoded value: %s\n", bencoded_value);
        exit(1);
    }
}

char* decode_list(const char* bencoded_value) {
    int length = strlen(bencoded_value) - 2;

    printf("[]\n");

    exit(0);

}

char* decode_bencode(const char* bencoded_value) {
    int len = strlen(bencoded_value) - 1;

    if (is_digit(bencoded_value[0])) return decode_string(bencoded_value);
    if (bencoded_value[0] == 'i' && bencoded_value[len] == 'e') return decode_integer(bencoded_value);
    if (bencoded_value[0] == 'l' && bencoded_value[len] == 'e') return decode_list(bencoded_value);

    fprintf(stderr, "Only strings and integer are supported at the moment\n");
    exit(1);
    }

int process_command(const char* command,const char* encoded_str) {

    if (strcmp(command, "decode") == 0) {
        char* decoded_str = decode_bencode(encoded_str);

        free(decoded_str);
    }
    else {
        fprintf(stderr, "Unknown command: %s\n", command);
        return 1;
    }

    return 0;
}

int main(int argc, char* argv[]) {
	// Disable output buffering
	setbuf(stdout, NULL);
 	setbuf(stderr, NULL);

    if (argc < 3) {
        fprintf(stderr, "Usage: your_bittorrent.sh <command> <args>\n");
        return 1;
    }

    const char* command = argv[1];
    const char* encoded_str = argv[2];

    process_command(command, encoded_str);

    return 0;
}

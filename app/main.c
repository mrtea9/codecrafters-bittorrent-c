#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

enum {
    VAL_NUMBER,
    VAL_STRING,
    VAL_LIST
};

typedef struct value {
    int type;

    int number;
    char* string;

} value;

void value_print(value* value) {

    switch (value->type) {
        case VAL_NUMBER:
            printf("%s\n", value->number);
            break;
        case VAL_STRING:
            printf("\"%s\"\n", value->string);
            break;
    }

}

void value_println(value* value) {
    value_print(value);
    putchar('\n');
}

bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

void decode_string(char* bencoded_value) {
    int length = atoi(bencoded_value);
    char* colon_index = strchr(bencoded_value, ':');

    if (colon_index != NULL) {
        char* start = colon_index + 1;
        char* decoded_str = (char*)malloc(length + 1);

        strncpy(decoded_str, start, length);
        decoded_str[length] = '\0';
        printf("\"%s\"\n", decoded_str);
    }
    else {
        fprintf(stderr, "Invalid encoded value: %s\n", bencoded_value);
        exit(1);
    }
}

void decode_integer(char* bencoded_value) {
    int length = strlen(bencoded_value) - 2;
    char* colon_index = strchr(bencoded_value, 'i');

    if (colon_index != NULL) {
        char* start = colon_index + 1;
        char* decoded_str = (char*)malloc(length + 1);

        strncpy(decoded_str, start, length);
        decoded_str[length] = '\0';

        printf("%s\n", decoded_str);
    }
    else {
        fprintf(stderr, "Invalid encoded value: %s\n", bencoded_value);
        exit(1);
    }
}

void decode_list(char* bencoded_value) {
    int length = strlen(bencoded_value) - 2;
    char* encoded = bencoded_value + 1;
    encoded[length] = '\0';

    if (length == 0) {
        printf("[]\n");
        exit(0);
    }

    for (int i = 0; i < length; i++) {

        if (is_digit(encoded[i])) {
            int length = atoi(encoded);

            char* colon_index = strchr(encoded, ':');

            if (colon_index != NULL) {
                char* start = colon_index + 1;
                char* decoded_str = (char*)malloc(length + 1);

                strncpy(decoded_str, start, length);
                decoded_str[length] = '\0';
                printf("\"%s\"\n", decoded_str);
            }

            exit(1);
        }

        printf("%c ", encoded[i]);

    }

    printf("%s\n%i\n", encoded, length);


}

void decode_bencode(char* bencoded_value) {
    int len = strlen(bencoded_value) - 1;

    if (is_digit(bencoded_value[0])) decode_string(bencoded_value);
    if (bencoded_value[0] == 'i' && bencoded_value[len] == 'e') decode_integer(bencoded_value);
    if (bencoded_value[0] == 'l' && bencoded_value[len] == 'e') decode_list(bencoded_value);

    fprintf(stderr, "Only strings and integer are supported at the moment\n");
    exit(1);
    }

int process_command(char* command, char* encoded_str) {

    if (strcmp(command, "decode") == 0) {
        decode_bencode(encoded_str);

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

    char* command = argv[1];
    char* encoded_str = argv[2];

    process_command(command, encoded_str);

    return 0;
}

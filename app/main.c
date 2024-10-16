#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

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

    int count;
    value** cell;
};

void value_print(value* val);
value* value_copy(value* val);
value* decode_list(char** bencoded_value);
value* decode_dict(char** bencoded_value);

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

value* value_string(char* string) {
    value* val = malloc(sizeof(value));
    val->type = VAL_STRING;
    val->string = malloc(strlen(string) + 1);
    strcpy(val->string, string);
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
            x->string = malloc(strlen(val->string) + 1);
            strcpy(x->string, val->string);
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
        value* result = value_string(decoded_str);
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

unsigned char* read_file(const char* filename, size_t* bytesRead) {
    FILE* file = fopen(filename, "r");

    if (file == NULL) return NULL;

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return NULL;
    }

    int filesize = ftell(file);

    if (filesize == 0) {
        fclose(file);
        return NULL;
    }

    fseek(file, 0, SEEK_SET);

    char* buffer = malloc(filesize);

    if (buffer == 0) {
        fclose(file);
        return NULL;
    }

    *bytesRead = fread(buffer, sizeof(char), filesize, file);

    fclose(file);
    return buffer;
}

char* hex_dump_to_char(const unsigned char* buffer, size_t length) {
    char* output = malloc(length * 3 + 1);
    if (!output) return NULL;

    size_t pos = 0;

    for (size_t i = 0; i < length; i++) {
        if (isprint(buffer[i])) {
            output[pos++] = buffer[i];
        }
        else {
            output[pos++] = '?';
        }
    }
    output[pos] = '\0';

    return output;
}

int process_command(char* command, char* encoded_str) {
    if (strcmp(command, "decode") == 0) {
        value* result = decode_bencode(encoded_str);
        value* announce = value_get(result, "announce");
        value* length = value_get(result, "length");

        printf("%s\n", encoded_str);
       // value_println(result);
     //   value_print_info(announce);
       // putchar('\n');
      //  value_print_info(length);
       // putchar('\n');

        value_delete(result);
        value_delete(announce);
        value_delete(length);
    }
    else if (strcmp(command, "info") == 0) {
        
        size_t bytesRead = 0;
        unsigned char* file_content = read_file(encoded_str, &bytesRead);
        value* result = decode_bencode(file_content);
        value* announce = value_get(result, "announce");
        value* length = value_get(result, "length");
        value* info = value_get(result, "info");
        
        value_println(result);
        value_println(info);

        value_print_info(announce);
        putchar('\n');
        value_print_info(length);
        putchar('\n');

        value_delete(result);
        value_delete(announce);
        value_delete(length);
        value_delete(info);
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

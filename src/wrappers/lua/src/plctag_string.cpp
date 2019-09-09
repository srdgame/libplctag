#include <stdio.h>
#include <stdlib.h>
#include <string>

extern "C" {
#include "platform.h"
}
#include "lib/libplctag.h"
#include "lib/tag.h"

#define ARRAY_1_DIM_SIZE (48)
#define ARRAY_2_DIM_SIZE (6)
#define STRING_DATA_SIZE (82)
#define ELEM_COUNT 1
#define ELEM_SIZE 88
#define DATA_TIMEOUT 5000


std::string plc_tag_get_string(int32_t tag, int offset)
{
    char str_data[STRING_DATA_SIZE];
    int str_index;
    int str_len;
    int num_strings = plc_tag_get_size(tag) / ELEM_SIZE;
    int i;

	if (offset > num_strings) {
		return std::string("");
	}

	/* get the string length */
	str_len = plc_tag_get_int32(tag, offset * ELEM_SIZE);

	/* copy the data */
	for(str_index=0; str_index<str_len; str_index++) {
		str_data[str_index] = (char)plc_tag_get_uint8(tag,(offset * ELEM_SIZE)+4+str_index);
	}

	/* pad with zeros */
	for(;str_index<STRING_DATA_SIZE; str_index++) {
		str_data[str_index] = 0;
	}

    return std::string(str_data);
}



int plc_tag_set_string(int32_t tag, int offset, const std::string& str_str)
{
    int str_len;
    int base_offset = offset * ELEM_SIZE;
    int str_index;
	int rc;
	const char* str = str_str.c_str();

    /* now write the data */
    str_len = str_str.length();

    /* set the length */
    rc = plc_tag_set_int32(tag, base_offset, str_len);

    /* copy the data */
    for(str_index=0; str_index < str_len && str_index < STRING_DATA_SIZE; str_index++) {
        rc = plc_tag_set_uint8(tag,base_offset + 4 + str_index, (uint8_t)str[str_index]);
    }

    /* pad with zeros */
    for(;str_index<STRING_DATA_SIZE; str_index++) {
        rc = plc_tag_set_uint8(tag,base_offset + 4 + str_index, 0);
    }

	return rc;
}

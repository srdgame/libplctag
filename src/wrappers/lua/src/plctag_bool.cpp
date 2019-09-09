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


bool plc_tag_get_bool(int32_t tag, int offset)
{
	uint8_t rc = plc_tag_get_uint8(tag, offset);

	return rc == 255;
}



int plc_tag_set_bool(int32_t tag, int offset, bool bval)
{
	return plc_tag_set_uint8(tag, offset, bval ? 255 : 0);
}

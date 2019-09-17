
#include <stdlib.h>
#include <string.h>
#include "log.h"
#include "tags.h"


tag_data *tags = NULL;
size_t num_tags = 0;

void init_tags()
{
    num_tags = 8;

    tags = (tag_data *)calloc(num_tags, sizeof(tag_data));

    tags[0].name = "TestDINTArray";
    tags[0].data_type[0] = 0xc4;
    tags[0].data_type[1] = 0x00;
    tags[0].elem_count = 10;
    tags[0].elem_size = 4;
    tags[0].data = (uint8_t *)calloc(tags[0].elem_size, tags[0].elem_count);

    tags[1].name = "TestBigArray";
    tags[1].data_type[0] = 0xc4;
    tags[1].data_type[1] = 0x00;
    tags[1].elem_count = 1000;
    tags[1].elem_size = 4;
    tags[1].data = (uint8_t *)calloc(tags[1].elem_size, tags[1].elem_count);

    tags[2].name = "TestByteArray";
    tags[2].data_type[0] = 0xc2;
    tags[2].data_type[1] = 0x00;
    tags[2].elem_count = 10;
    tags[2].elem_size = 1;
    tags[2].data = (uint8_t *)calloc(tags[2].elem_size, tags[2].elem_count);

    tags[3].name = "TestShortArray";
    tags[3].data_type[0] = 0xc3;
    tags[3].data_type[1] = 0x00;
    tags[3].elem_count = 10;
    tags[3].elem_size = 2;
    tags[3].data = (uint8_t *)calloc(tags[3].elem_size, tags[3].elem_count);

    tags[4].name = "TestIntArray";
    tags[4].data_type[0] = 0xc4;
    tags[4].data_type[1] = 0x00;
    tags[4].elem_count = 10;
    tags[4].elem_size = 4;
    tags[4].data = (uint8_t *)calloc(tags[4].elem_size, tags[4].elem_count);

    tags[5].name = "TestLongArray";
    tags[5].data_type[0] = 0xc5;
    tags[5].data_type[1] = 0x00;
    tags[5].elem_count = 10;
    tags[5].elem_size = 8;
    tags[5].data = (uint8_t *)calloc(tags[5].elem_size, tags[5].elem_count);

    tags[6].name = "TestFloatArray";
    tags[6].data_type[0] = 0xca;
    tags[6].data_type[1] = 0x00;
    tags[6].elem_count = 10;
    tags[6].elem_size = 4;
    tags[6].data = (uint8_t *)calloc(tags[6].elem_size, tags[6].elem_count);

    tags[7].name = "TestFloat64Array";
    tags[7].data_type[0] = 0xcb;
    tags[7].data_type[1] = 0x00;
    tags[7].elem_count = 10;
    tags[7].elem_size = 8;
    tags[7].data = (uint8_t *)calloc(tags[7].elem_size, tags[7].elem_count);

}


tag_data *find_tag(const char *tag_name)
{
    log("find_data() finding tag %s\n", tag_name);

    for(size_t i=0; i<num_tags; i++) {
        if(strcmp(tags[i].name, tag_name) == 0) {
            return &(tags[i]);
        }
    }

    log("find_tag() unable to find tag %s\n", tag_name);

    return NULL;
}

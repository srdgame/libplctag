# libplctag library Lua binding

pltag api document [link](https://github.com/kyle-github/libplctag/wiki/API)

## Functions

* [decode_error](#decode_error)
* [create](https://github.com/kyle-github/libplctag/wiki/API#creating-a-tag)
* [lock](https://github.com/kyle-github/libplctag/wiki/API#controlling-multithreaded-access-to-a-tag)
* [unlock](https://github.com/kyle-github/libplctag/wiki/API#controlling-multithreaded-access-to-a-tag)
* [abort](https://github.com/kyle-github/libplctag/wiki/API#aborting-an-operation)
* [destroy](https://github.com/kyle-github/libplctag/wiki/API#destroying-a-tag)
* [read](https://github.com/kyle-github/libplctag/wiki/API#reading-a-tag)
* [write](https://github.com/kyle-github/libplctag/wiki/API#writing-a-tag)
* [status](https://github.com/kyle-github/libplctag/wiki/API#retrieving-tag-status)
* [get_size](https://github.com/kyle-github/libplctag/wiki/API#retrieving-tag-size)

* [get_uint32](https://github.com/kyle-github/libplctag/wiki/API#data-accessor-functions)
* [set_uint32](https://github.com/kyle-github/libplctag/wiki/API#data-accessor-functions)
* [get_int32](https://github.com/kyle-github/libplctag/wiki/API#data-accessor-functions)
* [set_int32](https://github.com/kyle-github/libplctag/wiki/API#data-accessor-functions)
* [get_uint16](https://github.com/kyle-github/libplctag/wiki/API#data-accessor-functions)
* [set_uint16](https://github.com/kyle-github/libplctag/wiki/API#data-accessor-functions)
* [get_int16](https://github.com/kyle-github/libplctag/wiki/API#data-accessor-functions)
* [set_int16](https://github.com/kyle-github/libplctag/wiki/API#data-accessor-functions)
* [get_uint8](https://github.com/kyle-github/libplctag/wiki/API#data-accessor-functions)
* [set_uint8](https://github.com/kyle-github/libplctag/wiki/API#data-accessor-functions)
* [get_int8](https://github.com/kyle-github/libplctag/wiki/API#data-accessor-functions)
* [set_int8](https://github.com/kyle-github/libplctag/wiki/API#data-accessor-functions)
* [get_float32](https://github.com/kyle-github/libplctag/wiki/API#data-accessor-functions)
* [set_float32](https://github.com/kyle-github/libplctag/wiki/API#data-accessor-functions)

* [sleep](#sleep)
* [time](#time)

#### decode_error
> Get error code text. Refer to plc_tag_decode_error. 


#### sleep
> void sleep(ms)


#### time
> time_64_t time()



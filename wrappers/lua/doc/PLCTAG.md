# libplctag library Lua binding

pltag api document [link](https://github.com/kyle-github/libplctag/wiki/API)

## Functions

* [decode\_error](#decode\_error)
* [create](https://github.com/kyle-github/libplctag/wiki/API#creating-a-tag)
* [lock](https://github.com/kyle-github/libplctag/wiki/API#controlling-multithreaded-access-to-a-tag)
* [unlock](https://github.com/kyle-github/libplctag/wiki/API#controlling-multithreaded-access-to-a-tag)
* [abort](https://github.com/kyle-github/libplctag/wiki/API#aborting-an-operation)
* [destroy](https://github.com/kyle-github/libplctag/wiki/API#destroying-a-tag)
* [read](https://github.com/kyle-github/libplctag/wiki/API#reading-a-tag)
* [write](https://github.com/kyle-github/libplctag/wiki/API#writing-a-tag)
* [status](https://github.com/kyle-github/libplctag/wiki/API#retrieving-tag-status)
* [get\_size](https://github.com/kyle-github/libplctag/wiki/API#retrieving-tag-size)

* [get\_uint32](https://github.com/kyle-github/libplctag/wiki/API#data-accessor-functions)
* [set\_uint32](https://github.com/kyle-github/libplctag/wiki/API#data-accessor-functions)
* [get\_int32](https://github.com/kyle-github/libplctag/wiki/API#data-accessor-functions)
* [set\_int32](https://github.com/kyle-github/libplctag/wiki/API#data-accessor-functions)
* [get\_uint16](https://github.com/kyle-github/libplctag/wiki/API#data-accessor-functions)
* [set\_uint16](https://github.com/kyle-github/libplctag/wiki/API#data-accessor-functions)
* [get\_int16](https://github.com/kyle-github/libplctag/wiki/API#data-accessor-functions)
* [set\_int16](https://github.com/kyle-github/libplctag/wiki/API#data-accessor-functions)
* [get\_uint8](https://github.com/kyle-github/libplctag/wiki/API#data-accessor-functions)
* [set\_uint8](https://github.com/kyle-github/libplctag/wiki/API#data-accessor-functions)
* [get\_int8](https://github.com/kyle-github/libplctag/wiki/API#data-accessor-functions)
* [set\_int8](https://github.com/kyle-github/libplctag/wiki/API#data-accessor-functions)
* [get\_float32](https://github.com/kyle-github/libplctag/wiki/API#data-accessor-functions)
* [set\_float32](https://github.com/kyle-github/libplctag/wiki/API#data-accessor-functions)

* [sleep](#sleep)
* [time](#time)

#### decode\_error
> Get error code text. Refer to plc\_tag\_decode\_error. 


#### sleep
> void sleep(ms)


#### time
> time\_64\_t time()



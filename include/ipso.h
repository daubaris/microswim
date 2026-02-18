#pragma once
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint16_t ipso_oid_t;
typedef uint16_t ipso_iid_t;
typedef uint16_t ipso_rid_t;

typedef enum {
    IPSO_T_I64,
    IPSO_T_U64,
    IPSO_T_F64,
    IPSO_T_BOOL,
    IPSO_T_STR,
    IPSO_T_OPAQUE
} ipso_type_t;

typedef struct {
    const void* ptr;
    size_t len;
} ipso_bytes_t;

typedef union {
    int64_t i64;
    uint64_t u64;
    double f64;
    uint8_t b;
    ipso_bytes_t bytes;
} ipso_value_t;

typedef struct ipso_resource ipso_resource_t;
typedef int (*ipso_set_fn_t)(ipso_resource_t* res, const ipso_value_t* v);

struct ipso_resource {
    ipso_rid_t rid;
    ipso_type_t type;
    void* storage;
    size_t storage_len;
    ipso_set_fn_t set;
};

typedef struct {
    ipso_oid_t oid;
    ipso_iid_t iid;
    ipso_resource_t* resources;
    size_t resource_count;
} ipso_instance_t;

typedef struct {
    ipso_instance_t* instances;
    size_t instance_count;
} ipso_registry_t;

ipso_instance_t* ipso_find_instance(ipso_registry_t* reg, ipso_oid_t oid, ipso_iid_t iid);
ipso_resource_t* ipso_find_resource(ipso_instance_t* inst, ipso_rid_t rid);

int ipso_read(ipso_registry_t* reg, ipso_oid_t oid, ipso_iid_t iid, ipso_rid_t rid, ipso_value_t* out);
int ipso_write(ipso_registry_t* reg, ipso_oid_t oid, ipso_iid_t iid, ipso_rid_t rid, const ipso_value_t* in);

#ifdef __cplusplus
}
#endif

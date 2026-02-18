#include "ipso.h"
#include <string.h>

enum { IPSO_OK = 0, IPSO_ENOENT = -1, IPSO_ERO = -2, IPSO_EINVAL = -3 };

ipso_instance_t* ipso_find_instance(ipso_registry_t* reg, ipso_oid_t oid, ipso_iid_t iid) {
    if (!reg)
        return NULL;
    for (size_t i = 0; i < reg->instance_count; i++) {
        ipso_instance_t* inst = &reg->instances[i];
        if (inst->oid == oid && inst->iid == iid)
            return inst;
    }
    return NULL;
}

ipso_resource_t* ipso_find_resource(ipso_instance_t* inst, ipso_rid_t rid) {
    if (!inst)
        return NULL;
    for (size_t i = 0; i < inst->resource_count; i++) {
        if (inst->resources[i].rid == rid)
            return &inst->resources[i];
    }
    return NULL;
}

static int read_scalar(ipso_type_t t, const void* storage, ipso_value_t* out) {
    if (!storage || !out)
        return IPSO_EINVAL;
    switch (t) {
        case IPSO_T_I64:
            out->i64 = *(const int64_t*)storage;
            return IPSO_OK;
        case IPSO_T_U64:
            out->u64 = *(const uint64_t*)storage;
            return IPSO_OK;
        case IPSO_T_F64:
            out->f64 = *(const double*)storage;
            return IPSO_OK;
        case IPSO_T_BOOL:
            out->b = *(const uint8_t*)storage ? 1 : 0;
            return IPSO_OK;
        default:
            return IPSO_EINVAL;
    }
}

int ipso_read(ipso_registry_t* reg, ipso_oid_t oid, ipso_iid_t iid, ipso_rid_t rid, ipso_value_t* out) {
    ipso_instance_t* inst = ipso_find_instance(reg, oid, iid);
    if (!inst)
        return IPSO_ENOENT;
    ipso_resource_t* res = ipso_find_resource(inst, rid);
    if (!res)
        return IPSO_ENOENT;

    if (res->type == IPSO_T_STR || res->type == IPSO_T_OPAQUE) {
        out->bytes.ptr = res->storage;
        out->bytes.len = res->storage_len;
        return IPSO_OK;
    }
    return read_scalar(res->type, res->storage, out);
}

static int default_write(ipso_resource_t* res, const ipso_value_t* in) {
    if (!res || !in || !res->storage)
        return IPSO_EINVAL;
    switch (res->type) {
        case IPSO_T_I64:
            *(int64_t*)res->storage = in->i64;
            return IPSO_OK;
        case IPSO_T_U64:
            *(uint64_t*)res->storage = in->u64;
            return IPSO_OK;
        case IPSO_T_F64:
            *(double*)res->storage = in->f64;
            return IPSO_OK;
        case IPSO_T_BOOL:
            *(uint8_t*)res->storage = in->b ? 1 : 0;
            return IPSO_OK;

        case IPSO_T_STR:
        case IPSO_T_OPAQUE:
            if (in->bytes.len > res->storage_len)
                return IPSO_EINVAL;
            memcpy(res->storage, in->bytes.ptr, in->bytes.len);
            if (res->type == IPSO_T_STR && in->bytes.len < res->storage_len) {
                ((char*)res->storage)[in->bytes.len] = '\0';
            }
            return IPSO_OK;

        default:
            return IPSO_EINVAL;
    }
}

int ipso_write(ipso_registry_t* reg, ipso_oid_t oid, ipso_iid_t iid, ipso_rid_t rid, const ipso_value_t* in) {
    ipso_instance_t* inst = ipso_find_instance(reg, oid, iid);
    if (!inst)
        return IPSO_ENOENT;
    ipso_resource_t* res = ipso_find_resource(inst, rid);
    if (!res)
        return IPSO_ENOENT;
    if (!res->set &&
        (res->type == IPSO_T_STR || res->type == IPSO_T_OPAQUE || res->type == IPSO_T_I64 ||
         res->type == IPSO_T_U64 || res->type == IPSO_T_F64 || res->type == IPSO_T_BOOL)) {
        return default_write(res, in);
    }
    if (!res->set)
        return IPSO_ERO;
    return res->set(res, in);
}

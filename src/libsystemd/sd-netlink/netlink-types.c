/* SPDX-License-Identifier: LGPL-2.1-or-later */

#include <linux/netlink.h>

#include "netlink-internal.h"
#include "netlink-types-internal.h"

static const NLType empty_types[1] = {
        /* fake array to avoid .types==NULL, which denotes invalid type-systems */
};

const NLTypeSystem empty_type_system = {
        .count = 0,
        .types = empty_types,
};

static const NLType error_types[] = {
        [NLMSGERR_ATTR_MSG]  = { .type = NETLINK_TYPE_STRING },
        [NLMSGERR_ATTR_OFFS] = { .type = NETLINK_TYPE_U32 },
};

const NLTypeSystem error_type_system = {
        .count = ELEMENTSOF(error_types),
        .types = error_types,
};

uint16_t type_get_type(const NLType *type) {
        assert(type);
        return type->type;
}

size_t type_get_size(const NLType *type) {
        assert(type);
        return type->size;
}

const NLTypeSystem *type_get_type_system(const NLType *nl_type) {
        assert(nl_type);
        assert(nl_type->type == NETLINK_TYPE_NESTED);
        assert(nl_type->type_system);
        return nl_type->type_system;
}

const NLTypeSystemUnion *type_get_type_system_union(const NLType *nl_type) {
        assert(nl_type);
        assert(nl_type->type == NETLINK_TYPE_UNION);
        assert(nl_type->type_system_union);
        return nl_type->type_system_union;
}

uint16_t type_system_get_count(const NLTypeSystem *type_system) {
        assert(type_system);
        return type_system->count;
}

int type_system_root_get_type(sd_netlink *nl, const NLType **ret, uint16_t type) {
        if (!nl)
                return rtnl_get_type(type, ret);

        switch(nl->protocol) {
        case NETLINK_ROUTE:
                return rtnl_get_type(type, ret);
        case NETLINK_NETFILTER:
                return nfnl_get_type(type, ret);
        case NETLINK_GENERIC:
                return genl_get_type(nl, type, ret);
        default:
                return -EOPNOTSUPP;
        }
}

int type_system_get_type(const NLTypeSystem *type_system, const NLType **ret, uint16_t type) {
        const NLType *nl_type;

        assert(ret);
        assert(type_system);
        assert(type_system->types);

        if (type >= type_system->count)
                return -EOPNOTSUPP;

        nl_type = &type_system->types[type];

        if (nl_type->type == NETLINK_TYPE_UNSPEC)
                return -EOPNOTSUPP;

        *ret = nl_type;
        return 0;
}

int type_system_get_type_system(const NLTypeSystem *type_system, const NLTypeSystem **ret, uint16_t type) {
        const NLType *nl_type;
        int r;

        assert(ret);

        r = type_system_get_type(type_system, &nl_type, type);
        if (r < 0)
                return r;

        *ret = type_get_type_system(nl_type);
        return 0;
}

int type_system_get_type_system_union(const NLTypeSystem *type_system, const NLTypeSystemUnion **ret, uint16_t type) {
        const NLType *nl_type;
        int r;

        assert(ret);

        r = type_system_get_type(type_system, &nl_type, type);
        if (r < 0)
                return r;

        *ret = type_get_type_system_union(nl_type);
        return 0;
}

NLMatchType type_system_union_get_match_type(const NLTypeSystemUnion *type_system_union) {
        assert(type_system_union);
        return type_system_union->match_type;
}

uint16_t type_system_union_get_match_attribute(const NLTypeSystemUnion *type_system_union) {
        assert(type_system_union);
        assert(type_system_union->match_type == NL_MATCH_SIBLING);
        return type_system_union->match_attribute;
}

int type_system_union_get_type_system_by_string(const NLTypeSystemUnion *type_system_union, const NLTypeSystem **ret, const char *key) {
        int type;

        assert(type_system_union);
        assert(type_system_union->match_type == NL_MATCH_SIBLING);
        assert(type_system_union->lookup);
        assert(type_system_union->type_systems);
        assert(ret);
        assert(key);

        type = type_system_union->lookup(key);
        if (type < 0)
                return -EOPNOTSUPP;

        assert(type < type_system_union->num);

        *ret = &type_system_union->type_systems[type];
        return 0;
}

int type_system_union_get_type_system_by_protocol(const NLTypeSystemUnion *type_system_union, const NLTypeSystem **ret, uint16_t protocol) {
        const NLTypeSystem *type_system;

        assert(type_system_union);
        assert(type_system_union->type_systems);
        assert(type_system_union->match_type == NL_MATCH_PROTOCOL);
        assert(ret);

        if (protocol >= type_system_union->num)
                return -EOPNOTSUPP;

        type_system = &type_system_union->type_systems[protocol];
        if (!type_system->types)
                return -EOPNOTSUPP;

        *ret = type_system;
        return 0;
}

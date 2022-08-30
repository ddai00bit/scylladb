/*
 *
 * Modified by ScyllaDB
 * Copyright (C) 2015-present ScyllaDB
 */

/*
 * SPDX-License-Identifier: (AGPL-3.0-or-later and Apache-2.0)
 */

#include "locator/snitch_base.hh"
#include "gms/gossiper.hh"
#include "gms/application_state.hh"

namespace locator {
void snitch_base::sort_by_proximity(
    inet_address address, inet_address_vector_replica_set& addresses) {

    std::sort(addresses.begin(), addresses.end(),
              [this, &address](inet_address& a1, inet_address& a2)
    {
        return compare_endpoints(address, a1, a2) < 0;
    });
}

int snitch_base::compare_endpoints(
    inet_address& address, inet_address& a1, inet_address& a2) {

    //
    // if one of the Nodes IS the Node we are comparing to and the other one
    // IS NOT - then return the appropriate result.
    //
    if (address == a1 && address != a2) {
        return -1;
    }

    if (address == a2 && address != a1) {
        return 1;
    }

    // ...otherwise perform the similar check in regard to Data Center
    sstring address_datacenter = get_datacenter(address);
    sstring a1_datacenter = get_datacenter(a1);
    sstring a2_datacenter = get_datacenter(a2);

    if (address_datacenter == a1_datacenter &&
        address_datacenter != a2_datacenter) {
        return -1;
    } else if (address_datacenter == a2_datacenter &&
               address_datacenter != a1_datacenter) {
        return 1;
    } else if (address_datacenter == a2_datacenter &&
               address_datacenter == a1_datacenter) {
        //
        // ...otherwise (in case Nodes belong to the same Data Center) check
        // the racks they belong to.
        //
        sstring address_rack = get_rack(address);
        sstring a1_rack = get_rack(a1);
        sstring a2_rack = get_rack(a2);

        if (address_rack == a1_rack && address_rack != a2_rack) {
            return -1;
        }

        if (address_rack == a2_rack && address_rack != a1_rack) {
            return 1;
        }
    }
    //
    // We don't differentiate between Nodes if all Nodes belong to different
    // Data Centers, thus make them equal.
    //
    return 0;
}

std::list<std::pair<gms::application_state, gms::versioned_value>> snitch_base::get_app_states() const {
    return {
        {gms::application_state::DC, gms::versioned_value::datacenter(_my_dc)},
        {gms::application_state::RACK, gms::versioned_value::rack(_my_rack)},
    };
}

snitch_ptr::snitch_ptr(const snitch_config cfg, sharded<gms::gossiper>& g)
        : _gossiper(g) {
    i_endpoint_snitch::ptr_type s;
    try {
        s = create_object<i_endpoint_snitch>(cfg.name, cfg);
    } catch (no_such_class& e) {
        i_endpoint_snitch::logger().error("Can't create snitch {}: not supported", cfg.name);
        throw;
    } catch (...) {
        throw;
    }
    s->set_backreference(*this);
    _ptr = std::move(s);
}

} // namespace locator

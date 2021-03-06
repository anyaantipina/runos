#include "FluidOXMAdapter.hh"
#include "oxm/field_set.hh"
#include "openflow/common.hh"
#include "types/exception.hh"

using namespace fluid_msg;

namespace runos {

typedef boost::error_info< struct tag_oxm_ns, unsigned >
    errinfo_oxm_ns;
typedef boost::error_info< struct tag_oxm_field, unsigned >
    errinfo_oxm_field;

FluidOXMAdapter::FluidOXMAdapter(const oxm::field<> f)
    : OXMTLV(static_cast<uint16_t>(f.type().ns()),
             static_cast<uint8_t>(f.type().id()),
             not f.exact(),
             f.type().nbytes() * (f.exact() ? 1 : 2)),
      field_(f)
{
    //check openflow field
    switch (f.type().ns()) {
        case unsigned(of::oxm::ns::NXM_0): break;
        case unsigned(of::oxm::ns::NXM_1): break;
        case unsigned(of::oxm::ns::OPENFLOW_BASIC): break;
        case unsigned(of::oxm::ns::EXPERIMENTER): break;
        default :
            RUNOS_THROW(
                    runtime_error() <<
                    errinfo_msg("non openflow oxm field") <<
                    errinfo_oxm_ns(f.type().ns()) <<
                    errinfo_oxm_field(f.type().id()));
    }
    create_oxm_req(0, 0, 0, 0);
}

of13::OXMTLV& FluidOXMAdapter::operator=(const of13::OXMTLV& other)
{
    const FluidOXMAdapter& rhs =
        dynamic_cast<const FluidOXMAdapter&>(other);
    OXMTLV::operator=(rhs);
    field_ = rhs.field_;
    return *this;
}

bool FluidOXMAdapter::equals(const of13::OXMTLV& other)
{
    if (const FluidOXMAdapter* other_adapter =
            dynamic_cast<const FluidOXMAdapter*>(&other))
    {
        return field_ == other_adapter->field_;
    } else {
        return false;
    }
}

FluidOXMAdapter* FluidOXMAdapter::clone() const
{
    return new FluidOXMAdapter(*this);
}

size_t FluidOXMAdapter::pack(uint8_t* buffer)
{
    OXMTLV::pack(buffer);
    field_.value_bits().to_buffer(buffer + of13::OFP_OXM_HEADER_LEN);
    if (not field_.exact()) {
        field_.mask_bits().to_buffer(buffer + of13::OFP_OXM_HEADER_LEN
                                            + field_.type().nbytes());
    }
    return 0;
}

of_error FluidOXMAdapter::unpack(uint8_t* buffer)
{
    OXMTLV::unpack(buffer);

    auto value_bits = bits<>(field_.type().nbits(),
                             buffer + of13::OFP_OXM_HEADER_LEN);
    if (has_mask_) {
        auto mask_bits = bits<>(field_.type().nbits(),
                                buffer
                                    + of13::OFP_OXM_HEADER_LEN
                                    + field_.type().nbytes());

        field_ = oxm::field<>(field_.type(), std::move(value_bits),
                                             std::move(mask_bits));
    } else {
        field_ = oxm::field<>(field_.type(), std::move(value_bits));
    }
    return 0;
}

of13::Match make_of_match(const oxm::field_set &match)
{
    of13::Match ret;
    for (const oxm::field<>& field : match)
        ret.add_oxm_field(new FluidOXMAdapter(field));
    return ret;
}

} // namespace runos

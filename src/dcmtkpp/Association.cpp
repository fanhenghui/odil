/*************************************************************************
 * dcmtkpp - Copyright (C) Universite de Strasbourg
 * Distributed under the terms of the CeCILL-B license, as published by
 * the CEA-CNRS-INRIA. Refer to the LICENSE file or to
 * http://www.cecill.info/licences/Licence_CeCILL-B_V1-en.html
 * for details.
 ************************************************************************/

#include "Association.h"

#include <algorithm>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "dcmtkpp/DataSet.h"
#include "dcmtkpp/Exception.h"
#include "dcmtkpp/uid.h"
#include "dcmtkpp/dul/StateMachine.h"
#include "dcmtkpp/message/Message.h"
#include "dcmtkpp/pdu/AAbort.h"
#include "dcmtkpp/pdu/AAssociate.h"
#include "dcmtkpp/pdu/AAssociateRJ.h"
#include "dcmtkpp/pdu/AReleaseRQ.h"
#include "dcmtkpp/pdu/ImplementationClassUID.h"
#include "dcmtkpp/pdu/ImplementationVersionName.h"
#include "dcmtkpp/pdu/PDataTF.h"
#include "dcmtkpp/pdu/PresentationContext.h"
#include "dcmtkpp/pdu/UserIdentityRQ.h"
#include "dcmtkpp/pdu/UserInformation.h"
#include "dcmtkpp/Reader.h"
#include "dcmtkpp/Writer.h"

namespace dcmtkpp
{

Association
::Association()
: _state_machine(), _peer_host(""), _peer_port(104), _peer_ae_title(""),
  _own_ae_title(""), _presentation_contexts(),
  _user_identity_type(UserIdentityType::None), _user_identity_primary_field(""),
  _user_identity_secondary_field("")
{
    // Nothing else
    this->_state_machine.set_timeout(boost::posix_time::seconds(5));
    this->_state_machine.get_transport().set_timeout(boost::posix_time::seconds(20));
}

Association
::Association(Association const & other)
: _state_machine(), _peer_host(other._peer_host),
  _peer_port(other._peer_port), _peer_ae_title(other._peer_ae_title),
  _own_ae_title(other._own_ae_title),
  _presentation_contexts(other._presentation_contexts),
  _user_identity_type(other._user_identity_type),
  _user_identity_primary_field(other._user_identity_primary_field),
  _user_identity_secondary_field(other._user_identity_secondary_field)
{
    // Nothing else
}

Association
::~Association()
{
    if(this->is_associated())
    {
        this->release();
    }
}

Association &
Association
::operator=(Association const & other)
{
    if(this != &other)
    {
        this->set_own_ae_title(other.get_own_ae_title());
        this->set_peer_host(other.get_peer_host());
        this->set_peer_port(other.get_peer_port());
        this->set_peer_ae_title(other.get_peer_ae_title());
        this->set_user_identity_type(other.get_user_identity_type());
        this->set_user_identity_primary_field(other.get_user_identity_primary_field());
        this->set_user_identity_secondary_field(other.get_user_identity_secondary_field());
    }

    return *this;
}

std::string const &
Association
::get_own_ae_title() const
{
    return this->_own_ae_title;
}

void
Association
::set_own_ae_title(std::string const & ae_title)
{
    if(this->is_associated())
    {
        throw Exception("Cannot set member while associated");
    }

    this->_own_ae_title = ae_title;
}

std::string const &
Association
::get_peer_host() const
{
    return this->_peer_host;
}

void
Association
::set_peer_host(std::string const & host)
{
    if(this->is_associated())
    {
        throw Exception("Cannot set member while associated");
    }

    this->_peer_host = host;
}

uint16_t
Association
::get_peer_port() const
{
    return this->_peer_port;
}

void
Association
::set_peer_port(uint16_t port)
{
    if(this->is_associated())
    {
        throw Exception("Cannot set member while associated");
    }

    this->_peer_port = port;
}

std::string const &
Association
::get_peer_ae_title() const
{
    return this->_peer_ae_title;
}

void
Association
::set_peer_ae_title(std::string const & ae_title)
{
    if(this->is_associated())
    {
        throw Exception("Cannot set member while associated");
    }

    this->_peer_ae_title = ae_title;
}

std::vector<Association::PresentationContext> const &
Association
::get_presentation_contexts() const
{
    return this->_presentation_contexts;
}

void
Association
::set_presentation_contexts(std::vector<PresentationContext> const & value)
{
    if(this->is_associated())
    {
        throw Exception("Cannot set member while associated");
    }

    this->_presentation_contexts = value;
}

UserIdentityType
Association
::get_user_identity_type() const
{
    return this->_user_identity_type;
}

void
Association
::set_user_identity_type(UserIdentityType type)
{
    if(this->is_associated())
    {
        throw Exception("Cannot set member while associated");
    }

    this->_user_identity_type = type;
}

std::string const &
Association
::get_user_identity_primary_field() const
{
    return this->_user_identity_primary_field;
}

void
Association
::set_user_identity_primary_field(std::string const & value)
{
    if(this->is_associated())
    {
        throw Exception("Cannot set member while associated");
    }

    this->_user_identity_primary_field = value;
}

std::string const &
Association
::get_user_identity_secondary_field() const
{
    return this->_user_identity_secondary_field;
}

void
Association
::set_user_identity_secondary_field(std::string const & value)
{
    if(this->is_associated())
    {
        throw Exception("Cannot set member while associated");
    }

    this->_user_identity_secondary_field = value;
}

void
Association
::set_user_identity_to_none()
{
    this->set_user_identity_type(UserIdentityType::None);
    this->set_user_identity_primary_field("");
    this->set_user_identity_secondary_field("");
}

void
Association
::set_user_identity_to_username(std::string const & username)
{
    this->set_user_identity_type(UserIdentityType::Username);
    this->set_user_identity_primary_field(username);
    this->set_user_identity_secondary_field("");
}

void
Association
::set_user_identity_to_username_and_password(
    std::string const & username, std::string const & password)
{
    this->set_user_identity_type(UserIdentityType::UsernameAndPassword);
    this->set_user_identity_primary_field(username);
    this->set_user_identity_secondary_field(password);
}

void
Association
::set_user_identity_to_kerberos(std::string const & ticket)
{
    this->set_user_identity_type(UserIdentityType::Kerberos);
    this->set_user_identity_primary_field(ticket);
    this->set_user_identity_secondary_field("");
}

void
Association
::set_user_identity_to_saml(std::string const & assertion)
{
    this->set_user_identity_type(UserIdentityType::SAML);
    this->set_user_identity_primary_field(assertion);
    this->set_user_identity_secondary_field("");
}

bool
Association
::is_associated() const
{
    return this->_state_machine.get_transport().is_open();
}

void
Association
::associate()
{
    boost::asio::ip::tcp::resolver resolver(
        this->_state_machine.get_transport().get_service());
    boost::asio::ip::tcp::resolver::query const query(this->_peer_host, "");
    auto const endpoint_it = resolver.resolve(query);

    dul::EventData data;
    data.peer_endpoint = *endpoint_it;
    data.peer_endpoint.port(this->_peer_port);

    auto request = std::make_shared<pdu::AAssociate>(pdu::AAssociate::Type::RQ);
    request->set_protocol_version(1);
    request->set_application_context(std::string("1.2.840.10008.3.1.1.1"));
    request->set_called_ae_title(this->_peer_ae_title);
    request->set_calling_ae_title(this->_own_ae_title);

    std::vector<pdu::PresentationContext> presentation_contexts;
    presentation_contexts.reserve(this->_presentation_contexts.size());
    for(int i=0; i<this->_presentation_contexts.size(); ++i)
    {
        auto const & source = this->_presentation_contexts[i];
        pdu::PresentationContext destination(
            source.abstract_syntax, source.transfer_syntaxes);
        destination.set_id(1+2*i);
        presentation_contexts.push_back(destination);
    }
    request->set_presentation_contexts(presentation_contexts);

    pdu::UserInformation user_information;

    // TODO
    user_information.set_sub_item<pdu::MaximumLength>(16384);

    user_information.set_sub_item<pdu::ImplementationClassUID>(
        implementation_class_uid);
    user_information.set_sub_item<pdu::ImplementationVersionName>(
        implementation_version_name);

    if(this->_user_identity_type != UserIdentityType::None)
    {
        pdu::UserIdentityRQ identity;
        identity.set_type(static_cast<int>(this->_user_identity_type));
        identity.set_primary_field(this->_user_identity_primary_field);
        identity.set_secondary_field(this->_user_identity_secondary_field);

        // TODO
        identity.set_positive_response_requested(true);

        user_information.set_sub_item<pdu::UserIdentityRQ>(identity);
    }

    request->set_user_information(user_information);

    data.pdu = request;

    this->_state_machine.send_pdu(data);
    this->_state_machine.receive_pdu(data);

    if(data.pdu == nullptr)
    {
        throw Exception("No response received");
    }
    else
    {
        auto const acceptation = std::dynamic_pointer_cast<pdu::AAssociate>(data.pdu);
        auto const rejection = std::dynamic_pointer_cast<pdu::AAssociateRJ>(data.pdu);
        if(acceptation != nullptr && acceptation->get_type() == pdu::AAssociate::Type::AC)
        {
            std::cout << "Accepted" << std::endl;
            this->_transfer_syntaxes_by_abstract_syntax.clear();
            this->_transfer_syntaxes_by_id.clear();
            auto const requested_pc = request->get_presentation_contexts();
            for(auto const & accepted_pc: acceptation->get_presentation_contexts())
            {
                if(accepted_pc.get_result_reason() != 0)
                {
                    continue;
                }

                auto const id = accepted_pc.get_id();
                auto const ts = accepted_pc.get_transfer_syntax();

                this->_transfer_syntaxes_by_id[id] = ts;

                auto const requested_pc_it = std::find_if(
                    requested_pc.begin(), requested_pc.end(),
                    [&id](pdu::PresentationContext const & pc)
                    {
                        return pc.get_id() == id;
                    }
                );

                if(requested_pc_it == requested_pc.end())
                {
                    throw Exception("No such presentation context ID");
                }

                auto const as = requested_pc_it->get_abstract_syntax();
                this->_transfer_syntaxes_by_abstract_syntax[as] = ts;
            }
        }
        else if(rejection != nullptr)
        {
            std::cout <<
                int(rejection->get_result()) << " " <<
                int(rejection->get_source()) << " " <<
                int(rejection->get_reason()) << " " << std::endl;
            throw Exception("Association rejected");
        }
        else
        {
            throw Exception("Invalid response");
        }
    }
}

void
Association
::receive_association()
{
    this->receive_association([](Association const &) { return true; });
}

void
Association
::receive_association(std::function<bool (const Association &)> acceptor)
{
    dul::EventData data;
    data.peer_endpoint = dul::Transport::Socket::endpoint_type(
        boost::asio::ip::tcp::v4(), 11112);

    this->_state_machine.is_association_acceptable =
        []()
        {
            return std::make_pair(false, std::tuple<int, int, int>(1, 2, 3));
        };

    this->_state_machine.receive(data);
    this->_state_machine.receive_pdu(data);

    if(data.pdu == NULL)
    {
        // We have rejected the request
        //return false;
    }
    else
    {
        auto const request = std::dynamic_pointer_cast<pdu::AAssociate>(data.pdu);
        if(request == nullptr || request->get_type() != pdu::AAssociate::Type::RQ)
        {
            throw Exception("Invalid response");
        }
        //return true;
    }
}

void
Association
::release()
{
    if(!this->is_associated())
    {
        throw Exception("Not associated");
    }

    auto pdu = std::make_shared<pdu::AReleaseRQ>();
    dul::EventData data;
    data.pdu = pdu;
    this->_state_machine.send_pdu(data);
}

void
Association
::abort(int source, int reason)
{
    if(!this->is_associated())
    {
        throw Exception("Not associated");
    }

    auto pdu = std::make_shared<pdu::AAbort>(source, reason);
    dul::EventData data;
    data.pdu = pdu;
    this->_state_machine.send_pdu(data);

    this->_state_machine.get_transport().close();
}

message::Message
Association
::receive_message()
{
    bool done = false;
    DataSet command_set, data_set;

    bool command_set_received=false;
    bool has_data_set=true;
    bool data_set_received=false;

    while(!done)
    {
        dul::EventData data;
        data.pdu = nullptr;
        this->_state_machine.receive_pdu(data);

        auto const p_data_tf = std::dynamic_pointer_cast<pdu::PDataTF>(data.pdu);
        if(p_data_tf == nullptr)
        {
            throw Exception("Invalid PDU received");
        }

        for(auto const & pdv: p_data_tf->get_pdv_items())
        {
            bool & received = pdv.is_command()?command_set_received:data_set_received;
            received |= pdv.is_last_fragment();

            auto const & fragment_data = pdv.get_fragment();
            std::stringstream stream;
            stream.write(&fragment_data[0], fragment_data.size());

            auto transfer_syntax = registry::ImplicitVRLittleEndian;
            if(!pdv.is_command())
            {
                auto const id = pdv.get_presentation_context_id();
                auto const transfer_syntax_it =
                    this->_transfer_syntaxes_by_id.find(id);
                if(transfer_syntax_it == this->_transfer_syntaxes_by_id.end())
                {
                    throw Exception("No such Presentation Context ID");
                }
                transfer_syntax = transfer_syntax_it->second;
            }

            Reader reader(stream, transfer_syntax);
            auto const fragment = reader.read_data_set();

            auto & destination = pdv.is_command()?command_set:data_set;
            for(auto const & element: fragment)
            {
                destination.add(element.first, element.second);
            }

            if(pdv.is_command() && command_set.has(registry::CommandDataSetType))
            {
                auto const & value =
                    command_set.as_int(registry::CommandDataSetType);
                if(value == Value::Integers({message::Message::DataSetType::ABSENT}))
                {
                    has_data_set = false;
                }
            }
        }

        done = command_set_received && (!has_data_set || data_set_received);
    }

    if(has_data_set)
    {
        return message::Message(command_set, data_set);
    }
    else
    {
        return message::Message(command_set);
    }
}

void
Association
::send_message(
    message::Message const & message, std::string const & abstract_syntax)
{
    std::vector<pdu::PDataTF::PresentationDataValueItem> pdv_items;

    std::ostringstream command_stream;
    Writer command_writer(
        command_stream, registry::ImplicitVRLittleEndian, // implicit vr for command
        Writer::ItemEncoding::ExplicitLength, true); // true for Command
    command_writer.write_data_set(message.get_command_set());
    pdv_items.push_back(
        pdu::PDataTF::PresentationDataValueItem(1, 3, command_stream.str()));

    if (message.has_data_set())
    {
        auto const transfer_syntax_it =
            this->_transfer_syntaxes_by_abstract_syntax.find(abstract_syntax);
        if(transfer_syntax_it == this->_transfer_syntaxes_by_abstract_syntax.end())
        {
            throw Exception("No transfer syntax for "+abstract_syntax);
        }

        std::ostringstream data_stream;
        Writer data_writer(
            data_stream, transfer_syntax_it->second,
            Writer::ItemEncoding::ExplicitLength, false);
        data_writer.write_data_set(message.get_data_set());
        pdv_items.push_back(
            pdu::PDataTF::PresentationDataValueItem(1, 2, data_stream.str()));
    }

    auto pdu = std::make_shared<pdu::PDataTF>(pdv_items);

    dul::EventData data;
    data.pdu = pdu;

    this->_state_machine.send_pdu(data);
}

}

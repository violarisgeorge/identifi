// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Identifi developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/lexical_cast.hpp>
#include "main.h"
#include "identifirpc.h"
#include "data.h"
#include "net.h"

using namespace json_spirit;
using namespace std;

Array packetVectorToJSONArray(vector<CIdentifiPacket> packets, bool findNames = true) {
    Array packetsJSON;
    BOOST_FOREACH(CIdentifiPacket packet, packets) {
        Object packetJSON = packet.GetJSON().get_obj();
        if (findNames) {
            vector<string> nameTypes;
            nameTypes.push_back("name");
            nameTypes.push_back("nickname");
            
            pair<string_pair, string_pair> linkedIDs = pidentifidb->GetPacketLinkedIdentifiers(packet, nameTypes);

            vector<CSignature> signatures = packet.GetSignatures();
            string_pair signerName = make_pair("", "");
            if (!signatures.empty())
                signerName = pidentifidb->GetLinkedIdentifier(make_pair("keyID", signatures.front().GetSignerKeyID()), nameTypes);

            Array author, recipient, signer;
            author.push_back(linkedIDs.first.first);
            author.push_back(linkedIDs.first.second);
            recipient.push_back(linkedIDs.second.first);
            recipient.push_back(linkedIDs.second.second);
            packetJSON.push_back(Pair("authorName", author));
            packetJSON.push_back(Pair("recipientName", recipient));

            if (signerName.first != "") {
                signer.push_back(signerName.first);
                signer.push_back(signerName.second);
                packetJSON.push_back(Pair("signerName", signer));
            }
        }
        packetsJSON.push_back(packetJSON);
    }
    return packetsJSON; 
}

Value getpacketcount(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getpacketcount\n"
            "Returns the number of stored packets.");

    return pidentifidb->GetPacketCount();
}

Value getidentifiercount(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getpacketcount\n"
            "Returns the number of stored identifiers.");

    return pidentifidb->GetIdentifierCount();
}

Value getpacketbyhash(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "getpacketbyhash <hash>\n"
            "Looks for a packet that matches the given hash.");

    vector<CIdentifiPacket> packets;
    try {
        CIdentifiPacket packet = pidentifidb->GetPacketByHash(params[0].get_str()); 
        packets.push_back(packet);       
    } catch (runtime_error) {

    }

    return packetVectorToJSONArray(packets);
}

Value gettruststep(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 4)
        throw runtime_error(
            "gettruststep <start_predicate> <start_id> <end_predicate> <end_id>\n"
            "Returns the hash of the next packet on the trust path from from start to end.");

    string trustStep = pidentifidb->GetTrustStep(make_pair(params[0].get_str(), params[1].get_str()), make_pair(params[2].get_str(), params[3].get_str()));

    return trustStep;
}

Value getpacketsbyauthor(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "getpacketsbyauthor <id_value> | <id_type> <id_value>\n"
            "Returns a list of packets associated with the given author identifier.");

    vector<CIdentifiPacket> packets;
    int limit = 0, offset = 0;
    if (params.size() == 1)
        packets = pidentifidb->GetPacketsByAuthor(make_pair("", params[0].get_str()), limit, offset, false);
    else
        packets = pidentifidb->GetPacketsByAuthor(make_pair(params[0].get_str(), params[1].get_str()), limit, offset, false);

    return packetVectorToJSONArray(packets);
}

Value getpacketsbyrecipient(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "getpacketsbyrecipient <id_value> | <id_type> <id_value>\n"
            "Returns a list of packets associated with the given recipient identifier.");

    vector<CIdentifiPacket> packets;
    int limit = 0, offset = 0;
    if (params.size() == 1)
        packets = pidentifidb->GetPacketsByRecipient(make_pair("", params[0].get_str()), limit, offset, false);
    else
        packets = pidentifidb->GetPacketsByRecipient(make_pair(params[0].get_str(), params[1].get_str()), limit, offset, false);

    return packetVectorToJSONArray(packets);

}

Value getpacketsafter(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2 )
        throw runtime_error(
            "getpacketsafter <timestamp> <limit=20>\n"
            "Get a list of packets after the given timestamp, limited to the given number of entries.");

    time_t timestamp = boost::lexical_cast<int>(params[0].get_str());
    int nLimit;
    if (params.size() == 2)
        nLimit = boost::lexical_cast<int>(params[1].get_str());
    else
        nLimit = 20;

    vector<CIdentifiPacket> packets = pidentifidb->GetPacketsAfterTimestamp(timestamp, nLimit);
    return packetVectorToJSONArray(packets);

}

Value getlatestpackets(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 2 )
        throw runtime_error(
            "getlatestpackets <limit=20> <offset=0>\n"
            "Get a list of packets after the given timestamp, limited to the given number of entries.");

    int nLimit, nOffset;
    if (params.size() > 0)
        nLimit = boost::lexical_cast<int>(params[0].get_str());
    else
        nLimit = 20;

    if (params.size() == 2)
        nOffset = boost::lexical_cast<int>(params[1].get_str());
    else
        nOffset = 0;

    vector<CIdentifiPacket> packets = pidentifidb->GetLatestPackets(nLimit, nOffset);
    return packetVectorToJSONArray(packets);

}

Value getpath(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 4 || params.size() > 5)
        throw runtime_error(
            "getpath <id1predicate> <id1> <id2predicate> <id2> <search_depth=3>\n"
            "Returns an array of packets that connect id1 and id2 with given predicates and optional max search depth.");

    Array packetsJSON;
    vector<CIdentifiPacket> packets;
    if (params.size() == 4)
        packets = pidentifidb->SearchForPath(make_pair(params[0].get_str(), params[1].get_str()), make_pair(params[2].get_str(), params[3].get_str()), false);
    else
        packets = pidentifidb->SearchForPath(make_pair(params[0].get_str(), params[1].get_str()), make_pair(params[2].get_str(), params[3].get_str()), false, boost::lexical_cast<int>(params[4].get_str()));
    for (vector<CIdentifiPacket>::iterator it = packets.begin(); it != packets.end(); ++it) {
        packetsJSON.push_back(it->GetJSON());
    }

    return packetsJSON;
}

Value getsavedpath(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 4 || params.size() > 5)
        throw runtime_error(
            "getsavedpath <id1predicate> <id1> <id2predicate> <id2> <search_depth=3>\n"
            "Returns an array of packets that connect id1 and id2 with given predicates and optional max search depth.");

    Array packetsJSON;
    vector<CIdentifiPacket> packets;
    if (params.size() == 4)
        packets = pidentifidb->GetSavedPath(make_pair(params[0].get_str(), params[1].get_str()), make_pair(params[2].get_str(), params[3].get_str()));
    else
        packets = pidentifidb->GetSavedPath(make_pair(params[0].get_str(), params[1].get_str()), make_pair(params[2].get_str(), params[3].get_str()), boost::lexical_cast<int>(params[4].get_str()));
    for (vector<CIdentifiPacket>::iterator it = packets.begin(); it != packets.end(); ++it) {
        packetsJSON.push_back(it->GetJSON());
    }

    return packetsJSON;
}

Value search(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "search <query> <predicate=\"\"> <limit=10> <offset=0>\n"
            "Returns a list of predicate / identifier pairs matching the query and predicate (optional).");

    Array resultsJSON;
    string_pair query;
    int limit = 10, offset = 0;
    if (params.size() == 1)
        query = make_pair("", params[0].get_str());
    if (params.size() >= 2)
        query = make_pair(params[1].get_str(), params[0].get_str());
    if (params.size() >= 3)
        limit = boost::lexical_cast<int>(params[2].get_str());
    if (params.size() == 4)
        offset = boost::lexical_cast<int>(params[3].get_str());

    vector<string_pair> results = pidentifidb->SearchForID(query, limit, offset);

    BOOST_FOREACH(string_pair result, results) {
        Array pair;
        pair.push_back(result.first);
        pair.push_back(result.second);
        resultsJSON.push_back(pair);
    }

    return resultsJSON;
}

Value savepacket(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 6 || params.size() > 7)
        throw runtime_error(
            "savepacket <author_id_type> <author_id_value> <recipient_id_type> <recipient_id_value> <packet_comment> <rating[-10..10]> <publish=false>\n"
            "Save a packet");

    Array author, author1, recipient, recipient1, signatures;
    author1.push_back(params[0].get_str());
    author1.push_back(params[1].get_str());
    author.push_back(author1);
    recipient1.push_back(params[2].get_str());
    recipient1.push_back(params[3].get_str());
    recipient.push_back(recipient1);

    time_t now = time(NULL);
    Object data, signedData;
    signedData.push_back(Pair("timestamp", lexical_cast<int64_t>(now)));
    signedData.push_back(Pair("author", author));
    signedData.push_back(Pair("recipient", recipient));
    signedData.push_back(Pair("type", "review"));
    signedData.push_back(Pair("comment",params[4].get_str()));
    signedData.push_back(Pair("rating",lexical_cast<int>(params[5].get_str())));
    signedData.push_back(Pair("maxRating",10));
    signedData.push_back(Pair("minRating",-10));

    data.push_back(Pair("signedData", signedData));
    data.push_back(Pair("signatures", signatures));

    string strData = write_string(Value(data), false);
    CIdentifiPacket packet(strData);
    CKey defaultKey = pidentifidb->GetDefaultKey();
    packet.Sign(defaultKey);

    bool publish = (params.size() == 7 && params[6].get_str() == "true");
    if (publish) {
        packet.SetPublished();
        RelayPacket(packet);
    }
    return pidentifidb->SavePacket(packet);
}

Value saveconnection(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 6 || params.size() > 7)
        throw runtime_error(
            "saveconnection <author_id_type> <author_id_value> <connected_id1_type> <connected_id1_value> <connected_id2_type> <connected_id2_value> <publish=false>\n"
            "Save a connection between id1 and id2");

    Array author, author1, recipient, connected1, connected2, signatures;
    author1.push_back(params[0].get_str());
    author1.push_back(params[1].get_str());
    author.push_back(author1);
    connected1.push_back(params[2].get_str());
    connected1.push_back(params[3].get_str());
    connected2.push_back(params[4].get_str());
    connected2.push_back(params[5].get_str());
    recipient.push_back(connected1);
    recipient.push_back(connected2);

    time_t now = time(NULL);
    Object data, signedData;
    signedData.push_back(Pair("timestamp", lexical_cast<int64_t>(now)));
    signedData.push_back(Pair("author", author));
    signedData.push_back(Pair("recipient", recipient));
    signedData.push_back(Pair("type", "connection"));
    // signedData.push_back(Pair("comment",params[4].get_str()));
    signedData.push_back(Pair("rating",0));
    signedData.push_back(Pair("maxRating",1));
    signedData.push_back(Pair("minRating",-1));

    data.push_back(Pair("signedData", signedData));
    data.push_back(Pair("signatures", signatures));

    string strData = write_string(Value(data), false);
    CIdentifiPacket packet(strData);
    CKey defaultKey = pidentifidb->GetDefaultKey();
    packet.Sign(defaultKey);

    bool publish = (params.size() == 7 && params[6].get_str() == "true");
    if (publish) {
        packet.SetPublished();
        RelayPacket(packet);
    }
    return pidentifidb->SavePacket(packet);
}

Value savepacketfromdata(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 3)
        throw runtime_error(
            "savepacketfromdata <packet_json_data> <publish=false> <sign=true>\n"
            "Save a packet.");

    CIdentifiPacket packet;
    packet.SetData(params[0].get_str());
    CKey defaultKey = pidentifidb->GetDefaultKey();
    bool publish = (params.size() >= 2 && params[1].get_str() == "true");
    if (publish || !(params.size() == 3 && params[2].get_str() == "false")) {
        packet.Sign(defaultKey);
    }
    if (publish) {
        packet.SetPublished();
        RelayPacket(packet);
    }
    return pidentifidb->SavePacket(packet);
}

Value getlinkedidentifier(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 3)
        throw runtime_error(
            "getlinkedidentifier <start_predicate> <start_id> <end_predicate>\n"
            "For example, find a nickname that corresponds to the given identifier.");

    vector<string> searchTypes;
    searchTypes.push_back(params[2].get_str());
    string_pair result = pidentifidb->GetLinkedIdentifier(make_pair(params[0].get_str(), params[1].get_str()), searchTypes);
    Array a;
    a.push_back(result.first);
    a.push_back(result.second);
    return a;
}

Value deletepacket(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "deletepacket <packet_hash>\n"
            "Delete a packet from the local database");

    pidentifidb->DropPacket(params[0].get_str());

    return true;
}


Value listmykeys(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "listmykeys\n"
            "List the private keys you own");

    CKey defaultKey = pidentifidb->GetDefaultKey();
    string strDefaultKey = EncodeBase58(defaultKey.GetPubKey().Raw());
    vector<IdentifiKey> keys = pidentifidb->GetMyKeys();
    Array keysJSON;

    for (vector<IdentifiKey>::iterator key = keys.begin(); key != keys.end(); ++key) {
        Object keyJSON;
        keyJSON.push_back(Pair("pubkey", key->pubKey));
        keyJSON.push_back(Pair("pubkey ID", key->keyID));
        keyJSON.push_back(Pair("privkey", key->privKey));
        keyJSON.push_back(Pair("default", key->pubKey == strDefaultKey));
        keysJSON.push_back(keyJSON);
    }
    return keysJSON;
}

Value importprivkey(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "importprivkey <key>\n"
            "Import a private key");
    pidentifidb->ImportPrivKey(params[0].get_str());

    return true;
}

Value setdefaultkey(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "setdefaultkey <key>\n"
            "Set the default signing key");

    pidentifidb->SetDefaultKey(params[0].get_str());

    return true;
}

Value addsignature(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 3)
        throw runtime_error(
            "addsignature <signed_packet_hash> <signer_pubkey> <signature>\n"
            "Add a signature to a packet");

    CSignature sig(params[1].get_str(), params[2].get_str());
    CIdentifiPacket packet = pidentifidb->GetPacketByHash(params[0].get_str());

    if (!packet.AddSignature(sig))
        throw runtime_error("Invalid signature");

    pidentifidb->SavePacket(packet);

    return true;
}

Value publish(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "publish <packet_hash>\n"
            "Publish a previously local-only packet to the network");

    CIdentifiPacket rel = pidentifidb->GetPacketByHash(params[0].get_str());
    rel.SetPublished();
    RelayPacket(rel);
    pidentifidb->SavePacket(rel);

    return true;
}

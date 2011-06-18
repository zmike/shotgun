#include <Eina.h>
#include "shotgun_private.h"
#include "xml.h"
#include "pugixml.hpp"
#include <iterator>

#define XML_NS_ROSTER "jabber:iq:roster"

using namespace pugi;

struct xml_memory_writer : xml_writer
{
   char  *buffer;
   size_t capacity;

   size_t result;

   xml_memory_writer() : buffer(0), capacity(0), result(0)
   {
   }

   xml_memory_writer(char  *buffer,
                     size_t capacity) : buffer(buffer), capacity(capacity), result(0)
   {
   }

   size_t
   written_size() const
   {
      return result < capacity ? result : capacity;
   }

   virtual void
   write(const void *data,
         size_t      size)
   {
      if (result < capacity)
        {
           size_t chunk = (capacity - result < size) ? capacity - result : size;
           memcpy(buffer + result, data, chunk);
        }

      result += size;
   }
};

static char *
xmlnode_to_buf(xml_node node,
               size_t *len,
               Eina_Bool leave_open)
{
   xml_memory_writer counter;
   char *buffer;

   node.print(counter);
   buffer = static_cast<char*>(calloc(1, counter.result ));
   xml_memory_writer writer(buffer, counter.result);
   node.print(writer);
   buffer[writer.written_size() - 1] = 0;
   if (leave_open) buffer[writer.written_size() - 3] = ' ';
   *len = writer.written_size();

   return buffer;
}

char *
xml_stream_init_create(const char *user, const char *to, const char *lang, size_t *len)
{
/*
C: <stream:stream
     from='juliet@im.example.com'
     to='im.example.com'
     version='1.0'
     xml:lang='en'
     xmlns='jabber:client'
     xmlns:stream='http://etherx.jabber.org/streams'>
*/
   xml_document doc;
   xml_node stream;
   char buf[256];

   snprintf(buf, sizeof(buf), "%s@%s", user, to);
   stream = doc.append_child("stream:stream");
   stream.append_attribute("from").set_value(buf);
   stream.append_attribute("to").set_value(to);
   stream.append_attribute("version").set_value("1.0");
   stream.append_attribute("xml:lang").set_value(lang);
   stream.append_attribute("xmlns").set_value("jabber:client");
   stream.append_attribute("xmlns:stream").set_value("http://etherx.jabber.org/streams");

   return xmlnode_to_buf(stream, len, EINA_TRUE);
}

static Eina_Bool
xml_stream_init_read_mechanisms(Shotgun_Auth *auth, xml_node stream, xml_node node)
{
  xml_attribute attr;
/*
S: <stream:features>
     <starttls xmlns="urn:ietf:params:xml:ns:xmpp-tls">
       <required/>
     </starttls>
     <mechanisms xmlns="urn:ietf:params:xml:ns:xmpp-sasl">
       <mechanism>X-GOOGLE-TOKEN</mechanism>
       <mechanism>X-OAUTH2</mechanism>
     </mechanisms>
    </stream:features>
*/

   if (!stream.child("starttls").empty())
      auth->features.starttls = EINA_TRUE;

   for (attr = node.first_attribute(); attr; attr = attr.next_attribute())
      if (!strcmp(attr.name(), "xmlns"))
        {
           const char *colon;

           colon = strrchr(attr.value(), ':');
           if (colon && (!strcmp(colon + 1, "xmpp-sasl")))
             auth->features.sasl = EINA_TRUE;
           break;
        }
   /* lots more auth mechanisms here but who cares */
   return EINA_TRUE;
}

Eina_Bool
xml_stream_init_read(Shotgun_Auth *auth, char *xml, size_t size)
{
/*
S: <stream:stream
     from='im.example.com'
     id='t7AMCin9zjMNwQKDnplntZPIDEI='
     to='juliet@im.example.com'
     version='1.0'
     xml:lang='en'
     xmlns='jabber:client'
     xmlns:stream='http://etherx.jabber.org/streams'>
*/
   xml_document doc;
   xml_node stream, node;
   xml_attribute attr;
   xml_parse_result res;

   res = doc.load_buffer_inplace(xml, size, parse_default, encoding_auto);
   if (res.status != status_ok)
     {
        ERR("%s", res.description());
        //goto error;
     }

   stream = doc.first_child();
   if (!strcmp(stream.name(), "stream:stream"))
     {
        for (attr = stream.first_attribute(); attr; attr = attr.next_attribute())
          {
             if (!strcmp(attr.name(), "from"))
               {
                  eina_stringshare_replace(&auth->from, attr.value());
                  break;
               }
          }

        if (stream.first_child().empty()) return EINA_FALSE;
        stream = stream.first_child();
     }

   node = stream.child("mechanisms");
   if (!node.empty())
     return xml_stream_init_read_mechanisms(auth, stream, node);

   node = stream.child("bind");
   /* something something */
   return EINA_TRUE;
}

Eina_Bool
xml_starttls_read(char *xml, size_t size __UNUSED__)
{
/*
S: <proceed xmlns='urn:ietf:params:xml:ns:xmpp-tls'/>

S: <failure xmlns='urn:ietf:params:xml:ns:xmpp-tls'/>
      </stream:stream>
*/
   /* massively cheating again! */
   return xml[1] == 'p';
}

char *
xml_sasl_write(const char *sasl, size_t *len)
{
/*
http://code.google.com/apis/talk/jep_extensions/jid_domain_change.html
<auth xmlns="urn:ietf:params:xml:ns:xmpp-sasl"
      mechanism="PLAIN"
      xmlns:ga='http://www.google.com/talk/protocol/auth'
      ga:client-uses-full-bind-result='true'>
... encoded user name and password ... user=example@gmail.com password=supersecret
</auth>
*/
   xml_document doc;
   xml_node auth;

   auth = doc.append_child("auth");
   auth.append_attribute("xmlns").set_value("urn:ietf:params:xml:ns:xmpp-sasl");
   auth.append_attribute("mechanism").set_value("PLAIN");
   auth.append_attribute("xmlns:ga").set_value("http://www.google.com/talk/protocol/auth");
   auth.append_attribute("ga:client-uses-full-bind-result").set_value("true");
   auth.append_child(node_pcdata).set_value(sasl);

   return xmlnode_to_buf(auth, len, EINA_FALSE);
}

Eina_Bool
xml_sasl_read(const unsigned char *xml, size_t size __UNUSED__)
{
/*
S: <success xmlns="urn:ietf:params:xml:ns:xmpp-sasl"/>

S: <failure xmlns="urn:ietf:params:xml:ns:xmpp-sasl"><not-authorized/></failure
*/
   /* I don't even care at this point */
   return xml[1] == 's';
}

char *
xml_iq_write(Shotgun_Auth *auth, Shotgun_Iq_Preset p, size_t *len)
{
   xml_document doc;
   xml_node iq, node;

   iq = doc.append_child("iq");
   switch (p)
     {
      case SHOTGUN_IQ_PRESET_BIND:
/*
<iq type="set" id="0">
  <bind xmlns="urn:ietf:params:xml:ns:xmpp-bind">
    <resource>
      pcp
    </resource>
  </bind>
</iq>
*/
        iq.append_attribute("type").set_value("set");
        iq.append_attribute("id").set_value("0");

        node = iq.append_child("bind");
        node.append_attribute("xmlns").set_value("urn:ietf:params:xml:ns:xmpp-bind");

        node = node.append_child("resource");
        node.append_child(node_pcdata).set_value(auth->resource);
        break;
      case SHOTGUN_IQ_PRESET_ROSTER:
/*
<iq from='juliet@example.com/balcony' type='get' id='roster_1'>
  <query xmlns='jabber:iq:roster'/>
</iq>
*/
        iq.append_attribute("type").set_value("get");
        iq.append_attribute("id").set_value("roster");

        node = iq.append_child("query");
        node.append_attribute("xmlns").set_value(XML_NS_ROSTER);
      default:
        break;
     }
   return xmlnode_to_buf(doc, len, EINA_FALSE);
}

static Shotgun_Iq_Type
xml_iq_type_get(xml_node node)
{
   const char *type;

   type = node.attribute("type").value();
   if (!strcmp(type, "get"))
     return SHOTGUN_IQ_TYPE_GET;
   if (!strcmp(type, "set"))
     return SHOTGUN_IQ_TYPE_SET;
   if (!strcmp(type, "result"))
     return SHOTGUN_IQ_TYPE_RESULT;
   return SHOTGUN_IQ_TYPE_ERROR;
}

static Shotgun_User_Subscription
xml_iq_user_subscription_get(xml_node node)
{
   const char *s;
   s = node.attribute("subscription").value();
   switch (s[0])
     {
      case 't':
        return SHOTGUN_USER_SUBSCRIPTION_TO;
      case 'f':
        return SHOTGUN_USER_SUBSCRIPTION_FROM;
      case 'b':
        return SHOTGUN_USER_SUBSCRIPTION_BOTH;
      default:
        break;
     }
   return SHOTGUN_USER_SUBSCRIPTION_NONE;
}

static Shotgun_Event_Iq *
xml_iq_roster_read(Shotgun_Auth *auth, xml_node node)
{
/*
<iq to='juliet@example.com/balcony' type='result' id='roster_1'>
  <query xmlns='jabber:iq:roster'>
    <item jid='romeo@example.net'
          name='Romeo'
          subscription='both'>
      <group>Friends</group>
    </item>
    <item jid='mercutio@example.org'
          name='Mercutio'
          subscription='from'>
      <group>Friends</group>
    </item>
    <item jid='benvolio@example.org'
          name='Benvolio'
          subscription='both'>
      <group>Friends</group>
    </item>
  </query>
</iq>
*/
   Shotgun_Event_Iq *ret;

   ret = static_cast<Shotgun_Event_Iq*>(calloc(1, sizeof(Shotgun_Event_Iq)));
   ret->type = SHOTGUN_IQ_EVENT_TYPE_ROSTER;
   ret->account = auth;
   
   for (xml_node it = node.first_child(); it; it = it.next_sibling())
     {
        Shotgun_User *user;

        user = static_cast<Shotgun_User*>(calloc(1, sizeof(Shotgun_User)));
        user->account = auth;
        user->name = eina_stringshare_add(it.attribute("name").value());
        user->jid = eina_stringshare_add(it.attribute("jid").value());
        user->subscription = xml_iq_user_subscription_get(it);
        ret->ev = eina_list_append((Eina_List*)ret->ev, (void*)user);
     }
   return ret;
}

Shotgun_Event_Iq *
xml_iq_read(Shotgun_Auth *auth, char *xml, size_t size)
{
   xml_document doc;
   xml_node node;
   xml_parse_result res;
   Shotgun_Iq_Type type;

   res = doc.load_buffer_inplace(xml, size, parse_default, encoding_auto);
   if (res.status != status_ok)
     {
        ERR("%s", res.description());
        return NULL;
     }
   type = xml_iq_type_get(doc.first_child());
   node = doc.first_child().first_child();
   if (strcmp(node.name(), "query"))
     {
        ERR("unhandled node: '%s'", node.name());
        return NULL;
     }
   switch (type)
     {
      case SHOTGUN_IQ_TYPE_RESULT:
        if (!strcmp(node.attribute("xmlns").value(), XML_NS_ROSTER))
          return xml_iq_roster_read(auth, node);
      case SHOTGUN_IQ_TYPE_GET:
      case SHOTGUN_IQ_TYPE_SET:
      default:
        return NULL;
     }
}

char *
xml_message_write(Shotgun_Message *msg, size_t *len)
{
/*
C: <message from='juliet@im.example.com/balcony'
            id='ju2ba41c'
            to='romeo@example.net'
            type='chat'
            xml:lang='en'>
     <body>Art thou not Romeo, and a Montague?</body>
   </message>
*/

   xml_document doc;
   xml_node node;
   char buf[256];

   snprintf(buf, sizeof(buf), "%s@%s", msg->account->user, msg->account->from);
   node = doc.append_child("message");
   node.append_attribute("from").set_value(buf);
   node.append_attribute("to").set_value(msg->user);
   node.append_attribute("type").set_value("chat");
   node.append_attribute("xml:lang").set_value("en");

   node = node.append_child("body");
   node.append_child(node_pcdata).set_value(msg->msg);
   
   return xmlnode_to_buf(doc, len, EINA_FALSE);
}

Shotgun_Message *
xml_message_read(Shotgun_Auth *auth, char *xml, size_t size)
{
/*
E: <message from='romeo@example.net/orchard'
            id='ju2ba41c'
            to='juliet@im.example.com/balcony'
            type='chat'
            xml:lang='en'>
     <body>Neither, fair saint, if either thee dislike.</body>
   </message>
*/
   xml_document doc;
   xml_node node;
   xml_attribute attr;
   xml_parse_result res;
   Shotgun_Message *ret;

   res = doc.load_buffer_inplace(xml, size, parse_default, encoding_auto);
   if (res.status != status_ok)
     {
        ERR("%s", res.description());
        return NULL;
     }

   node = doc.first_child();
   if (strcmp(node.name(), "message"))
     {
        ERR("Not a message tag: %s", node.name());
        return NULL;
     }
   ret = shotgun_message_new(auth);
   for (attr = node.first_attribute(); attr; attr = attr.next_attribute())
     {
        if (!strcmp(attr.name(), "from"))
          {
             eina_stringshare_replace(&ret->user, attr.value());
             break;
          }
     }
   ret->msg = strdup(node.first_child().child_value());
   return ret;
}

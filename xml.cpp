#include <Eina.h>
#include "shotgun_private.h"
#include "xml.h"
#include "pugixml.hpp"
#include <iterator>

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

   stream = doc.append_child("stream:stream");
   snprintf(buf, sizeof(buf), "%s@%s", user, to);
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
xml_bind_write(size_t *len)
{
/*
<iq type="set" id="0">
  <bind xmlns="urn:ietf:params:xml:ns:xmpp-bind">
    <resource>
      pcp
    </resource>
  </bind>
</iq>
*/
   /* stub for "resource" setting later */
   xml_document doc;
   xml_node iq, node;

   iq = doc.append_child("iq");
   iq.append_attribute("type").set_value("set");
   iq.append_attribute("id").set_value("0");

   node = iq.append_child("bind");
   node.append_attribute("xmlns").set_value("urn:ietf:params:xml:ns:xmpp-bind");

   node = node.append_child("resource");
   node.append_child(node_pcdata).set_value("SHOTGUN!");

   return xmlnode_to_buf(doc, len, EINA_FALSE);
}

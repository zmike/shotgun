#include <Eina.h>
#include "shotgun_private.h"
#include "xml.h"
#include "pugixml.hpp"
#include <iterator>

#define STARTTLS "<starttls xmlns='urn:ietf:params:xml:ns:xmpp-tls'/>"

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
               int64_t *len)
{
   xml_memory_writer counter;
   char *buffer;

   node.print(counter);
   buffer = static_cast<char*>(calloc(1, counter.result + 1));
   xml_memory_writer writer(buffer, counter.result);
   node.print(writer);
   buffer[writer.written_size()] = 0;
   *len = static_cast<int64_t> (writer.written_size());

   return buffer;
}

char *
xml_stream_init_create(const char *from, const char *to, const char *lang, int64_t *len)
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

   stream = doc.append_child("stream:stream");
   stream.append_attribute("from").set_value(from);
   stream.append_attribute("to").set_value(to);
   stream.append_attribute("version").set_value("1.0");
   stream.append_attribute("xml:lang").set_value(lang);
   stream.append_attribute("xmlns").set_value("jabber:client");
   stream.append_attribute("xmlns:stream").set_value("http://etherx.jabber.org/streams");

   return xmlnode_to_buf(stream, len);   
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
   xpath_node_set q_res;
   xml_parse_result res;
   static xpath_query q_tls("/stream:features/starttls");
   static xpath_query q_mech("/stream:features/mechanisms");

   res = doc.load_buffer_inplace(xml, size, parse_default, encoding_auto);
   if (res.status != status_ok)
     {
        ERR("%s", res.description());
        //goto error;
     }

   stream = doc.first_child();
   if (!auth->from)
     {
        EINA_SAFETY_ON_TRUE_GOTO(strcmp(stream.name(), "stream:stream"), error);

        for (attr = stream.first_attribute(); attr; attr = attr.next_attribute())
          {
             if (!strcmp(attr.name(), "from"))
               {
                  auth->from = eina_stringshare_add(attr.value());
                  break;
               }
          }

        if (stream.first_child().empty()) return EINA_TRUE;
        stream = stream.first_child();
     }
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
   
   node = stream.child("starttls");
   if (!node.empty())
      auth->features.starttls = EINA_TRUE;

   node = stream.child("mechanisms");
   EINA_SAFETY_ON_TRUE_GOTO(node.empty(), error);

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
   auth->state = SHOTGUN_STATE_FEATURES;
   return EINA_TRUE;
error:
   fprintf(stderr, "could not parse login xml!\n");
   return EINA_FALSE;
}

const char *
xml_starttls_write(int64_t *size)
{
   /* cheating super hard right here */
   *size = (int64_t)sizeof(STARTTLS) - 1;

   return STARTTLS;
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


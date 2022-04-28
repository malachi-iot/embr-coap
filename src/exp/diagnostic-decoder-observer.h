// 12/8/2019 - Still very useful, still experimental - ideally we'd send this to a specialized
//             clog which could retain contextual information (loglevel, tag, etc)

#include <estd/span.h>
#include <estd/string.h>
#include "../coap/decoder/events.h"
#include "../coap/uint.h"

namespace embr { namespace coap {

// NOTE: Not really experimental, but uses events whose location and naming have not yet
// settled down and they are in 'experimental' namespace, so convenient to put this here
namespace experimental {

struct DiagnosticMessageObserver
{
    static void opaque_dump_line(estd::const_buffer b, bool indent = false, int max_size = 0)
    {
        if(indent)
        {
            putchar(' ');
            putchar(' ');
            putchar(' ');
        }

        int i = 0;

        for(; i < b.size(); i++)
            printf("%02X ", b[i]);

        for(;i < max_size; i++)
        {
            putchar('.');
            putchar('.');
            putchar(' ');
        }

        estd::layer3::const_string s = b;

        putchar(' ');
        putchar(' ');
        putchar(' ');

        // FIX: Looks like const_string doesn't have proper
        // impl offset for this, so cheating and using 'b'
        for(int i = 0; i < s.size(); i++)
            putchar(b[i]);

        puts("");
    }

    static void opaque_dump(estd::const_buffer b)
    {
        /*
        int len = b.size();

        while(len > 0)
        {
            if(b.size() > 16)
                opaque_dump_line()
        } */
    }

    static void on_notify(const event::header& e)
    {
        printf("[DEBUG] Header: tkl=%d mid=%x\n",
            e.h.token_length(),
            e.h.message_id());
    }

    static void on_notify(const event::option& e)
    {
        const char* description = get_description(e.option_number);

        if(description == nullptr) description = "Unknown";

        printf("[DEBUG] Option: (%d) len=%3d %-10s value = ",
            e.option_number,
            (int)e.chunk.size(),
            description
            );

        auto format = OptionDecoder::get_format(e.option_number);

        switch(format)
        {
            case OptionDecoder::String:
            {
                estd::layer3::const_string s = e.chunk;

                char debug_print[128];

                // basic_string::copy is not null terminated
                debug_print[s.copy(debug_print, 128)] = 0;

                puts(debug_print);

                break;
            }

            case OptionDecoder::Empty:
                puts("(Empty)");
                break;

            case OptionDecoder::Opaque:
                opaque_dump_line(e.chunk);
                break;

            case OptionDecoder::UInt:
            {
                auto v = UInt::get<long unsigned>(e.chunk);

                printf("%lu\n", v);
                break;
            }

            default:
                puts("N/A");
                break;
        }
    }

    static void on_notify(const event::token& e)
    {
        printf("[DEBUG] Token: ");
        opaque_dump_line(e.chunk);
    }

    static void on_notify(const event::payload& e)
    {
        printf("[DEBUG] Payload: len=%d\n", 
            (int)e.chunk.size());
        opaque_dump_line(e.chunk, true, 16);
    }

    // FIX: We only get this when there's a payload
    static void on_notify(event::completed)
    {
        printf("[DEBUG] Completed\n");
    }
};

}

}}


#include "ChatBubble.h"

#include "../utils/Assert.h"
#include "../utils/Humanize.h"

#define _USE_MATH_DEFINES
#include <math.h>

namespace EGL3::Widgets {
    ChatBubble::ChatBubble(Storage::Models::ChatMessage& Message) :
        Message(Message)
    {
        Content.set_text(Message.Content);

        Construct();
    }

    ChatBubble::operator Gtk::Widget& () {
        return BaseContainer;
    }

    void ChatBubble::Construct()
    {
        Content.set_line_wrap(true);
        Content.set_line_wrap_mode(Pango::WRAP_WORD_CHAR);

        Content.set_halign(Gtk::ALIGN_CENTER);
        Content.set_xalign(0);
        Content.set_yalign(0);
        Content.set_margin_start(12);
        Content.set_margin_end(12);
        Content.set_margin_top(7);
        Content.set_margin_bottom(7);

        BubbleContainer.set_margin_bottom(5);
        BubbleContainer.set_halign(Message.Recieved ? Gtk::ALIGN_START : Gtk::ALIGN_END);

        if (Message.Recieved) {
            BubbleContainer.get_style_context()->add_class("chatbubble-recieved");
        }
        else {
            BubbleContainer.get_style_context()->add_class("chatbubble-sent");
        }

        BubbleContainer.set_has_tooltip(true);
        BubbleContainer.signal_query_tooltip().connect([this](int x, int y, bool keyboard_tooltip, const Glib::RefPtr<Gtk::Tooltip>& tooltip) {
            tooltip->set_text(Utils::Humanize(Message.Time));
            return true;
        });

        BubbleContainer.signal_draw().connect([this](const Cairo::RefPtr<Cairo::Context>& Ctx) {
            auto Alloc = BubbleContainer.get_allocation();

            DrawBubble(Ctx, Alloc, Message.Recieved);
            Ctx->clip();
            BubbleContainer.get_style_context()->render_background(Ctx, 0, 0, Alloc.get_width(), Alloc.get_height());

            return false;
        }, false);

        TextContainer.pack_start(Content, true, true, 0);

        BubbleContainer.add(TextContainer);
        BubbleContainer.add_overlay(Background);

        BaseContainer.pack_start(BubbleContainer, false, false, 0);

        BaseContainer.show_all();
    }

    void ChatBubble::DrawBubble(const Cairo::RefPtr<Cairo::Context>& Ctx, Gtk::Allocation Alloc, bool Recieved) {
        double x = 0;
        double y = 0;
        double width = Alloc.get_width();
        double height = Alloc.get_height();
        double radius = std::min(std::min(width, height), 40.) / 2;

        Ctx->arc(x + width - radius, y + radius, radius, -.5 * M_PI, 0);
        if (Recieved) {
            Ctx->arc(x + width - radius, y + height - radius, radius, 0, .5 * M_PI);
            Ctx->line_to(x, y + height);
        }
        else {
            Ctx->line_to(x + width, y + height);
            Ctx->arc(x + radius, y + height - radius, radius, .5 * M_PI, M_PI);
        }
        Ctx->arc(x + radius, y + radius, radius, M_PI, 1.5 * M_PI);
    }
}
